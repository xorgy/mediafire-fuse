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


#ifndef _STRING_V_H_
#define _STRING_V_H_

#include <stddef.h>

#define     STRINGV_FREE_ALL    1

// count number of strings in a NULL in a string vector
size_t      stringv_len(char **array);

// free all of the strings in a vector and optionally the vector pointer
void        stringv_free(char **array,int b_free);

// deep copy of string vector.  returns a new vector pointer
char**      stringv_copy(char **array);

// returns a NULL terminated vector array to every location 'token' is found
char**      stringv_find(char *string,char *token,int limit);

// returns a NULL terminated vector array of items delimited by 'token'
char**      stringv_split(char *string,char *token,int limit);



#endif
