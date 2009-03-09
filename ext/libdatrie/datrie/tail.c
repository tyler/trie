/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * tail.c - trie tail for keeping suffixes
 * Created: 2006-08-15
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "tail.h"
#include "fileutils.h"

/*----------------------------------*
 *    INTERNAL TYPES DECLARATIONS   *
 *----------------------------------*/

/*-----------------------------------*
 *    PRIVATE METHODS DECLARATIONS   *
 *-----------------------------------*/

static TrieIndex    tail_alloc_block (Tail *t);
static void         tail_free_block (Tail *t, TrieIndex block);

/* ==================== BEGIN IMPLEMENTATION PART ====================  */

/*------------------------------------*
 *   INTERNAL TYPES IMPLEMENTATIONS   *
 *------------------------------------*/

/*------------------------------*
 *    PRIVATE DATA DEFINITONS   *
 *------------------------------*/

typedef struct {
    TrieIndex   next_free;
    TrieData    data;
    TrieChar   *suffix;
} TailBlock;

struct _Tail {
    TrieIndex   num_tails;
    TailBlock  *tails;
    TrieIndex   first_free;

    FILE       *file;
    Bool        is_dirty;
};

/*-----------------------------*
 *    METHODS IMPLEMENTAIONS   *
 *-----------------------------*/

#define TAIL_SIGNATURE      0xDFFD
#define TAIL_START_BLOCKNO  1

Tail *
tail_open (const char *path, const char *name, TrieIOMode mode)
{
    Tail       *t;
    TrieIndex   i;
    uint16      sig;
    long        file_size;

    t = (Tail *) malloc (sizeof (Tail));

    t->file = file_open (path, name, ".tl", mode);
    if (!t->file)
        goto exit1;

    file_size = file_length (t->file);
    if (file_size != 0 && file_read_int16 (t->file, (int16 *) &sig)
        && sig != TAIL_SIGNATURE)
    {
        goto exit2;
    }

    /* init tails data */
    if (file_size == 0) {
        t->first_free = 0;
        t->num_tails  = 0;
        t->tails      = NULL;
        t->is_dirty   = TRUE;
    } else {
        file_read_int16 (t->file, &t->first_free);
        file_read_int16 (t->file, &t->num_tails);
        t->tails = (TailBlock *) malloc (t->num_tails * sizeof (TailBlock));
        if (!t->tails)
            goto exit2;
        for (i = 0; i < t->num_tails; i++) {
            int8    length;

            file_read_int16 (t->file, &t->tails[i].next_free);
            file_read_int16 (t->file, &t->tails[i].data);

            file_read_int8 (t->file, &length);
            t->tails[i].suffix    = (TrieChar *) malloc (length + 1);
            if (length > 0)
                file_read_chars (t->file, (char *)t->tails[i].suffix, length);
            t->tails[i].suffix[length] = '\0';
        }
        t->is_dirty = FALSE;
    }

    return t;

exit2:
    fclose (t->file);
exit1:
    free (t);
    return NULL;
}

int
tail_close (Tail *t)
{
    int         ret;
    TrieIndex   i;

    if (0 != (ret = tail_save (t)))
        return ret;
    if (0 != (ret = fclose (t->file)))
        return ret;
    if (t->tails) {
        for (i = 0; i < t->num_tails; i++)
            if (t->tails[i].suffix)
                free (t->tails[i].suffix);
        free (t->tails);
    }
    free (t);

    return 0;
}

int
tail_save (Tail *t)
{
    TrieIndex   i;

    if (!t->is_dirty)
        return 0;

    rewind (t->file);
    if (!file_write_int16 (t->file, TAIL_SIGNATURE) ||
        !file_write_int16 (t->file, t->first_free)  ||
        !file_write_int16 (t->file, t->num_tails))
    {
        return -1;
    }
    for (i = 0; i < t->num_tails; i++) {
        int8    length;

        if (!file_write_int16 (t->file, t->tails[i].next_free) ||
            !file_write_int16 (t->file, t->tails[i].data))
        {
            return -1;
        }

        length = t->tails[i].suffix ? strlen ((const char *)t->tails[i].suffix)
                                    : 0;
        if (!file_write_int8 (t->file, length))
            return -1;
        if (length > 0 &&
            !file_write_chars (t->file, (char *)t->tails[i].suffix, length))
        {
            return -1;
        }
    }
    t->is_dirty = FALSE;

    return 0;
}


