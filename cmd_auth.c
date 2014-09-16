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

#include "strings.h"
#include "mfshell.h"
#include "private.h"
#include "command.h"
#include "console.h"

static char*
_get_login_from_user(void);

static char*
_get_passwd_from_user(void);

int
mfshell_cmd_auth(mfshell_t *mfshell, int argc, char **argv)
{
    int retval;

    if(mfshell == NULL) return -1;
    if(mfshell->server == NULL) return -1;

    // free and invalidate existing user name
    if(mfshell->user != NULL)
    {
        free(mfshell->user);
        mfshell->user = NULL;
    }

    // free and invalidate existing passwd
    if(mfshell->passwd != NULL)
    {
        free(mfshell->passwd);
        mfshell->passwd = NULL;
    }

    switch (argc) {
        case 1:
            mfshell->user = _get_login_from_user();
            mfshell->passwd = _get_passwd_from_user();
            break;
        case 2:
            mfshell->user = argv[1];
            mfshell->passwd = _get_passwd_from_user();
            break;
        case 3:
            mfshell->user = argv[1];
            mfshell->passwd = argv[2];
            break;
        default:
            fprintf(stderr, "Invalid number of arguments\n");
            return -1;
    }

    if(mfshell->user == NULL || mfshell->passwd == NULL) return -1;

    retval = mfshell->get_session_token(mfshell);

    if(retval == 0)
        printf("\n\rAuthentication SUCCESS\n\r");
    else
        printf("\n\rAuthentication FAILURE\n\r");

    return retval;
}

char*
_get_login_from_user(void)
{
    char        *login = NULL;
    size_t      len;
    ssize_t     bytes_read;

    printf("login: ");
    bytes_read = getline(&login,&len,stdin);
    string_chomp(login);

    if(bytes_read < 3)
    {
        if(login != NULL)
        {
            free(login);
            login = NULL;
        }
    }

    return login;
}

char*
_get_passwd_from_user(void)
{
    char        *passwd = NULL;
    size_t      len;
    ssize_t     bytes_read;

    printf("passwd: ");

    // disable screen echo
    console_save_state();
    console_echo_off();

    bytes_read = getline(&passwd,&len,stdin);
    string_chomp(passwd);

    // re-enable screen echo
    console_restore_state();

    if(bytes_read < 3)
    {
        if(passwd != NULL)
        {
            free(passwd);
            passwd = NULL;
        }
    }

    return passwd;
}

