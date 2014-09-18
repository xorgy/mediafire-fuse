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
_decode_file_get_links(http_t *conn, void *data);

int
mfconn_api_file_get_links(mfconn_t *mfconn,file_t *file,char *quickkey)
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

    api_call = mfconn_create_signed_get(mfconn,0,"file/get_links.php",
        "?quick_key=%s&response_format=json", quickkey);

    http_t *conn = http_create();
    retval = http_get_buf(conn, api_call, _decode_file_get_links, file);
    http_destroy(conn);

    return retval;
}

static int
_decode_file_get_links(http_t *conn, void *data)
{
    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *quickkey;
    json_t          *share_link;
    json_t          *direct_link;
    json_t          *onetime_link;
    json_t          *links_array;
    int             retval = 0;
    file_t         *file;

    if(data == NULL) return -1;

    file = (file_t *)data;

    root = http_parse_buf_json(conn, 0, &error);

    node = json_object_by_path(root,"response");

    links_array = json_object_get(node,"links");
    if(!json_is_array(links_array))
    {
        json_decref(root);
        return -1;
    }

    // just get the first one.  maybe later support multi-quickkey
    node = json_array_get(links_array,0);

    quickkey = json_object_get(node,"quickkey");
    if(quickkey != NULL)
        file_set_key(file,(char*)json_string_value(quickkey));

    share_link = json_object_get(node,"normal_download");
    if(share_link != NULL)
        file_set_share_link(file,(char*)json_string_value(share_link));

    direct_link = json_object_get(node,"direct_download");
    if(direct_link != NULL)
    {
        file_set_direct_link(file,(char*)json_string_value(direct_link));
    }

    onetime_link = json_object_get(node,"one_time_download");
    if(onetime_link != NULL)
    {
        file_set_onetime_link(file,(char*)json_string_value(onetime_link));
    }

    // if this is false something went horribly wrong
    if(share_link == NULL) retval = -1;

    if(root != NULL) json_decref(root);

    return retval;
}

