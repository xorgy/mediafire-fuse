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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stringv.h"

size_t stringv_len(char **array)
{
    size_t          count = 0;
    char          **pos;

    if (array == NULL)
        return 0;

    pos = array;
    while (pos[0] != NULL) {
        pos++;
        count++;
    }

    return count;
}

void stringv_free(char **array, int b_free)
{
    char          **pos;

    if (array == NULL)
        return;

    pos = array;

    while ((*pos) != NULL) {
        free(*pos);
        ++pos;
    }

    if (b_free == STRINGV_FREE_ALL)
        free(array);

    return;
}

char          **stringv_copy(char **array)
{
    uint32_t        array_len;
    char          **array_pos;

    char          **dup_array;
    char          **dup_pos;

    if (array == NULL)
        return (char **)NULL;

    array_pos = array;

    array_len = stringv_len(array);

    if (array_len > UINT32_MAX - 1)
        array_len = UINT32_MAX - 1;

    dup_array = (char **)calloc(array_len, sizeof(char *));
    dup_pos = dup_array;

    while ((*array_pos) != NULL) {
        *dup_pos = strdup((const char *)*array_pos);

        array_pos++;
        dup_pos++;
    }

    return dup_array;
}

char          **stringv_find(char *string, char *token, int limit)
{
    char          **results = NULL;
    char           *pos = NULL;
    int             count = 0;

    if (string == NULL)
        return (char **)NULL;
    if (token == NULL)
        return (char **)NULL;
    if (limit == 0)
        return (char **)NULL;

    pos = string;

    if (strlen(token) > strlen(string))
        return (char **)NULL;

    while (count != limit) {
        pos = strstr(pos, token);
        if (pos == NULL)
            break;

        count++;
        results = (char **)realloc((void *)results, sizeof(char *) * count + 1);

        results[count - 1] = pos;
    }

    if (count == 0)
        return (char **)NULL;

    results[count] = (char *)NULL;

    return results;
}

char          **stringv_split(char *string, char *token, int limit)
{
    char          **results = NULL;
    char           *curr = NULL;
    char           *next = NULL;
    int             count = 0;
    unsigned int    len;
    size_t          copy_len = 0;

    if (string == NULL)
        return (char **)NULL;
    if (token == NULL)
        return (char **)NULL;
    if (limit == 0)
        return (char **)NULL;

    len = strlen(string);
    if (strlen(token) > len)
        return (char **)NULL;

    curr = string;

    do {
        // alloc space for current item plus NULL vector terminator
        results = (char **)realloc(results, sizeof(char *) * (count + 2));

        // find the next occurrence
        next = strstr(curr, token);

        if (next != NULL)
            copy_len = next - curr;
        else
            copy_len = strlen(curr);

        results[count] = (char *)calloc(copy_len + 1, sizeof(char));
        memcpy(results[count], curr, copy_len);

        count++;

        if (next == NULL)
            break;

        curr = next;
        curr++;
    }
    while (count < limit);

    results[count] = NULL;

    return results;
}
