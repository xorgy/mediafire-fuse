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
#include <pwd.h>
#include <wordexp.h>

#include "../mfapi/mfconn.h"
#include "../mfapi/apicalls.h"
#include "hashtbl.h"
#include "../utils/strings.h"

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

static void parse_config_file(FILE * fp, int *argc, char ***argv)
{
    // read the config file line by line and pass each line to wordexp to
    // retain proper quoting
    char           *line = NULL;
    size_t          len = 0;
    ssize_t         read;
    wordexp_t       p;
    int             ret;
    size_t          i;

    while ((read = getline(&line, &len, fp)) != -1) {
        if (line[0] == '#')
            continue;

        // replace possible trailing newline by zero
        if (line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';
        ret = wordexp(line, &p, WRDE_SHOWERR | WRDE_UNDEF);
        if (ret != 0) {
            switch (ret) {
                case WRDE_BADCHAR:
                    fprintf(stderr, "wordexp: WRDE_BADCHAR\n");
                    break;
                case WRDE_BADVAL:
                    fprintf(stderr, "wordexp: WRDE_BADVAL\n");
                    break;
                case WRDE_CMDSUB:
                    fprintf(stderr, "wordexp: WRDE_CMDSUB\n");
                    break;
                case WRDE_NOSPACE:
                    fprintf(stderr, "wordexp: WRDE_NOSPACE\n");
                    break;
                case WRDE_SYNTAX:
                    fprintf(stderr, "wordexp: WRDE_SYNTAX\n");
                    break;
            }
            wordfree(&p);
            continue;
        }
        // now insert those arguments into argv right after the first
        *argv = (char **)realloc(*argv, sizeof(char *) * (*argc + p.we_wordc));
        memmove((*argv) + p.we_wordc + 1, (*argv) + 1,
                sizeof(char *) * (*argc - 1));
        *argc += p.we_wordc;
        for (i = 0; i < p.we_wordc; i++) {
            (*argv)[i + 1] = strdup(p.we_wordv[i]);
        }
        wordfree(&p);
    }
    free(line);
}

static void parse_config(int *argc, char ***argv, char *configfile)
{
    FILE           *fp;
    char           *homedir;

    // parse the configuration file
    // if a value is not set (it is NULL) then it has not been set by the
    // commandline and should be set by the configuration file

    // try set config file first if set
    if (configfile != NULL && (fp = fopen(configfile, "r")) != NULL) {
        parse_config_file(fp, argc, argv);
        fclose(fp);
        return;
    }
    // try "./.mediafire-tools.conf" second
    if ((fp = fopen("./.mediafire-tools.conf", "r")) != NULL) {
        parse_config_file(fp, argc, argv);
        fclose(fp);
        return;
    }
    // try "~/.mediafire-tools.conf" third
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    homedir = strdup_printf("%s/.mediafire-tools.conf", homedir);
    if ((fp = fopen("./.mediafire-tools.conf", "r")) != NULL) {
        parse_config_file(fp, argc, argv);
        fclose(fp);
    }
    free(homedir);
}

static void parse_arguments(int *argc, char ***argv,
                            struct mediafirefs_user_options *options)
{
    char          **argv_new;

    /* In the first pass, we only search for the help, version and config file
     * options.
     *
     * In case help or version options are found, the program will display
     * them and quit
     *
     * Otherwise, the program will read the config file (possibly supplied
     * through the option being set in the first pass), prepend its arguments
     * to argv and parse the arguments again as a second pass.
     */

    struct fuse_opt mediafirefs_opts_fst[] = {
        FUSE_OPT_KEY("-h", KEY_HELP),
        FUSE_OPT_KEY("--help", KEY_HELP),
        FUSE_OPT_KEY("-V", KEY_VERSION),
        FUSE_OPT_KEY("--version", KEY_VERSION),
        {"-c %s", offsetof(struct mediafirefs_user_options, configfile), 0},
        {"--config %s", offsetof(struct mediafirefs_user_options, configfile),
         0},
        FUSE_OPT_END
    };

    // copy argv into a new area so that we can realloc later
    argv_new = malloc(sizeof(char *) * (*argc));
    memcpy(argv_new, *argv, sizeof(char *) * (*argc));
    *argv = argv_new;

    struct fuse_args args_fst = FUSE_ARGS_INIT(*argc, *argv);

    if (fuse_opt_parse
        (&args_fst, options, mediafirefs_opts_fst,
         mediafirefs_opt_proc) == -1) {
        exit(1);
    }
    *argc = args_fst.argc;
    *argv = args_fst.argv;

    parse_config(argc, argv, options->configfile);

    struct fuse_opt mediafirefs_opts_snd[] = {
        {"-c %s", offsetof(struct mediafirefs_user_options, configfile), 0},
        {"--config %s", offsetof(struct mediafirefs_user_options, configfile),
         0},
        {"-u %s", offsetof(struct mediafirefs_user_options, username), 0},
        {"--username %s", offsetof(struct mediafirefs_user_options, username),
         0},
        {"-p %s", offsetof(struct mediafirefs_user_options, password), 0},
        {"--password %s", offsetof(struct mediafirefs_user_options, password),
         0},
        {"--server %s", offsetof(struct mediafirefs_user_options, server), 0},
        {"-i %d", offsetof(struct mediafirefs_user_options, app_id), 0},
        {"--app-id %d", offsetof(struct mediafirefs_user_options, app_id), 0},
        {"-k %s", offsetof(struct mediafirefs_user_options, api_key), 0},
        {"--api-key %s", offsetof(struct mediafirefs_user_options, api_key),
         0},
        FUSE_OPT_END
    };

    struct fuse_args args_snd = FUSE_ARGS_INIT(*argc, *argv);

    if (fuse_opt_parse(&args_snd, options, mediafirefs_opts_snd, NULL)
        == -1) {
        exit(1);
    }

    *argc = args_snd.argc;
    *argv = args_snd.argv;
}

static void connect_mf(struct mediafirefs_user_options *options)
{
    FILE           *fd;

    if (options->app_id == -1) {
        options->app_id = 42709;
    }

    if (options->server == NULL) {
        options->server = "www.mediafire.com";
    }

    if (options->username == NULL || options->password == NULL) {
        fprintf(stderr, "You must specify username and password\n");
        exit(1);
    }

    conn = mfconn_create(options->server,
                         options->username,
                         options->password, options->app_id, options->api_key);

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
}

int main(int argc, char *argv[])
{
    struct mediafirefs_user_options options = {
        NULL, NULL, NULL, NULL, -1, NULL
    };

    parse_arguments(&argc, &argv, &options);

    connect_mf(&options);

    return fuse_main(argc, argv, &mediafirefs_oper, NULL);
}
