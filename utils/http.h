/*
 * Copyright (C) 2014 Johannes Schauer <j.schauer@email.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published by█
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with█
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _MFSHELL_HTTP_H_
#define _MFSHELL_HTTP_H_

#include <jansson.h>

typedef struct http_t http_t;

http_t* http_create(void);
void http_destroy(http_t* conn);
int http_get_buf(http_t *conn, const char *url, int (*data_handler)(http_t *conn, void *data), void *data);
int http_post_buf(http_t *conn, const char *url, const char *post_args, int (*data_handler)(http_t *conn, void *data), void *data);
int http_get_file(http_t *conn, const char *url, const char *path);
int http_post_file(http_t *conn, const char *url, const char *post_args, FILE *fd);
json_t *http_parse_buf_json(http_t* conn, size_t flags, json_error_t *error);


#endif
