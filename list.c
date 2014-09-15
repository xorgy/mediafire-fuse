#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <curl/curl.h>
#include <jansson.h>

#include "mfshell.h"
#include "private.h"
#include "cfile.h"
#include "strings.h"
#include "json.h"
#include "list.h"

static int
_decode_folder_get_content_folders(mfshell_t *mfshell,cfile_t *cfile);

static int
_decode_folder_get_content_files(mfshell_t *mfshell,cfile_t *cfile);

long
_folder_get_content(mfshell_t *mfshell,int mode)
{
    cfile_t     *cfile;
    char        *api_call;
    int         retval;
    char        *rx_buffer;
    char        *content_type;

    if(mfshell == NULL) return -1;
    if(mfshell->user_signature == NULL) return -1;
    if(mfshell->session_token == NULL) return -1;

    // create the object as a sender
    cfile = cfile_create();

    // take the traditional defaults
    cfile_set_defaults(cfile);

    // cfile_set_opts(cfile,CFILE_OPT_ENABLE_SSL_LAX);

    if(mode == 0)
        content_type = "folders";
    else
        content_type = "files";

    api_call = mfshell->create_signed_get(mfshell,0,"folder/get_content.php",
        "?session_token=%s"
        "&folder_key=%s"
        "&content_type=%s"
        "&response_format=json",
        mfshell->session_token,
        folder_get_key(mfshell->folder_curr),
        content_type);

    cfile_set_url(cfile,api_call);

    retval = cfile_exec(cfile);

    // print an error code if something went wrong
    if(retval != CURLE_OK) printf("error %d\n\r",retval);

    // rx_buffer = cfile_get_rx_buffer(cfile);
    // printf("\n\r%s\n\r",rx_buffer);

    if(mode == 0)
        retval = _decode_folder_get_content_folders(mfshell,cfile);
    else
        retval = _decode_folder_get_content_files(mfshell,cfile);

    cfile_destroy(cfile);



    return retval;
}

static int
_decode_folder_get_content_folders(mfshell_t *mfshell,cfile_t *cfile)
{
    extern int      term_width;

    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *data;

    json_t          *folders_array;
    json_t          *folderkey;
    json_t          *folder_name;
    char            *folder_name_tmp;

    int             array_sz;
    int             i = 0;

    if(mfshell == NULL) return -1;
    if(cfile == NULL) return -1;

    root = json_loads(cfile_get_rx_buffer(cfile),0,&error);

    node = json_object_by_path(root,"response/folder_content");

    folders_array = json_object_get(node,"folders");
    if(!json_is_array(folders_array))
    {
        json_decref(root);
        return -1;
    }

    array_sz = json_array_size(folders_array);
    for(i = 0;i < array_sz;i++)
    {
        data = json_array_get(folders_array,i);

        if(json_is_object(data))
        {
            folderkey = json_object_get(data,"folderkey");

            folder_name = json_object_get(data,"name");

            if(folderkey != NULL && folder_name != NULL)
            {
                folder_name_tmp = strdup_printf("< %s >",
                    json_string_value(folder_name));

                printf("   %-15.13s   %-*.*s\n\r",
                    json_string_value(folderkey),
                    term_width - 22,term_width - 22,
                    folder_name_tmp);

                free(folder_name_tmp);
            }
        }
    }

    if(root != NULL) json_decref(root);

    return 0;
}

static int
_decode_folder_get_content_files(mfshell_t *mfshell,cfile_t *cfile)
{
    extern int      term_width;

    json_error_t    error;
    json_t          *root;
    json_t          *node;
    json_t          *data;

    json_t          *files_array;
    json_t          *quickkey;
    json_t          *file_name;
    int             array_sz;
    int             i = 0;

    if(mfshell == NULL) return -1;
    if(cfile == NULL) return -1;

    root = json_loads(cfile_get_rx_buffer(cfile),0,&error);

    node = json_object_by_path(root,"response/folder_content");

    files_array = json_object_get(node,"files");
    if(!json_is_array(files_array))
    {
        json_decref(root);
        return -1;
    }

    array_sz = json_array_size(files_array);
    for(i = 0;i < array_sz;i++)
    {
        data = json_array_get(files_array,i);

        if(json_is_object(data))
        {
            quickkey = json_object_get(data,"quickkey");

            file_name = json_object_get(data,"filename");

            if(quickkey != NULL && file_name != NULL)
            {
                printf("   %-15.15s   %-*.*s\n\r",
                    json_string_value(quickkey),
                    term_width - 22,term_width - 22,
                    json_string_value(file_name));
            }
        }
    }

    if(root != NULL) json_decref(root);

    return 0;
}
