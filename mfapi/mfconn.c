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

#include <curl/curl.h>
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#include "../utils/strings.h"
#include "mfconn.h"
#include "apicalls.h"

typedef struct _mfconn_s mfconn_t;

struct _mfconn_s
{
    char        *server;
    uint32_t    secret_key;
    char        *secret_time;
    char        *session_token;
};

mfconn_t*
mfconn_create(char *server, char *username, char *password, int app_id, char *app_key)
{
    mfconn_t *mfconn;
    int retval;

    mfconn = (mfconn_t *)calloc(1,sizeof(mfconn_t));

    mfconn->server = strdup(server);
    retval = mfconn_api_user_get_session_token(mfconn, mfconn->server,
            username, password, app_id, app_key, &(mfconn->secret_key),
            &(mfconn->secret_time), &(mfconn->session_token));

    if (retval == 0)
        return mfconn;
    else
        return NULL;
}

void
mfconn_destroy(mfconn_t *mfconn)
{
    free(mfconn->server);
    free(mfconn->secret_time);
    free(mfconn->session_token);
    free(mfconn);
}

void
mfconn_update_secret_key(mfconn_t *mfconn)
{
    uint64_t    new_val;

    if(mfconn == NULL) return;

    new_val = ((uint64_t)mfconn->secret_key) * 16807;
    new_val %= 0x7FFFFFFF;

    mfconn->secret_key = new_val;

    return;
}

char*
mfconn_create_user_signature(mfconn_t *mfconn, char *username, char *password,
        int app_id, char *app_key)
{
    char            *signature_raw;
    unsigned char   signature_enc[20];      // sha1 is 160 bits
    char            signature_hex[41];
    int             i;

    if(mfconn == NULL) return NULL;

    signature_raw = strdup_printf("%s%s%d%s",
        username, password, app_id, app_key);

    SHA1((const unsigned char *)signature_raw,
        strlen(signature_raw),signature_enc);

    free(signature_raw);

    for(i = 0;i < 20;i++)
    {
        sprintf(&signature_hex[i*2],"%02x",signature_enc[i]);
    }
    signature_hex[40] = '\0';

    return strdup((const char *)signature_hex);
}

char*
mfconn_create_call_signature(mfconn_t *mfconn,char *url,char *args)
{
    char            *signature_raw;
    unsigned char   signature_enc[16];      // md5 is 128 bits
    char            signature_hex[33];
    char            *api;
    int             i;

    if(mfconn == NULL) return NULL;
    if(url == NULL) return NULL;
    if(args == NULL) return NULL;

    // printf("url: %s\n\rargs: %s\n\r",url,args);

    api = strstr(url,"/api/");

    if(api == NULL) return NULL;

    signature_raw = strdup_printf("%d%s%s%s",
        (mfconn->secret_key % 256),
        mfconn->secret_time,
        api,args);

    MD5((const unsigned char *)signature_raw,
        strlen(signature_raw),signature_enc);

    free(signature_raw);

    for(i = 0;i < 16;i++)
    {
        sprintf(&signature_hex[i*2],"%02x",signature_enc[i]);
    }
    signature_hex[32] = '\0';

    return strdup((const char *)signature_hex);
}

char*
mfconn_create_signed_get(mfconn_t *mfconn,int ssl,char *api,char *fmt,...)
{
    char        *api_request = NULL;
    char        *api_args = NULL;
    char        *signature;
    char        *call_hash;
    char        *session_token;
    int         bytes_to_alloc;
    int         api_args_len;
    int         api_len;
    va_list     ap;

    if(mfconn == NULL) return NULL;
    if(mfconn->server == NULL) return NULL;
    if(mfconn->secret_time == NULL) return NULL;
    if(mfconn->session_token == NULL) return NULL;

    // make sure the api (ex: user/get_info.php) is sane
    if(api == NULL) return NULL;
    api_len = strlen(api);
    if(api_len < 3) return NULL;

    // calculate how big of a buffer we need
    va_start(ap, fmt);
    api_args_len = (vsnprintf(NULL, 0, fmt, ap) + 1);    // + 1 for NULL
    va_end(ap);

    session_token = strdup_printf("&session_token=%s", mfconn->session_token);

    api_args_len += strlen(session_token);

    // create the correctly sized buffer and process the args
    api_args = (char*)calloc(api_args_len,sizeof(char));

    // printf("\n\r%d\n\r",api_args_len);

    va_start(ap, fmt);
    vsnprintf(api_args, api_args_len, fmt, ap);
    va_end(ap);

    strcat(api_args, session_token);
    free(session_token);

    // correct user error of trailing slash
    if(api[api_len - 1] == '/') api[api_len - 1] = '\0';

    api_request = strdup_printf("%s//%s/api/%s",
        (ssl ? "https:" : "http:"), mfconn->server,api);

    call_hash = mfconn_create_call_signature(mfconn,api_request,api_args);
    signature = strdup_printf("&signature=%s",call_hash);
    free(call_hash);

    // compute the amount of space requred to realloc() the request
    bytes_to_alloc = api_args_len;
    bytes_to_alloc += strlen(api_request);
    bytes_to_alloc += strlen(signature);
    bytes_to_alloc += 1;                    // null termination

    // append api GET args to api request
    api_request = (char*)realloc(api_request,bytes_to_alloc);
    strncat(api_request,api_args,api_args_len);
    strcat(api_request,signature);

    free(signature);
    free(api_args);

    return api_request;
}

const char*
mfconn_get_session_token(mfconn_t *mfconn)
{
    return mfconn->session_token;
}

const char*
mfconn_get_secret_time(mfconn_t *mfconn)
{
    return mfconn->secret_time;
}

uint32_t
mfconn_get_secret_key(mfconn_t *mfconn)
{
    return mfconn->secret_key;
}
