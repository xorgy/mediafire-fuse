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
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "../../utils/http.h"
#include "../mfconn.h"
#include "../apicalls.h"        // IWYU pragma: keep

struct upload_poll_upload_response {
    int             status;
    int             fileerror;
};

static int      _decode_upload_poll_upload(mfhttp * conn, void *data);

int
mfconn_api_upload_poll_upload(mfconn * conn, const char *upload_key,
                              int *status, int *fileerror)
{
    const char     *api_call;
    int             retval;
    mfhttp         *http;
    struct upload_poll_upload_response response;

    if (conn == NULL)
        return -1;

    if (upload_key == NULL)
        return -1;

    // make an UNSIGNED get
    api_call = mfconn_create_unsigned_get(conn, 0,
                                          "upload/poll_upload.php",
                                          "?response_format=json"
                                          "&key=%s", upload_key);

    http = http_create();
    retval = http_get_buf(http, api_call, _decode_upload_poll_upload,
                          &response);
    http_destroy(http);

    free((void *)api_call);

    *status = response.status;
    *fileerror = response.fileerror;

    return retval;
}

static int _decode_upload_poll_upload(mfhttp * conn, void *user_ptr)
{
    json_error_t    error;
    json_t         *root;
    json_t         *node;
    json_t         *j_obj;
    int             retval;

    struct upload_poll_upload_response *response;

    response = (struct upload_poll_upload_response *)user_ptr;
    if (response == NULL)
        return -1;

    response->status = 0;
    response->fileerror = 0;

    root = http_parse_buf_json(conn, 0, &error);

    if (root == NULL) {
        fprintf(stderr, "http_parse_buf_json failed at line %d\n", error.line);
        fprintf(stderr, "error message: %s\n", error.text);
        return -1;
    }

    node = json_object_get(root, "response");

    retval = mfapi_check_response(node, "upload/poll_upload");
    if (retval != 0) {
        fprintf(stderr, "invalid response\n");
        json_decref(root);
        return retval;
    }

    node = json_object_get(node, "doupload");

    // make sure that the result code is zero (success)
    j_obj = json_object_get(node, "result");
    if (j_obj == NULL || strcmp(json_string_value(j_obj), "0") != 0) {
        json_decref(root);
        return -1;
    }

    j_obj = json_object_get(node, "status");
    if (j_obj != NULL) {
        response->status = atol(json_string_value(j_obj));
    }

    j_obj = json_object_get(node, "fileerror");
    if (j_obj != NULL) {
        response->fileerror = atol(json_string_value(j_obj));
    }

    json_decref(root);

    return 0;
}
