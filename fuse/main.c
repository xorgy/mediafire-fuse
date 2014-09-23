/*
 * Copyright (C) 2013 Bryan Christ <bryan.christ@mediafire.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include <openssl/ssl.h>

#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

enum
{
    KEY_HELP,
    KEY_VERSION,
};

struct mediafirefs_user_options {
    char *username;
    char *password;
    char *configfile;
} mediafirefs_user_options = {
    NULL, NULL, NULL
};

static struct fuse_opt mediafirefs_opts[] = {
    FUSE_OPT_KEY("-h", KEY_HELP),
    FUSE_OPT_KEY("--help", KEY_HELP),
    FUSE_OPT_KEY("-V", KEY_VERSION),
    FUSE_OPT_KEY("--version", KEY_VERSION),
    {"-c %s", offsetof(struct mediafirefs_user_options, configfile), 0},
    {"--configfile=%s", offsetof(struct mediafirefs_user_options, configfile), 0},
    {"-u %s", offsetof(struct mediafirefs_user_options, username), 0},
    {"--username %s", offsetof(struct mediafirefs_user_options, username), 0},
    {"-p %s", offsetof(struct mediafirefs_user_options, password), 0},
    {"--password %s", offsetof(struct mediafirefs_user_options, password), 0},
    FUSE_OPT_END
};

static void usage(const char *progname)
{
    fprintf(stderr, "Usage %s [options] mountpoint\n"
            "\n"
            "general options:\n"
            "    -o opt[,opt...] mount options\n"
            "    -h, --help      show this help\n"
            "    -V, --version   show version information\n"
            "\n"
            "MediaFire FS options:\n"
            "    -o user=str     username\n"
            "    -o password=str password\n"
            "    -c config=str   configuration file\n"
            "\n", progname);
}

static int mediafirefs_getattr(const char *path, struct stat *stbuf)
{
    // uid and gid are the one of the effective fuse user
    stbuf->st_uid = geteuid();
    stbuf->st_gid = getegid();
    if (!strcmp(path, "/")) {
        stbuf->st_mode = S_IFDIR | 0755;
        // FIXME: calculate number of directory entries
        stbuf->st_nlink = 2;
        return 0;
    }
    return -ENOSYS;
}

static int mediafirefs_readdir(const char *path, void *buf,
        fuse_fill_dir_t filldir, off_t offset, struct fuse_file_info *info)
{
    (void)path;
    (void)offset;
    (void)info;

    filldir(buf, ".", NULL, 0);
    filldir(buf, "..", NULL, 0);

    // FIXME: add entries for all files and directories

    return 0;
}

static struct fuse_operations mediafirefs_oper = {
    .getattr = mediafirefs_getattr,
    .readdir = mediafirefs_readdir,
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
    (void)data; // unused
    (void)arg; // unused

    if(key == KEY_HELP)
    {
        usage(outargs->argv[0]);
        fuse_opt_add_arg(outargs, "-ho");
        fuse_main(outargs->argc, outargs->argv, &mediafirefs_oper, NULL);
        exit(0);
    }

    if(key == KEY_VERSION)
    {
        fuse_opt_add_arg(outargs, "--version");
        fuse_main(outargs->argc, outargs->argv, &mediafirefs_oper, NULL);
        exit(0);
    }

    return 1;
}

int
main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if(fuse_opt_parse
       (&args, &mediafirefs_user_options, mediafirefs_opts,
        mediafirefs_opt_proc) == -1)
    {
        exit(1);
    }
    return fuse_main(args.argc, args.argv, &mediafirefs_oper, NULL);
}
