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


#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../mfapi/apicalls.h"
#include "../mfshell.h"
#include "../../mfapi/file.h"
#include "../../mfapi/mfconn.h"
#include "../commands.h" // IWYU pragma: keep

int
mfshell_cmd_get(mfshell *mfshell, int argc, char **argv)
{
    mffile      *file;
    int         len;
    int         retval;
    ssize_t     bytes_read;
    const char *quickkey;

    if(mfshell == NULL) return -1;

    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        return -1;
    }

    quickkey = argv[1];
    if(quickkey == NULL) return -1;

    len = strlen(quickkey);

    if(len != 11 && len != 15) return -1;

    file = file_alloc();

    // get file name
    retval = mfconn_api_file_get_info(mfshell->conn,file,(char*)quickkey);
    mfconn_update_secret_key(mfshell->conn);

    if(retval == -1)
    {
        file_free(file);
        return -1;
    }

    // request a direct download (streaming) link
    retval = mfconn_api_file_get_links(mfshell->conn,file,(char*)quickkey);
    mfconn_update_secret_key(mfshell->conn);

    if(retval == -1)
    {
        file_free(file);
        return -1;
    }

    // make sure we have a valid working directory to download to
    if(mfshell->local_working_dir == NULL)
    {
        mfshell->local_working_dir = (char*)calloc(PATH_MAX + 1,sizeof(char));
        getcwd(mfshell->local_working_dir,PATH_MAX);
    }

    bytes_read = file_download_direct(file, mfshell->local_working_dir);

    if(bytes_read != -1)
        printf("\r   Downloaded %zd bytes OK!\n\r",bytes_read);
    else
        printf("\r\n   Download FAILED!\n\r");

    file_free(file);

    return 0;
}

