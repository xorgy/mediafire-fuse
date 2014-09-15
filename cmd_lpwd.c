#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "command.h"
#include "mfshell.h"
#include "private.h"

int
mfshell_cmd_lpwd(mfshell_t *mfshell)
{
    extern int  term_width;
    int         trim_size;

    if(mfshell == NULL) return -1;

    if(mfshell->local_working_dir == NULL)
    {
        mfshell->local_working_dir = (char*)calloc(PATH_MAX + 1,sizeof(char));
        getcwd(mfshell->local_working_dir,PATH_MAX);
    }

    trim_size = term_width - 1;

    printf("%-*.*s\n\r",
        trim_size,trim_size,
        mfshell->local_working_dir);

    return 0;
}

