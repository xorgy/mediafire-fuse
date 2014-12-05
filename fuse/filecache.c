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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <bits/fcntl-linux.h>
#include <stdint.h>
#include <stdlib.h>

#include "../utils/hash.h"
#include "../utils/xdelta3.h"
#include "../mfapi/file.h"
#include "../mfapi/apicalls.h"
#include "../mfapi/patch.h"
#include "../mfapi/mfconn.h"
#include "../utils/http.h"
#include "../utils/strings.h"

static int      filecache_update_file(const char *filecache_path,
                                      mfconn * conn, const char *quickkey,
                                      uint64_t local_revision,
                                      uint64_t remote_revision);
static int      filecache_download_file(const char *filecache_path,
                                        const char *quickkey,
                                        uint64_t remote_revision,
                                        mfconn * conn);
static int      filecache_download_patch(mfconn * conn, const char *quickkey,
                                         uint64_t source_revision,
                                         uint64_t target_revision,
                                         const char *phash,
                                         const char *filecache_path);
static int      filecache_patch_file(const char *filecache_path,
                                     const char *quickkey,
                                     uint64_t source_revision,
                                     uint64_t target_revision);

int filecache_open_file(const char *quickkey, uint64_t local_revision,
                        uint64_t remote_revision, uint64_t fsize,
                        const unsigned char *fhash,
                        const char *filecache_path, mfconn * conn)
{
    char           *cachefile;
    int             fd;
    int             retval;

    /* check if the requested file is already in the cache */
    cachefile =
        strdup_printf("%s/%s_%d", filecache_path, quickkey, remote_revision);
    fd = open(cachefile, O_RDWR);
    if (fd > 0) {
        /* file existed - return handle */
        free(cachefile);
        return fd;
    }
    free(cachefile);

    /* if the file with remote revision didn't exist, then check whether an
     * old revision exists and in that case update that.
     *
     * Otherwise, download the file anew */

    cachefile =
        strdup_printf("%s/%s_%d", filecache_path, quickkey, local_revision);
    fd = open(cachefile, O_RDWR);
    free(cachefile);
    if (fd > 0) {
        close(fd);
        /* file exists, so we have to update it with one or more patches from
         * the remote */
        retval = filecache_update_file(filecache_path, conn, quickkey,
                                       local_revision, remote_revision);
        if (retval != 0) {
            fprintf(stderr, "update_file failed\n");
            return -1;
        }

    } else {
        /* download the file */
        retval = filecache_download_file(filecache_path, quickkey,
                                         remote_revision, conn);
        if (retval != 0) {
            fprintf(stderr, "filecache_download_file failed\n");
            return -1;
        }
    }

    /* check whether the patched or newly downloaded file matches the hash we
     * have stored */
    cachefile =
        strdup_printf("%s/%s_%d", filecache_path, quickkey, remote_revision);
    retval = file_check_integrity(cachefile, fsize, fhash);
    if (retval != 0) {
        fprintf(stderr, "checking integrity failed\n");
        free(cachefile);
        return -1;
    }

    fd = open(cachefile, O_RDWR);

    free(cachefile);

    /* return the file handle */
    return fd;
}

static int filecache_download_file(const char *filecache_path,
                                   const char *quickkey,
                                   uint64_t remote_revision, mfconn * conn)
{
    const char     *url;
    mffile         *file;
    mfhttp         *http;
    char           *cachefile;
    int             retval;

    cachefile = strdup_printf("%s/%s_%d", filecache_path, quickkey,
                              remote_revision);

    file = file_alloc();
    retval = mfconn_api_file_get_links(conn, file, (char *)quickkey);

    if (retval == -1) {
        fprintf(stderr, "mfconn_api_file_get_links failed\n");
        free(cachefile);
        file_free(file);
        return -1;
    }

    url = file_get_direct_link(file);

    if (url == NULL) {
        fprintf(stderr, "file_get_direct_link failed\n");
        free(cachefile);
        file_free(file);
        return -1;
    }

    http = http_create();
    retval = http_get_file(http, url, cachefile);
    http_destroy(http);

    if (retval != 0) {
        fprintf(stderr, "download failed\n");
        free(cachefile);
        file_free(file);
        return -1;
    }

    free(cachefile);
    file_free(file);

    return 0;
}

