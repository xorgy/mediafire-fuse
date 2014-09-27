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

#define FUSE_USE_VERSION 30

#include <fuse/fuse.h>
#include <fuse/fuse_opt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../mfapi/mfconn.h"
#include "hashtbl.h"

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
    folder_tree_update(tree, conn);
    return folder_tree_getattr(tree, path, stbuf);
}

static int mediafirefs_readdir(const char *path, void *buf,
                               fuse_fill_dir_t filldir, off_t offset,
                               struct fuse_file_info *info)
{
    (void)offset;
    (void)info;

    folder_tree_update(tree, conn);
    return folder_tree_readdir(tree, path, buf, filldir);
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

static struct fuse_operations mediafirefs_oper = {
    .getattr = mediafirefs_getattr,
    .readdir = mediafirefs_readdir,
    .destroy = mediafirefs_destroy,
/*    .create = mediafirefs_create,
    .fsync = mediafirefs_fsync,
    .getattr = mediafirefs_getattr,
    .getxattr = mediafirefs_getxattr,
    .init = mediafirefs_init,
    .listxattr = mediafirefs_listxattr,
    .open = mediafirefs_open,
    .opendir = mediafirefs_opendir,
    .read = mediafirefs_read,
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
    int             ret;
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

        ret = folder_tree_rebuild(tree, conn);
    }

    //folder_tree_housekeep(tree);

    //folder_tree_debug(tree, NULL, 0);

    return fuse_main(args.argc, args.argv, &mediafirefs_oper, NULL);
}
