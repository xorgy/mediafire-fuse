#include <stdio.h>
#include <inttypes.h>

#include "mfshell.h"
#include "private.h"

void
_update_secret_key(mfshell_t *mfshell)
{
    uint64_t    new_val;

    if(mfshell == NULL) return;

    new_val = ((uint64_t)mfshell->secret_key) * 16807;
    new_val %= 0x7FFFFFFF;

    mfshell->secret_key = new_val;

    return;
}

