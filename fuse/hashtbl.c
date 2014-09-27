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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fuse/fuse.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>

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
    char            key[MFAPI_MAX_LEN_KEY + 1];
    /* a filename is maximum 255 characters long */
    char            name[MFAPI_MAX_LEN_NAME + 1];
    /* we do not add the parent here because it is not used anywhere */
    /* char parent[20]; */
    /* local revision */
    uint64_t        revision;
    /* creation time */
    uint64_t        ctime;
    /* the containing folder */
    union {
        /* during runtime this is a pointer to the containing h_entry */
        h_entry        *parent;
        /* when storing on disk, this is the offset of the stored h_entry */
        uint64_t        parent_offs;
    };

    /********************
     * only for folders *
     ********************/
    /* number of children (number of files plus number of folders). Set to
     * zero when storing on disk */
    uint64_t        num_children;
    /*
     * Array of pointers to its children. Set to zero when storing on disk.
     *
     * This member could also be an array of keys which would not require
     * lookups on updating but we expect more reads than writes so we
     * sacrifice slower updates for faster lookups */
    h_entry       **children;

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
 * bytes 12-19  -> number of h_entry objects including root (num_hts)
 * bytes 20...  -> h_entry objects, the first one being root
 *
 * the children pointer member of the h_entry object is useless when stored,
 * should be set to zero and not used when reading the file
 */
int folder_tree_store(folder_tree * tree, FILE * stream)
{
    /* to allow a quick mapping from keys to their offsets in the array that
     * will be stored, we create a hashtable of the same structure as the
     * folder_tree but instead of storing pointers to h_entries in the buckets
     * we store their integer offset. This way, when one known in which bucket
     * and in which position in a bucket a h_entry is, one can retrieve the
     * associated integer offset. */

    uint64_t      **integer_buckets;
    uint64_t        i,
                    j,
                    k,
                    num_hts;
    size_t          ret;
    h_entry        *tmp_parent;
    int             bucket_id;
    bool            found;

    integer_buckets = (uint64_t **) malloc(NUM_BUCKETS * sizeof(uint64_t *));
    if (integer_buckets == NULL) {
        fprintf(stderr, "cannot malloc");
        return -1;
    }

    /* start counting with one because the root is also stored */
    num_hts = 1;
    for (i = 0; i < NUM_BUCKETS; i++) {
        if (tree->bucket_lens[i] == 0)
            continue;

        integer_buckets[i] =
            (uint64_t *) malloc(tree->bucket_lens[i] * sizeof(uint64_t));
        for (j = 0; j < tree->bucket_lens[i]; j++) {
            integer_buckets[i][j] = num_hts;
            num_hts++;
        }
    }

    /* write four header bytes */
    ret = fwrite("MFS\0", 1, 4, stream);
    if (ret != 4) {
        fprintf(stderr, "cannot fwrite\n");
        return -1;
    }

    /* write revision */
    ret = fwrite(&(tree->revision), sizeof(tree->revision), 1, stream);
    if (ret != 1) {
        fprintf(stderr, "cannot fwrite\n");
        return -1;
    }

    /* write number of h_entries */
    ret = fwrite(&num_hts, sizeof(num_hts), 1, stream);
    if (ret != 1) {
        fprintf(stderr, "cannot fwrite\n");
        return -1;
    }

    /* write the root */
    ret = fwrite(&(tree->root), sizeof(h_entry), 1, stream);
    if (ret != 1) {
        fprintf(stderr, "cannot fwrite\n");
        return -1;
    }

    for (i = 0; i < NUM_BUCKETS; i++) {
        if (tree->bucket_lens[i] == 0)
            continue;

        for (j = 0; j < tree->bucket_lens[i]; j++) {
            /* save the old value of the parent to restore it later */
            tmp_parent = tree->buckets[i][j]->parent;
            if (tmp_parent == &(tree->root)) {
                tree->buckets[i][j]->parent_offs = 0;
            } else {
                bucket_id = HASH_OF_KEY(tmp_parent->key);
                found = false;
                for (k = 0; k < tree->bucket_lens[bucket_id]; k++) {
                    if (tree->buckets[bucket_id][k] == tmp_parent) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    fprintf(stderr, "parent of %s was not found!\n",
                            tree->buckets[i][j]->key);
                    return -1;
                }
                tree->buckets[i][j]->parent_offs =
                    integer_buckets[bucket_id][k];
            }

            /* write out modified record */
            ret = fwrite(tree->buckets[i][j], sizeof(h_entry), 1, stream);
            if (ret != 1) {
                fprintf(stderr, "cannot fwrite\n");
                return -1;
            }

            /* restore original value for parent */
            tree->buckets[i][j]->parent = tmp_parent;
        }
    }

    for (i = 0; i < NUM_BUCKETS; i++) {
        if (tree->bucket_lens[i] > 0) {
            free(integer_buckets[i]);
        }
    }
    free(integer_buckets);

    return 0;
}

