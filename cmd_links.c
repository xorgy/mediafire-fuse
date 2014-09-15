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


#include <stdio.h>
#include <string.h>

#include "command.h"
#include "mfshell.h"
#include "private.h"

int
mfshell_cmd_links(mfshell_t *mfshell,const char *quickkey)
{
    extern int  term_width;
    file_t      *file;
    int         len;
    int         retval;

    if(mfshell == NULL) return -1;
    if(quickkey == NULL) return -1;

    len = strlen(quickkey);

    if(len != 11 && len != 15) return -1;

    file = file_alloc();

    retval = mfshell->file_get_links(mfshell,file,(char*)quickkey);
    mfshell->update_secret_key(mfshell);

    if(file->share_link != NULL)
        printf("   %-15.15s   %-*.*s\n\r",
        "sharing url:",
        term_width - 22,term_width -22,
        file->share_link);

    if(file->direct_link != NULL)
        printf("   %-15.15s   %-*.*s\n\r",
        "direct url:",
        term_width - 22,term_width -22,
        file->direct_link);

    if(file->onetime_link != NULL)
        printf("   %-15.15s   %-*.*s\n\r",
        "1-time url:",
        term_width - 22,term_width -22,
        file->onetime_link);

    file_free(file);

    return 0;
}

