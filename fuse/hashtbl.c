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
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>

#include "hashtbl.h"
#include "../mfapi/mfconn.h"
#include "../mfapi/file.h"
#include "../mfapi/folder.h"
#include "../mfapi/apicalls.h"

/*
 * we build a hashtable using the first three characters of the file or folder
 * key. Since the folder key is encoded in base 36 (10 digits and 26 letters),
 * this means that the resulting hashtable has to have 36^3=46656 buckets.
 */
#define NUM_BUCKETS 46656

/*
 * we use this table to convert from a base36 char to an integer
 * we "waste" these 128 bytes of memory so that we don't need branching
 * instructions when decoding
 * we only need 128 bytes because the input is a *signed* char
 */
static unsigned char base36_decoding_table[] = {
/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x30 */ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x60 */ 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
/* 0x70 */ 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 0, 0, 0, 0, 0
};

/*
 * a macro to convert a char* of the key into a hash of its first three
 * characters
 */
#define HASH_OF_KEY(key) base36_decoding_table[(int)(key)[0]]*36*36+\
    base36_decoding_table[(int)(key)[1]]*36+\
    base36_decoding_table[(int)(key)[2]]

struct h_entry {
    /*
     * keys are either 13 (folders) or 15 (files) long since the structure
     * members are most likely 8-byte aligned anyways, it does not make sense
     * to differentiate between them */
    char            key[16];
    /* a filename is maximum 255 characters long */
    char            name[256];
    /* we do not add the parent here because it is not used anywhere */
    /* char parent[20]; */
    /* local revision */
    uint64_t        revision;
    /* creation time */
    uint64_t        ctime;
    /* for maintenance, set to zero when storing on disk */
    char            visited;

    /********************
     * only for folders *
     ********************/
    /* number of children (files plus folders) */
    uint64_t        num_children;
    /*
     * Array of pointers to its children. Set to zero when storing on disk.
     * This member could also be an array of keys which would not require
     * lookups on updating but we expect more reads than writes so we
     * sacrifice slower updates for faster lookups */
    struct h_entry **children;

    /******************
     * only for files *
     ******************/
    /* SHA256 is 256 bits = 32 bytes */
    unsigned char   hash[32];
    /*
     * last access time to remove old locally cached files
     * atime is also never zero for files
     * a file that has never been accessed has an atime of 1 */
    uint64_t        atime;
    /* file size */
    uint64_t        fsize;
};

/*
 * Each bucket is an array of pointers instead of an array of h_entry so that
 * the array can be changed without the memory location of the h_entry
 * changing because the children of each h_entry point to those locations
 *
 * This also allows us to implement each bucket as a sorted list in the
 * future. Queries could then be done using bisection (O(log(n))) instead of
 * having to traverse all elements in the bucket (O(n)). But with 46656
 * buckets this should not make a performance difference often.
 */

struct folder_tree {
    uint64_t        revision;
    uint64_t        bucket_lens[NUM_BUCKETS];
    h_entry       **buckets[NUM_BUCKETS];
    h_entry         root;
};

/* persistant storage file layout:
 *
 * byte 0: 0x4D -> ASCII M
 * byte 1: 0x46 -> ASCII F
 * byte 2: 0x53 -> ASCII S  --> MFS == MediaFire Storage
 * byte 3: 0x00 -> version information
 * bytes 4-11   -> last seen device revision
 * bytes 12-19  -> number of h_entry objects (num_hts)
 * bytes 20...  -> h_entry objects, each is 368 byte long
 * bytes xxx    -> immediately following the num_hts objects are arrays of the
 *                 children of the preceding h_entry objects in the same order
 *                 as the h_entry objects. The children are identified by
 *                 uint64_t numbers indicating the index of the objects in the
 *                 preceding array of h_entry objects. The number of children
 *                 per h_entry object is given in its num_children attribute
 *
 * the children pointer member of the h_entry object is useless when stored,
 * should be set to zero and not used when reading the file
 */

folder_tree    *folder_tree_create(void)
{
    folder_tree    *tree;

    tree = (folder_tree *) calloc(1, sizeof(folder_tree));

    return tree;
}

