/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * sb-trie.c - Single-byte domain trie front end
 * Created: 2006-08-19
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#include <string.h>
#include <stdlib.h>

#include "sb-trie.h"
#include "trie.h"
#include "alpha-map.h"

/*------------------------*
 *   INTERNAL FUNCTIONS   *
 *------------------------*/
static TrieChar * sb_map_char_to_alphabet_str (const AlphaMap   *alpha_map,
                                               const SBChar     *str);

static SBChar * sb_map_alphabet_to_char_str (const AlphaMap     *alpha_map,
                                             const TrieChar     *str);

/* ==================== BEGIN IMPLEMENTATION PART ====================  */

/*----------------------------------------*
 *   INTERNAL FUNCTIONS IMPLEMENTATIONS   *
 *----------------------------------------*/

static TrieChar *
sb_map_char_to_alphabet_str (const AlphaMap *alpha_map, const SBChar *str)
{
    TrieChar   *alphabet_str, *p;

    alphabet_str = (TrieChar *) malloc (strlen (str) + 1);
    for (p = alphabet_str; *str; p++, str++)
        *p = alpha_map_char_to_alphabet (alpha_map, *str);
    *p = '\0';

    return alphabet_str;
}

static SBChar *
sb_map_alphabet_to_char_str (const AlphaMap *alpha_map, const TrieChar *str)
{
    SBChar     *sb_str, *p;

    sb_str = (SBChar *) malloc (strlen ((const char *)str) + 1);
    for (p = sb_str; *str; p++, str++)
        *p = (SBChar) alpha_map_alphabet_to_char (alpha_map, *str);
    *p = '\0';

    return sb_str;
}

/*------------------------------*
 *    PRIVATE DATA DEFINITONS   *
 *------------------------------*/
struct _SBTrie {
    Trie       *trie;
    AlphaMap   *alpha_map;
};

struct _SBTrieState {
    SBTrie         *sb_trie;
    TrieState      *trie_state;
};

/*-----------------------------*
 *    METHODS IMPLEMENTAIONS   *
 *-----------------------------*/

/*-----------------------*
 *   GENERAL FUNCTIONS   *
 *-----------------------*/

SBTrie *
sb_trie_open (const char *path, const char *name, TrieIOMode mode)
{
    SBTrie *sb_trie;

    sb_trie = (SBTrie *) malloc (sizeof (SBTrie));
    if (!sb_trie)
        return NULL;

    sb_trie->trie = trie_open (path, name, mode);
    if (!sb_trie->trie)
        goto exit1;

    sb_trie->alpha_map = alpha_map_open (path, name, ".sbm");
    if (!sb_trie->alpha_map)
        goto exit2;

    return sb_trie;

exit2:
    trie_close (sb_trie->trie);
exit1:
    free (sb_trie);
    return NULL;
}

int
sb_trie_close (SBTrie *sb_trie)
{
    if (!sb_trie)
        return -1;

    alpha_map_free (sb_trie->alpha_map);
    return trie_close (sb_trie->trie);
}

int
sb_trie_save (SBTrie *sb_trie)
{
    if (!sb_trie)
        return -1;

    return trie_save (sb_trie->trie);
}


/*------------------------------*
 *   GENERAL QUERY OPERATIONS   *
 *------------------------------*/

Bool
sb_trie_retrieve (SBTrie *sb_trie, const SBChar *key, TrieData *o_data)
{
    TrieChar   *trie_key;
    Bool        ret;

    if (!sb_trie)
        return FALSE;

    trie_key = sb_map_char_to_alphabet_str (sb_trie->alpha_map, key);
    ret = trie_retrieve (sb_trie->trie, trie_key, o_data);
    free (trie_key);

    return ret;
}

Bool
sb_trie_store (SBTrie *sb_trie, const SBChar *key, TrieData data)
{
    TrieChar   *trie_key;
    Bool        ret;

    if (!sb_trie)
        return FALSE;

    trie_key = sb_map_char_to_alphabet_str (sb_trie->alpha_map, key);
    ret = trie_store (sb_trie->trie, trie_key, data);
    free (trie_key);

    return ret;
}

