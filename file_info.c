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
_decode_file_get_info(mfshell_t *mfshell,cfile_t *cfile,file_t *file);

int
_file_get_info(mfshell_t *mfshell,file_t *file,char *quickkey)
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

    api_call = mfshell->create_signed_get(mfshell,0,"file/get_info.php",
        "?quick_key=%s"
        "&session_token=%s"
        "&response_format=json",
        quickkey,mfshell->session_token);

    cfile_set_url(cfile,api_call);

    retval = cfile_exec(cfile);

    if(retval != CURLE_OK) printf("error %d\n\r",retval);

    retval = _decode_file_get_info(mfshell,cfile,file);

    cfile_destroy(cfile);

    return retval;
}

static int
_decode_file_get_info(mfshell_t *mfshell,cfile_t *cfile,file_t *file)
{
    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *quickkey;
    json_t          *file_hash;
    json_t          *file_name;
    json_t          *file_folder;
    int             retval = 0;

    if(mfshell == NULL) return -1;
    if(cfile == NULL) return -1;

    root = json_loads(cfile_get_rx_buffer(cfile),0,&error);

    node = json_object_by_path(root,"response/file_info");

    quickkey = json_object_get(node,"quickkey");
    if(quickkey != NULL)
        file_set_key(file,(char*)json_string_value(quickkey));

    file_name = json_object_get(node,"filename");
    if(file_name != NULL)
        file_set_name(file,(char*)json_string_value(file_name));

    file_hash = json_object_get(node,"hash");
    if(file_hash != NULL)
    {
        file_set_hash(file,(char*)json_string_value(file_hash));
    }

    if(quickkey == NULL) retval = -1;

    if(root != NULL) json_decref(root);

    return retval;
}

