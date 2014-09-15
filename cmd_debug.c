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
#include <stdlib.h>

#include "strings.h"
#include "mfshell.h"
#include "private.h"
#include "command.h"

int
mfshell_cmd_debug(mfshell_t *mfshell)
{
    printf("   %-15.15s   %s\n\r",
        "server:",
        mfshell->server);

    if(mfshell->session_token != NULL && mfshell->secret_time != NULL)
    {
        printf("   %-15.15s   %u\n\r",
            "secret key:",
            mfshell->secret_key);

        printf("   %-15.15s   %s\n\r",
            "secret time:",
            mfshell->secret_time);

        printf("   %-15.15s   %s\n\r",
            "status:",
            "Authenticated");
    }
    else
    {
        printf("   %-15.15s   %s\n\r",
            "status:",
            "Not authenticated");
    }

    return 0;
}


