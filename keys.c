/*
 * Copyright (C) 2013 Bryan Christ <bryan.christ@mediafire.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */


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

