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


#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "private.h"

#ifndef file_t
#define file_t    _file_t
#endif

file_t*
file_alloc(void)
{
    file_t    *file;

    file = (file_t*)calloc(1,sizeof(file_t));

    return file;
}

void
file_free(file_t *file)
{
    if(file == NULL) return;

    if(file->share_link != NULL) free(file->share_link);
    if(file->direct_link != NULL) free(file->direct_link);
    if(file->onetime_link != NULL) free(file->onetime_link);

    free(file);

    return;
}

int
file_set_key(file_t *file,const char *key)
{
    int len;

    if(file == NULL) return -1;
    if(key == NULL) return -1;

    len = strlen(key);
    if(len != 11 && len != 15) return -1;

    memset(file->quickkey,0,sizeof(file->quickkey));
    strncpy(file->quickkey,key,sizeof(file->quickkey) - 1);

    return 0;
}

const char*
file_get_key(file_t *file)
{
    if(file == NULL) return NULL;

    return file->quickkey;
}

int
file_set_hash(file_t *file,const char *hash)
{
    if(file == NULL) return -1;
    if(hash == NULL) return -1;

    // system supports SHA256 (current) and MD5 (legacy)
    if(strlen(hash) < 32) return -1;

    memset(file->hash,0,sizeof(file->hash));
    strncpy(file->hash,hash,sizeof(file->hash) - 1);

    return 0;
}

const char*
file_get_hash(file_t *file)
{
    if(file == NULL) return NULL;

    return file->hash;
}

int
file_set_name(file_t *file,const char *name)
{
    if(file == NULL) return -1;
    if(name == NULL) return -1;

    if(strlen(name) > 255) return -1;

    memset(file->name,0,sizeof(file->name));
    strncpy(file->name,name,sizeof(file->name) - 1);

    return 0;
}

const char*
file_get_name(file_t *file)
{
    if(file == NULL) return NULL;

    return file->name;
}

int
file_set_share_link(file_t *file,const char *share_link)
{
    if(file == NULL) return -1;
    if(share_link == NULL) return -1;

    if(file->share_link != NULL)
    {
        free(file->share_link);
        file->share_link = NULL;
    }

    file->share_link = strdup(share_link);

    return 0;
}

const char*
file_get_share_link(file_t *file)
{
    if(file == NULL) return NULL;

    return file->share_link;
}

int
file_set_direct_link(file_t *file,const char *direct_link)
{
    if(file == NULL) return -1;
    if(direct_link == NULL) return -1;

    if(file->direct_link != NULL)
    {
        free(file->direct_link);
        file->direct_link = NULL;
    }

    file->direct_link = strdup(direct_link);

    return 0;
}

const char*
file_get_direct_link(file_t *file)
{
    if(file == NULL) return NULL;

    return file->direct_link;
}

int
file_set_onetime_link(file_t *file,const char *onetime_link)
{
    if(file == NULL) return -1;
    if(onetime_link == NULL) return -1;

    if(file->onetime_link != NULL)
    {
        free(file->onetime_link);
        file->onetime_link = NULL;
    }

    file->onetime_link = strdup(onetime_link);

    return 0;
}

const char*
file_get_onetime_link(file_t *file)
{
    if(file == NULL) return NULL;

    return file->onetime_link;
}