folder_tree    *folder_tree_load(FILE * stream)
{
    folder_tree    *tree;
    unsigned char   tmp_buffer[4];
    size_t          ret;
    uint64_t        num_hts;
    uint64_t        i;
    h_entry       **ordered_entries;
    h_entry        *tmp_entry;
    h_entry        *parent;
    int             bucket_id;

    /* read and check the first four bytes */
    ret = fread(tmp_buffer, 1, 4, stream);
    if (ret != 4) {
        fprintf(stderr, "cannot fread\n");
        return NULL;
    }

    if (tmp_buffer[0] != 'M' || tmp_buffer[1] != 'F'
        || tmp_buffer[2] != 'S' || tmp_buffer[3] != 0) {
        fprintf(stderr, "invalid magic\n");
        return NULL;
    }

    tree = (folder_tree *) calloc(1, sizeof(folder_tree));

    /* read revision */
    ret = fread(&(tree->revision), sizeof(tree->revision), 1, stream);
    if (ret != 1) {
        fprintf(stderr, "cannot fread\n");
        return NULL;
    }

    /* read number of h_entries to read */
    ret = fread(&num_hts, sizeof(num_hts), 1, stream);
    if (ret != 1) {
        fprintf(stderr, "cannot fread\n");
        return NULL;
    }

    /* read root */
    ret = fread(&(tree->root), sizeof(tree->root), 1, stream);
    if (ret != 1) {
        fprintf(stderr, "cannot fread\n");
        return NULL;
    }
    /* zero out its array of children just in case */
    tree->root.num_children = 0;
    tree->root.children = NULL;

    /* to effectively map integer offsets to addresses we load the file into
     * an array of pointers to h_entry objects and free that array after we're
     * done with setting up the hashtable */

    /* populate the array of children */
    ordered_entries = (h_entry **) malloc(num_hts * sizeof(h_entry *));

    /* the first entry in this array points to the memory allocated for the
     * root */
    ordered_entries[0] = &(tree->root);

    /* read the remaining entries one by one */
    for (i = 1; i < num_hts; i++) {
        tmp_entry = (h_entry *) malloc(sizeof(h_entry));
        ret = fread(tmp_entry, sizeof(h_entry), 1, stream);
        if (ret != 1) {
            fprintf(stderr, "cannot fread\n");
            return NULL;
        }
        /* zero out the array of children */
        tmp_entry->num_children = 0;
        tmp_entry->children = NULL;
        /* store pointer to it in the array */
        ordered_entries[i] = tmp_entry;
    }

    /* turn the parent offset value into a pointer to the memory we allocated
     * earlier, populate the array of children for each entry and sort them
     * into the hashtable */
    for (i = 1; i < num_hts; i++) {
        /* the parent of this entry is at the given offset in the array */
        parent = ordered_entries[ordered_entries[i]->parent_offs];
        ordered_entries[i]->parent = parent;

        /* use the parent information to populate the array of children */
        parent->num_children++;
        parent->children =
            (h_entry **) realloc(parent->children,
                                 parent->num_children * sizeof(h_entry *));
        if (parent->children == NULL) {
            fprintf(stderr, "realloc failed\n");
            return NULL;
        }
        parent->children[parent->num_children - 1] = ordered_entries[i];

        /* put the entry into the hashtable */
        bucket_id = HASH_OF_KEY(ordered_entries[i]->key);
        tree->bucket_lens[bucket_id]++;
        tree->buckets[bucket_id] =
            (h_entry **) realloc(tree->buckets[bucket_id],
                                 tree->bucket_lens[bucket_id] *
                                 sizeof(h_entry *));
        if (tree->buckets[bucket_id] == NULL) {
            fprintf(stderr, "realloc failed\n");
            return NULL;
        }
        tree->buckets[bucket_id][tree->bucket_lens[bucket_id] - 1] =
            ordered_entries[i];
    }

    free(ordered_entries);

    return tree;
}

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
    free(tree->root.children);
    tree->root.children = NULL;
    tree->root.num_children = 0;
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

    if (key == NULL || key[0] == '\0') {
        return &(tree->root);
    }
    /* retrieve the right bucket for this key */
    bucket_id = HASH_OF_KEY(key);

    for (i = 0; i < tree->bucket_lens[bucket_id]; i++) {
        if (strcmp(tree->buckets[bucket_id][i]->key, key) == 0) {
            return tree->buckets[bucket_id][i];
        }
    }

    fprintf(stderr, "cannot find h_entry for key %s\n", key);
    return NULL;
}