static void folder_tree_free_entries(folder_tree * tree)
{
    uint64_t        i,
                    j;

    for (i = 0; i < NUM_BUCKETS; i++) {
        for (j = 0; j < tree->bucket_lens[i]; j++) {
            free(tree->buckets[i][j]->children);
            free(tree->buckets[i][j]);
        }
        free(tree->buckets[i]);
        tree->buckets[i] = NULL;
        tree->bucket_lens[i] = 0;
    }
}

void folder_tree_destroy(folder_tree * tree)
{
    folder_tree_free_entries(tree);
    free(tree);
}

/*
 * given a folderkey, lookup the h_entry of it in the tree
 *
 * if key is NULL, then a pointer to the root is returned
 *
 * if no matching h_entry is found, NULL is returned
 */
static h_entry *folder_tree_lookup_key(folder_tree * tree, const char *key)
{
    int             bucket_id;
    uint64_t        i;

    if (key == NULL) {
        return &(tree->root);
    }
    /* retrieve the right bucket for this key */
    bucket_id = HASH_OF_KEY(key);

    for (i = 0; i < tree->bucket_lens[bucket_id]; i++) {
        if (strcmp(tree->buckets[bucket_id][i]->key, key) == 0) {
            return tree->buckets[bucket_id][i];
        }
    }

    return NULL;
}

/*
 * given a path, return the h_entry of the last component
 *
 * the path must start with a slash
 */
/*
static h_entry *folder_tree_lookup_path(folder_tree *tree, const char *path)
{
    char *tmp_path;
    char *new_path;
    char *slash_pos;
    h_entry *curr_dir;
    h_entry *result;
    uint64_t i;
    bool success;

    if (path[0] != '/') {
        fprintf(stderr, "Path must start with a slash\n");
        return NULL;
    }

    curr_dir = &(tree->root);

    // if the root is requested, return directly
    if (strcmp(path, "/") == 0) {
        return curr_dir;
    }

    // strip off the leading slash
    new_path = strdup(path+1);
    tmp_path = new_path;
    result = NULL;

    for (;;) {
        // path with a trailing slash, so the remainder is of zero length
        if (tmp_path[0] == '\0') {
            // return curr_dir
            result = curr_dir;
            break;
        }
        slash_pos = index(tmp_path, '/');
        if (slash_pos == NULL) {
            // no slash found in the remaining path:
            // find entry in current directory and return it
            for (i = 0; i < curr_dir->num_children; i++) {
                if (strcmp(curr_dir->children[i]->name, tmp_path) == 0) {
                    // return this directory
                    result = curr_dir->children[i];
                    break;
                }
            }

            // no matter whether the last part was found or not, iteration
            // stops here
            break;
        }

        *slash_pos = '\0';

        // a slash was found, so recurse into the directory of that name or
        // abort if the name matches a file
        success = false;
        for (i = 0; i < curr_dir->num_children; i++) {
            if (strcmp(curr_dir->children[i]->name, tmp_path) == 0) {
                // test if a file matched
                if (curr_dir->children[i]->atime != 0) {
                    fprintf(stderr, "A file can only be at the end of a path\n");
                    break;
                }

                // a directory matched, break out of this loop and recurse
                // deeper in the next iteration
                curr_dir = curr_dir->children[i];
                success = true;
                break;
            }
        }

        // either a file was part of a path or a folder of matching name was
        // not found, so we break out of this loop too
        if (!success) {
            break;
        }

        // point tmp_path to the character after the last found slash
        tmp_path = slash_pos + 1;
    }

    free(new_path);

    return result;
}
*/

/*
 * When adding an existing key, the old key is overwritten.
 * Return the inserted or updated key
 */
