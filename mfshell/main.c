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

#include "../utils/strings.h"
#include "mfshell.h"

static void     mfshell_run(mfshell * shell);

static void     mfshell_parse_commands(mfshell * shell, char *command);

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
}

void
parse_argv(int argc, char *const argv[], char **username,
           char **password, char **server, char **command)
{
    static struct option long_options[] = {
        {"command", required_argument, 0, 'c'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"server", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'}
    };
    int             c;

    for (;;) {
        c = getopt_long(argc, argv, "c:u:p:s:hv", long_options, NULL);
        if (c == -1)
            break;

        switch (c) {
            case 'c':
                *command = strdup(optarg);
                break;
            case 'u':
                *username = strdup(optarg);
                break;
            case 'p':
                *password = strdup(optarg);
                break;
            case 's':
                *server = strdup(optarg);
                break;
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

    if (*password != NULL && *username == NULL) {
        fprintf(stderr, "You cannot pass the password without the username\n");
        exit(1);
    }
}

int main(int argc, char *const argv[])
{
    mfshell        *shell;
    char           *server = "www.mediafire.com";
    char           *username = NULL;
    char           *password = NULL;
    char           *command = NULL;
    char           *auth_cmd;

    SSL_library_init();

    parse_argv(argc, argv, &username, &password, &server, &command);

    shell = mfshell_create(35860, "2c6dq0gb2sr8rgsue5a347lzpjnaay46yjazjcjg",
                           server);

    if (username != NULL) {
        if (password != NULL) {
            auth_cmd = strdup_printf("auth %s %s", username, password);
        } else {
            auth_cmd = strdup_printf("auth %s", username);
        }
        mfshell_parse_commands(shell, auth_cmd);
        free(auth_cmd);
    }

    if (command == NULL) {
        // begin shell mode
        mfshell_run(shell);
    } else {
        // interpret command
        mfshell_parse_commands(shell, command);
    }

    mfshell_destroy(shell);

    if (server != NULL && strcmp("www.mediafire.com", server) != 0)
        free(server);
    if (username != NULL)
        free(username);
    if (password != NULL)
        free(password);
    if (command != NULL)
        free(command);

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
