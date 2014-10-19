/*
 * Copyright (C) 2014 Johannes Schauer <j.schauer@email.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#define _POSIX_C_SOURCE 200809L // for strdup and struct timespec

#define FUSE_USE_VERSION 30

#include <fuse/fuse.h>
#include <fuse/fuse_opt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <bits/fcntl-linux.h>
#include <fuse/fuse_common.h>

#include "../mfapi/mfconn.h"
#include "../mfapi/apicalls.h"
#include "hashtbl.h"

/* what you can safely assume about requests to your filesystem
 *
 * from: http://sourceforge.net/p/fuse/wiki/FuseInvariants/
 *
 * There are a number of assumptions that one can safely make when
 * implementing a filesystem using fuse. This page should be completed with a
 * set of such assumptions.
 *
 *  - All requests are absolute, i.e. all paths begin with "/" and include the
 *    complete path to a file or a directory. Symlinks, "." and ".." are
 *    already resolved.
 *
 *  - For every request you can get except for "Getattr()", "Read()" and
 *    "Write()", usually for every path argument (both source and destination
 *    for link and rename, but only the source for symlink), you will get a
 *    "Getattr()" request just before the callback.
 *
 * For example, suppose I store file names of files in a filesystem also into a
 * database. To keep data in sync, I would like, for each filesystem operation
 * that succeeds, to check if the file exists on the database. I just do this in
 * the "Getattr()" call, since all other calls will be preceded by a getattr.
 *
 *  - The value of the "st_dev" attribute in the "Getattr()" call are ignored
 *    by fuse and an appropriate anomynous device number is inserted instead.
 *
 *  - The arguments for every request are already verified as much as
 *    possible. This means that, for example "readdir()" is only called with
 *    an existing directory name, "Readlink()" is only called with an existing
 *    symlink, "Symlink()" is only called if there isn't already another
 *    object with the requested linkname, "read()" and "Write()" are only
 *    called if the file has been opened with the correct flags.
 *
 *  - The VFS also takes care of avoiding race conditions:
 *
 *  - while "Unlink()" is running on a specific file, it cannot be interrupted
 *    by a "Chmod()", "Link()" or "Open()" call from a different thread on the
 *    same file.
 *
 *  - while "Rmdir()" is running, no files can be created in the directory
 *    that "Rmdir()" is acting on.
 *
 *  - If a request returns invalid values (e.g. in the structure returned by
 *    "Getattr()" or in the link target returned by "Symlink()") or if a
 *    request appears to have failed (e.g. if a "Create()" request succeds but
 *    a subsequent "Getattr()" (that fuse calls automatically) ndicates that
 *    no regular file has been created), the syscall returns EIO to the
 *    caller.
 */

enum {
    KEY_HELP,
    KEY_VERSION,
};

mfconn         *conn;
folder_tree    *tree;

struct mediafirefs_user_options {
    char           *username;
    char           *password;
    char           *configfile;
    char           *server;
    int             app_id;
    char           *api_key;
} mediafirefs_user_options = {
NULL, NULL, NULL, NULL, -1, NULL};

static struct fuse_opt mediafirefs_opts[] = {
    FUSE_OPT_KEY("-h", KEY_HELP),
    FUSE_OPT_KEY("--help", KEY_HELP),
    FUSE_OPT_KEY("-V", KEY_VERSION),
    FUSE_OPT_KEY("--version", KEY_VERSION),
    {"-c %s", offsetof(struct mediafirefs_user_options, configfile), 0},
    {"--config=%s", offsetof(struct mediafirefs_user_options, configfile), 0},
    {"-u %s", offsetof(struct mediafirefs_user_options, username), 0},
    {"--username %s", offsetof(struct mediafirefs_user_options, username), 0},
    {"-p %s", offsetof(struct mediafirefs_user_options, password), 0},
    {"--password %s", offsetof(struct mediafirefs_user_options, password), 0},
    {"--server %s", offsetof(struct mediafirefs_user_options, server), 0},
    {"-i %d", offsetof(struct mediafirefs_user_options, app_id), 0},
    {"--app-id %d", offsetof(struct mediafirefs_user_options, app_id), 0},
    {"-k %s", offsetof(struct mediafirefs_user_options, api_key), 0},
    {"--api-key %s", offsetof(struct mediafirefs_user_options, api_key), 0},
    FUSE_OPT_END
};

