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


