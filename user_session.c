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
#include <string.h>
#include <inttypes.h>

#include <curl/curl.h>
#include <jansson.h>

#include "mfshell.h"
#include "private.h"
#include "user_session.h"
#include "cfile.h"
#include "strings.h"
#include "json.h"

static int
_decode_get_session_token(mfshell_t *mfshell,cfile_t *cfile);

int
_get_session_token(mfshell_t *mfshell)
{
    cfile_t     *cfile;
    char        *login_url;
    char        *post_args;
    int         retval;

    if(mfshell == NULL) return -1;

    // create the object as a sender
    cfile = cfile_create();

    cfile_set_defaults(cfile);

    // cfile_set_opts(cfile,CFILE_OPT_ENABLE_SSL_LAX);

    // configure url for operation
    login_url = strdup_printf("https://%s/api/user/get_session_token.php",
        mfshell->server);
    cfile_set_url(cfile,login_url);
    free(login_url);

    // invalidate an existing user signature
    if(mfshell->user_signature != NULL)
    {
        free(mfshell->user_signature);
        mfshell->user_signature = NULL;
    }

    // create user signature
    if(mfshell->user_signature == NULL)
        mfshell->user_signature = mfshell->create_user_signature(mfshell);

    post_args = strdup_printf(
        "email=%s"
        "&password=%s"
        "&application_id=35860"
        "&signature=%s"
        "&token_version=2"
        "&response_format=json",
        mfshell->user,mfshell->passwd,mfshell->user_signature);

    cfile_set_args(cfile,CFILE_ARGS_POST,post_args);
    free(post_args);

    retval = cfile_exec(cfile);

    // print an error code if something went wrong
    if(retval != CURLE_OK) printf("error %d\n\r",retval);

    // printf("\n\r%s\n\r",cfile_get_rx_buffer(cfile));
    retval = _decode_get_session_token(mfshell,cfile);

    cfile_destroy(cfile);

    return retval;
}

static int
_decode_get_session_token(mfshell_t *mfshell,cfile_t *cfile)
{
    json_error_t    error;
    json_t          *root = NULL;
    json_t          *data;
    json_t          *session_token;
    json_t          *secret_key;
    json_t          *secret_time;

    if(mfshell == NULL) return -1;
    if(cfile == NULL) return -1;

    root = json_loads(cfile_get_rx_buffer(cfile),0,&error);

    data = json_object_by_path(root,"response");
    if(data == NULL) return -1;

    session_token = json_object_get(data,"session_token");
    if(session_token == NULL)
    {
        json_decref(root);
        return -1;
    }

    mfshell->session_token = strdup(json_string_value(session_token));

    secret_key = json_object_get(data,"secret_key");
    if(secret_key != NULL)
        mfshell->secret_key = atoll(json_string_value(secret_key));

    /*
        time looks like a float but we must store it as a string to
        remain congruent with the server on decimal place presentation.
    */
    secret_time = json_object_get(data,"time");
    if(secret_time != NULL)
       mfshell->secret_time = strdup(json_string_value(secret_time));

    if(root != NULL) json_decref(root);

    return 0;
}

// sample user callback
/*
static void
_mycallback(char *data,size_t sz,cfile_t *cfile)
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
