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

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/strings.h"
#include "apicalls.h"
#include "mfconn.h"

struct mfconn {
    char           *server;
    uint32_t        secret_key;
    char           *secret_time;
    char           *session_token;
};

mfconn         *mfconn_create(char *server, char *username, char *password,
                              int app_id, char *app_key)
{
    mfconn         *conn;
    int             retval;

    conn = (mfconn *) calloc(1, sizeof(mfconn));

    conn->server = strdup(server);
    retval = mfconn_api_user_get_session_token(conn, conn->server,
                                               username, password, app_id,
                                               app_key,
                                               &(conn->secret_key),
                                               &(conn->secret_time),
                                               &(conn->session_token));

    if (retval == 0)
        return conn;
    else
        return NULL;
}

void mfconn_destroy(mfconn * conn)
{
    free(conn->server);
    free(conn->secret_time);
    free(conn->session_token);
    free(conn);
}

void mfconn_update_secret_key(mfconn * conn)
{
    uint64_t        new_val;

    if (conn == NULL)
        return;

    new_val = ((uint64_t) conn->secret_key) * 16807;
    new_val %= 0x7FFFFFFF;

    conn->secret_key = new_val;

    return;
}

char           *mfconn_create_user_signature(mfconn * conn, char *username,
                                             char *password, int app_id,
                                             char *app_key)
{
    char           *signature_raw;
    unsigned char   signature_enc[20];  // sha1 is 160 bits
    char            signature_hex[41];
    int             i;

    if (conn == NULL)
        return NULL;

    signature_raw = strdup_printf("%s%s%d%s",
                                  username, password, app_id, app_key);

    SHA1((const unsigned char *)signature_raw,
         strlen(signature_raw), signature_enc);

    free(signature_raw);

    for (i = 0; i < 20; i++) {
        sprintf(&signature_hex[i * 2], "%02x", signature_enc[i]);
    }
    signature_hex[40] = '\0';

    return strdup((const char *)signature_hex);
}

char           *mfconn_create_call_signature(mfconn * conn, char *url,
                                             char *args)
{
    char           *signature_raw;
    unsigned char   signature_enc[16];  // md5 is 128 bits
    char            signature_hex[33];
    char           *api;
    int             i;

    if (conn == NULL)
        return NULL;
    if (url == NULL)
        return NULL;
    if (args == NULL)
        return NULL;

    // printf("url: %s\n\rargs: %s\n\r",url,args);

    api = strstr(url, "/api/");

    if (api == NULL)
        return NULL;

    signature_raw = strdup_printf("%d%s%s%s",
                                  (conn->secret_key % 256),
                                  conn->secret_time, api, args);

    MD5((const unsigned char *)signature_raw,
        strlen(signature_raw), signature_enc);

    free(signature_raw);

    for (i = 0; i < 16; i++) {
        sprintf(&signature_hex[i * 2], "%02x", signature_enc[i]);
    }
    signature_hex[32] = '\0';

    return strdup((const char *)signature_hex);
}

char           *mfconn_create_signed_get(mfconn * conn, int ssl, char *api,
                                         char *fmt, ...)
{
    char           *api_request = NULL;
    char           *api_args = NULL;
    char           *signature;
    char           *call_hash;
    char           *session_token;
    int             bytes_to_alloc;
    int             api_args_len;
    int             api_len;
    va_list         ap;

    if (conn == NULL)
        return NULL;
    if (conn->server == NULL)
        return NULL;
    if (conn->secret_time == NULL)
        return NULL;
    if (conn->session_token == NULL)
        return NULL;

    // make sure the api (ex: user/get_info.php) is sane
    if (api == NULL)
        return NULL;
    api_len = strlen(api);
    if (api_len < 3)
        return NULL;

    // calculate how big of a buffer we need
    va_start(ap, fmt);
    api_args_len = (vsnprintf(NULL, 0, fmt, ap) + 1);   // + 1 for NULL
    va_end(ap);

    session_token = strdup_printf("&session_token=%s", conn->session_token);

    api_args_len += strlen(session_token);

    // create the correctly sized buffer and process the args
    api_args = (char *)calloc(api_args_len, sizeof(char));

    // printf("\n\r%d\n\r",api_args_len);

    va_start(ap, fmt);
    vsnprintf(api_args, api_args_len, fmt, ap);
    va_end(ap);

    strcat(api_args, session_token);
    free(session_token);

    // correct user error of trailing slash
    if (api[api_len - 1] == '/')
        api[api_len - 1] = '\0';

    api_request = strdup_printf("%s//%s/api/%s",
                                (ssl ? "https:" : "http:"), conn->server, api);

    call_hash = mfconn_create_call_signature(conn, api_request, api_args);
    signature = strdup_printf("&signature=%s", call_hash);
    free(call_hash);

    // compute the amount of space requred to realloc() the request
    bytes_to_alloc = api_args_len;
    bytes_to_alloc += strlen(api_request);
    bytes_to_alloc += strlen(signature);
    bytes_to_alloc += 1;        // null termination

    // append api GET args to api request
    api_request = (char *)realloc(api_request, bytes_to_alloc);
    strncat(api_request, api_args, api_args_len);
    strcat(api_request, signature);

    free(signature);
    free(api_args);

    return api_request;
}

const char     *mfconn_get_session_token(mfconn * conn)
{
    return conn->session_token;
}

const char     *mfconn_get_secret_time(mfconn * conn)
{
    return conn->secret_time;
}

uint32_t mfconn_get_secret_key(mfconn * conn)
{
    return conn->secret_key;
}
