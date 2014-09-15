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

void
mfshell_cmd_help(void)
{
    printf(
        "  arguments:\n\r"
        "           <optional>\n\r"
        "           [required]\n\r");

    printf("\n\r");

    printf(
        "   help                    show this help\n\r"
        "   debug                   show debug information\n\r"

        "   host    <server>        change target server\n\r"
        "   auth                    authenticate with active server\n\r"
        "   whoami                  show basic user info\n\r"

        "   ls                      show contents of active folder\n\r"
        "   cd      [folderkey]     change active folder\n\r"
        "   pwd                     show the active folder\n\r"
        "   lpwd                    show the local working directory\n\r"
        "   lcd     [dir]           change the local working directory\n\r"
        "   mkdir   [folder name]   create a new folder\n\r"

        "   file    [quickkey]      show file information\n\r"
        "   links   [quickkey]      show access urls for the file\n\r"
        "   get     [quickkey]      download a file\n\r");

    return;
}


