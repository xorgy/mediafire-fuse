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

#include <stdlib.h>
#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#define bufsize 32768

int calc_md5(FILE * file, unsigned char *hash)
{
    int             bytesRead;
    char           *buffer;
    MD5_CTX         md5;

    MD5_Init(&md5);
    buffer = malloc(bufsize);
    if (buffer == NULL) {
        return -1;
    }
    while ((bytesRead = fread(buffer, 1, bufsize, file))) {
        MD5_Update(&md5, buffer, bytesRead);
    }
    MD5_Final(hash, &md5);
    free(buffer);
    return 0;
}

int calc_sha256(FILE * file, unsigned char *hash)
{
    int             bytesRead;
    char           *buffer;
    SHA256_CTX      sha256;

    SHA256_Init(&sha256);
    buffer = malloc(bufsize);
    if (buffer == NULL) {
        return -1;
    }
    while ((bytesRead = fread(buffer, 1, bufsize, file))) {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);
    free(buffer);
    return 0;
}
