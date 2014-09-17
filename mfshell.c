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
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#include <curl/curl.h>

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
#include "connection.h"

struct _cmd_s commands[] = {
    {"help",   "",              "show this help", mfshell_cmd_help},
    {"debug",  "",              "show debug information", mfshell_cmd_debug},
    {"host",   "<server>",      "change target server", mfshell_cmd_host},
    {"auth",   "<user <pass>>", "authenticate with active server",
        mfshell_cmd_auth},
    {"whoami", "",              "show basic user info", mfshell_cmd_whoami},
    {"ls",     "",              "show contents of active folder",
        mfshell_cmd_list},
    {"cd",     "[folderkey]",   "change active folder", mfshell_cmd_chdir},
    {"pwd",    "",              "show the active folder", mfshell_cmd_pwd},
    {"lpwd",   "",              "show the local working directory",
        mfshell_cmd_lpwd},
    {"lcd",    "[dir]",         "change the local working directory",
        mfshell_cmd_lcd},
    {"mkdir",  "[folder name]", "create a new folder", mfshell_cmd_mkdir},
    {"file",   "[quickkey]",    "show file information", mfshell_cmd_file},
    {"links",  "[quickkey]",    "show access urls for the file",
        mfshell_cmd_links},
    {"get",    "[quickkey]",    "download a file", mfshell_cmd_get},
    {NULL,     NULL,            NULL,              NULL}
};

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

    mfshell->exec = _execute;
    mfshell->exec_string = _execute_shell_command;

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

    // shell commands
    mfshell->commands = commands;

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