static h_entry *folder_tree_add_file(folder_tree * tree, mffile * file)
{
    h_entry        *entry;
    int             bucket_id;
    const char     *key;

    key = file_get_key(file);

    entry = folder_tree_lookup_key(tree, key);

    if (entry == NULL) {
        /* entry was not found, so append it to the end of the bucket */
        entry = (h_entry *) calloc(1, sizeof(h_entry));
        bucket_id = HASH_OF_KEY(key);
        tree->bucket_lens[bucket_id]++;
        tree->buckets[bucket_id] =
            realloc(tree->buckets[bucket_id],
                    sizeof(h_entry *) * tree->bucket_lens[bucket_id]);
        if (tree->buckets[bucket_id] == NULL) {
            fprintf(stderr, "realloc failed\n");
            return NULL;
        }
        tree->buckets[bucket_id][tree->bucket_lens[bucket_id] - 1] = entry;
    }

    strncpy(entry->key, key, sizeof(entry->key));
    strncpy(entry->name, file_get_name(file), sizeof(entry->name));
    entry->revision = file_get_revision(file);
    entry->ctime = file_get_created(file);

    /* mark this h_entry as a file if its atime is not set yet */
    if (entry->atime == 0)
        entry->atime = 1;

    return entry;
}

/* given an mffolder, add its information to a new h_entry, or update an
 * existing h_entry in the hashtable
 *
 * returns the added or updated h_entry
 */
static h_entry *folder_tree_add_folder(folder_tree * tree, mffolder * folder)
{
    h_entry        *entry;
    int             bucket_id;
    const char     *key;
    const char     *name;

    key = folder_get_key(folder);

    entry = folder_tree_lookup_key(tree, key);

    if (entry == NULL) {
        /* entry was not found, so append it to the end of the bucket */
        entry = (h_entry *) calloc(1, sizeof(h_entry));
        bucket_id = HASH_OF_KEY(key);
        tree->bucket_lens[bucket_id]++;
        tree->buckets[bucket_id] =
            realloc(tree->buckets[bucket_id],
                    sizeof(h_entry *) * tree->bucket_lens[bucket_id]);
        if (tree->buckets[bucket_id] == NULL) {
            fprintf(stderr, "realloc failed\n");
            return NULL;
        }
        tree->buckets[bucket_id][tree->bucket_lens[bucket_id] - 1] = entry;
    }
    /* can be NULL for root */
    if (key != NULL)
        strncpy(entry->key, key, sizeof(entry->key));
    /* can be NULL for root */
    name = folder_get_name(folder);
    if (name != NULL)
        strncpy(entry->name, name, sizeof(entry->name));
    entry->revision = folder_get_revision(folder);
    entry->ctime = folder_get_created(folder);

    return entry;
}

/* When trying to delete a non-existing key, nothing happens */
static void folder_tree_remove(folder_tree * tree, const char *key)
{
    int             bucket_id;
    int             found;
    uint64_t        i;

    if (key == NULL) {
        fprintf(stderr, "cannot remove root");
        return;
    }

    bucket_id = HASH_OF_KEY(key);

    /* check if the key exists */
    found = 0;
    for (i = 0; i < tree->bucket_lens[bucket_id]; i++) {
        if (strcmp(tree->buckets[bucket_id][i]->key, key) == 0) {
            found = 1;
            break;
        }
    }

    /* if found, use the last value of i to adjust the bucket */
    if (found) {
        /* remove its possible children */
        free(tree->buckets[bucket_id][i]->children);
        /* remove entry */
        free(tree->buckets[bucket_id][i]);
        /* move the items on the right one place to the left */
        memmove(tree->buckets[bucket_id][i], tree->buckets[bucket_id][i + 1],
                sizeof(h_entry *) * (tree->bucket_lens[bucket_id] - i - 1));
        /* change bucket size */
        tree->bucket_lens[bucket_id]--;
        if (tree->bucket_lens[bucket_id] == 0) {
            free(tree->buckets[bucket_id]);
            tree->buckets[bucket_id] = NULL;
        } else {
            tree->buckets[bucket_id] =
                realloc(tree->buckets[bucket_id],
                        sizeof(h_entry) * tree->bucket_lens[bucket_id]);
            if (tree->buckets[bucket_id] == NULL) {
                fprintf(stderr, "realloc failed\n");
                return;
            }
        }
    }
}

/*
 * check if a h_entry is the parent of another
 *
 * this checks only pointer equivalence and does not compare the key for
 * better performance
 *
 * thus, this function relies on the fact that only one h_entry per key exists
 */
static bool folder_tree_is_parent_of(h_entry * parent, h_entry * child)
{
    uint64_t        i;
    bool            found;

    found = false;
    for (i = 0; i < parent->num_children; i++) {
        if (parent->children[i] == child) {
            found = true;
            break;
        }
    }

    return found;
}

