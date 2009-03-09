/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * alpha-map.c - map between character codes and trie alphabet
 * Created: 2006-08-19
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "alpha-map.h"
#include "fileutils.h"

/*-----------------------------------*
 *    PRIVATE METHODS DECLARATIONS   *
 *-----------------------------------*/
static AlphaMap * alpha_map_new ();

/*------------------------------*
 *    PRIVATE DATA DEFINITONS   *
 *------------------------------*/

typedef struct _AlphaRange {
    struct _AlphaRange *next;

    UniChar             begin;
    UniChar             end;
} AlphaRange;

struct _AlphaMap {
    AlphaRange     *first_range;
    AlphaRange     *last_range;
};

/*-----------------------------*
 *    METHODS IMPLEMENTAIONS   *
 *-----------------------------*/

AlphaMap *
alpha_map_open (const char *path, const char *name, const char *ext)
{
    FILE       *file;
    char        line[256];
    AlphaMap   *alpha_map;

    file = file_open (path, name, ext, TRIE_IO_READ);
    if (!file)
        return NULL;

    /* prepare data */
    alpha_map = alpha_map_new ();
    if (!alpha_map)
        goto exit1;

    /* read character ranges */
    while (fgets (line, sizeof line, file)) {
        AlphaRange *range;
        int         b, e;

        range = (AlphaRange *) malloc (sizeof (AlphaRange));

        /* read the range
         * format: [b,e]
         * where: b = begin char, e = end char; both in hex values
         */ 
        if (sscanf (line, " [ %x , %x ] ", &b, &e) != 2)
            continue;
        if (b > e) {
            fprintf (stderr, "Range begin (%x) > range end (%x)\n", b, e);
            free (range);
            continue;
        }
        range->begin = b;
        range->end   = e;

        /* append it to list of ranges */
        range->next = NULL;
        if (alpha_map->last_range)
            alpha_map->last_range->next = range;
        else
            alpha_map->first_range = range;
        alpha_map->last_range = range;
    }

    fclose (file);
    return alpha_map;

exit1:
    fclose (file);
    return NULL;
}

static AlphaMap *
alpha_map_new ()
{
    AlphaMap   *alpha_map;

    alpha_map = (AlphaMap *) malloc (sizeof (AlphaMap));
    if (!alpha_map)
        return NULL;

    alpha_map->first_range = alpha_map->last_range = NULL;

    return alpha_map;
}

void
alpha_map_free (AlphaMap *alpha_map)
{
    AlphaRange *p, *q;

    p = alpha_map->first_range; 
    while (p) {
        q = p->next;
        free (p);
        p = q;
    }

    free (alpha_map);
}

TrieChar
alpha_map_char_to_alphabet (const AlphaMap *alpha_map, UniChar uc)
{
    TrieChar    alpha_begin;
    AlphaRange *range;

    if (uc == 0)
        return 0;

    alpha_begin = 1;
    for (range = alpha_map->first_range;
         range && (uc < range->begin || range->end < uc);
         range = range->next)
    {
        alpha_begin += range->end - range->begin + 1;
    }
    if (range)
        return alpha_begin + (uc - range->begin);

    return TRIE_CHAR_MAX;
}

UniChar
alpha_map_alphabet_to_char (const AlphaMap *alpha_map, TrieChar tc)
{
    TrieChar    alpha_begin;
    AlphaRange *range;

    if (tc == 0)
        return 0;

    alpha_begin = 1;
    for (range = alpha_map->first_range;
         range && alpha_begin + (range->end - range->begin) < tc;
         range = range->next)
    {
        alpha_begin += range->end - range->begin + 1;
    }
    if (range)
        return range->begin + (tc - alpha_begin);

    return UNI_CHAR_ERROR;
}

/*
vi:ts=4:ai:expandtab
*/
