#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "command.h"
#include "private.h"
#include "mfshell.h"

int
mfshell_cmd_lcd(mfshell_t *mfshell,const char *dir)
{
    int retval;

    if(mfshell == NULL) return -1;
    if(dir == NULL) return -1;

    if(strlen(dir) < 1) return -1;

    retval = chdir(dir);
    if(retval == 0)
    {
        if(mfshell->local_working_dir != NULL)
        {
            free(mfshell->local_working_dir);
            mfshell->local_working_dir = NULL;
        }

        mfshell->local_working_dir = strdup(dir);
    }

    return retval;
}

