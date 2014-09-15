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
