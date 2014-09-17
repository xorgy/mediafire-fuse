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

#include <stdlib.h>
#include <string.h>

#include "private.h"
#include "mfshell.h"

static int conn_progress_cb(void *user_ptr, double dltotal, double dlnow, double ultotal, double ulnow);
static size_t conn_read_buf_cb(char *data, size_t size, size_t nmemb, void *user_ptr);
static size_t conn_write_buf_cb(char *data, size_t size, size_t nmemb, void *user_ptr);
static size_t conn_write_file_cb(char *data, size_t size, size_t nmemb, void *user_ptr);

/*
 * This set of functions is made such that the conn_t struct and the curl
 * handle it stores can be reused for multiple operations
 *
 * We do not use a single global instance because mediafire does not support
 * keep-alive anyways.
 */

conn_t*
conn_create(void)
{
    conn_t *conn;
    CURL        *curl_handle;
    curl_handle = curl_easy_init();
    if(curl_handle == NULL) return NULL;

    conn = (conn_t*)calloc(1,sizeof(conn_t));
    conn->curl_handle = curl_handle;

    conn->show_progress = false;
    curl_easy_setopt(conn->curl_handle, CURLOPT_NOPROGRESS,0);
    curl_easy_setopt(conn->curl_handle, CURLOPT_PROGRESSFUNCTION, conn_progress_cb);
    curl_easy_setopt(conn->curl_handle, CURLOPT_PROGRESSDATA, (void *)conn);

    curl_easy_setopt(conn->curl_handle, CURLOPT_FOLLOWLOCATION, 1);

    curl_easy_setopt(conn->curl_handle, CURLOPT_SSLENGINE, NULL);
    curl_easy_setopt(conn->curl_handle, CURLOPT_SSLENGINE_DEFAULT, 1L);

    curl_easy_setopt(conn->curl_handle, CURLOPT_ERRORBUFFER, conn->error_buf);

    return conn;
}

void
conn_destroy(conn_t* conn)
{
    curl_easy_cleanup(conn->curl_handle);
    free(conn->write_buf);
    free(conn);
}

static int conn_progress_cb(void *user_ptr, double dltotal, double dlnow,
            double ultotal, double ulnow)
{
    conn_t *conn;

    if (user_ptr == NULL) return 0;

    conn = (conn_t *)user_ptr;

    conn->ul_len = ultotal;
    conn->ul_now = ulnow;

    conn->dl_len = dltotal;
    conn->dl_now = dlnow;

    return 0;
}

int
conn_get_buf(conn_t *conn, char *url, int (*data_handler)(conn_t *conn, void *data), void *data)
{
    int retval;
    curl_easy_reset(conn->curl_handle);
    curl_easy_setopt(conn->curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(conn->curl_handle, CURLOPT_READFUNCTION, conn_read_buf_cb);
    curl_easy_setopt(conn->curl_handle, CURLOPT_READDATA, (void*)conn);
    curl_easy_setopt(conn->curl_handle, CURLOPT_WRITEFUNCTION, conn_write_buf_cb);
    curl_easy_setopt(conn->curl_handle, CURLOPT_WRITEDATA, (void*)conn);
    retval = curl_easy_perform(conn->curl_handle);
    if(retval != CURLE_OK) {
        fprintf(stderr, "error curl_easy_perform %s\n\r", conn->error_buf);
        return retval;
    }
    if (data_handler != NULL)
        retval = data_handler(conn, data);
    conn->write_buf_len = 0;
    return retval;
}

static size_t
conn_read_buf_cb(char *data, size_t size, size_t nmemb, void *user_ptr)
{
    conn_t *conn;
    size_t data_len;

    if (user_ptr == NULL) return 0;

    conn = (conn_t*)user_ptr;
    data_len = size*nmemb;

    if (data_len > 0) {
        fprintf(stderr, "Not implemented");
        exit(1);
    }

    return 0;
}

static size_t
conn_write_buf_cb(char *data, size_t size, size_t nmemb, void *user_ptr)
{
    conn_t *conn;
    size_t data_len;

    if (user_ptr == NULL) return 0;

    conn = (conn_t*)user_ptr;
    data_len = size*nmemb;

    if (data_len > 0) {
        conn->write_buf = (char *)realloc(conn->write_buf,
                conn->write_buf_len + data_len);
        memcpy(conn->write_buf + conn->write_buf_len, data, data_len);
        conn->write_buf_len += data_len;
    }

    return data_len;
}

int
conn_post_buf(conn_t *conn, char *url, char *post_args, int (*data_handler)(conn_t *conn, void *data), void *data)
{
    int retval;
    curl_easy_reset(conn->curl_handle);
    curl_easy_setopt(conn->curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(conn->curl_handle, CURLOPT_READFUNCTION, conn_read_buf_cb);
    curl_easy_setopt(conn->curl_handle, CURLOPT_READDATA, (void*)conn);
    curl_easy_setopt(conn->curl_handle, CURLOPT_WRITEFUNCTION, conn_write_buf_cb);
    curl_easy_setopt(conn->curl_handle, CURLOPT_WRITEDATA, (void*)conn);
    curl_easy_setopt(conn->curl_handle, CURLOPT_POSTFIELDS, post_args);
    retval = curl_easy_perform(conn->curl_handle);
    if(retval != CURLE_OK) {
        fprintf(stderr, "error curl_easy_perform %s\n\r", conn->error_buf);
        return retval;
    }
    if (data_handler != NULL)
        retval = data_handler(conn, data);
    conn->write_buf_len = 0;
    return retval;
}

int
conn_get_file(conn_t *conn, char *url, char *path)
{
    int retval;
    curl_easy_reset(conn->curl_handle);
    curl_easy_setopt(conn->curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(conn->curl_handle, CURLOPT_READFUNCTION, conn_read_buf_cb);
    curl_easy_setopt(conn->curl_handle, CURLOPT_READDATA, (void*)conn);
    curl_easy_setopt(conn->curl_handle, CURLOPT_WRITEFUNCTION, conn_write_file_cb);
    curl_easy_setopt(conn->curl_handle, CURLOPT_WRITEDATA, (void*)conn);
    // FIXME: handle fopen() return value
    conn->stream = fopen(path, "w+");
    retval = curl_easy_perform(conn->curl_handle);
    fclose(conn->stream);
    if(retval != CURLE_OK) {
        fprintf(stderr, "error curl_easy_perform %s\n\r", conn->error_buf);
        return retval;
    }
    return retval;
}

static size_t
conn_write_file_cb(char *data, size_t size, size_t nmemb, void *user_ptr)
{
    conn_t *conn;

    if (user_ptr == NULL) return 0;
    conn = (conn_t*)user_ptr;

    fwrite(data, size, nmemb, conn->stream);

    fprintf(stderr, "\r   %.0f / %.0f", conn->dl_now, conn->dl_len);
}

int
conn_post_file(conn_t *conn, char *url, char *post_args, FILE *fd)
{
}
