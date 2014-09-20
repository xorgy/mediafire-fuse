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


#include <stdlib.h>

#include "json.h"
#include "stringv.h"

json_t*
json_object_by_path(json_t *start,const char *path)
{
    char    **path_nodes;
    char    **curr;
    json_t  *node = NULL;
    json_t  *data = NULL;

    if(start == NULL) return NULL;
    if(path == NULL) return NULL;

    path_nodes = stringv_split((char*)path,"/",10);

    if(path_nodes == NULL) return NULL;
    curr = path_nodes;
    node = start;

    while(curr != NULL)
    {
        if(*curr == NULL) break;

        node = json_object_get(node,*curr);
        if(node == NULL) break;

        if(!json_is_object(node))
        {
            stringv_free(path_nodes,STRINGV_FREE_ALL);
            return NULL;
        }

        curr++;
        data = node;
    }

    stringv_free(path_nodes,STRINGV_FREE_ALL);

    return data;
}

    // fix a path with leading slashes
/*
    while(strlen(path) > 0)
    {
        if(*path[0] == '\')
        {
            path++;
            continue;
        }

        len = strlen(path);
        buffer = (char*)calloc(len + 1,sizeof(char));
        strncpy(buffer,path,len);
    }

    // something went horribly wrong
    if(buffer == NULL) return NULL;

    pos = buffer[strlen(buffer) - 1];

    // fix a path with trailing slashes
    while(pos != buffer)
    {
        if(pos[0] == '/')
        {
            pos[0] = '\0';
            pos--;
            continue;
        }

        break;
    }

    // something else went horribly wrong
    if(pos == buffer)
    {
        free(buffer);
        return NULL;
    }
*/