static int filecache_update_file(const char *filecache_path, mfconn * conn,
                                 const char *quickkey,
                                 uint64_t local_revision,
                                 uint64_t remote_revision)
{
    unsigned char   hash2[SHA256_DIGEST_LENGTH];
    int             retval;
    int             i;
    uint64_t        last_target_revision;
    char           *cachefile;

    mfpatch       **patches = NULL;

    retval = mfconn_api_device_get_updates(conn, quickkey,
                                           local_revision, remote_revision,
                                           &patches);

    if (retval != 0) {
        fprintf(stderr, "device/get_updates api call unsuccessful\n");
        return -1;
    }
    // if no patches are returned, then the full file has to be downloaded
    if (patches[0] == NULL) {
        free(patches);

        retval = filecache_download_file(filecache_path, quickkey,
                                         remote_revision, conn);
        if (retval != 0) {
            fprintf(stderr, "filecache_download_file failed\n");
            return -1;
        }

        return 0;
    }

    last_target_revision = local_revision;
    // else, go through all patches and download and apply them
    for (i = 0; patches[i] != NULL; i++) {
        /* verify that the source revision is equal to the last target
         * revision */
        if (patch_get_source_revision(patches[i]) != last_target_revision) {
            fprintf(stderr, "the source revision is unequal the last "
                    "target revision\n");
            break;
        }
        last_target_revision = patch_get_target_revision(patches[i]);

        retval =
            filecache_download_patch(conn, quickkey,
                                     patch_get_source_revision(patches[i]),
                                     patch_get_target_revision(patches[i]),
                                     patch_get_hash(patches[i]),
                                     filecache_path);
        if (retval != 0) {
            fprintf(stderr, "filecache_download_patch failed\n");
            break;
        }

        /* verify that the file to patch has the right hash */
        cachefile =
            strdup_printf("%s/%s_%d", filecache_path, quickkey,
                          patch_get_source_revision(patches[i]));
        hex2binary(patch_get_source_hash(patches[i]), hash2);
        retval = file_check_integrity_hash(cachefile, hash2);
        free(cachefile);
        if (retval != 0) {
            fprintf(stderr, "the source file has the wrong hash\n");
            break;
        }

        /* now apply the patch in patchfile to the file in cachefile */
        retval = filecache_patch_file(filecache_path, quickkey,
                                      patch_get_source_revision(patches[i]),
                                      patch_get_target_revision(patches[i]));
        if (retval != 0) {
            fprintf(stderr, "filecache_patch_file failed\n");
            break;
        }

        /* verify that the patched file has the right hash */
        cachefile =
            strdup_printf("%s/%s_%d", filecache_path, quickkey,
                          patch_get_target_revision(patches[i]));
        hex2binary(patch_get_target_hash(patches[i]), hash2);
        retval = file_check_integrity_hash(cachefile, hash2);
        free(cachefile);
        if (retval != 0) {
            fprintf(stderr, "the target file has the wrong hash\n");
            break;
        }

        free(patches[i]);
    }

    /* check if the terminating NULL was reached or if processing was aborted
     * before that */
    if (patches[i] != NULL) {
        for (; patches[i] != NULL; i++)
            free(patches[i]);
        free(patches);
        return -1;
    }

    free(patches);

    /* verify that the last target revision is equal to the requested remote
     * revision */
    if (last_target_revision != remote_revision) {
        fprintf(stderr, "last_target_revision is not equal to the requested "
                "remote revision\n");
        return -1;
    }

    return 0;
}

