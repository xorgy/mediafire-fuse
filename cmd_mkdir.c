#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "strings.h"
#include "mfshell.h"
#include "private.h"
#include "command.h"

int
mfshell_cmd_mkdir(mfshell_t *mfshell,const char *name)
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

    folder_curr = folder_get_key(mfshell->folder_curr);

    retval = mfshell->folder_create(mfshell,(char*)folder_curr,(char*)name);
    mfshell->update_secret_key(mfshell);

    return retval;
}


