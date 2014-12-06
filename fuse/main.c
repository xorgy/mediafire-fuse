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
#include <sys/stat.h>
#include <bits/fcntl-linux.h>
#include <pwd.h>
#include <wordexp.h>
#include <fcntl.h>
#include <stdbool.h>

#include "../mfapi/mfconn.h"
#include "hashtbl.h"
#include "operations.h"
#include "../utils/strings.h"

enum {
    KEY_HELP,
    KEY_VERSION,
};

struct mediafirefs_user_options {
    char           *username;
    char           *password;
    char           *configfile;
    char           *server;
    int             app_id;
    char           *api_key;
};

static struct fuse_operations mediafirefs_oper = {
    .getattr = mediafirefs_getattr,
    .readdir = mediafirefs_readdir,
    .destroy = mediafirefs_destroy,
    .mkdir = mediafirefs_mkdir,
    .rmdir = mediafirefs_rmdir,
    .open = mediafirefs_open,
    .read = mediafirefs_read,
    .write = mediafirefs_write,
    .release = mediafirefs_release,
    .unlink = mediafirefs_unlink,
    .create = mediafirefs_create,
/*    .fsync = mediafirefs_fsync,
    .getxattr = mediafirefs_getxattr,
    .init = mediafirefs_init,
    .listxattr = mediafirefs_listxattr,
    .opendir = mediafirefs_opendir,
    .releasedir = mediafirefs_releasedir,
    .setxattr = mediafirefs_setxattr,
    .statfs = mediafirefs_statfs,
    .truncate = mediafirefs_truncate,
    .utime = mediafirefs_utime,*/
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

static void parse_config_file(int *argc, char ***argv, char *configfile)
{
    // read the config file line by line and pass each line to wordexp to
    // retain proper quoting
    char           *line = NULL;
    size_t          len = 0;
    ssize_t         read;
    wordexp_t       p;
    int             ret;
    size_t          i;
    FILE           *fp;

    if ((fp = fopen(configfile, "r")) == NULL) {
        fprintf(stderr, "Cannot open configuration file %s\n", configfile);
        return;
    }

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

    fclose(fp);
}

static void parse_arguments(int *argc, char ***argv,
                            struct mediafirefs_user_options *options,
                            char *configfile)
{
    int             i;

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

    struct fuse_args args_fst = FUSE_ARGS_INIT(*argc, *argv);

    if (fuse_opt_parse
        (&args_fst, options, mediafirefs_opts_fst,
         mediafirefs_opt_proc) == -1) {
        exit(1);
    }

    *argc = args_fst.argc;
    *argv = args_fst.argv;

    if (options->configfile != NULL) {
        parse_config_file(argc, argv, options->configfile);
    } else if (configfile != NULL) {
        parse_config_file(argc, argv, configfile);
    }

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

    if (*argv != args_snd.argv) {
        for (i = 0; i < *argc; i++) {
            free((*argv)[i]);
        }
        free(*argv);
    }

    *argc = args_snd.argc;
    *argv = args_snd.argv;
}

static void connect_mf(struct mediafirefs_user_options *options,
                       struct mediafirefs_context_private *ctx)
{
    FILE           *fp;

    if (options->app_id == -1) {
        options->app_id = 42709;
    }

    if (options->server == NULL) {
        options->server = "www.mediafire.com";
    }

    ctx->conn = mfconn_create(options->server, options->username,
                              options->password, options->app_id,
                              options->api_key, 3);

    if (ctx->conn == NULL) {
        fprintf(stderr, "Cannot establish connection\n");
        exit(1);
    }

    fp = fopen(ctx->dircache, "r");
    if (fp != NULL) {
        // file exists
        fprintf(stderr, "loading hashtable from %s\n", ctx->dircache);

        ctx->tree = folder_tree_load(fp, ctx->filecache);

        if (ctx->tree == NULL) {
            fprintf(stderr, "cannot load directory hashtable\n");
            exit(1);
        }

        fclose(fp);

        folder_tree_update(ctx->tree, ctx->conn, false);
    } else {
        // file doesn't exist
        fprintf(stderr, "creating new hashtable\n");
        ctx->tree = folder_tree_create(ctx->filecache);

        folder_tree_rebuild(ctx->tree, ctx->conn);
    }

    //folder_tree_housekeep(tree);

    fprintf(stderr, "tree before starting fuse:\n");
    folder_tree_debug(ctx->tree);
}

static void setup_conf_and_cache_dir(struct mediafirefs_context_private *ctx)
{
    const char     *homedir;
    const char     *configdir;
    const char     *cachedir;
    int             fd;

    homedir = getenv("HOME");
    if (homedir == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }

    configdir = getenv("XDG_CONFIG_HOME");
    if (configdir == NULL) {
        // $HOME/.config/mediafire-tools
        configdir = strdup_printf("%s/.config/mediafire-tools", homedir);
    } else {
        // $XDG_CONFIG_HOME/mediafire-tools
        configdir = strdup_printf("%s/mediafire-tools", configdir);
    }

    cachedir = getenv("XDG_CACHE_HOME");
    if (cachedir == NULL) {
        // $HOME/.cache/mediafire-tools
        cachedir = strdup_printf("%s/.cache/mediafire-tools", homedir);
    } else {
        // $XDG_CONFIG_HOME/mediafire-tools
        cachedir = strdup_printf("%s/mediafire-tools", cachedir);
    }

    /* EEXIST is okay, so only fail if it is something else */
    if (mkdir(configdir, 0755) != 0 && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }
    if (mkdir(cachedir, 0755) != 0 && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }

    ctx->configfile = strdup_printf("%s/config", configdir);
    /* test if the configuration file can be opened */
    fd = open(ctx->configfile, O_RDONLY);
    if (fd < 0) {
        free(ctx->configfile);
        ctx->configfile = NULL;
    } else {
        close(fd);
    }

    ctx->dircache = strdup_printf("%s/directorytree", cachedir);

    ctx->filecache = strdup_printf("%s/files", cachedir);
    if (mkdir(ctx->filecache, 0755) != 0 && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }

    free((void *)configdir);
    free((void *)cachedir);
}

int main(int argc, char *argv[])
{
    int             ret,
                    i;
    struct mediafirefs_context_private *ctx;

    struct mediafirefs_user_options options = {
        NULL, NULL, NULL, NULL, -1, NULL
    };

    ctx = calloc(1, sizeof(struct mediafirefs_context_private));

    setup_conf_and_cache_dir(ctx);

    parse_arguments(&argc, &argv, &options, ctx->configfile);

    if (options.username == NULL) {
        printf("login: ");
        options.username = string_line_from_stdin(false);
    }
    if (options.password == NULL) {
        printf("passwd: ");
        options.password = string_line_from_stdin(true);
    }

    connect_mf(&options, ctx);

    ctx->tmpfiles = NULL;
    ctx->num_tmpfiles = 0;

    ret = fuse_main(argc, argv, &mediafirefs_oper, ctx);

    for (i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);

    free(ctx->configfile);
    free(ctx->dircache);
    free(ctx->filecache);
    free(ctx);

    return ret;
}
