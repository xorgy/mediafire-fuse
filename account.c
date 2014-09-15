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
#include "account.h"
#include "cfile.h"
#include "strings.h"
#include "json.h"

static int
_decode_user_get_info(mfshell_t *mfshell,cfile_t *cfile);

int
_user_get_info(mfshell_t *mfshell)
{
    cfile_t     *cfile;
    char        *api_call;
    int         retval;
    // char        *rx_buffer;

    if(mfshell == NULL) return -1;
    if(mfshell->user_signature == NULL) return -1;

    // create the object as a sender
    cfile = cfile_create();

    // take the traditional defaults
    cfile_set_defaults(cfile);

    api_call = mfshell->create_signed_get(mfshell,0,"user/get_info.php",
        "?session_token=%s"
        "&response_format=json",
        mfshell->session_token);

    cfile_set_url(cfile,api_call);

    retval = cfile_exec(cfile);

    // print an error code if something went wrong
    if(retval != CURLE_OK) printf("error %d\n\r",retval);

    retval = _decode_user_get_info(mfshell,cfile);

    cfile_destroy(cfile);

    return retval;
}

static int
_decode_user_get_info(mfshell_t *mfshell,cfile_t *cfile)
{
    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *email;
    json_t          *first_name;
    json_t          *last_name;

    if(mfshell == NULL) return -1;
    if(cfile == NULL) return -1;

    root = json_loads(cfile_get_rx_buffer(cfile),0,&error);

    node = json_object_by_path(root,"response/user_info");

    email = json_object_get(node,"email");
    if(email != NULL)
        printf("Email: %s\n\r",(char*)json_string_value(email));

    first_name = json_object_get(node,"first_name");
    if(first_name != NULL)
        printf("Name: %s ",(char*)json_string_value(first_name));

    last_name = json_object_get(node,"last_name");
    if(node != NULL)
        printf("%s",(char*)json_string_value(last_name));

    printf("\n\r");

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
