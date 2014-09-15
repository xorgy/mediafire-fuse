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


#ifndef _CFILE_H_
#define _CFILE_H_

#include <stdint.h>

typedef struct _cfile_s             cfile_t;

typedef void (*CFileFunc)           (cfile_t *cfile);

#define CFILE_MODE_TEXT             1
#define CFILE_MODE_BINARY           2

#define CFILE_OPT_FOLLOW_REDIRECTS  (1 << 0)
#define CFILE_OPT_ENABLE_SSL        (1 << 1)
#define CFILE_OPT_ENABLE_SSL_LAX    (1 << 2)

enum
{
    CFILE_ARGS_POST,
    CFILE_ARGS_GET,
    CFILE_ARGS_HEADER,
};

cfile_t*    cfile_create(void);

int         cfile_exec(cfile_t *cfile);

void        cfile_destroy(cfile_t *cfile);

void        cfile_set_mode(cfile_t *cfile,int mode);

void        cfile_set_defaults(cfile_t *cfile);

void        cfile_set_url(cfile_t *cfile,const char *url);

int         cfile_get_url(cfile_t *cfile,char *buf,size_t buf_sz);

void        cfile_set_args(cfile_t *cfile,int type,const char *args);

void        cfile_add_args(cfile_t *cfile,int type,const char *args);

int         cfile_set_opts(cfile_t *cfile,uint32_t opts);

uint32_t    cfile_get_opts(cfile_t *cfile);

void        cfile_set_io_func(cfile_t *cfile,CFileFunc io_func);

void        cfile_set_progress_func(cfile_t *cfile,CFileFunc progress);

const char* cfile_get_rx_buffer(cfile_t *cfile);

size_t      cfile_get_rx_buffer_size(cfile_t *cfile);

void        cfile_reset_rx_buffer(cfile_t *cfile);

const char* cfile_get_tx_buffer(cfile_t *cfile);

size_t      cfile_get_tx_buffer_size(cfile_t *cfile);

void        cfile_reset_tx_buffer(cfile_t *cfile);

void        cfile_copy_buffer(cfile_t *cfile,char *buffer,size_t sz);

double      cfile_get_tx_length(cfile_t *cfile);

double      cfile_get_rx_length(cfile_t *cfile);

double      cfile_get_rx_count(cfile_t *cfile);

double      cfile_get_tx_count(cfile_t *cfile);

void        cfile_set_userptr(cfile_t *cfile,void *anything);

void*       cfile_get_userptr(cfile_t *cfile);

#endif

