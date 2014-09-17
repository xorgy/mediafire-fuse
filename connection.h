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

#ifndef _MFSHELL_CONNECTION_H_
#define _MFSHELL_CONNECTION_H_

conn_t* conn_create(void);
void conn_destroy(conn_t* conn);
int conn_get_buf(conn_t *conn, const char *url, int (*data_handler)(conn_t *conn, void *data), void *data);
int conn_post_buf(conn_t *conn, const char *url, char *post_args, int (*data_handler)(conn_t *conn, void *data), void *data);
int conn_get_file(conn_t *conn, const char *url, char *path);
int conn_post_file(conn_t *conn, const char *url, char *post_args, FILE *fd);

#endif
