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
#include "strings.h"
#include "download.h"
#include "connection.h"

ssize_t
download_direct(mfshell_t *mfshell, file_t *file, char *local_dir)
{
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

    if(local_dir[strlen(local_dir) - 1] == '/')
        file_path = strdup_printf("%s%s",local_dir,file_name);
    else
        file_path = strdup_printf("%s/%s",local_dir,file_name);

    conn_t *conn = conn_create();
    retval = conn_get_file(conn, url, file_path);
    conn_destroy(conn);

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
