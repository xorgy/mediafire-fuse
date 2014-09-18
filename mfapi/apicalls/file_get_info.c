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
#include "../../utils/strings.h"
#include "../../utils/json.h"
#include "../../utils/http.h"

static int
_decode_file_get_info(http_t *conn, void *data);

int
mfconn_api_file_get_info(mfconn_t *mfconn,file_t *file,char *quickkey)
{
    char        *api_call;
    int         retval;
    int         len;

    if(mfconn == NULL) return -1;

    if(file == NULL) return -1;
    if(quickkey == NULL) return -1;

    len = strlen(quickkey);

    // key must either be 11 or 15 chars
    if(len != 11 && len != 15) return -1;

    api_call = mfconn_create_signed_get(mfconn, 1, "file/get_info.php",
        "?quick_key=%s&response_format=json", quickkey);

    http_t *conn = http_create();
    retval = http_get_buf(conn, api_call, _decode_file_get_info, file);
    http_destroy(conn);

    return retval;
}

static int
_decode_file_get_info(http_t *conn, void *data)
{
    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *quickkey;
    json_t          *file_hash;
    json_t          *file_name;
    json_t          *file_folder;
    int             retval = 0;
    file_t         *file;

    if(data == NULL) return -1;

    file = (file_t *)data;

    root = http_parse_buf_json(conn, 0, &error);

    node = json_object_by_path(root,"response/file_info");

    quickkey = json_object_get(node,"quickkey");
    if(quickkey != NULL)
        file_set_key(file,(char*)json_string_value(quickkey));

    file_name = json_object_get(node,"filename");
    if(file_name != NULL)
        file_set_name(file,(char*)json_string_value(file_name));

    file_hash = json_object_get(node,"hash");
    if(file_hash != NULL)
    {
        file_set_hash(file,(char*)json_string_value(file_hash));
    }

    if(quickkey == NULL) retval = -1;

    if(root != NULL) json_decref(root);

    return retval;
}

