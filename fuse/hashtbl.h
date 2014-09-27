/*
 * Copyright (C) 2014 Johannes Schauer <j.schauer@email.de>
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

#ifndef _MFFUSE_HASHTBL_H_
#define _MFFUSE_HASHTBL_H_

#include <fuse/fuse.h>

#include "../mfapi/mfconn.h"

typedef struct folder_tree folder_tree;
typedef struct h_entry h_entry;

folder_tree    *folder_tree_create(void);

void            folder_tree_destroy(folder_tree * tree);

int             folder_tree_rebuild(folder_tree * tree, mfconn * conn);

void            folder_tree_housekeep(folder_tree * tree, mfconn * conn);

void            folder_tree_debug(folder_tree * tree, h_entry * ent,
                                  int depth);

int             folder_tree_getattr(folder_tree * tree, const char *path,
                                    struct stat *stbuf);

int             folder_tree_readdir(folder_tree * tree, const char *path,
                                    void *buf, fuse_fill_dir_t filldir);

void            folder_tree_update(folder_tree * tree, mfconn * conn);

#endif
