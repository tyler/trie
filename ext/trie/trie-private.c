#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "darray.h"
#include "tail.h"
#include "trie.h"

Trie* trie_new() {
	Trie *trie = (Trie*) malloc(sizeof(Trie));
	trie->da = da_new();
	trie->tail = tail_new();
	return trie;
}

void trie_free(Trie *trie) {
	da_free(trie->da);
	tail_free(trie->tail);
	free(trie);
}

static Bool trie_branch_in_branch (Trie *trie, TrieIndex sep_node, const TrieChar *suffix, TrieData data) {
    TrieIndex new_da, new_tail;

    new_da = da_insert_branch (trie->da, sep_node, *suffix);
    if (TRIE_INDEX_ERROR == new_da)
        return FALSE;

    if ('\0' != *suffix)
        ++suffix;

    new_tail = tail_add_suffix (trie->tail, suffix);
    tail_set_data (trie->tail, new_tail, data);
    trie_da_set_tail_index (trie->da, new_da, new_tail);

    // trie->is_dirty = TRUE;
    return TRUE;
}

static Bool trie_branch_in_tail(Trie *trie, TrieIndex sep_node, const TrieChar *suffix, TrieData data) {
    TrieIndex old_tail, old_da, s;
    const TrieChar *old_suffix, *p;

    /* adjust separate point in old path */
    old_tail = trie_da_get_tail_index (trie->da, sep_node);
    old_suffix = tail_get_suffix (trie->tail, old_tail);
    if (!old_suffix)
        return FALSE;

    for (p = old_suffix, s = sep_node; *p == *suffix; p++, suffix++) {
        TrieIndex t = da_insert_branch (trie->da, s, *p);
        if (TRIE_INDEX_ERROR == t)
            goto fail;
        s = t;
    }

    old_da = da_insert_branch (trie->da, s, *p);
    if (TRIE_INDEX_ERROR == old_da)
        goto fail;

    if ('\0' != *p)
        ++p;
    tail_set_suffix (trie->tail, old_tail, p);
    trie_da_set_tail_index (trie->da, old_da, old_tail);

    /* insert the new branch at the new separate point */
    return trie_branch_in_branch (trie, s, suffix, data);

fail:
    /* failed, undo previous insertions and return error */
    da_prune_upto (trie->da, sep_node, s);
    trie_da_set_tail_index (trie->da, sep_node, old_tail);
    return FALSE;
}

Bool trie_store (Trie *trie, const TrieChar *key, TrieData data) {
    TrieIndex        s, t;
    short            suffix_idx;
    const TrieChar *p, *sep;
	size_t len;

    /* walk through branches */
    s = da_get_root (trie->da);
    for (p = key; !trie_da_is_separate (trie->da, s); p++) {
        if (!da_walk (trie->da, &s, *p))
            return trie_branch_in_branch (trie, s, p, data);
        if (0 == *p)
            break;
    }

    /* walk through tail */
    sep = p;
    t = trie_da_get_tail_index (trie->da, s);
    suffix_idx = 0;
    len = strlen ((const char *) p) + 1;    /* including null-terminator */
    if (tail_walk_str (trie->tail, t, &suffix_idx, p, len) != len)
        return trie_branch_in_tail (trie, s, p, data);

    /* duplicated key, overwrite val */
    tail_set_data (trie->tail, t, data);
    // trie->is_dirty = TRUE;
    return TRUE;
}


Bool trie_has_key (const Trie *trie, const TrieChar *key) {
    TrieIndex        s;
    short            suffix_idx;
    const TrieChar *p;

    /* walk through branches */
    s = da_get_root (trie->da);
    for (p = key; !trie_da_is_separate (trie->da, s); p++) {
        if (!da_walk (trie->da, &s, *p))
            return FALSE;
        if (0 == *p)
            break;
    }

    /* walk through tail */
    s = trie_da_get_tail_index (trie->da, s);
    suffix_idx = 0;
    for ( ; ; p++) {
        if (!tail_walk_char (trie->tail, s, &suffix_idx, *p))
            return FALSE;
        if (0 == *p)
            break;
    }

    return TRUE;
}


Bool trie_retrieve (const Trie *trie, const TrieChar *key, TrieData *o_data) {
    TrieIndex        s;
    short            suffix_idx;
    const TrieChar *p;

    /* walk through branches */
    s = da_get_root (trie->da);
    for (p = key; !trie_da_is_separate (trie->da, s); p++) {
        if (!da_walk (trie->da, &s, *p))
            return FALSE;
        if (0 == *p)
            break;
    }

    /* walk through tail */
    s = trie_da_get_tail_index (trie->da, s);
    suffix_idx = 0;
    for ( ; ; p++) {
        if (!tail_walk_char (trie->tail, s, &suffix_idx, *p))
            return FALSE;
        if (0 == *p)
            break;
    }

    /* found, set the val and return */
    if (o_data)
        *o_data = tail_get_data (trie->tail, s);
    return TRUE;
}