static int filecache_download_patch(mfconn * conn, const char *quickkey,
                                    uint64_t source_revision,
                                    uint64_t target_revision,
                                    const char *phash,
                                    const char *filecache_path)
{
    mfpatch        *patch;
    const char     *url;
    mfhttp         *http;
    int             retval;
    char           *patchfile;
    unsigned char   hash2[SHA256_DIGEST_LENGTH];

    /* first retrieve the patch url */
    patch = patch_alloc();
    retval = mfconn_api_device_get_patch(conn, patch, quickkey,
                                         source_revision, target_revision);

    if (retval == -1) {
        fprintf(stderr, "mfconn_api_device_get_patch failed\n");
        patch_free(patch);
        return -1;
    }

    /* verify if the retrieved patch hash is the expected patch hash */
    if (strcmp(phash, patch_get_hash(patch)) != 0) {
        fprintf(stderr, "the expected patch hash is not equal the hash "
                "returned by device/get_patch\n");
        patch_free(patch);
        return -1;
    }

    /* then download the patch */
    url = patch_get_link(patch);

    if (url == NULL || url[0] == '\0') {
        fprintf(stderr, "patch_get_link failed\n");
        patch_free(patch);
        return -1;
    }

    patchfile =
        strdup_printf("%s/%s_patch_%d_%d", filecache_path, quickkey,
                      source_revision, target_revision);

    http = http_create();
    retval = http_get_file(http, url, patchfile);
    http_destroy(http);

    if (retval != 0) {
        fprintf(stderr, "download failed\n");
        free(patchfile);
        patch_free(patch);
        return -1;
    }

    /* verify the integrity of the patch */
    hex2binary(patch_get_hash(patch), hash2);
    retval = file_check_integrity_hash(patchfile, hash2);

    if (retval != 0) {
        fprintf(stderr, "file_check_integrity_hash failed for patch\n");
        patch_free(patch);
        return -1;
    }

    patch_free(patch);

    return 0;
}

static int filecache_patch_file(const char *filecache_path,
                                const char *quickkey,
                                uint64_t source_revision,
                                uint64_t target_revision)
{
    char           *patchfile;
    char           *sourcefile;
    char           *targetfile;
    FILE           *sourcefile_fh;
    FILE           *patchfile_fh;
    FILE           *targetfile_fh;
    int             retval;

    sourcefile =
        strdup_printf("%s/%s_%d", filecache_path, quickkey, source_revision);
    sourcefile_fh = fopen(sourcefile, "r");
    if (sourcefile_fh == NULL) {
        fprintf(stderr, "cannot open %s\n", sourcefile);
        free(sourcefile);
        return -1;
    }

    free(sourcefile);

    patchfile =
        strdup_printf("%s/%s_patch_%d_%d", filecache_path, quickkey,
                      source_revision, target_revision);
    patchfile_fh = fopen(patchfile, "r");
    if (patchfile_fh == NULL) {
        fprintf(stderr, "cannot open %s\n", patchfile);
        free(patchfile);
        fclose(sourcefile_fh);
        return -1;
    }

    free(patchfile);

    targetfile =
        strdup_printf("%s/%s_%d", filecache_path, quickkey, target_revision);
    targetfile_fh = fopen(targetfile, "w");
    if (targetfile_fh == NULL) {
        fprintf(stderr, "cannot open %s\n", targetfile);
        fclose(sourcefile_fh);
        fclose(patchfile_fh);
        free(targetfile);
        return -1;
    }

    free(targetfile);

    retval = xdelta3_patch(sourcefile_fh, patchfile_fh, targetfile_fh);
    if (retval != 0) {
        fprintf(stderr, "unable to patch\n");
        fclose(sourcefile_fh);
        fclose(patchfile_fh);
        fclose(targetfile_fh);
        return -1;
    }

    fclose(sourcefile_fh);
    fclose(patchfile_fh);
    fclose(targetfile_fh);

    return 0;
}
