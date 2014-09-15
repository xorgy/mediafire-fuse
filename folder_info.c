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
_decode_folder_get_info(mfshell_t *mfshell,cfile_t *cfile,folder_t *folder);

int
_folder_get_info(mfshell_t *mfshell,folder_t *folder,char *folderkey)
{
    cfile_t     *cfile;
    char        *api_call;
    int         retval;

    if(mfshell == NULL) return -1;
    if(mfshell->user_signature == NULL) return -1;
    if(mfshell->session_token == NULL) return -1;

    if(folder == NULL) return -1;
    if(folderkey == NULL) return -1;

    // key must either be 11 chars or "myfiles"
    if(strlen(folderkey) != 13)
    {
        if(strcmp(folderkey,"myfiles") == 0) return -1;
    }

    // create the object as a sender
    cfile = cfile_create();

    // take the traditional defaults
    cfile_set_defaults(cfile);

    api_call = mfshell->create_signed_get(mfshell,0,"folder/get_info.php",
        "?folder_key=%s"
        "&session_token=%s"
        "&response_format=json",
        folderkey,mfshell->session_token);

    cfile_set_url(cfile,api_call);

    retval = cfile_exec(cfile);

    if(retval != CURLE_OK) printf("error %d\n\r",retval);

    retval = _decode_folder_get_info(mfshell,cfile,folder);

    cfile_destroy(cfile);

    return retval;
}

static int
_decode_folder_get_info(mfshell_t *mfshell,cfile_t *cfile,folder_t *folder)
{
    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *folderkey;
    json_t          *folder_name;
    json_t          *parent_folder;
    int             retval = 0;

    if(mfshell == NULL) return -1;
    if(cfile == NULL) return -1;

    root = json_loads(cfile_get_rx_buffer(cfile),0,&error);

    node = json_object_by_path(root,"response/folder_info");

    folderkey = json_object_get(node,"folderkey");
    if(folderkey != NULL)
        folder_set_key(folder,(char*)json_string_value(folderkey));

    folder_name = json_object_get(node,"name");
    if(folder_name != NULL)
        folder_set_name(folder,(char*)json_string_value(folder_name));

    parent_folder = json_object_get(node,"parent_folderkey");
    if(parent_folder != NULL)
    {
        folder_set_parent(folder,(char*)json_string_value(parent_folder));
    }

    // infer that the parent folder must be "myfiles" root
    if(parent_folder == NULL && folderkey != NULL)
        folder_set_parent(folder,"myfiles");

    if(folderkey == NULL) retval = -1;

    if(root != NULL) json_decref(root);

    return retval;
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