/*
 * given a h_entry, this function gets the remote content of that folder and
 * fills its children
 *
 * it then recurses for each child that is a directory and does the same in a
 * full remote directory walk
 */
static int folder_tree_rebuild_helper(folder_tree * tree, mfconn * conn,
                                      h_entry * curr_entry, bool recurse)
{
    int             retval;
    mffolder      **folder_result;
    mffile        **file_result;
    h_entry        *entry;
    int             i;

    /*
     * free the old children array of this folder
     * we don't free the children it references because they might be
     * referenced by someone else
     */
    free(curr_entry->children);
    curr_entry->children = NULL;
    curr_entry->num_children = 0;

    /* first folders */
    folder_result = NULL;
    retval =
        mfconn_api_folder_get_content(conn, 0, curr_entry->key, &folder_result,
                                      NULL);
    mfconn_update_secret_key(conn);
    if (retval != 0) {
        fprintf(stderr, "folder/get_content failed\n");
        if (folder_result != NULL) {
            for (i = 0; folder_result[i] != NULL; i++) {
                free(folder_result[i]);
            }
            free(folder_result);
        }
        return -1;
    }

    for (i = 0; folder_result[i] != NULL; i++) {
        if (folder_get_key(folder_result[i]) == NULL) {
            fprintf(stderr, "folder_get_key returned NULL\n");
            folder_free(folder_result[i]);
        }
        entry = folder_tree_add_folder(tree, folder_result[i]);
        /*
         * make sure that this entry is not already a child of the current
         * folder
         */
        if (!folder_tree_is_parent_of(curr_entry, entry)) {
            /* add this entry to the children of the current folder */
            curr_entry->num_children++;
            curr_entry->children =
                realloc(curr_entry->children,
                        curr_entry->num_children * sizeof(h_entry));
            if (curr_entry->children == NULL) {
                fprintf(stderr, "realloc failed\n");
                folder_free(folder_result[i]);
                continue;
            }
            curr_entry->children[curr_entry->num_children - 1] = entry;
        }
        /* recurse */
        if (recurse)
            folder_tree_rebuild_helper(tree, conn, entry, true);
        folder_free(folder_result[i]);
    }
    free(folder_result);

    /* then files */
    file_result = NULL;
    retval =
        mfconn_api_folder_get_content(conn, 1, curr_entry->key, NULL,
                                      &file_result);
    mfconn_update_secret_key(conn);
    if (retval != 0) {
        fprintf(stderr, "folder/get_content failed\n");
        if (file_result != NULL) {
            for (i = 0; file_result[i] != NULL; i++) {
                file_free(file_result[i]);
            }
            free(file_result);
        }
        return -1;
    }

    for (i = 0; file_result[i] != NULL; i++) {
        if (file_get_key(file_result[i]) == NULL) {
            fprintf(stderr, "file_get_key returned NULL\n");
            file_free(file_result[i]);
        }
        entry = folder_tree_add_file(tree, file_result[i]);
        /*
         * make sure that this entry is not already a child of the current
         * folder
         */
        if (!folder_tree_is_parent_of(curr_entry, entry)) {
            /* add this entry to the children of the current file */
            curr_entry->num_children++;
            curr_entry->children =
                realloc(curr_entry->children,
                        curr_entry->num_children * sizeof(h_entry));
            if (curr_entry->children == NULL) {
                fprintf(stderr, "realloc failed\n");
                file_free(file_result[i]);
                continue;
            }
            curr_entry->children[curr_entry->num_children - 1] = entry;
        }
        file_free(file_result[i]);
    }
    free(file_result);

    return 0;
}

/*
 * update the fields of a file
 */
static int folder_tree_update_file_info(folder_tree * tree, mfconn * conn,
                                        char *key)
{
    mffile         *file;
    int             retval;

    file = file_alloc();

    retval = mfconn_api_file_get_info(conn, file, key);
    mfconn_update_secret_key(conn);
    if (retval != 0) {
        fprintf(stderr, "api call unsuccessful\n");
        file_free(file);
        return -1;
    }

    folder_tree_add_file(tree, file);

    file_free(file);

    return 0;
}