static void usage(const char *progname)
{
    fprintf(stderr, "Usage %s [options] mountpoint\n"
            "\n"
            "general options:\n"
            "    -o opt[,opt...]        mount options\n"
            "    -h, --help             show this help\n"
            "    -V, --version          show version information\n"
            "\n"
            "MediaFire FS options:\n"
            "    -o, --username str     username\n"
            "    -p, --password str     password\n"
            "    -c, --config file      configuration file\n"
            "    --server domain        server domain\n"
            "    -i, --app-id id        App ID\n"
            "    -k, --api-key key      API Key\n"
            "\n"
            "Notice that long options are separated from their arguments by\n"
            "a space and not an equal sign.\n" "\n", progname);
}

static int mediafirefs_getattr(const char *path, struct stat *stbuf)
{
    /*
     * since getattr is called before every other call (except for getattr,
     * read and write) wee only call folder_tree_update in the getattr call
     * and not the others
     */
    folder_tree_update(tree, conn);
    return folder_tree_getattr(tree, conn, path, stbuf);
}

static int mediafirefs_readdir(const char *path, void *buf,
                               fuse_fill_dir_t filldir, off_t offset,
                               struct fuse_file_info *info)
{
    (void)offset;
    (void)info;

    return folder_tree_readdir(tree, conn, path, buf, filldir);
}

static void mediafirefs_destroy()
{
    FILE           *fd;

    fprintf(stderr, "storing hashtable\n");

    fd = fopen("hashtable.dump", "w+");

    folder_tree_store(tree, fd);

    fclose(fd);

    folder_tree_destroy(tree);
}

static int mediafirefs_mkdir(const char *path, mode_t mode)
{
    (void)mode;

    char           *dirname;
    int             retval;
    char           *basename;
    const char     *key;

    /* we don't need to check whether the path already existed because the
     * getattr call made before this one takes care of that
     */

    /* before calling the remote function we check locally */

    dirname = strdup(path);

    /* remove possible trailing slash */
    if (dirname[strlen(dirname) - 1] == '/') {
        dirname[strlen(dirname) - 1] = '\0';
    }

    /* split into dirname and basename */

    basename = strrchr(dirname, '/');
    if (basename == NULL) {
        fprintf(stderr, "cannot find slash\n");
        return -ENOENT;
    }

    /* replace the slash by a terminating zero */
    basename[0] = '\0';

    /* basename points to the string after the last slash */
    basename++;

    /* check if the dirname is of zero length now. If yes, then the directory
     * is to be created in the root */
    if (dirname[0] == '\0') {
        key = NULL;
    } else {
        key = folder_tree_path_get_key(tree, conn, dirname);
    }

    retval = mfconn_api_folder_create(conn, key, basename);
    mfconn_update_secret_key(conn);
    if (retval != 0) {
        fprintf(stderr, "mfconn_api_folder_create unsuccessful\n");
        // FIXME: find better errno in this case
        return -EAGAIN;
    }

    free(dirname);

    folder_tree_update(tree, conn);

    return 0;
}

static int mediafirefs_rmdir(const char *path)
{
    const char     *key;
    int             retval;

    /* no need to check
     *  - if path is directory
     *  - if directory is empty
     *  - if directory is root
     *
     * because getattr was called before and already made sure
     */

    key = folder_tree_path_get_key(tree, conn, path);
    if (key == NULL) {
        fprintf(stderr, "key is NULL\n");
        return -ENOENT;
    }

    retval = mfconn_api_folder_delete(conn, key);
    mfconn_update_secret_key(conn);
    if (retval != 0) {
        fprintf(stderr, "mfconn_api_folder_create unsuccessful\n");
        // FIXME: find better errno in this case
        return -EAGAIN;
    }

    /* retrieve remote changes to not get out of sync */
    folder_tree_update(tree, conn);

    return 0;
}

