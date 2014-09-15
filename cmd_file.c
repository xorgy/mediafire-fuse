#include <stdio.h>
#include <string.h>

#include "command.h"
#include "mfshell.h"
#include "private.h"

int
mfshell_cmd_file(mfshell_t *mfshell,const char *quickkey)
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

    retval = mfshell->file_get_info(mfshell,file,(char*)quickkey);
    mfshell->update_secret_key(mfshell);

    if(file->name[0] != '\0')
        printf("   %-15.15s   %-*.*s\n\r",
        "filename:",
        term_width - 22,term_width -22,
        file->name);

    if(file->quickkey[0] != '\0')
        printf("   %-15.15s   %-*.*s\n\r",
        "quickkey:",
        term_width - 22,term_width - 22,
        file->quickkey);

    if(file->hash[0] != '\0')
        printf("   %-15.15s   %-*.*s\n\r",
        "hash:",
        term_width - 22,term_width - 22,
        file->hash);

    file_free(file);

    return 0;
}

