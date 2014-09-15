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
#include <stdlib.h>

#include <curl/curl.h>

#include "cfile.h"
#include "strings.h"
#include "mfshell.h"
#include "private.h"
#include "user_session.h"
#include "command.h"
#include "account.h"
#include "list.h"
#include "folder_info.h"
#include "file_info.h"
#include "file_links.h"
#include "folder_create.h"


mfshell_t*
mfshell_create(int app_id,char *app_key,char *server)
{
    mfshell_t   *mfshell;

    if(app_id <= 0) return NULL;
    if(app_key == NULL) return NULL;
    if(server == NULL) return NULL;

    /*
        check to see if the server contains a forward-slash.  if so,
        the caller did not understand the API and passed in the wrong
        type of server resource.
    */
    if(strchr(server,'/') != NULL) return NULL;

    mfshell = (mfshell_t*)calloc(1,sizeof(mfshell_t));

    mfshell->app_id = app_id;
    mfshell->app_key = strdup(app_key);
    mfshell->server = strdup(server);

    mfshell->update_secret_key = _update_secret_key;

    mfshell->create_user_signature = _create_user_signature;
    mfshell->create_call_signature = _create_call_signature;
    mfshell->create_signed_get = _create_signed_get;

    mfshell->exec = _execute_shell_command;

    // console commands
    mfshell->debug = mfshell_cmd_debug;
    mfshell->list = mfshell_cmd_list;
    mfshell->chdir = mfshell_cmd_chdir;
    mfshell->pwd = mfshell_cmd_pwd;
    mfshell->help = mfshell_cmd_help;
    mfshell->file = mfshell_cmd_file;
    mfshell->links = mfshell_cmd_links;
    mfshell->host = mfshell_cmd_host;
    mfshell->auth = mfshell_cmd_auth;
    mfshell->lpwd = mfshell_cmd_lpwd;
    mfshell->lcd = mfshell_cmd_lcd;
    mfshell->mkdir = mfshell_cmd_mkdir;
    mfshell->get = mfshell_cmd_get;

    // configure REST API callbacks
    mfshell->get_session_token = _get_session_token;
    mfshell->user_get_info = _user_get_info;
    mfshell->folder_get_content = _folder_get_content;
    mfshell->folder_get_info = _folder_get_info;
    mfshell->folder_create = _folder_create;

    mfshell->file_get_info = _file_get_info;
    mfshell->file_get_links = _file_get_links;

    // object to track folder location
    mfshell->folder_curr = folder_alloc();
    folder_set_key(mfshell->folder_curr,"myfiles");

    return mfshell;
}

int
mfshell_set_login(mfshell_t *mfshell,char *user,char *passwd)
{
    if(mfshell == NULL) return -1;
    if(user == NULL) return -1;
    if(passwd == NULL) return -1;

    mfshell->user = strdup(user);
    mfshell->passwd = strdup(passwd);

    return 0;
}

