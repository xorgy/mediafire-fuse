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


#ifndef _MFSHELL_PRIVATE_H_
#define _MFSHELL_PRIVATE_H_

#include <inttypes.h>

typedef struct _mfshell_s   _mfshell_t;
typedef struct _folder_s    _folder_t;
typedef struct _file_s      _file_t;

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

    int         (*exec)                     (_mfshell_t*,char*);

    /* console commands */
    void        (*help)                     (void);
    int         (*debug)                    (_mfshell_t*);
    int         (*whoami)                   (_mfshell_t*);
    int         (*list)                     (_mfshell_t*);
    int         (*chdir)                    (_mfshell_t*,const char*);
    int         (*pwd)                      (_mfshell_t*);
    int         (*file)                     (_mfshell_t*,const char*);
    int         (*links)                    (_mfshell_t*,const char*);
    int         (*host)                     (_mfshell_t*,const char*);
    int         (*auth)                     (_mfshell_t*);
    int         (*get)                      (_mfshell_t*,const char*);
    int         (*lpwd)                     (_mfshell_t*);
    int         (*lcd)                      (_mfshell_t*,const char*);
    int         (*mkdir)                    (_mfshell_t*,const char*);

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
_execute_shell_command(_mfshell_t *mfshell,char *command);

#endif
