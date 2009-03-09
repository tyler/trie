/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * alpha-map.h - map between character codes and trie alphabet
 * Created: 2006-08-19
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#ifndef __ALPHA_MAP_H
#define __ALPHA_MAP_H

#include "typedefs.h"
#include "triedefs.h"

typedef uint16  UniChar;

#define UNI_CHAR_ERROR   (~(UniChar)0)

typedef struct _AlphaMap    AlphaMap;

AlphaMap *  alpha_map_open (const char *path,
                            const char *name,
                            const char *ext);

void        alpha_map_free (AlphaMap *alpha_map);

TrieChar    alpha_map_char_to_alphabet (const AlphaMap *alpha_map, UniChar uc);

UniChar     alpha_map_alphabet_to_char (const AlphaMap *alpha_map, TrieChar tc);


#endif /* __ALPHA_MAP_H */


/*
vi:ts=4:ai:expandtab
*/
