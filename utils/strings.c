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

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strings.h"
#include "stringv.h"

char           *strdup_printf(char *fmt, ...)
{
    // Good for glibc 2.1 and above. Fedora5 is 2.4.

    char           *ret_str = NULL;
    va_list         ap;
    int             bytes_to_allocate;

    va_start(ap, fmt);
    bytes_to_allocate = vsnprintf(ret_str, 0, fmt, ap);
    va_end(ap);

    // Add one for '\0'
    bytes_to_allocate++;

    ret_str = (char *)malloc(bytes_to_allocate * sizeof(char));

    va_start(ap, fmt);
    bytes_to_allocate = vsnprintf(ret_str, bytes_to_allocate, fmt, ap);
    va_end(ap);

    return ret_str;
}

char           *strdup_join(char *string1, char *string2)
{
    char           *new_string;
    size_t          string1_len;
    size_t          string2_len;

    if (string1 == NULL || string2 == NULL)
        return NULL;

    string1_len = strlen(string1);
    string2_len = strlen(string2);

    new_string = (char *)malloc(string1_len + string2_len + 1);

    strncpy(new_string, string1, string1_len);
    strncat(new_string, string2, string2_len);

    new_string[string1_len + string2_len] = '\0';

    return new_string;
}

char           *strdup_subst(char *string, char *token, char *subst,
                             unsigned int max)
{
    size_t          string_len = 0;
    size_t          subst_len = 0;
    size_t          token_len = 0;
    size_t          total_len = 0;
    size_t          copy_len = 0;
    size_t          token_count;
    char           *str_new = NULL;
    char          **vectors;
    char          **rewind;

    if (string == NULL)
        return NULL;

    if (token == NULL || subst == NULL)
        return NULL;

    string_len = strlen(string);
    token_len = strlen(token);

    // return on conditions that we could never handle.
    if (token_len > string_len)
        return NULL;
    if (token[0] == '\0')
        return NULL;

    vectors = stringv_find(string, token, max);
    if (vectors == NULL)
        return NULL;
    rewind = vectors;

    // count the number of tokens found in the string
    token_count = stringv_len(vectors);

    if (token_count > max)
        token_count = max;

    // start with the original string size;
    total_len = string_len;

    // subtract the total number of token chars to be removed
    total_len -= (token_len * token_count);

    // add back the total number of subst chars to be inserted
    total_len += (subst_len * token_count);

    str_new = (char *)malloc((total_len + 1) * sizeof(char));
    str_new[0] = '\0';

    while (*vectors != NULL) {
        // calculate distance to the next token from current position
        copy_len = *vectors - string;

        if (copy_len > 0) {
            strncat(str_new, string, copy_len);
            string += copy_len;

            // when total_len == 0 the process is complete
            total_len -= copy_len;
        }

        strncat(str_new, token, token_len);

        // when total_len == 0 the process is complete
        total_len -= token_len;

        vectors++;
    }

    // might have one more copy operation to complete
    if (total_len > 0) {
        strcat(str_new, string);
    }
    // todo:  can't free vectors directly cuz it was incremented
    free(rewind);

    return str_new;
}

void string_chomp(char *string)
{
    size_t          len;
    char           *pos;

    if (string == NULL)
        return;

    len = strlen(string);

    if (len == 0)
        return;

    pos = &string[len - 1];

    while (isspace((int)*pos) != 0) {
        *pos = '\0';
        pos--;
    }

    return;
}

/*
void
string_strip_head(char *string,int c)
{
    int     count = 0;
    int     len;
    char    *pos;

    if(string == NULL) return;

    len = strlen(string);
    if(len == 0) return;

    pos = string;

    while(count < len)
    {
        
        if(c <= 0)
        {
            if(isaspace((char)pos[0])
            {
                pos++;
                count++;
                continue;
            }

            break;
        }

        if(

    // fix a path with leading slashes
    while(strlen(string) > 0)
    {
        if(c > 0)
        {
            if(string[0] == c)
            {
                

        if(string[0] == (c)
        {
            string++;
            continue;
        }

        len = strlen(path);
        buffer = (char*)calloc(len + 1,sizeof(char));
        strncpy(buffer,path,len);
    }
*/

/*
    A negative value shifts the string left, while a positive value
    shifts the string right.  The vacuum-space is zero filled.
*/
/*
void
string_shift(char *string,ssize_t vector)
{
    char        *pos;
    ssize_t     len;
    ssize_t     i;

    if(string == NULL) return;

    len = strlen(string);
    if(len <= ABSINT(vector)) return;

    // shift left
    if(vector < 0)
    {
        pos = string + (ABSINT(vector));
        strcpy(string,pos);
        string[len - ABSINT(vector) + 1] = '\0';

        return;
    }
}
*/
