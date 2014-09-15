/*
 * Copyright (C) 2013 Bryan Christ <bryan.christ@mediafire.com>
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

#include "cfile.h"
#include "strings.h"
#include "private.h"

#ifndef mfshell_t
#define mfshell_t _mfshell_t
#endif

char*
_create_user_signature(mfshell_t *mfshell)
{
    char            *signature_raw;
    unsigned char   signature_enc[20];      // sha1 is 160 bits
    unsigned char   signature_hex[41];
    int             i;

    if(mfshell == NULL) return NULL;

    if(mfshell->app_id <= 0) return NULL;
    if(mfshell->app_key == NULL) return NULL;
    if(mfshell->user == NULL) return NULL;
    if(mfshell->passwd == NULL) return NULL;

    signature_raw = strdup_printf("%s%s%d%s",
        mfshell->user,mfshell->passwd,mfshell->app_id,mfshell->app_key);

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
_create_call_signature(mfshell_t *mfshell,char *url,char *args)
{
    char            *signature_raw;
    unsigned char   signature_enc[16];      // md5 is 128 bits
    unsigned char   signature_hex[33];
    char            *api;
    int             i;

    if(mfshell == NULL) return NULL;
    if(url == NULL) return NULL;
    if(args == NULL) return NULL;

    // printf("url: %s\n\rargs: %s\n\r",url,args);

    api = strstr(url,"/api/");

    if(api == NULL) return NULL;

    signature_raw = strdup_printf("%d%s%s%s",
        (mfshell->secret_key % 256),
        mfshell->secret_time,
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
_create_signed_get(mfshell_t *mfshell,int ssl,char *api,char *fmt,...)
{
    char        *api_request = NULL;
    char        *api_args = NULL;
    char        *signature;
    char        *call_hash;
    int         bytes_to_alloc;
    int         api_request_len;
    int         api_args_len;
    int         api_len;
    va_list     ap;

    if(mfshell == NULL) return NULL;
    if(mfshell->server == NULL) return NULL;
    if(mfshell->secret_time == NULL) return NULL;
    if(mfshell->session_token == NULL) return NULL;

    // make sure the api (ex: user/get_info.php) is sane
    if(api == NULL) return NULL;
    api_len = strlen(api);
    if(api_len < 3) return NULL;

    // calculate how big of a buffer we need
    va_start(ap, fmt);
    api_args_len = (vsnprintf(NULL, 0, fmt, ap) + 1);    // + 1 for NULL
    va_end(ap);

    // create the correctly sized buffer and process the args
    api_args = (char*)calloc(api_args_len,sizeof(char));

    // printf("\n\r%d\n\r",api_args_len);

    va_start(ap, fmt);
    vsnprintf(api_args, api_args_len, fmt, ap);
    va_end(ap);

    // correct user error of trailing slash
    if(api[api_len - 1] == '/') api[api_len - 1] = '\0';

    api_request = strdup_printf("%s//%s/api/%s",
        (ssl ? "https:" : "http:"), mfshell->server,api);

    call_hash = mfshell->create_call_signature(mfshell,api_request,api_args);
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
