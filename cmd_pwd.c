#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "strings.h"
#include "mfshell.h"
#include "private.h"
#include "command.h"

int
mfshell_cmd_pwd(mfshell_t *mfshell)
{
    const char  *folder_name;
    char        *folder_name_tmp = NULL;

    if(mfshell == NULL) return -1;
    if(mfshell->folder_curr == NULL) return -1;

    folder_name = folder_get_name(mfshell->folder_curr);
    if(folder_name[0] == '\0') return -1;

    folder_name_tmp = strdup_printf("< %s >",folder_name);

    printf("%-15.13s   %-50.50s\n\r",
        folder_get_key(mfshell->folder_curr),
        folder_name_tmp);

    free(folder_name_tmp);

    return 0;
}