/*
 * update the fields of a folder without checking its children
 *
 * we identify the folder to update by its key instead of its h_entry because
 * this function is to fill the h_entry in the first place
 */
static int folder_tree_update_folder_info(folder_tree * tree, mfconn * conn,
                                          char *key)
{
    mffolder       *folder;
    int             retval;

    folder = folder_alloc();

    retval = mfconn_api_folder_get_info(conn, folder, key);
    mfconn_update_secret_key(conn);
    if (retval != 0) {
        fprintf(stderr, "api call unsuccessful\n");
        folder_free(folder);
        return -1;
    }

    folder_tree_add_folder(tree, folder);

    folder_free(folder);

    return 0;
}

/*
 * update the contents of a folder and its direct children. Does not recurse
 * further
 *
 * this function cannot complete if there is no reference to the new children
 * in the hashtable
 *
 */
static int folder_tree_update_folder_contents(folder_tree * tree,
                                              mfconn * conn, char *key)
{
    h_entry        *curr_entry;

    curr_entry = folder_tree_lookup_key(tree, key);

    if (curr_entry == NULL) {
        fprintf(stderr, "folder_tree_lookup_key failed\n");
        return -1;
    }

    folder_tree_rebuild_helper(tree, conn, curr_entry, false);

    return 0;
}

/*
 * ask the remote if there are changes after the locally stored revision
 *
 * if yes, integrate those changes
 */
static void folder_tree_update(folder_tree * tree, mfconn * conn)
{
    uint64_t        revision_remote;
    uint64_t        i;
    struct mfconn_device_change *changes;
    int             retval;

    mfconn_api_device_get_status(conn, &revision_remote);
    mfconn_update_secret_key(conn);

    if (tree->revision == revision_remote) {
        fprintf(stderr, "Request to update but nothing to do\n");
        return;
    }
    /*
     * we can ignore the information about the parent because if a file was
     * created or moved, then the parent directory was updated as well
     * if only the content of a file was modified, then the information about
     * the parent is useless
     */

    /*
     * we have to manually check the root because it never shows up in the
     * results from device_get_changes
     */

    folder_tree_update_folder_contents(tree, conn, NULL);

    /*
     * changes have to be applied in the right order but the result of
     * mfconn_api_device_get_changes is already sorted by revision
     */

    changes = NULL;
    retval = mfconn_api_device_get_changes(conn, tree->revision, &changes);
    mfconn_update_secret_key(conn);
    if (retval != 0) {
        fprintf(stderr, "device/get_changes() failed\n");
        return;
    }

    for (i = 0; changes[i].revision != 0; i++) {
        switch (changes[i].change) {
            case MFCONN_DEVICE_CHANGE_DELETED_FOLDER:
            case MFCONN_DEVICE_CHANGE_DELETED_FILE:
                folder_tree_remove(tree, changes[i].key);
                break;
            case MFCONN_DEVICE_CHANGE_UPDATED_FOLDER:
                /* if a folder has been updated then either its name... */
                folder_tree_update_folder_info(tree, conn, changes[i].key);
                /* ...or its contents changed */
                folder_tree_update_folder_contents(tree, conn, changes[i].key);
                break;
            case MFCONN_DEVICE_CHANGE_UPDATED_FILE:
                /* if a file changed, update its info */
                folder_tree_update_file_info(tree, conn, changes[i].key);
                break;
        }
    }

    /* the new revision of the tree is the revision of the last change */
    tree->revision = changes[i - 1].revision;
}

/*
 * rebuild the folder_tree by a walk of the remote filesystem
 *
 * is called to initialize the folder_tree on first use
 *
 * might also be called when local and remote version get out of sync
 */
