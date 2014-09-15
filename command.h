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


#ifndef _MFSHELL_COMMAND_H_
#define _MFSHELL_COMMAND_H_

#include "mfshell.h"

void    mfshell_cmd_help(void);

int     mfshell_cmd_debug(mfshell_t *mfshell);

int     mfshell_cmd_host(mfshell_t *mfshell,const char *host);

int     mfshell_cmd_auth(mfshell_t *mfshell);

int     mfshell_cmd_whomai(mfshell_t *mfshell);

int     mfshell_cmd_list(mfshell_t *mfshell);

int     mfshell_cmd_chdir(mfshell_t *mfshell,const char *folderkey);

int     mfshell_cmd_pwd(mfshell_t *mfshell);

int     mfshell_cmd_lpwd(mfshell_t *mfshell);

int     mfshell_cmd_lcd(mfshell_t *mfshell,const char *dir);

int     mfshell_cmd_file(mfshell_t *mfshell,const char *quickkey);

int     mfshell_cmd_links(mfshell_t *mfshell,const char *quickkey);

int     mfshell_cmd_mkdir(mfshell_t *mfshell,const char *name);

int     mfshell_cmd_get(mfshell_t *mfshell,const char *quickkey);

#endif
