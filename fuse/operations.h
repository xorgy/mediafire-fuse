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

#ifndef __FUSE_OPERATIONS_H__
#define __FUSE_OPERATIONS_H__

#include <fuse/fuse.h>
#include <stddef.h>
#include <sys/types.h>
#include "../mfapi/mfconn.h"
#include "hashtbl.h"

struct mediafirefs_context_private {
    mfconn         *conn;
    folder_tree    *tree;
    char           *configfile;
    char           *dircache;
    char           *filecache;
    /* stores all currently open temporary files which are to be uploaded when
     * they are closed.
     * we use a normal array because the number of open temporary files will
     * never be very high but is limited by the number of threads */
    char          **tmpfiles;
    size_t          num_tmpfiles;
};

int             mediafirefs_getattr(const char *path, struct stat *stbuf);
int             mediafirefs_readdir(const char *path, void *buf,
                                    fuse_fill_dir_t filldir, off_t offset,
                                    struct fuse_file_info *info);
void            mediafirefs_destroy();
int             mediafirefs_mkdir(const char *path, mode_t mode);
int             mediafirefs_rmdir(const char *path);
int             mediafirefs_unlink(const char *path);
int             mediafirefs_open(const char *path,
                                 struct fuse_file_info *file_info);
int             mediafirefs_read(const char *path, char *buf, size_t size,
                                 off_t offset,
                                 struct fuse_file_info *file_info);
int             mediafirefs_write(const char *path, const char *buf,
                                  size_t size, off_t offset,
                                  struct fuse_file_info *file_info);
int             mediafirefs_release(const char *path,
                                    struct fuse_file_info *file_info);
int             mediafirefs_create(const char *path, mode_t mode,
                                   struct fuse_file_info *file_info);

#endif
