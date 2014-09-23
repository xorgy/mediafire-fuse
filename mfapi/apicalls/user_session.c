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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../utils/http.h"
#include "../../utils/json.h"
#include "../../utils/strings.h"
#include "../mfconn.h"
#include "../apicalls.h"        // IWYU pragma: keep

static int      _decode_get_session_token(mfhttp * conn, void *data);

struct user_get_session_token_response {
    uint32_t        secret_key;
    char           *secret_time;
    char           *session_token;
};

int
mfconn_api_user_get_session_token(mfconn * conn, const char *server,
                                  const char *username, const char *password,
                                  int app_id, const char *app_key,
                                  uint32_t * secret_key,
                                  char **secret_time,
                                  char **session_token)
{
    char           *login_url;
    char           *post_args;
    const char     *user_signature;
    int             retval;
    struct user_get_session_token_response response;
    mfhttp         *http;

    if (conn == NULL)
        return -1;

    // configure url for operation
    login_url = strdup_printf("https://%s/api/user/get_session_token.php",
                              server);

    // create user signature
    user_signature =
        mfconn_create_user_signature(conn, username, password, app_id, app_key);

    post_args = strdup_printf("email=%s"
                              "&password=%s"
                              "&application_id=%d"
                              "&signature=%s"
                              "&token_version=2"
                              "&response_format=json",
                              username, password, app_id, user_signature);
    free((void *)user_signature);

    http = http_create();
    retval =
        http_post_buf(http, login_url, post_args,
                      _decode_get_session_token, (void *)(&response));
    http_destroy(http);

    free(login_url);
    free(post_args);

    *secret_key = response.secret_key;
    *secret_time = response.secret_time;
    *session_token = response.session_token;

    return retval;
}

static int _decode_get_session_token(mfhttp * conn, void *user_ptr)
{
    json_error_t    error;
    json_t         *root = NULL;
    json_t         *data;
    json_t         *session_token;
    json_t         *secret_key;
    json_t         *secret_time;
    struct user_get_session_token_response *response;

    if (user_ptr == NULL)
        return -1;

    response = (struct user_get_session_token_response *)user_ptr;

    root = http_parse_buf_json(conn, 0, &error);

    data = json_object_by_path(root, "response");
    if (data == NULL)
        return -1;

    session_token = json_object_get(data, "session_token");
    if (session_token == NULL) {
        json_decref(root);
        return -1;
    }

    response->session_token = strdup(json_string_value(session_token));

    secret_key = json_object_get(data, "secret_key");
    if (secret_key != NULL)
        response->secret_key = atoll(json_string_value(secret_key));

    /*
       time looks like a float but we must store it as a string to
       remain congruent with the server on decimal place presentation.
     */
    secret_time = json_object_get(data, "time");
    if (secret_time != NULL)
        response->secret_time = strdup(json_string_value(secret_time));

    if (root != NULL)
        json_decref(root);

    return 0;
}

// sample user callback
/*
static void
_mycallback(char *data,size_t sz,cmffile *cfile)
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