static int mediafirefs_open(const char *path, struct fuse_file_info *file_info)
{
    (void)path;

    if ((file_info->flags & O_ACCMODE) != O_RDONLY) {
        fprintf(stderr, "can only open read-only");
        return -EACCES;
    }

    /* check if file has already been downloaded */

    /* check if downloaded version is the current one */

    /* download file from remote */

    /* update local file with patch */

    return -ENOSYS;
}

static int mediafirefs_read(const char *path, char *buf, size_t size,
                            off_t offset, struct fuse_file_info *file_info)
{
    (void)path;
    (void)buf;
    (void)size;
    (void)offset;
    (void)file_info;

    return -ENOSYS;
}

static struct fuse_operations mediafirefs_oper = {
    .getattr = mediafirefs_getattr,
    .readdir = mediafirefs_readdir,
    .destroy = mediafirefs_destroy,
    .mkdir = mediafirefs_mkdir,
    .rmdir = mediafirefs_rmdir,
    .open = mediafirefs_open,
    .read = mediafirefs_read,
/*    .create = mediafirefs_create,
    .fsync = mediafirefs_fsync,
    .getxattr = mediafirefs_getxattr,
    .init = mediafirefs_init,
    .listxattr = mediafirefs_listxattr,
    .opendir = mediafirefs_opendir,
    .releasedir = mediafirefs_releasedir,
    .setxattr = mediafirefs_setxattr,
    .statfs = mediafirefs_statfs,
    .truncate = mediafirefs_truncate,
    .unlink = mediafirefs_unlink,
    .utime = mediafirefs_utime,
    .write = mediafirefs_write,*/
};

static int
mediafirefs_opt_proc(void *data, const char *arg, int key,
                     struct fuse_args *outargs)
{
    (void)data;                 // unused
    (void)arg;                  // unused

    if (key == KEY_HELP) {
        usage(outargs->argv[0]);
        fuse_opt_add_arg(outargs, "-ho");
        fuse_main(outargs->argc, outargs->argv, &mediafirefs_oper, NULL);
        exit(0);
    }

    if (key == KEY_VERSION) {
        fuse_opt_add_arg(outargs, "--version");
        fuse_main(outargs->argc, outargs->argv, &mediafirefs_oper, NULL);
        exit(0);
    }

    return 1;
}

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    FILE           *fd;

    if (fuse_opt_parse
        (&args, &mediafirefs_user_options, mediafirefs_opts,
         mediafirefs_opt_proc) == -1) {
        exit(1);
    }

    if (mediafirefs_user_options.app_id == -1) {
        mediafirefs_user_options.app_id = 42709;
    }

    if (mediafirefs_user_options.server == NULL) {
        mediafirefs_user_options.server = "www.mediafire.com";
    }

    if (mediafirefs_user_options.username == NULL ||
        mediafirefs_user_options.password == NULL) {
        fprintf(stderr, "You must specify username and pasword\n");
        exit(1);
    }

    conn = mfconn_create(mediafirefs_user_options.server,
                         mediafirefs_user_options.username,
                         mediafirefs_user_options.password,
                         mediafirefs_user_options.app_id,
                         mediafirefs_user_options.api_key);

    if (conn == NULL) {
        fprintf(stderr, "Cannot establish connection\n");
        exit(1);
    }

    if (access("hashtable.dump", F_OK) != -1) {
        // file exists
        fprintf(stderr, "loading hashtable\n");
        fd = fopen("hashtable.dump", "r");

        tree = folder_tree_load(fd);

        fclose(fd);

        folder_tree_update(tree, conn);
    } else {
        // file doesn't exist
        tree = folder_tree_create();

        folder_tree_rebuild(tree, conn);
    }

    //folder_tree_housekeep(tree);

    fprintf(stderr, "tree before starting fuse:\n");
    folder_tree_debug(tree);

    return fuse_main(args.argc, args.argv, &mediafirefs_oper, NULL);
}
