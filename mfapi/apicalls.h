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
#include "mfconn.h"

int             mfconn_api_file_get_info(mfconn * conn, mffile * file,
                                         char *quickkey);

int             mfconn_api_file_get_links(mfconn * conn, mffile * file,
                                          char *quickkey);

int             mfconn_api_folder_create(mfconn * conn, char *parent,
                                         char *name);

long            mfconn_api_folder_get_content(mfconn * conn, int mode,
                                              mffolder * folder_curr);

int             mfconn_api_folder_get_info(mfconn * conn, mffolder * folder,
                                           char *folderkey);

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

#endif
