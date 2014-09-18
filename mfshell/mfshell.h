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


#ifndef _MFSHELL_H_
#define _MFSHELL_H_

#include "../mfapi/folder.h"
#include "../mfapi/mfconn.h"

typedef struct _cmd_s       _cmd_t;
typedef struct _mfshell_s   mfshell_t;

struct _cmd_s
{
    char *name;
    char *argstring;
    char *help;
    int  (*handler) (mfshell_t *mfshell, int argc, char **argv);
};

struct _mfshell_s
{
    int         app_id;
    char        *app_key;
    char        *server;

    /* REST API tracking */
    folder_t   *folder_curr;

    /* Local tracking */
    char        *local_working_dir;

    /* shell commands */
    _cmd_t      *commands;

    mfconn_t    *mfconn;
};

typedef struct  _mfshell_s      mfshell_t;
typedef struct  _folder_s       folder_t;
typedef struct  _cmd_s          cmd_t;

mfshell_t*  mfshell_create(int app_id,char *app_key,char *server);

int         mfshell_authenticate_user(mfshell_t *mfshell);

int mfshell_exec(mfshell_t *mfshell, int argc, char **argv);

int mfshell_exec_shell_command(mfshell_t *mfshell,char *command);

#endif