/*
 * given a path, return the h_entry of the last component
 *
 * the path must start with a slash
 */
static h_entry *folder_tree_lookup_path(folder_tree * tree, const char *path)
{
    char           *tmp_path;
    char           *new_path;
    char           *slash_pos;
    h_entry        *curr_dir;
    h_entry        *result;
    uint64_t        i;
    bool            success;

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
    new_path = strdup(path + 1);
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
                    fprintf(stderr,
                            "A file can only be at the end of a path\n");
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

int folder_tree_getattr(folder_tree * tree, const char *path,
                        struct stat *stbuf)
{
    h_entry        *entry;

    entry = folder_tree_lookup_path(tree, path);

    if (entry == NULL) {
        return -ENOENT;
    }

    stbuf->st_uid = geteuid();
    stbuf->st_gid = getegid();
    stbuf->st_ctime = entry->ctime;
    stbuf->st_mtime = entry->ctime;
    if (entry->atime == 0) {
        /* folder */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = entry->num_children + 2;
        stbuf->st_atime = entry->ctime;
        stbuf->st_size = 1024;
    } else {
        /* file */
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_nlink = 1;
        stbuf->st_atime = entry->atime;
        stbuf->st_size = entry->fsize;
    }

    return 0;
}

int folder_tree_readdir(folder_tree * tree, const char *path, void *buf,
                        fuse_fill_dir_t filldir)
{
    h_entry        *entry;
    uint64_t        i;

    entry = folder_tree_lookup_path(tree, path);

    /* either directory not found or found entry is not a directory */
    if (entry == NULL || entry->atime != 0) {
        return -ENOENT;
    }

    filldir(buf, ".", NULL, 0);
    filldir(buf, "..", NULL, 0);

    for (i = 0; i < entry->num_children; i++) {
        filldir(buf, entry->children[i]->name, NULL, 0);
    }

    return 0;
}

static bool folder_tree_is_root(h_entry * entry)
{
    if (entry == NULL) {
        fprintf(stderr, "entry is NULL\n");
        return false;
    }

    return (entry->name[0] == '\0'
            || (strncmp(entry->name, "myfiles", sizeof(entry->name)) == 0))
        && entry->key[0] == '\0';
}

/*
 * given a key and the new parent, this function makes sure to allocate new
 * memory if necessary and adjust the children arrays of the former and new
 * parent to accommodate for the change
 */
static h_entry *folder_tree_allocate_entry(folder_tree * tree, const char *key,
                                           h_entry * new_parent)
{
    h_entry        *entry;
    int             bucket_id;
    h_entry        *old_parent;
    uint64_t        i;
    bool            found;

    if (tree == NULL) {
        fprintf(stderr, "tree cannot be NULL\n");
        return NULL;
    }

    if (new_parent == NULL) {
        fprintf(stderr, "new parent cannot be NULL\n");
        return NULL;
    }

    entry = folder_tree_lookup_key(tree, key);

    if (entry == NULL) {
        fprintf(stderr,
                "key is NULL but this is fine, we just create it now\n");
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

        /* since this entry is new, add it to the children of its parent
         *
         * since the key of this file or folder did not exist in the
         * hashtable, we do not have to check whether the parent already has
         * it as a child but can just append to its list of children
         */
        new_parent->num_children++;
        new_parent->children =
            (h_entry **) realloc(new_parent->children,
                                 new_parent->num_children * sizeof(h_entry *));
        if (new_parent->children == NULL) {
            fprintf(stderr, "realloc failed\n");
            return NULL;
        }
        new_parent->children[new_parent->num_children - 1] = entry;

        return entry;
    }

    /* Entry was found so check if the old parent is different from the
     * new parent. If yes, we need to adjust the children of the old and new
     * parent.
     */

    old_parent = entry->parent;
    /* parent stays the same - nothing to do */
    if (old_parent == new_parent)
        return entry;

    /* check whether entry does not have a parent (this is the case for the
     * root node) */
    if (old_parent != NULL) {
        /* remove the file or folder from the old parent */
        for (i = 0; i < old_parent->num_children; i++) {
            if (old_parent->children[i] == entry) {
                /* move the entries on the right one place to the left */
                memmove(old_parent->children[i], old_parent->children[i + 1],
                        sizeof(h_entry *) * (old_parent->num_children - i -
                                             1));
                old_parent->num_children--;
                /* change the children size */
                if (old_parent->num_children) {
                    free(old_parent->children);
                    old_parent->children = NULL;
                } else {
                    old_parent->children =
                        (h_entry **) realloc(old_parent->children,
                                             old_parent->num_children *
                                             sizeof(h_entry *));
                    if (old_parent->children == NULL) {
                        fprintf(stderr, "realloc failed\n");
                        return NULL;
                    }
                }
            }
        }
    } else {
        /* sanity check: if the parent was NULL then this entry must be the
         * root */
        if (!folder_tree_is_root(entry)) {
            fprintf(stderr,
                    "the parent was NULL so this node should be root but is not\n");
            fprintf(stderr, "name: %s, key: %s\n", entry->name, entry->key);
            return NULL;
        }
    }

    /* and add it to the new */
    /* since the entry already existed, it can be that the new parent
     * already contains the child */
    found = false;

    for (i = 0; i < new_parent->num_children; i++) {
        if (new_parent->children[i] == entry) {
            found = true;
            break;
        }
    }

    if (!found) {
        new_parent->num_children++;
        new_parent->children =
            (h_entry **) realloc(new_parent->children,
                                 new_parent->num_children * sizeof(h_entry *));
        if (new_parent->children == 0) {
            fprintf(stderr, "realloc failed\n");
            return NULL;
        }
        new_parent->children[new_parent->num_children - 1] = entry;
    }

    return entry;
}

/*
 * When adding an existing key, the old key is overwritten.
 * Return the inserted or updated key
 */
static h_entry *folder_tree_add_file(folder_tree * tree, mffile * file,
                                     h_entry * parent)
{
    h_entry        *entry;
    const char     *key;

    if (tree == NULL) {
        fprintf(stderr, "tree cannot be NULL\n");
        return NULL;
    }

    if (file == NULL) {
        fprintf(stderr, "file cannot be NULL\n");
        return NULL;
    }

    if (parent == NULL) {
        fprintf(stderr, "parent cannot be NULL\n");
        return NULL;
    }

    key = file_get_key(file);

    entry = folder_tree_allocate_entry(tree, key, parent);

    strncpy(entry->key, key, sizeof(entry->key));
    strncpy(entry->name, file_get_name(file), sizeof(entry->name));
    entry->parent = parent;
    entry->revision = file_get_revision(file);
    entry->ctime = file_get_created(file);
    entry->fsize = file_get_size(file);

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
static h_entry *folder_tree_add_folder(folder_tree * tree, mffolder * folder,
                                       h_entry * parent)
{
    h_entry        *entry;
    const char     *key;
    const char     *name;

    if (tree == NULL) {
        fprintf(stderr, "tree cannot be NULL\n");
        return NULL;
    }

    if (folder == NULL) {
        fprintf(stderr, "folder cannot be NULL\n");
        return NULL;
    }

    if (parent == NULL) {
        fprintf(stderr, "parent cannot be NULL\n");
        return NULL;
    }

    key = folder_get_key(folder);

    entry = folder_tree_allocate_entry(tree, key, parent);

    /* can be NULL for root */
    if (key != NULL)
        strncpy(entry->key, key, sizeof(entry->key));
    /* can be NULL for root */
    name = folder_get_name(folder);
    if (name != NULL)
        strncpy(entry->name, name, sizeof(entry->name));
    entry->revision = folder_get_revision(folder);
    entry->ctime = folder_get_created(folder);
    entry->parent = parent;

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
 *
 * This function does not use the parent member of the child. If you want to
 * rely on that, then use it directly.
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
    const char     *key;
    bool            found;

    /*
     * free the old children array of this folder to make sure that any
     * entries that do not exist on the remote are removed locally
     *
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
        key = folder_get_key(folder_result[i]);
        if (key == NULL) {
            fprintf(stderr, "folder_get_key returned NULL\n");
            folder_free(folder_result[i]);
        }
        /* if this folder existed before, then folder_tree_add_folder will not
         * add it to this folder's children. Thus we do this now */
        found = false;
        if (folder_tree_lookup_key(tree, key) != NULL) {
            curr_entry->num_children++;
            curr_entry->children =
                (h_entry **) realloc(curr_entry->children,
                                     curr_entry->num_children *
                                     sizeof(h_entry *));
            if (curr_entry->children == NULL) {
                fprintf(stderr, "realloc failed\n");
                return -1;
            }
            found = true;
        }
        entry = folder_tree_add_folder(tree, folder_result[i], curr_entry);
        /* we are using a new variable "found" here because after doing a
         * folder_tree_add_folder, folder_tree_lookup_key will always succeed
         *
         * if this entry was not present before, then folder_tree_add_folder
         * already added it to this folder's children
         */
        if (found) {
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
        key = file_get_key(file_result[i]);
        if (key == NULL) {
            fprintf(stderr, "file_get_key returned NULL\n");
            file_free(file_result[i]);
        }
        /* if this folder existed before, then folder_tree_add_folder will not
         * add it to this folder's children. Thus we do this now */
        found = false;
        if (folder_tree_lookup_key(tree, key) != NULL) {
            curr_entry->num_children++;
            curr_entry->children =
                (h_entry **) realloc(curr_entry->children,
                                     curr_entry->num_children *
                                     sizeof(h_entry *));
            if (curr_entry->children == NULL) {
                fprintf(stderr, "realloc failed\n");
                return -1;
            }
            found = true;
        }
        entry = folder_tree_add_file(tree, file_result[i], curr_entry);
        /* we are using a new variable "found" here because after doing a
         * folder_tree_add_folder, folder_tree_lookup_key will always succeed
         *
         * if this entry was not present before, then folder_tree_add_folder
         * already added it to this folder's children
         */
        if (found) {
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
    h_entry        *parent;

    file = file_alloc();

    retval = mfconn_api_file_get_info(conn, file, key);
    mfconn_update_secret_key(conn);
    if (retval != 0) {
        fprintf(stderr, "api call unsuccessful\n");
        /* TODO: check reason. It might be that the remote object does not
         * exist anymore in which case it has to be removed locally */
        file_free(file);
        return -1;
    }

    parent = folder_tree_lookup_key(tree, file_get_parent(file));
    if (parent == NULL) {
        fprintf(stderr, "file_tree_lookup_key failed\n");
        return -1;
    }

    folder_tree_add_file(tree, file, parent);

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
    h_entry        *parent;

    if (key != NULL && strcmp(key, "trash")) {
        fprintf(stderr, "cannot get folder info of trash\n");
        return -1;
    }

    folder = folder_alloc();

    retval = mfconn_api_folder_get_info(conn, folder, key);
    mfconn_update_secret_key(conn);
    if (retval != 0) {
        fprintf(stderr, "api call unsuccessful\n");
        /* TODO: check reason. It might be that the remote object does not
         * exist anymore in which case it has to be removed locally */
        folder_free(folder);
        return -1;
    }

    parent = folder_tree_lookup_key(tree, folder_get_parent(folder));
    if (parent == NULL) {
        fprintf(stderr, "folder_tree_lookup_key failed\n");
        return -1;
    }

    folder_tree_add_folder(tree, folder, parent);

    folder_free(folder);

    return 0;
}

/*
 * ask the remote if there are changes after the locally stored revision
 *
 * if yes, integrate those changes
 */
void folder_tree_update(folder_tree * tree, mfconn * conn)
{
    uint64_t        revision_remote;
    uint64_t        i;
    struct mfconn_device_change *changes;
    int             retval;
    h_entry        *tmp_entry;

    mfconn_api_device_get_status(conn, &revision_remote);
    mfconn_update_secret_key(conn);

    if (tree->revision == revision_remote) {
        fprintf(stderr, "Request to update but nothing to do\n");
        return;
    }

    /*
     * we maintain the information of each entries parent but that does not
     * mean that we can rely on it when fetching updates via
     * device/get_changes. If a remote object has been permanently removed
     * (trash was emptied) then it will not show up in the results of
     * device/get_changes and thus a remote file will vanish without that file
     * showing up in the device/get_changes output. The only way to clean up
     * those removed files is to use folder/get_content for all folders that
     * changed.
     */

    /*
     * we have to manually check the root because it never shows up in the
     * results from device_get_changes
     *
     * we don't need to be recursive here because we rely on
     * device/get_changes reporting all changes to its children
     */

    folder_tree_rebuild_helper(tree, conn, &(tree->root), false);

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

    for (i = 0; changes[i].change != MFCONN_DEVICE_CHANGE_END; i++) {
        switch (changes[i].change) {
            case MFCONN_DEVICE_CHANGE_DELETED_FOLDER:
            case MFCONN_DEVICE_CHANGE_DELETED_FILE:
                folder_tree_remove(tree, changes[i].key);
                break;
            case MFCONN_DEVICE_CHANGE_UPDATED_FOLDER:
                /* ignore updates of the folder key "trash" or folders with
                 * the parent folder key "trash" */
                if (strcmp(changes[i].key, "trash") == 0)
                    continue;
                if (strcmp(changes[i].parent, "trash") == 0)
                    continue;
                /* if a folder has been updated then its name or location
                 * might have changed... */
                folder_tree_update_folder_info(tree, conn, changes[i].key);
                /* ...or its contents changed
                 * the last call made sure that an entry for this folder has
                 * been added locally, so the following should succeed */
                tmp_entry = folder_tree_lookup_key(tree, changes[i].key);
                if (tmp_entry == NULL) {
                    fprintf(stderr, "folder_tree_lookup_key failed\n");
                    continue;
                }
                /* we don't need to be recursive here because we rely on
                 * device/get_changes reporting all changes to its children */
                folder_tree_rebuild_helper(tree, conn, tmp_entry, false);
                break;
            case MFCONN_DEVICE_CHANGE_UPDATED_FILE:
                /* ignore files updated in trash */
                if (strcmp(changes[i].parent, "trash") == 0)
                    continue;
                /* if a file changed, update its info */
                folder_tree_update_file_info(tree, conn, changes[i].key);
                break;
            case MFCONN_DEVICE_CHANGE_END:
                break;
        }
    }

    /* the new revision of the tree is the revision of the terminating change
     * */
    tree->revision = changes[i].revision;

    /*
     * it can happen that another change happened remotely while we were
     * trying to integrate the changes reported by the last device/get_changes
     * results. In that case, the file and folder information we retrieve will
     * have a revision greater than the local device revision we store. This
     * can also lead to lost changes. But this will only be temporary as the
     * situation should be rectified once the next device/get_changes is done.
     *
     * Just remember that due to this it can happen that the revision of the
     * local tree is less than the highest revision of a h_entry it stores.
     */

    /* now fix up any possible errors */

    /* clean the resulting folder_tree of any dangling objects */
    fprintf(stderr, "tree before cleaning:\n");
    folder_tree_debug(tree, NULL, 0);
    folder_tree_housekeep(tree, conn);
    fprintf(stderr, "tree after cleaning:\n");
    folder_tree_debug(tree, NULL, 0);
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

    return 0;
}

/*
 * clean up files and folders that are never referenced
 *
 * first find all folders that have children that do not reference their
 * parent. If a discrepancy is found, ask the remote for the true list of
 * children of that folder.
 *
 * then find all files and folders that have a parent that does not reference
 * them. If a discrepancy is found, ask the remote for the true parent of that
 * file.
 */

void folder_tree_housekeep(folder_tree * tree, mfconn * conn)
{
    uint64_t        i,
                    j,
                    k;
    bool            found;

    /*
     * find objects with children who claim to have a different parent
     */

    /* first check the root as a special case */

    found = false;
    for (k = 0; k < tree->root.num_children; k++) {
        /* only compare pointers and not keys. This relies on keys
         * being unique */
        if (tree->root.children[k]->parent != &(tree->root)) {
            fprintf(stderr,
                    "root claims that %s is its child but %s doesn't think so\n",
                    tree->root.children[k]->key, tree->root.children[k]->key);
            found = true;
            break;
        }
    }
    if (found) {
        folder_tree_rebuild_helper(tree, conn, &(tree->root), false);
    }

    /* then check the hashtable */
    for (i = 0; i < NUM_BUCKETS; i++) {
        for (j = 0; j < tree->bucket_lens[i]; j++) {
            found = false;
            for (k = 0; k < tree->buckets[i][j]->num_children; k++) {
                /* only compare pointers and not keys. This relies on keys
                 * being unique */
                if (tree->buckets[i][j]->children[k]->parent !=
                    tree->buckets[i][j]) {
                    fprintf(stderr,
                            "%s claims that %s is its child but %s doesn't think so\n",
                            tree->buckets[i][j]->key,
                            tree->buckets[i][j]->children[k]->key,
                            tree->buckets[i][j]->children[k]->key);
                    found = true;
                    break;
                }
            }
            if (found) {
                /* an entry was found that claims to have a different parent,
                 * so ask the remote to retrieve the real list of children */
                folder_tree_rebuild_helper(tree, conn, tree->buckets[i][j],
                                           false);
            }
        }
    }

    /* find objects whose parents do not match their actual parents */
    for (i = 0; i < NUM_BUCKETS; i++) {
        for (j = 0; j < tree->bucket_lens[i]; j++) {
            if (!folder_tree_is_parent_of
                (tree->buckets[i][j]->parent, tree->buckets[i][j])) {
                fprintf(stderr,
                        "%s claims that %s is its parent but it is not\n",
                        tree->buckets[i][j]->key,
                        tree->buckets[i][j]->parent->key);
                if (tree->buckets[i][j]->atime == 0) {
                    /* folder */
                    folder_tree_update_folder_info(tree, conn,
                                                   tree->buckets[i][j]->key);
                } else {
                    /* file */
                    folder_tree_update_file_info(tree, conn,
                                                 tree->buckets[i][j]->key);
                }
            }
        }
    }

    /* TODO: remove unreferenced cached files */

    /*
     * for file in cache directory:
     *     e = folder_tree_lookup_key(file.key)
     *     if e == None:
     *         delete(file)
     */
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
            fprintf(stderr, "%*s d:%s k:%s p:%s\n", depth + 1, " ",
                    ent->children[i]->name, ent->children[i]->key,
                    ent->children[i]->parent->key);
            folder_tree_debug(tree, ent->children[i], depth + 1);
        } else {
            /* file */
            fprintf(stderr, "%*s f:%s k:%s p:%s\n", depth + 1, " ",
                    ent->children[i]->name, ent->children[i]->key,
                    ent->children[i]->parent->key);
        }
    }
}
