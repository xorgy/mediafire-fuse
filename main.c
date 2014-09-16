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
#include <termios.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "mfshell.h"
#include "console.h"
#include "private.h"
#include "cfile.h"
#include "strings.h"
#include "signals.h"

static void
mfshell_run(mfshell_t *mfshell);

int term_resized = 0;
int term_height = 0;
int term_width = 0;

int main(int argc,char **argv)
{
    extern int          term_height;
    extern int          term_width;
    mfshell_t           *mfshell;
    char                *server = "www.mediafire.com";
    size_t              len;
    int                 retval;

    SSL_library_init();

    retval = console_get_metrics(&term_height,&term_width);
    if(retval != 0)
    {
        // maybe the system doesn't support it.  we'll guess at it.
        term_height = 25;
        term_width = 80;
    }

    sig_install_SIGWINCH();

    if(argc > 1)
    {
        if(argv[1] != NULL) server = argv[1];
    }

    mfshell = mfshell_create(35860,
        "2c6dq0gb2sr8rgsue5a347lzpjnaay46yjazjcjg",server);

    printf("\n\r");
    mfshell->exec(mfshell, "auth");

    // begin shell mode
    mfshell_run(mfshell);

    return 0;
}

static void
mfshell_run(mfshell_t *mfshell)
{
    char    *cmd = NULL;
    size_t  len;
    int     abort = 0;
    int     retval;

    do
    {
        printf("\n\rmfshell > ");

        getline(&cmd,&len,stdin);
        string_chomp(cmd);

        printf("\n\r");

        if(strcmp(cmd,"exit") == 0)
        {
            abort = 1;
            continue;
        }

        if(strcmp(cmd,"quit") == 0)
        {
            abort = 1;
            continue;
        }

        retval = mfshell->exec(mfshell,cmd);
        free(cmd);
        cmd = NULL;
    }
    while(abort == 0);

    return;
}
