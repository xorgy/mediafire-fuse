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

#include "../apicalls.h"
#include "../mfconn.h"
#include "../folder.h"
#include "../../utils/strings.h"
#include "../../utils/json.h"
#include "../../utils/http.h"

static int
_decode_folder_get_content_folders(mfhttp *conn, void *data);

static int
_decode_folder_get_content_files(mfhttp *conn, void *data);

long
mfconn_api_folder_get_content(mfconn *conn, int mode, mffolder *folder_curr)
{
    char        *api_call;
    int         retval;
    char        *content_type;

    if(conn == NULL) return -1;

    if(mode == 0)
        content_type = "folders";
    else
        content_type = "files";

    const char *folderkey = folder_get_key(folder_curr);
    if (folderkey == NULL) {
        fprintf(stderr, "folder_get_key NULL\n");
        return 0;
    }
    /*if (folderkey[0] == '\0') {
        fprintf(stderr, "folder_get_key '\\0'\n");
        return 0;
    }*/
    api_call = mfconn_create_signed_get(conn,0,"folder/get_content.php",
        "?folder_key=%s"
        "&content_type=%s"
        "&response_format=json",
        folderkey,
        content_type);

    mfhttp* http = http_create();

    if(mode == 0)
        retval = http_get_buf(http, api_call, _decode_folder_get_content_folders, NULL);
    else
        retval = http_get_buf(http, api_call, _decode_folder_get_content_files, NULL);
    http_destroy(http);

    return retval;
}

static int
_decode_folder_get_content_folders(mfhttp *conn, void *user_ptr)
{
    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *data;

    json_t          *folders_array;
    json_t          *folderkey;
    json_t          *folder_name;
    char            *folder_name_tmp;

    int             array_sz;
    int             i = 0;

    if (user_ptr != NULL)
        return -1;

    root = http_parse_buf_json(conn, 0, &error);

    /*json_t *result = json_object_by_path(root, "response/action");
    fprintf(stderr, "response/action: %s\n", (char*)json_string_value(result));*/

    node = json_object_by_path(root,"response/folder_content");

    folders_array = json_object_get(node,"folders");
    if(!json_is_array(folders_array))
    {
        json_decref(root);
        return -1;
    }

    array_sz = json_array_size(folders_array);
    for(i = 0;i < array_sz;i++)
    {
        data = json_array_get(folders_array,i);

        if(json_is_object(data))
        {
            folderkey = json_object_get(data,"folderkey");

            folder_name = json_object_get(data,"name");

            if(folderkey != NULL && folder_name != NULL)
            {
                folder_name_tmp = strdup_printf("< %s >",
                    json_string_value(folder_name));

                printf("   %-15.13s   %s\n\r",
                    json_string_value(folderkey),
                    folder_name_tmp);

                free(folder_name_tmp);
            }
        }
    }

    if(root != NULL) json_decref(root);

    return 0;
}

static int
_decode_folder_get_content_files(mfhttp *conn, void *user_ptr)
{
    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *data;

    json_t          *files_array;
    json_t          *quickkey;
    json_t          *file_name;
    int             array_sz;
    int             i = 0;

    if (user_ptr != NULL)
        return -1;

    root = http_parse_buf_json(conn, 0, &error);

    node = json_object_by_path(root,"response/folder_content");

    files_array = json_object_get(node,"files");
    if(!json_is_array(files_array))
    {
        json_decref(root);
        return -1;
    }

    array_sz = json_array_size(files_array);
    for(i = 0;i < array_sz;i++)
    {
        data = json_array_get(files_array,i);

        if(json_is_object(data))
        {
            quickkey = json_object_get(data,"quickkey");

            file_name = json_object_get(data,"filename");

            if(quickkey != NULL && file_name != NULL)
            {
                printf("   %-15.15s   %s\n\r",
                    json_string_value(quickkey),
                    json_string_value(file_name));
            }
        }
    }

    if(root != NULL) json_decref(root);

    return 0;
}
