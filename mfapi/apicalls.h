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

#ifndef _MFAPI_APICALLS_H_
#define _MFAPI_APICALLS_H_

#include <stdint.h>

#include "file.h"
#include "folder.h"
#include "patch.h"
#include "mfconn.h"

#define MFAPI_MAX_LEN_KEY 15
#define MFAPI_MAX_LEN_NAME 255

enum mfconn_device_change_type {
    MFCONN_DEVICE_CHANGE_DELETED_FOLDER,
    MFCONN_DEVICE_CHANGE_DELETED_FILE,
    MFCONN_DEVICE_CHANGE_UPDATED_FOLDER,
    MFCONN_DEVICE_CHANGE_UPDATED_FILE,
    MFCONN_DEVICE_CHANGE_END
};

struct mfconn_device_change {
    enum mfconn_device_change_type change;
    char            key[16];
    uint64_t        revision;
    char            parent[16];
};

int             mfconn_api_file_get_info(mfconn * conn, mffile * file,
                                         const char *quickkey);

int             mfconn_api_file_get_links(mfconn * conn, mffile * file,
                                          const char *quickkey);

int             mfconn_api_folder_create(mfconn * conn, const char *parent,
                                         const char *name);

long            mfconn_api_folder_get_content(mfconn * conn, int mode,
                                              const char *folderkey,
                                              mffolder *** folder_result,
                                              mffile *** file_result);

int             mfconn_api_folder_get_info(mfconn * conn, mffolder * folder,
                                           const char *folderkey);

int             mfconn_api_user_get_info(mfconn * conn);

int             mfconn_api_user_get_session_token(mfconn * conn,
                                                  const char *server,
                                                  const char *username,
                                                  const char *password,
                                                  int app_id,
                                                  const char *app_key,
                                                  uint32_t * secret_key,
                                                  char **secret_time,
                                                  char **session_token);

int             mfconn_api_folder_delete(mfconn * conn, const char *folderkey);

int             mfconn_api_file_delete(mfconn * conn, const char *quickkey);

int             mfconn_api_device_get_status(mfconn * conn,
                                             uint64_t * revision);

int             mfconn_api_device_get_changes(mfconn * conn, uint64_t revision, struct mfconn_device_change
                                              **changes);

int             mfconn_api_device_get_updates(mfconn * conn,
                                              const char *quickkey,
                                              uint64_t revision,
                                              uint64_t target_revision,
                                              mfpatch *** patches);

int             mfconn_api_device_get_patch(mfconn * conn, mfpatch * patch,
                                            const char *quickkey,
                                            uint64_t source_revision,
                                            uint64_t target_revision);

#endif
