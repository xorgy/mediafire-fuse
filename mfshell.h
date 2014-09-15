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


#ifndef _MFSHELL_H_
#define _MFSHELL_H_

typedef struct  _mfshell_s      mfshell_t;
typedef struct  _folder_s       folder_t;
typedef struct  _file_s         file_t;

mfshell_t*  mfshell_create(int app_id,char *app_key,char *server);

int         mfshell_set_login(mfshell_t *mfshell,char *user,char *passwd);

int         mfshell_authenticate_user(mfshell_t *mfshell);


folder_t*   folder_alloc(void);

void        folder_free(folder_t *folder);

int         folder_set_key(folder_t *folder,const char *folderkey);

const char* folder_get_key(folder_t *folder);

int         folder_set_parent(folder_t *folder,const char *folderkey);

const char* folder_get_parent(folder_t *folder);

int         folder_set_name(folder_t *folder,const char *name);

const char* folder_get_name(folder_t *folder);


file_t*     file_alloc(void);

void        file_free(file_t *file);

int         file_set_key(file_t *file,const char *quickkey);

const char* file_get_key(file_t *file);

int         file_set_hash(file_t *file,const char *hash);

const char* file_get_hash(file_t *file);

int         file_set_name(file_t *file,const char *name);

const char* file_get_name(file_t *file);

int         file_set_share_link(file_t *file,const char *share_link);

const char* file_get_share_link(file_t *file);

int         file_set_direct_link(file_t *file,const char *direct_link);

const char* file_get_direct_link(file_t *file);

int         file_set_onetime_link(file_t *file,const char *onetime_link);

const char* file_get_onetime_link(file_t *file);


#endif

