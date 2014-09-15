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


