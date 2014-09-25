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

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../../utils/http.h"
#include "../../utils/json.h"
#include "../../utils/strings.h"
#include "../folder.h"
#include "../mfconn.h"
#include "../apicalls.h"        // IWYU pragma: keep

static int      _decode_device_get_changes(mfhttp * conn, void *data);

int mfconn_api_device_get_changes(mfconn * conn, uint64_t revision)
{
    const char     *api_call;
    int             retval;
    mfhttp         *http;

    if (conn == NULL)
        return -1;

    api_call = mfconn_create_signed_get(conn, 0, "device/get_changes.php",
                                        "?revision=%" PRIu64
                                        "&response_format=json", revision);

    http = http_create();
    retval = http_get_buf(http, api_call, _decode_device_get_changes, NULL);
    http_destroy(http);

    free((void *)api_call);

    return retval;
}

static int _decode_device_get_changes(mfhttp * conn, void *user_ptr)
{
    json_error_t    error;
    json_t         *root;
    json_t         *node;
    json_t         *data;

    json_t         *obj_array;
    json_t         *key;
    json_t         *parent;
    json_t         *revision;

    int             array_sz;
    int             i = 0;

    if (user_ptr != NULL)
        return -1;

    root = http_parse_buf_json(conn, 0, &error);

    node = json_object_by_path(root, "response/updated");

    obj_array = json_object_get(node, "files");
    if (json_is_array(obj_array)) {
        array_sz = json_array_size(obj_array);
        printf("updated files:\n");
        for (i = 0; i < array_sz; i++) {
            data = json_array_get(obj_array, i);

            if (json_is_object(data)) {
                key = json_object_get(data, "quickkey");
                parent = json_object_get(data, "parent_folderkey");
                revision = json_object_get(data, "revision");

                printf("   %s   %s    %s\n\r", json_string_value(key),
                        json_string_value(parent), json_string_value(revision));
            }
        }
        printf("\n");
    }

    node = json_object_by_path(root, "response/updated");

    obj_array = json_object_get(node, "folders");
    if (json_is_array(obj_array)) {
        array_sz = json_array_size(obj_array);
        printf("updated folders:\n");
        for (i = 0; i < array_sz; i++) {
            data = json_array_get(obj_array, i);

            if (json_is_object(data)) {
                key = json_object_get(data, "folderkey");
                parent = json_object_get(data, "parent_folderkey");
                revision = json_object_get(data, "revision");

                printf("   %s   %s    %s\n\r", json_string_value(key),
                        json_string_value(parent), json_string_value(revision));
            }
        }
        printf("\n");
    }

    node = json_object_by_path(root, "response/deleted");

    obj_array = json_object_get(node, "files");
    if (json_is_array(obj_array)) {
        array_sz = json_array_size(obj_array);
        printf("deleted files:\n");
        for (i = 0; i < array_sz; i++) {
            data = json_array_get(obj_array, i);

            if (json_is_object(data)) {
                key = json_object_get(data, "quickkey");
                parent = json_object_get(data, "parent_folderkey");
                revision = json_object_get(data, "revision");

                printf("   %s   %s    %s\n\r", json_string_value(key),
                        json_string_value(parent), json_string_value(revision));
            }
        }
        printf("\n");
    }

    node = json_object_by_path(root, "response/deleted");

    obj_array = json_object_get(node, "folders");
    if (json_is_array(obj_array)) {
        array_sz = json_array_size(obj_array);
        printf("deleted folders:\n");
        for (i = 0; i < array_sz; i++) {
            data = json_array_get(obj_array, i);

            if (json_is_object(data)) {
                key = json_object_get(data, "folderkey");
                parent = json_object_get(data, "parent_folderkey");
                revision = json_object_get(data, "revision");

                printf("   %s   %s    %s\n\r", json_string_value(key),
                        json_string_value(parent), json_string_value(revision));
            }
        }
        printf("\n");
    }

    if (root != NULL)
        json_decref(root);

    return 0;
}
