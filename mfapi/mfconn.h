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

#ifndef __MFAPI_MFCONN_H__
#define __MFAPI_MFCONN_H__

#include <stdint.h>

#include "file.h"

typedef struct _mfconn_s mfconn_t;

struct _mfconn_s;

mfconn_t* mfconn_create(char *server, char *username, char *password, int app_id, char *app_key);

void mfconn_destroy(mfconn_t *mfconn);

ssize_t mfconn_download_direct(file_t *file,char *local_dir);

char* mfconn_create_signed_get(mfconn_t *mfconn,int ssl,char *api,char *fmt,...);

char* mfconn_create_user_signature(mfconn_t *mfconn, char *username,
        char *password, int app_id, char *app_key);

void mfconn_update_secret_key(mfconn_t *mfconn);

const char* mfconn_get_session_token(mfconn_t *mfconn);

const char* mfconn_get_secret_time(mfconn_t *mfconn);

uint32_t mfconn_get_secret_key(mfconn_t *mfconn);
#endif
