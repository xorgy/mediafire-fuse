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
#include <string.h>
#include <inttypes.h>

#include <curl/curl.h>

#include "mfshell.h"
#include "private.h"
#include "account.h"
#include "cfile.h"
#include "strings.h"

int
_folder_create(mfshell_t *mfshell,char *parent,char *name)
{
    cfile_t     *cfile;
    char        *api_call;
    int         retval;

    if(mfshell == NULL) return -1;
    if(mfshell->user_signature == NULL) return -1;
    if(mfshell->session_token == NULL) return -1;

    if(name == NULL) return -1;
    if(strlen(name) < 1) return -1;

    // key must either be 11 chars or "myfiles"
    if(parent != NULL)
    {
        if(strlen(parent) != 13)
        {
            // if it is myfiles, set paret to NULL
            if(strcmp(parent,"myfiles") == 0) parent = NULL;
        }
    }

    // create the object as a sender
    cfile = cfile_create();

    // take the traditional defaults
    cfile_set_defaults(cfile);

    if(parent != NULL)
    {
        api_call = mfshell->create_signed_get(mfshell,0,"folder/create.php",
            "?parent_key=%s"
            "&foldername=%s"
            "&session_token=%s"
            "&response_format=json",
            parent,name,mfshell->session_token);
    }
    else
    {
        api_call = mfshell->create_signed_get(mfshell,0,"folder/create.php",
            "?foldername=%s",
            "&session_token=%s"
            "&response_format=json",
            name,mfshell->session_token);
    }

    cfile_set_url(cfile,api_call);

    retval = cfile_exec(cfile);

    if(retval != CURLE_OK) printf("error %d\n\r",retval);

    cfile_destroy(cfile);

    return retval;
}

