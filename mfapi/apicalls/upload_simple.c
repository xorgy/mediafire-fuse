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

#define _POSIX_C_SOURCE 200809L // for strdup

#include <jansson.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <openssl/sha.h>

#include "../../utils/http.h"
#include "../../utils/hash.h"
#include "../mfconn.h"
#include "../apicalls.h"        // IWYU pragma: keep

static int      _decode_upload_simple(mfhttp * conn, void *data);

int
mfconn_api_upload_simple(mfconn * conn, const char *folderkey,
                         FILE * fh, const char *file_name, char **upload_key)
{
    const char     *api_call;
    int             retval;
    mfhttp         *http;
    unsigned char   hash[SHA256_DIGEST_LENGTH];
    char           *file_hash;
    uint64_t        file_size;

    if (conn == NULL)
        return -1;

    if (fh == NULL)
        return -1;

    // make sure that we are at the beginning of the file
    rewind(fh);

    // calculate hash
    retval = calc_sha256(fh, hash, &file_size);

    if (retval != 0) {
        fprintf(stderr, "failed to calculate hash\n");
        return -1;
    }

    file_hash = binary2hex(hash, SHA256_DIGEST_LENGTH);

    if (folderkey == NULL) {
        api_call = mfconn_create_signed_get(conn, 0,
                                            "upload/simple.php",
                                            "?response_format=json");
    } else {
        api_call = mfconn_create_signed_get(conn, 0,
                                            "upload/simple.php",
                                            "?response_format=json"
                                            "&folder_key=%s", folderkey);
    }

    // make sure that we are at the beginning of the file
    rewind(fh);

    http = http_create();
    retval = http_post_file(http, api_call, fh, file_name,
                            file_size, file_hash,
                            _decode_upload_simple, upload_key);
    http_destroy(http);
    mfconn_update_secret_key(conn);

    free(file_hash);
    free((void *)api_call);

    return retval;
}

static int _decode_upload_simple(mfhttp * conn, void *user_ptr)
{
    json_error_t    error;
    json_t         *root;
    json_t         *node;
    json_t         *j_obj;
    int             retval;

    char          **upload_key;

    upload_key = (char **)user_ptr;
    if (upload_key == NULL)
        return -1;

    root = http_parse_buf_json(conn, 0, &error);

    if (root == NULL) {
        fprintf(stderr, "http_parse_buf_json failed at line %d\n", error.line);
        fprintf(stderr, "error message: %s\n", error.text);
        return -1;
    }

    node = json_object_get(root, "response");

    retval = mfapi_check_response(node, "upload/simple");
    if (retval != 0) {
        fprintf(stderr, "invalid response\n");
        json_decref(root);
        return retval;
    }

    node = json_object_get(node, "doupload");

    j_obj = json_object_get(node, "key");
    if (j_obj != NULL) {
        if (strcmp(json_string_value(j_obj), "") == 0) {
            *upload_key = NULL;
        } else {
            *upload_key = strdup(json_string_value(j_obj));
        }
    } else {
        *upload_key = NULL;
    }

    json_decref(root);

    return 0;
}
