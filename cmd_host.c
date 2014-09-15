#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mfshell.h"
#include "command.h"
#include "private.h"
#include "strings.h"

static char*
_get_host_from_user(void);

int
mfshell_cmd_host(mfshell_t *mfshell,const char *host)
{
    char    *alt_host = NULL;

    if(mfshell == NULL) return -1;

    if(host == NULL)
    {
        alt_host =  _get_host_from_user();
        host = alt_host;
    }

    if(mfshell->server != NULL)
    {
        // do nothing if the server is exactly the same
        if(strcmp(mfshell->server,host) == 0)
        {
            if(alt_host != NULL) free(alt_host);
            return 0;
        }

        if(mfshell->server != NULL)
        {
            free(mfshell->server);
            mfshell->server = NULL;
        }
    }

    mfshell->server = strdup(host);

    // invalidate server auth credentials
    if(mfshell->session_token != NULL)
    {
        free(mfshell->session_token);
        mfshell->session_token = NULL;
    }

    if(mfshell->secret_time != NULL)
    {
        free(mfshell->secret_time);
        mfshell->secret_time = NULL;
    }

    if(alt_host != NULL) free(alt_host);

    return 0;
}

char*
_get_host_from_user(void)
{
    size_t  len = 0;
    char    *host = NULL;

    printf("host: [www.mediafire.com] ");
    getline(&host,&len,stdin);
    string_chomp(host);

    if(host == NULL)
        return strdup("www.mediafire.com");

    if(strlen(host) < 2)
    {
        free(host);
        return strdup("www.mediafire.com");
    }

    return host;
}
