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
_decode_file_get_links(mfshell_t *mfshell,cfile_t *cfile,file_t *file);

int
_file_get_links(mfshell_t *mfshell,file_t *file,char *quickkey)
{
    cfile_t     *cfile;
    char        *api_call;
    int         retval;
    int         len;

    if(mfshell == NULL) return -1;
    if(mfshell->user_signature == NULL) return -1;
    if(mfshell->session_token == NULL) return -1;

    if(file == NULL) return -1;
    if(quickkey == NULL) return -1;

    len = strlen(quickkey);

    // key must either be 11 or 15 chars
    if(len != 11 && len != 15) return -1;

    // create the object as a sender
    cfile = cfile_create();

    // take the traditional defaults
    cfile_set_defaults(cfile);

    // cfile_set_opts(cfile,CFILE_OPT_ENABLE_SSL_LAX);

    api_call = mfshell->create_signed_get(mfshell,0,"file/get_links.php",
        "?quick_key=%s"
        "&session_token=%s"
        "&response_format=json",
        quickkey,mfshell->session_token);

    cfile_set_url(cfile,api_call);

    retval = cfile_exec(cfile);

    if(retval != CURLE_OK) printf("error %d\n\r",retval);

    retval = _decode_file_get_links(mfshell,cfile,file);

    cfile_destroy(cfile);

    return retval;
}

static int
_decode_file_get_links(mfshell_t *mfshell,cfile_t *cfile,file_t *file)
{
    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *quickkey;
    json_t          *share_link;
    json_t          *direct_link;
    json_t          *onetime_link;
    json_t          *links_array;
    int             retval = 0;

    if(mfshell == NULL) return -1;
    if(cfile == NULL) return -1;

    root = json_loads(cfile_get_rx_buffer(cfile),0,&error);

    node = json_object_by_path(root,"response");

    links_array = json_object_get(node,"links");
    if(!json_is_array(links_array))
    {
        json_decref(root);
        return -1;
    }

    // just get the first one.  maybe later support multi-quickkey
    node = json_array_get(links_array,0);

    quickkey = json_object_get(node,"quickkey");
    if(quickkey != NULL)
        file_set_key(file,(char*)json_string_value(quickkey));

    share_link = json_object_get(node,"normal_download");
    if(share_link != NULL)
        file_set_share_link(file,(char*)json_string_value(share_link));

    direct_link = json_object_get(node,"direct_download");
    if(direct_link != NULL)
    {
        file_set_direct_link(file,(char*)json_string_value(direct_link));
    }

    onetime_link = json_object_get(node,"one_time_download");
    if(onetime_link != NULL)
    {
        file_set_onetime_link(file,(char*)json_string_value(onetime_link));
    }

    // if this is false something went horribly wrong
    if(share_link == NULL) retval = -1;

    if(root != NULL) json_decref(root);

    return retval;
}