Bool
sb_trie_delete (SBTrie *sb_trie, const SBChar *key)
{
    TrieChar   *trie_key;
    Bool        ret;

    if (!sb_trie)
        return FALSE;

    trie_key = sb_map_char_to_alphabet_str (sb_trie->alpha_map, key);
    ret = trie_delete (sb_trie->trie, trie_key);
    free (trie_key);

    return ret;
}

typedef struct {
    SBTrie         *sb_trie;
    SBTrieEnumFunc  enum_func;
    void           *user_data;
} _SBTrieEnumData;

static Bool
sb_trie_enum_func (const TrieChar *key, TrieData key_data, void *user_data)
{
    _SBTrieEnumData *enum_data;
    SBChar          *sb_key;
    Bool             ret;

    enum_data = (_SBTrieEnumData *) user_data;

    sb_key = sb_map_alphabet_to_char_str (enum_data->sb_trie->alpha_map, key);
    ret = (*enum_data->enum_func) (sb_key, key_data, enum_data->user_data);
    free (sb_key);

    return ret;
}

Bool
sb_trie_enumerate (SBTrie         *sb_trie,
                   SBTrieEnumFunc  enum_func,
                   void           *user_data)
{
    _SBTrieEnumData enum_data;

    if (!sb_trie)
        return FALSE;

    enum_data.sb_trie   = sb_trie;
    enum_data.enum_func = enum_func;
    enum_data.user_data = user_data;

    return trie_enumerate (sb_trie->trie, sb_trie_enum_func, &enum_data);
}


/*-------------------------------*
 *   STEPWISE QUERY OPERATIONS   *
 *-------------------------------*/

SBTrieState *
sb_trie_root (SBTrie *sb_trie)
{
    SBTrieState *sb_state;

    if (!sb_trie)
        return NULL;

    sb_state = (SBTrieState *) malloc (sizeof (SBTrieState));
    if (!sb_state)
        return NULL;

    sb_state->sb_trie    = sb_trie;
    sb_state->trie_state = trie_root (sb_trie->trie);

    return sb_state;
}


/*----------------*
 *   TRIE STATE   *
 *----------------*/

SBTrieState *
sb_trie_state_clone (const SBTrieState *s)
{
    SBTrieState *new_state;

    if (!s)
        return NULL;

    new_state = (SBTrieState *) malloc (sizeof (SBTrieState));
    if (!new_state)
        return NULL;

    new_state->sb_trie    = s->sb_trie;
    new_state->trie_state = trie_state_clone (s->trie_state);

    return new_state;
}

void
sb_trie_state_free (SBTrieState *s)
{
    if (!s)
        return;

    trie_state_free (s->trie_state);
    free (s);
}

void
sb_trie_state_rewind (SBTrieState *s)
{
    if (!s)
        return;

    trie_state_rewind (s->trie_state);
}

Bool
sb_trie_state_walk (SBTrieState *s, SBChar c)
{
    if (!s)
        return FALSE;

    return trie_state_walk (s->trie_state,
                            alpha_map_char_to_alphabet (s->sb_trie->alpha_map,
                                                        (UniChar) c));
}

Bool
sb_trie_state_is_walkable (const SBTrieState *s, SBChar c)
{
    if (!s)
        return FALSE;

    return trie_state_is_walkable (
               s->trie_state,
               alpha_map_char_to_alphabet (s->sb_trie->alpha_map, (UniChar) c)
           );
}

Bool
sb_trie_state_is_terminal (const SBTrieState *s)
{
    if (!s)
        return FALSE;

    return trie_state_is_terminal (s->trie_state);
}

Bool
sb_trie_state_is_leaf (const SBTrieState *s)
{
    if (!s)
        return FALSE;

    return trie_state_is_leaf (s->trie_state);
}

TrieData
sb_trie_state_get_data (const SBTrieState *s)
{
    if (!s)
        return TRIE_DATA_ERROR;

    return trie_state_get_data (s->trie_state);
}

/*
vi:ts=4:ai:expandtab
*/
