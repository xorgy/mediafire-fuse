#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "strings.h"
#include "mfshell.h"
#include "private.h"
#include "command.h"

int
mfshell_cmd_whoami(mfshell_t *mfshell)
{
    int retval;

    retval = mfshell->user_get_info(mfshell);
    mfshell->update_secret_key(mfshell);

    return retval;
}