const TrieChar *
tail_get_suffix (const Tail *t, TrieIndex index)
{
    index -= TAIL_START_BLOCKNO;
    return (index < t->num_tails) ? t->tails[index].suffix : NULL;
}

Bool
tail_set_suffix (Tail *t, TrieIndex index, const TrieChar *suffix)
{
    index -= TAIL_START_BLOCKNO;
    if (index < t->num_tails) {
        /* suffix and t->tails[index].suffix may overlap;
         * so, dup it before it's overwritten
         */
        TrieChar *tmp = NULL;
        if (suffix)
            tmp = strdup (suffix);
        if (t->tails[index].suffix)
            free (t->tails[index].suffix);
        t->tails[index].suffix = tmp;

        t->is_dirty = TRUE;
        return TRUE;
    }
    return FALSE;
}

TrieIndex
tail_add_suffix (Tail *t, const TrieChar *suffix)
{
    TrieIndex   new_block;

    new_block = tail_alloc_block (t);
    tail_set_suffix (t, new_block, suffix);

    return new_block;
}

static TrieIndex
tail_alloc_block (Tail *t)
{
    TrieIndex   block;

    if (0 != t->first_free) {
        block = t->first_free;
        t->first_free = t->tails[block].next_free;
    } else {
        block = t->num_tails;
        t->tails = (TailBlock *) realloc (t->tails,
                                          ++t->num_tails * sizeof (TailBlock));
    }
    t->tails[block].next_free = -1;
    t->tails[block].data = TRIE_DATA_ERROR;
    t->tails[block].suffix = NULL;
    
    return block + TAIL_START_BLOCKNO;
}

static void
tail_free_block (Tail *t, TrieIndex block)
{
    TrieIndex   i, j;

    block -= TAIL_START_BLOCKNO;

    if (block >= t->num_tails)
        return;

    t->tails[block].data = TRIE_DATA_ERROR;
    if (NULL != t->tails[block].suffix) {
        free (t->tails[block].suffix);
        t->tails[block].suffix = NULL;
    }

    /* find insertion point */
    j = 0;
    for (i = t->first_free; i != 0 && i < block; i = t->tails[i].next_free)
        j = i;

    /* insert free block between j and i */
    t->tails[block].next_free = i;
    if (0 != j)
        t->tails[j].next_free = block;
    else
        t->first_free = block;

    t->is_dirty = TRUE;
}

TrieData
tail_get_data (Tail *t, TrieIndex index)
{
    index -= TAIL_START_BLOCKNO;
    return (index < t->num_tails) ? t->tails[index].data : TRIE_DATA_ERROR;
}

Bool
tail_set_data (Tail *t, TrieIndex index, TrieData data)
{
    index -= TAIL_START_BLOCKNO;
    if (index < t->num_tails) {
        t->tails[index].data = data;
        t->is_dirty = TRUE;
        return TRUE;
    }
    return FALSE;
}

void
tail_delete (Tail *t, TrieIndex index)
{
    tail_free_block (t, index);
}

int
tail_walk_str  (Tail            *t,
                TrieIndex        s,
                short           *suffix_idx,
                const TrieChar  *str,
                int              len)
{
    const TrieChar *suffix;
    int             i;
    short           j;

    suffix = tail_get_suffix (t, s);
    if (!suffix)
        return FALSE;

    i = 0; j = *suffix_idx;
    while (i < len) {
        if (str[i] != suffix[j])
            break;
        ++i;
        /* stop and stay at null-terminator */
        if (0 == suffix[j])
            break;
        ++j;
    }
    *suffix_idx = j;
    return i;
}

Bool
tail_walk_char (Tail            *t,
                TrieIndex        s,
                short           *suffix_idx,
                TrieChar         c)
{
    const TrieChar *suffix;
    TrieChar        suffix_char;

    suffix = tail_get_suffix (t, s);
    if (!suffix)
        return FALSE;

    suffix_char = suffix[*suffix_idx];
    if (suffix_char == c) {
        if (0 != suffix_char)
            ++*suffix_idx;
        return TRUE;
    }
    return FALSE;
}

/*
vi:ts=4:ai:expandtab
*/
