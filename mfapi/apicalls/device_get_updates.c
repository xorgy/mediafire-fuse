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

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../utils/http.h"
#include "../../utils/json.h"
#include "../patch.h"
#include "../mfconn.h"
#include "../apicalls.h"        // IWYU pragma: keep

static int      _decode_device_get_updates(mfhttp * conn, void *data);

int mfconn_api_device_get_updates(mfconn * conn, const char *quickkey,
                                  uint64_t revision, uint64_t target_revision,
                                  mfpatch *** patches)
{
    const char     *api_call;
    int             len;
    mfhttp         *http;
    int             retval;

    if (conn == NULL)
        return -1;

    if (patches == NULL)
        return -1;

    if (quickkey == NULL)
        return -1;

    len = strlen(quickkey);

    if (len != 15)
        return -1;

    if (target_revision == 0) {
        api_call = mfconn_create_signed_get(conn, 0, "device/get_updates.php",
                                            "?quick_key=%s"
                                            "&revision=%" PRIu64
                                            "&response_format=json", quickkey,
                                            revision);
    } else {
        api_call = mfconn_create_signed_get(conn, 0, "device/get_updates.php",
                                            "?quick_key=%s"
                                            "&revision=%" PRIu64
                                            "&target_revision=%" PRIu64
                                            "&response_format=json", quickkey,
                                            revision, target_revision);
    }
    http = http_create();
    retval = http_get_buf(http, api_call, _decode_device_get_updates,
                          (void *)patches);
    http_destroy(http);
    mfconn_update_secret_key(conn);

    free((void *)api_call);

    return retval;
}

static int _decode_device_get_updates(mfhttp * conn, void *user_ptr)
{
    json_error_t    error;
    json_t         *root;
    json_t         *node;
    json_t         *obj_array;
    json_t         *data;

    json_t         *source_revision;
    json_t         *target_revision;
    json_t         *source_hash;
    json_t         *target_hash;
    json_t         *patch_hash;

    int             array_sz;
    int             i;
    int             retval;

    mfpatch      ***patches;
    mfpatch        *tmp_patch;
    size_t          len_patches;

    if (user_ptr == NULL)
        return -1;

    patches = (mfpatch ***) user_ptr;

    root = http_parse_buf_json(conn, 0, &error);

    if (root == NULL) {
        fprintf(stderr, "http_parse_buf_json failed at line %d\n", error.line);
        fprintf(stderr, "error message: %s\n", error.text);
        return -1;
    }

    node = json_object_by_path(root, "response");

    retval = mfapi_check_response(node, "device/get_updates");
    if (retval != 0) {
        fprintf(stderr, "invalid response\n");
        json_decref(root);
        return retval;
    }

    len_patches = 0;
    obj_array = json_object_get(node, "updates");
    if (json_is_array(obj_array)) {
        array_sz = json_array_size(obj_array);
        for (i = 0; i < array_sz; i++) {
            data = json_array_get(obj_array, i);
            if (!json_is_object(data))
                continue;
            source_revision = json_object_get(data, "source_revision");
            target_revision = json_object_get(data, "target_revision");
            source_hash = json_object_get(data, "source_hash");
            target_hash = json_object_get(data, "target_hash");
            patch_hash = json_object_get(data, "patch_hash");
            if (source_revision == NULL || target_revision == NULL
                || source_hash == NULL || target_hash == NULL
                || patch_hash == NULL) {
                fprintf(stderr, "patch with missing info\n");
            }
            tmp_patch = patch_alloc();
            patch_set_source_revision(tmp_patch,
                                      atoll(json_string_value
                                            (source_revision)));
            patch_set_target_revision(tmp_patch,
                                      atoll(json_string_value
                                            (target_revision)));
            patch_set_source_hash(tmp_patch, json_string_value(source_hash));
            patch_set_target_hash(tmp_patch, json_string_value(target_hash));
            patch_set_hash(tmp_patch, json_string_value(patch_hash));
            len_patches++;
            *patches = (mfpatch **) realloc(*patches,
                                            len_patches * sizeof(mfpatch *));
            (*patches)[len_patches - 1] = tmp_patch;
        }
    }
    // append a terminating empty patch
    len_patches++;
    *patches = (mfpatch **) realloc(*patches, len_patches * sizeof(mfpatch *));
    // write an empty last element
    (*patches)[len_patches - 1] = NULL;

    json_decref(root);

    return 0;
}
