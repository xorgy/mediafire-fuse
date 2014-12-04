/*
 * Copyright (C) 2014 Johannes Schauer <j.schauer@email.de>
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

#define _POSIX_C_SOURCE 200809L // for PATH_MAX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "../../mfapi/apicalls.h"
#include "../mfshell.h"
#include "../../mfapi/mfconn.h"
#include "../../mfapi/folder.h"
#include "../commands.h"        // IWYU pragma: keep

int mfshell_cmd_put(mfshell * mfshell, int argc, char *const argv[])
{
    int             retval;
    const char     *file_path;
    char           *temp;
    char           *file_name;
    char           *upload_key;
    int             status;
    int             fileerror;

    if (mfshell == NULL)
        return -1;

    if (mfshell->conn == NULL) {
        fprintf(stderr, "conn is NULL\n");
        return -1;
    }

    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        return -1;
    }

    if (argv[1] == NULL)
        return -1;

    file_path = argv[1];

    // create copies because basename modifies it
    temp = strdup(argv[1]);
    file_name = basename(temp);

    retval = mfconn_api_upload_simple(mfshell->conn,
                                      folder_get_key(mfshell->folder_curr),
                                      file_path, file_name, &upload_key);
    mfconn_update_secret_key(mfshell->conn);

    free(temp);

    if (retval != 0) {
        fprintf(stderr, "mfconn_api_upload_simple failed\n");
        return -1;
    }

    fprintf(stderr, "upload_key: %s\n", upload_key);

    for (;;) {
        retval = mfconn_api_upload_poll_upload(mfshell->conn, upload_key,
                                               &status, &fileerror);
        if (retval != 0) {
            fprintf(stderr, "mfconn_api_upload_poll_upload failed\n");
            return -1;
        }
        fprintf(stderr, "status: %d, filerror: %d\n", status, fileerror);
        if (status == 99) {
            fprintf(stderr, "done\n");
            break;
        }
        sleep(1);
    }

    free(upload_key);

    return 0;
}
