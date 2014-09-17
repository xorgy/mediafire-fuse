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

#include <curl/curl.h>
#include <jansson.h>

#include "mfshell.h"
#include "private.h"
#include "account.h"
#include "strings.h"
#include "json.h"
#include "connection.h"

static int
_decode_folder_get_info(conn_t *conn, void *data);

int
_folder_get_info(mfshell_t *mfshell,folder_t *folder,char *folderkey)
{
    char        *api_call;
    int         retval;

    if(mfshell == NULL) return -1;
    if(mfshell->user_signature == NULL) return -1;
    if(mfshell->session_token == NULL) return -1;

    if(folder == NULL) return -1;
    if(folderkey == NULL) return -1;

    // key must either be 11 chars or "myfiles"
    if(strlen(folderkey) != 13)
    {
        if(strcmp(folderkey,"myfiles") == 0) return -1;
    }

    api_call = mfshell->create_signed_get(mfshell,0,"folder/get_info.php",
        "?folder_key=%s"
        "&session_token=%s"
        "&response_format=json",
        folderkey,mfshell->session_token);

    conn_t *conn = conn_create();
    retval = conn_get_buf(conn, api_call, _decode_folder_get_info, folder);
    conn_destroy(conn);

    return retval;
}

static int
_decode_folder_get_info(conn_t *conn, void *data)
{
    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *folderkey;
    json_t          *folder_name;
    json_t          *parent_folder;
    int             retval = 0;
    folder_t       *folder;

    if(data == NULL) return -1;

    folder = (folder_t *)data;

    root = json_loadb(conn->write_buf, conn->write_buf_len, 0, &error);

    node = json_object_by_path(root,"response/folder_info");

    folderkey = json_object_get(node,"folderkey");
    if(folderkey != NULL)
        folder_set_key(folder,(char*)json_string_value(folderkey));

    folder_name = json_object_get(node,"name");
    if(folder_name != NULL)
        folder_set_name(folder,(char*)json_string_value(folder_name));

    parent_folder = json_object_get(node,"parent_folderkey");
    if(parent_folder != NULL)
    {
        folder_set_parent(folder,(char*)json_string_value(parent_folder));
    }

    // infer that the parent folder must be "myfiles" root
    if(parent_folder == NULL && folderkey != NULL)
        folder_set_parent(folder,"myfiles");

    if(folderkey == NULL) retval = -1;

    if(root != NULL) json_decref(root);

    return retval;
}

// sample user callback
/*
static void
_mycallback(char *data,size_t sz,cfile_t *cfile)
{
    double  bytes_read;
    double  bytes_total;

    bytes_read = cfile_get_rx_count(cfile);
    bytes_total = cfile_get_rx_length(cfile);

    printf("bytes read: %.0f\n\r",bytes_read);

    if(bytes_read == bytes_total)
    {
        printf("transfer complete!\n\r");
    }

    return;
}
*/