int folder_tree_rebuild(folder_tree * tree, mfconn * conn)
{
    uint64_t        revision_before;
    int             ret;

    /* free local folder_tree */
    folder_tree_free_entries(tree);

    /* get remote device revision before walking the tree */
    ret = mfconn_api_device_get_status(conn, &revision_before);
    mfconn_update_secret_key(conn);
    if (ret != 0) {
        fprintf(stderr, "device/get_status call unsuccessful\n");
        return -1;
    }
    tree->revision = revision_before;

    /* walk the remote tree to build the folder_tree */

    /* populate the root */
    ret = folder_tree_update_folder_info(tree, conn, NULL);
    if (ret != 0) {
        fprintf(stderr, "folder_tree_update_folder_info unsuccessful\n");
        return -1;
    }

    folder_tree_rebuild_helper(tree, conn, &(tree->root), true);

    /*
     * call device/get_changes to get possible remote changes while we walked
     * the tree.
     */
    folder_tree_update(tree, conn);

    /* clean the resulting folder_tree of any dangling objects */
    folder_tree_housekeep(tree);

    return 0;
}

static void mark_visited(h_entry * ent)
{
    uint64_t        i;

    ent->visited = 1;

    for (i = 0; i < ent->num_children; i++) {
        if (ent->children[i]->atime == 0) {
            /* folder */
            mark_visited(ent->children[i]);
        } else {
            /* file */
            ent->children[i]->visited = 1;
        }
    }
}

/* clean up files and folders that are never referenced */
void folder_tree_housekeep(folder_tree * tree)
{
    uint64_t        i,
                    j;

    /* mark all objects as unvisited, just in case */
    for (i = 0; i < NUM_BUCKETS; i++) {
        for (j = 0; j < tree->bucket_lens[i]; j++) {
            tree->buckets[i][j]->visited = 0;
        }
    }

    /* walk the tree, mark found objects as visited */
    mark_visited(&(tree->root));

    /*
     * remove unvisited objects
     *
     * do not remove unreferenced objects which have a revision that is equal
     * to the device revision because they might only be dangling right now
     * because we did this cleaning in the middle of a file or directory
     * movement and at some point during the movement, the moved object is not
     * referenced by either its source nor its destination
     */
    for (i = 0; i < NUM_BUCKETS; i++) {
        for (j = 0; j < tree->bucket_lens[i]; j++) {
            if (tree->buckets[i][j]->visited == 0 &&
                tree->buckets[i][j]->revision != tree->revision) {
                fprintf(stderr, "remove unvisited %s\n",
                        tree->buckets[i][j]->key);
                /* free memory of h_entry */
                free(tree->buckets[i][j]);
                /* move the items on the right one place to the left */
                memmove(tree->buckets[i][j], tree->buckets[i][j + 1],
                        sizeof(h_entry *) * (tree->bucket_lens[i] - j - 1));
                /* change bucket size */
                tree->bucket_lens[i]--;
                if (tree->bucket_lens[i] == 0) {
                    free(tree->buckets[i]);
                    tree->buckets[i] = NULL;
                } else {
                    tree->buckets[i] =
                        realloc(tree->buckets[i],
                                sizeof(h_entry) * tree->bucket_lens[i]);
                    if (tree->buckets[i] == NULL) {
                        fprintf(stderr, "realloc failed\n");
                        return;
                    }
                }
            }
        }
    }

    /* TODO: remove unvisited cached files */

    /*
     * Never remove an unreferenced cached file if its revision is equal to
     * the device revision
     *
     * for file in cache directory:
     *     e = folder_tree_lookup_key(file.key)
     *     if e == None:
     *         delete(file)
     *     elif e->visited == 0 && e->revision != tree->revision:
     *         delete(file)
     */

    /* mark all objects as unvisited again */
    tree->root.visited = 0;
    for (i = 0; i < NUM_BUCKETS; i++) {
        for (j = 0; j < tree->bucket_lens[i]; j++) {
            tree->buckets[i][j]->visited = 0;
        }
    }
}

void folder_tree_debug(folder_tree * tree, h_entry * ent, int depth)
{
    uint64_t        i;

    if (ent == NULL) {
        ent = &(tree->root);
    }

    for (i = 0; i < ent->num_children; i++) {
        if (ent->children[i]->atime == 0) {
            /* folder */
            fprintf(stderr, "%*s d:%s\n", depth + 1, " ",
                    ent->children[i]->name);
            folder_tree_debug(tree, ent->children[i], depth + 1);
        } else {
            /* file */
            fprintf(stderr, "%*s f:%s\n", depth + 1, " ",
                    ent->children[i]->name);
        }
    }
}
