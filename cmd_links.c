#include <stdio.h>
#include <string.h>

#include "command.h"
#include "mfshell.h"
#include "private.h"

int
mfshell_cmd_links(mfshell_t *mfshell,const char *quickkey)
{
    extern int  term_width;
    file_t      *file;
    int         len;
    int         retval;

    if(mfshell == NULL) return -1;
    if(quickkey == NULL) return -1;

    len = strlen(quickkey);

    if(len != 11 && len != 15) return -1;

    file = file_alloc();

    retval = mfshell->file_get_links(mfshell,file,(char*)quickkey);
    mfshell->update_secret_key(mfshell);

    if(file->share_link != NULL)
        printf("   %-15.15s   %-*.*s\n\r",
        "sharing url:",
        term_width - 22,term_width -22,
        file->share_link);

    if(file->direct_link != NULL)
        printf("   %-15.15s   %-*.*s\n\r",
        "direct url:",
        term_width - 22,term_width -22,
        file->direct_link);

    if(file->onetime_link != NULL)
        printf("   %-15.15s   %-*.*s\n\r",
        "1-time url:",
        term_width - 22,term_width -22,
        file->onetime_link);

    file_free(file);

    return 0;
}

