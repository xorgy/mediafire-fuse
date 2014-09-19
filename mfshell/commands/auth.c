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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "../mfshell.h"
#include "../commands.h"

static char*
_get_login_from_user(void);

static char*
_get_passwd_from_user(void);

int
mfshell_cmd_auth(mfshell_t *mfshell, int argc, char **argv)
{
    char *username;
    char *password;

    if(mfshell == NULL) return -1;
    if(mfshell->server == NULL) return -1;

    switch (argc) {
        case 1:
            username = _get_login_from_user();
            password = _get_passwd_from_user();
            break;
        case 2:
            username = argv[1];
            password = _get_passwd_from_user();
            break;
        case 3:
            username = argv[1];
            password = argv[2];
            break;
        default:
            fprintf(stderr, "Invalid number of arguments\n");
            return -1;
    }

    if(username == NULL || password == NULL) return -1;

    mfshell->mfconn = mfconn_create(mfshell->server, username, password,
            mfshell->app_id, mfshell->app_key);

    if (mfshell->mfconn != NULL)
        printf("\n\rAuthentication SUCCESS\n\r");
    else
        printf("\n\rAuthentication FAILURE\n\r");

    return (mfshell->mfconn != NULL);
}

char*
_get_login_from_user(void)
{
    char        *login = NULL;
    size_t      len;
    ssize_t     bytes_read;

    printf("login: ");
    bytes_read = getline(&login,&len,stdin);

    if(bytes_read < 3)
    {
        if(login != NULL)
        {
            free(login);
            login = NULL;
        }
    }

    if (login[strlen(login)-1] == '\n')
        login[strlen(login)-1] = '\0';


    return login;
}

char*
_get_passwd_from_user(void)
{
    char        *passwd = NULL;
    size_t      len;
    ssize_t     bytes_read;
    struct termios old, new;

    printf("passwd: ");

    if (tcgetattr(STDIN_FILENO, &old) != 0)
        return NULL;
    new = old;
    new.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new) != 0)
        return NULL;

    bytes_read = getline(&passwd,&len,stdin);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);

    if(bytes_read < 3)
    {
        if(passwd != NULL)
        {
            free(passwd);
            passwd = NULL;
        }
    }

    if (passwd[strlen(passwd)-1] == '\n')
        passwd[strlen(passwd)-1] = '\0';

    return passwd;
}

