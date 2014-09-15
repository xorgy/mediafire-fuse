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
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include "mfshell.h"
#include "private.h"
#include "account.h"
#include "cfile.h"
#include "strings.h"
#include "download.h"

void
_download_direct_cbio(cfile_t *cfile);

void
_download_direct_cbprogress(cfile_t *cfile);

ssize_t
download_direct(file_t *file,char *local_dir)
{
    cfile_t         *cfile;
    const char      *url;
    const char      *file_name;
    char            *file_path;
    struct stat     file_info;
    ssize_t         bytes_read = 0;
    int             retval;

    if(file == NULL) return -1;
    if(local_dir == NULL) return -1;

    url = file_get_direct_link(file);
    if(url == NULL) return -1;

    file_name = file_get_name(file);
    if(file_name == NULL) return -1;
    if(strlen(file_name) < 1) return -1;

    // create the object as a sender
    cfile = cfile_create();

    // take the defaults but switch to binary mode
    cfile_set_defaults(cfile);
    cfile_set_mode(cfile,CFILE_MODE_BINARY);

    cfile_set_url(cfile,url);
    cfile_set_io_func(cfile,_download_direct_cbio);
    cfile_set_progress_func(cfile,_download_direct_cbprogress);

    if(local_dir[strlen(local_dir) - 1] == '/')
        file_path = strdup_printf("%s%s",local_dir,file_name);
    else
        file_path = strdup_printf("%s/%s",local_dir,file_name);

    cfile_set_userptr(cfile,(void*)file_path);

    retval = cfile_exec(cfile);
    cfile_destroy(cfile);

    if(retval != CURLE_OK)
    {
        free(file_path);
        return -1;
    }

    /*
        it is preferable to have the vfs tell us how many bytes the
        transfer actually is.  it's really all that matters.
    */
    memset(&file_info,0,sizeof(file_info));
    retval = stat(file_path,&file_info);

    free(file_path);

    if(retval != 0) return -1;

    bytes_read = file_info.st_size;

    return bytes_read;
}

void
_download_direct_cbio(cfile_t *cfile)
{
    FILE        *file;
    char        *file_path;
    size_t      bytes_ready = 0;
    const char  *rx_buffer;

    if(cfile == NULL) return;

    file_path = (char*)cfile_get_userptr(cfile);
    if(file_path == NULL) return;

    bytes_ready = cfile_get_rx_buffer_size(cfile);
    if(bytes_ready == 0) return;

    file = fopen(file_path,"a+");

    if(file != NULL)
    {
        rx_buffer = cfile_get_rx_buffer(cfile);
        fwrite((const void*)rx_buffer,sizeof(char),bytes_ready,file);
    }

    fclose(file);

    cfile_reset_rx_buffer(cfile);

    return;
}

void
_download_direct_cbprogress(cfile_t *cfile)
{
    double      total;
    double      recv;

    if(cfile == NULL) return;

    total = cfile_get_rx_length(cfile);
    recv = cfile_get_rx_count(cfile);

    printf("\r   %.0f / %.0f",recv,total);

    return;
}