Bool trie_delete (Trie *trie, const TrieChar *key) {
    TrieIndex        s, t;
    short            suffix_idx;
    const TrieChar *p;

    /* walk through branches */
    s = da_get_root (trie->da);
    for (p = key; !trie_da_is_separate (trie->da, s); p++) {
        if (!da_walk (trie->da, &s, *p))
            return FALSE;
        if (0 == *p)
            break;
    }

    /* walk through tail */
    t = trie_da_get_tail_index (trie->da, s);
    suffix_idx = 0;
    for ( ; ; p++) {
        if (!tail_walk_char (trie->tail, t, &suffix_idx, *p))
            return FALSE;
        if (0 == *p)
            break;
    }

    tail_delete (trie->tail, t);
    da_set_base (trie->da, s, TRIE_INDEX_ERROR);
    da_prune (trie->da, s);

    //trie->is_dirty = TRUE;
    return TRUE;
}

/*-------------------------------*
 *   STEPWISE QUERY OPERATIONS   *
 *-------------------------------*/

TrieState * trie_root (const Trie *trie) {
    return trie_state_new (trie, da_get_root (trie->da), 0, FALSE);
}

/*----------------*
 *   TRIE STATE   *
 *----------------*/

static TrieState * trie_state_new (const Trie *trie, TrieIndex index, short suffix_idx, short is_suffix) {
    TrieState *s;

    s = (TrieState *) malloc (sizeof (TrieState));
    if (!s)
        return NULL;

    s->trie       = trie;
    s->index      = index;
    s->suffix_idx = suffix_idx;
    s->is_suffix  = is_suffix;

    return s;
}

TrieState * trie_state_clone (const TrieState *s) {
    return trie_state_new (s->trie, s->index, s->suffix_idx, s->is_suffix);
}

void trie_state_free (TrieState *s) {
    free (s);
}

void trie_state_rewind (TrieState *s) {
    s->index      = da_get_root (s->trie->da);
    s->is_suffix  = FALSE;
}

Bool trie_state_walk (TrieState *s, TrieChar c) {
    if (!s->is_suffix) {
        Bool ret;

        ret = da_walk (s->trie->da, &s->index, c);

        if (ret && trie_da_is_separate (s->trie->da, s->index)) {
            s->index = trie_da_get_tail_index (s->trie->da, s->index);
            s->suffix_idx = 0;
            s->is_suffix = TRUE;
        }

        return ret;
    } else {
        return tail_walk_char (s->trie->tail, s->index, &s->suffix_idx, c);
    }
}

Bool trie_state_is_walkable (const TrieState *s, TrieChar c) {
    if (!s->is_suffix)
        return da_is_walkable (s->trie->da, s->index, c);
    else 
        return tail_is_walkable_char (s->trie->tail, s->index, s->suffix_idx, c);
}

Bool trie_state_is_leaf (const TrieState *s) {
    return s->is_suffix && trie_state_is_terminal (s);
}

TrieData trie_state_get_data (const TrieState *s) {
    return s->is_suffix ? tail_get_data (s->trie->tail, s->index) : TRIE_DATA_ERROR;
}

int main(void) {
	Bool res;
	TrieData *data = (TrieData*)malloc(sizeof(TrieData));
	Trie *trie = trie_new();


	trie_store(trie, (const TrieChar*)"hello", 1);
	trie_store(trie, (const TrieChar*)"he", 4);
	trie_store(trie, (const TrieChar*)"hel", 3);
	trie_store(trie, (const TrieChar*)"h", 5);
	trie_store(trie, (const TrieChar*)"hell", 2);


	res = trie_retrieve(trie, (const TrieChar*)"hello", data);
	printf(res ? "Win!\n" : "Fail!\n");

	res = trie_retrieve(trie, (const TrieChar*)"hell", data);
	printf(res ? "Win!\n" : "Fail!\n");

	res = trie_retrieve(trie, (const TrieChar*)"hel", data);
	printf(res ? "Win!\n" : "Fail!\n");

	res = trie_retrieve(trie, (const TrieChar*)"he", data);
	printf(res ? "Win!\n" : "Fail!\n");

	res = trie_retrieve(trie, (const TrieChar*)"h", data);
	printf(res ? "Win!\n" : "Fail!\n");


	trie_free(trie);
	return 0;
}
