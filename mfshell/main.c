/*
 * Copyright (C) 2013 Bryan Christ <bryan.christ@mediafire.com>
 *               2014 Johannes Schauer <j.schauer@email.de>
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

#include <getopt.h>
#include <openssl/ssl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <pwd.h>
#include <unistd.h>

#include "../utils/strings.h"
#include "mfshell.h"

static void     mfshell_run(mfshell * shell);

static void     mfshell_parse_commands(mfshell * shell, char *command);

struct mfshell_user_options {
    char *username;
    char *password;
    char *command;
    char *server;
    char *config;
} mfshell_user_options = {
    NULL, NULL, NULL, NULL, NULL
};

void print_help(const char *cmd)
{
    fprintf(stderr, "A shell to access a MediaFire account.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [options]\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr, "  -h, --help            Print this help\n");
    fprintf(stderr, "  -v, --version         Print version information\n");
    fprintf(stderr, "  -c, --command=<CMD>   Run command CMD and exit\n");
    fprintf(stderr, "  -u, --username=<USER> Login username\n");
    fprintf(stderr, "  -p, --password=<PASS> Login password\n");
    fprintf(stderr, "  -s, --server=<SERVER> Login server\n");
    fprintf(stderr, "\n");
    fprintf(stderr,
            "Username and password are optional. If not given, they\n"
            "have to be entered via standard input.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "You should not pass your password as a commandline\n"
            "argument as other users with access to the list of\n"
            "running processes will then be able to see it.\n");
    fprintf(stderr, "\n");
    fprintf(stderr,
            "Use the \"help\" command to print a list of available\n"
            "commands in the interactive environment:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "     %s -c help\n", cmd);
    fprintf(stderr, "\n");
    fprintf(stderr,
            "If arguments are given multiple times, the first one will be\n"
            "used.\n");
    fprintf(stderr, "\n");
    fprintf(stderr,
            "Arguments given on the commandline will overwrite arguments\n"
            "given in the configuration file\n");
    fprintf(stderr, "\n");
    fprintf(stderr,
            "The configuration file is parsed as if it was an argument\n"
            "string except that linebreaks are allowed between arguments.\n"
            "Lines starting with a # (hash) are ignored\n");
}

void
parse_argv(int argc, char *const argv[])
{
    static struct option long_options[] = {
        {"command", required_argument, 0, 'c'},
        {"config", required_argument, 0, 'f'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"server", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    int             c;

    // resetting optind because we want to be able to call getopt_long
    // multiple times for the commandline arguments as well as for the
    // configuration file
    optind = 0;
    for (;;) {
        c = getopt_long(argc, argv, "c:u:p:s:hv", long_options, NULL);
        if (c == -1)
            break;

        switch (c) {
            case 'c':
                if (mfshell_user_options.command == NULL)
                    mfshell_user_options.command = strdup(optarg);
                break;
            case 'u':
                if (mfshell_user_options.username == NULL)
                    mfshell_user_options.username = strdup(optarg);
                break;
            case 'p':
                if (mfshell_user_options.password == NULL)
                    mfshell_user_options.password = strdup(optarg);
                break;
            case 's':
                if (mfshell_user_options.server == NULL)
                    mfshell_user_options.server = strdup(optarg);
                break;
            case 'f':
                if (mfshell_user_options.config == NULL)
                    mfshell_user_options.config = strdup(optarg);
            case 'h':
                print_help(argv[0]);
                exit(0);
            case 'v':
                exit(0);
            case '?':
                exit(1);
                break;
            default:
                fprintf(stderr, "getopt_long returned character code %c\n", c);
        }
    }

    if (optind < argc) {
        // TODO: handle non-option argv elements
        fprintf(stderr, "Unexpected positional arguments\n");
        exit(1);
    }

    if (mfshell_user_options.password != NULL &&
        mfshell_user_options.username == NULL) {
        fprintf(stderr, "You cannot pass the password without the username\n");
        exit(1);
    }
}

static void parse_config_file(FILE *fp)
{
    // read the config file line by line and pass each line to wordexp to
    // retain proper quoting
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    wordexp_t p;
    int ret, i;
    int argc;
    char **argv;

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
        // prepend a dummy program name so that getopt_long will be able to
        // parse it
        argc = p.we_wordc + 1;
        // allocate one more than argc for trailing NULL
        argv = (char **)malloc(sizeof(char*)*(argc+1));
        argv[0] = ""; // dummy program name
        for (i = 1; i < argc; i++)
            argv[i] = p.we_wordv[i-1];
        argv[argc] = NULL;
        parse_argv(argc, argv);
        free(argv);
        wordfree(&p);
    }
    free(line);
}

static void parse_config()
{
    FILE *fp;
    char *homedir;
    // parse the configuration file
    // if a value is not set (it is NULL) then it has not been set by the
    // commandline and should be set by the configuration file

    // try set config file first if set
    if (mfshell_user_options.config != NULL &&
        (fp = fopen(mfshell_user_options.config, "r")) != NULL) {
        parse_config_file(fp);
        fclose(fp);
        return;
    }

    // try "./.mediafire-tools.conf" second
    if ((fp = fopen("./.mediafire-tools.conf", "r")) != NULL) {
        parse_config_file(fp);
        fclose(fp);
        return;
    }

    // try "~/.mediafire-tools.conf" third
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    homedir = strdup_printf("%s/.mediafire-tools.conf", homedir);
    if ((fp = fopen("./.mediafire-tools.conf", "r")) != NULL) {
        parse_config_file(fp);
        fclose(fp);
    }
    free(homedir);
}

int main(int argc, char *const argv[])
{
    mfshell        *shell;
    char           *auth_cmd;

    SSL_library_init();

    parse_argv(argc, argv);

    parse_config();

    // if server was neither set on the commandline nor in the config, set it
    // to the default value
    if (mfshell_user_options.server == NULL) {
        mfshell_user_options.server = strdup("www.mediafire.com");
    }

    shell = mfshell_create(35860, "2c6dq0gb2sr8rgsue5a347lzpjnaay46yjazjcjg",
                           mfshell_user_options.server);

    // if at least username was set, authenticate automatically
    if (mfshell_user_options.username != NULL) {
        if (mfshell_user_options.password != NULL) {
            auth_cmd = strdup_printf("auth %s %s",
                                     mfshell_user_options.username,
                                     mfshell_user_options.password);
        } else {
            auth_cmd = strdup_printf("auth %s", mfshell_user_options.username);
        }
        mfshell_parse_commands(shell, auth_cmd);
        free(auth_cmd);
    }

    if (mfshell_user_options.command == NULL) {
        // begin shell mode
        mfshell_run(shell);
    } else {
        // interpret command
        mfshell_parse_commands(shell, mfshell_user_options.command);
    }

    mfshell_destroy(shell);

    if (mfshell_user_options.server != NULL)
        free(mfshell_user_options.server);
    if (mfshell_user_options.username != NULL)
        free(mfshell_user_options.username);
    if (mfshell_user_options.password != NULL)
        free(mfshell_user_options.password);
    if (mfshell_user_options.command != NULL)
        free(mfshell_user_options.command);
    if (mfshell_user_options.config != NULL)
        free(mfshell_user_options.config);

    return 0;
}

static void mfshell_parse_commands(mfshell * shell, char *command)
{
    char           *next;
    int             ret;
    wordexp_t       p;

    // FIXME: don't split by semicolon but by unescaped/unquoted semicolon
    while ((next = strsep(&command, ";")) != NULL) {
        // FIXME: handle non-zero return value of wordexp
        ret = wordexp(next, &p, WRDE_SHOWERR | WRDE_UNDEF);
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
        }
        if (p.we_wordc < 1) {
            fprintf(stderr, "Need more than zero arguments\n");
            exit(1);
        }
        mfshell_exec(shell, p.we_wordc, p.we_wordv);
        wordfree(&p);
    }
}

static void mfshell_run(mfshell * shell)
{
    char           *cmd = NULL;
    size_t          len;
    int             abort = 0;
    int             retval;

    do {
        printf("\n\rmfshell > ");

        retval = getline(&cmd, &len, stdin);
        if (retval == -1) {
            exit(1);
        }

        if (cmd[strlen(cmd) - 1] == '\n')
            cmd[strlen(cmd) - 1] = '\0';

        printf("\n\r");

        if (strcmp(cmd, "exit") == 0) {
            abort = 1;
            continue;
        }

        if (strcmp(cmd, "quit") == 0) {
            abort = 1;
            continue;
        }

        retval = mfshell_exec_shell_command(shell, cmd);
        free(cmd);
        cmd = NULL;
    }
    while (abort == 0);

    return;
}
