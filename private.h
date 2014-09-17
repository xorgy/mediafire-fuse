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


#ifndef _MFSHELL_PRIVATE_H_
#define _MFSHELL_PRIVATE_H_

#include <inttypes.h>
#include <stdbool.h>
#include <curl/curl.h>

typedef struct _mfshell_s   _mfshell_t;
typedef struct _folder_s    _folder_t;
typedef struct _file_s      _file_t;
typedef struct _cmd_s       _cmd_t;
typedef struct _conn_s      _conn_t;

struct _cmd_s
{
    char *name;
    char *argstring;
    char *help;
    int  (*handler) (_mfshell_t *mfshell, int argc, char **argv);
};

struct _folder_s
{
    char        folderkey[20];
    char        name[41];
    char        parent[20];
    uint64_t    revision;
    uint32_t    folder_count;
    uint32_t    file_count;
};

struct _file_s
{
    char        quickkey[18];
    char        hash[65];
    char        name[256];
    char        mtime[16];
    uint64_t    revision;

    char        *share_link;
    char        *direct_link;
    char        *onetime_link;
};

struct _conn_s
{
    CURL       *curl_handle;
    char       *write_buf;
    size_t      write_buf_len;
    double      ul_len;
    double      ul_now;
    double      dl_len;
    double      dl_now;
    bool        show_progress;
    char        error_buf[CURL_ERROR_SIZE];
    FILE       *stream;
};

struct _mfshell_s
{
    int         app_id;
    char        *app_key;
    char        *server;
    char        *user;
    char        *passwd;

    char        *user_signature;

    char        *session_token;
    char        *secret_time;
    uint32_t    secret_key;

    void        (*update_secret_key)        (_mfshell_t *);

    //char*       (*set_sever)                (_mfshell_t *,const char*);
    char*       (*set_login)                (_mfshell_t*,const char*);
    char*       (*set_passwd)               (_mfshell_t*,const char*);

    char*       (*create_user_signature)    (_mfshell_t*);
    char*       (*create_call_signature)    (_mfshell_t*,char*,char*);
    char*       (*create_signed_get)        (_mfshell_t*,int,char*,char*,...);
    char*       (*create_signed_post)       (_mfshell_t*,int,char*,char*,...);

    int         (*exec)                     (_mfshell_t*, int argc, char **argv);
    int         (*exec_string)              (_mfshell_t*,char*);

    /* REST API calls */
    int         (*get_session_token)        (_mfshell_t*);

    int         (*user_get_info)            (_mfshell_t*);

    long        (*folder_get_content)       (_mfshell_t*,int);
    int         (*folder_get_info)          (_mfshell_t*,_folder_t*,char*);
    int         (*folder_create)            (_mfshell_t*,char*,char*);

    int         (*file_get_info)            (_mfshell_t*,_file_t*,char*);
    int         (*file_get_links)           (_mfshell_t*,_file_t*,char*);

    /* REST API tracking */
    _folder_t   *folder_curr;

    /* Local tracking */
    char        *local_working_dir;

    /* shell commands */
    _cmd_t      *commands;
};

void
_update_secret_key(_mfshell_t *mfshell);

char*
_create_user_signature(_mfshell_t *mfshell);

char*
_create_call_signature(_mfshell_t *mfshell,char *url,char *args);

char*
_create_signed_get(_mfshell_t *mfshell,int ssl,char *api,char *fmt,...);

int
_execute(_mfshell_t *mfshell, int argc, char **argv);

int
_execute_shell_command(_mfshell_t *mfshell,char *command);

#endif
