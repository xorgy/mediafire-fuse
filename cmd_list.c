#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "strings.h"
#include "mfshell.h"
#include "private.h"
#include "command.h"

int
mfshell_cmd_list(mfshell_t *mfshell)
{
    int         retval;
    const char  *folder_curr;

    if(mfshell == NULL) return -1;

    folder_curr = folder_get_key(mfshell->folder_curr);

    // safety check... this should never happen
    if(folder_curr == NULL)
        folder_set_key(mfshell->folder_curr,"myfiles");

    // safety check... this should never happen
    if(folder_curr[0] == '\0')
        folder_set_key(mfshell->folder_curr,"myfiles");

    // first folders
    retval = mfshell->folder_get_content(mfshell,0);
    mfshell->update_secret_key(mfshell);

    // then files
    retval = mfshell->folder_get_content(mfshell,1);
    mfshell->update_secret_key(mfshell);

    return retval;
}


