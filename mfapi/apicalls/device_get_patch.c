/*
 * Copyright (C) 2014 Johannes Schauer <j.schauer@email.de>
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>

#include "../../utils/http.h"
#include "../../utils/json.h"
#include "../mfconn.h"
#include "../patch.h"
#include "../apicalls.h"        // IWYU pragma: keep

static int      _decode_device_get_patch(mfhttp * conn, void *data);

int mfconn_api_device_get_patch(mfconn * conn, mfpatch * patch,
                                const char *quickkey, uint64_t source_revision,
                                uint64_t target_revision)
{
    const char     *api_call;
    int             len;
    mfhttp         *http;
    int             retval;

    if (conn == NULL)
        return -1;

    if (patch == NULL)
        return -1;

    if (quickkey == NULL)
        return -1;

    len = strlen(quickkey);

    if (len != 15)
        return -1;

    api_call = mfconn_create_signed_get(conn, 0, "device/get_patch.php",
                                        "?quick_key=%s"
                                        "&source_revision=%" PRIu64
                                        "&target_revision=%" PRIu64
                                        "&response_format=json", quickkey,
                                        source_revision, target_revision);

    http = http_create();
    retval = http_get_buf(http, api_call, _decode_device_get_patch,
                          (void *)patch);
    http_destroy(http);
    mfconn_update_secret_key(conn);

    free((void *)api_call);

    return retval;
}

static int _decode_device_get_patch(mfhttp * conn, void *data)
{
    json_error_t    error;
    json_t         *obj;
    mfpatch        *patch;
    json_t         *root;
    json_t         *node;
    int             retval;

    if (data == NULL)
        return -1;

    patch = (mfpatch *) data;

    root = http_parse_buf_json(conn, 0, &error);

    if (root == NULL) {
        fprintf(stderr, "http_parse_buf_json failed at line %d\n", error.line);
        fprintf(stderr, "error message: %s\n", error.text);
        return -1;
    }

    node = json_object_by_path(root, "response");

    retval = mfapi_check_response(node, "device/get_patch");
    if (retval != 0) {
        fprintf(stderr, "invalid response\n");
        json_decref(root);
        return retval;
    }

    obj = json_object_get(node, "patch_hash");
    if (obj != NULL)
        patch_set_hash(patch, json_string_value(obj));

    obj = json_object_get(node, "patch_link");
    if (obj != NULL)
        patch_set_link(patch, json_string_value(obj));

    json_decref(root);

    return 0;
}
