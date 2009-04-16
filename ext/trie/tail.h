/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * tail.h - trie tail for keeping suffixes
 * Created: 2006-08-12
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#ifndef __TAIL_H
#define __TAIL_H

#include "triedefs.h"

/**
 * @file tail.h
 * @brief trie tail for keeping suffixes
 */

/**
 * @brief Double-array structure type
 */
typedef struct _Tail  Tail;

/**
 * @brief Create a new tail object
 *
 * Create a new empty tail object.
 */
Tail *   tail_new ();

/**
 * @brief Read tail data from file
 *
 * @param file : the file to read
 *
 * @return a pointer to the openned tail data, NULL on failure
 *
 * Read tail data from the opened file, starting from the current
 * file pointer until the end of tail data block. On return, the
 * file pointer is left at the position after the read block.
 */
Tail *   tail_read (FILE *file);

/**
 * @brief Free tail data
 *
 * @param t : the tail data
 *
 * @return 0 on success, non-zero on failure
 *
 * Free the given tail data.
 */
void     tail_free (Tail *t);

/**
 * @brief Write tail data
 *
 * @param t     : the tail data
 * @param file  : the file to write to
 *
 * @return 0 on success, non-zero on failure
 *
 * Write tail data to the given @a file, starting from the current file
 * pointer. On return, the file pointer is left after the tail data block.
 */
int      tail_write (const Tail *t, FILE *file);


/**
 * @brief Get suffix
 *
 * @param t     : the tail data
 * @param index : the index of the suffix
 *
 * @return an allocated string of the indexed suffix.
 *
 * Get suffix from tail with given @a index. The returned string is allocated.
 * The caller should free it with free().
 */
const TrieChar *    tail_get_suffix (const Tail *t, TrieIndex index);

/**
 * @brief Set suffix of existing entry
 *
 * @param t      : the tail data
 * @param index  : the index of the suffix
 * @param suffix : the new suffix
 *
 * Set suffix of existing entry of given @a index in tail.
 */
Bool     tail_set_suffix (Tail *t, TrieIndex index, const TrieChar *suffix);

/**
 * @brief Add a new suffix
 *
 * @param t      : the tail data
 * @param suffix : the new suffix
 *
 * @return the index of the newly added suffix.
 *
 * Add a new suffix entry to tail.
 */
TrieIndex tail_add_suffix (Tail *t, const TrieChar *suffix);

/**
 * @brief Get data associated to suffix entry
 *
 * @param t      : the tail data
 * @param index  : the index of the suffix
 *
 * @return the data associated to the suffix entry
 *
 * Get data associated to suffix entry @a index in tail data.
 */
TrieData tail_get_data (const Tail *t, TrieIndex index);

/**
 * @brief Set data associated to suffix entry
 *
 * @param t      : the tail data
 * @param index  : the index of the suffix
 * @param data   : the data to set
 *
 * @return boolean indicating success
 *
 * Set data associated to suffix entry @a index in tail data.
 */
Bool     tail_set_data (Tail *t, TrieIndex index, TrieData data);

/**
 * @brief Delete suffix entry
 *
 * @param t      : the tail data
 * @param index  : the index of the suffix to delete
 *
 * Delete suffix entry from the tail data.
 */
void     tail_delete (Tail *t, TrieIndex index);

/**
 * @brief Walk in tail with a string
 *
 * @param t          : the tail data
 * @param s          : the tail data index
 * @param suffix_idx : pointer to current character index in suffix
 * @param str        : the string to use in walking
 * @param len        : total characters in @a str to walk
 *
 * @return total number of characters successfully walked
 *
 * Walk in the tail data @a t at entry @a s, from given character position
 * @a *suffix_idx, using @a len characters of given string @a str. On return,
 * @a *suffix_idx is updated to the position after the last successful walk,
 * and the function returns the total number of character succesfully walked.
 */
int      tail_walk_str  (const Tail      *t,
                         TrieIndex        s,
                         short           *suffix_idx,
                         const TrieChar  *str,
                         int              len);

/**
 * @brief Walk in tail with a character
 *
 * @param t          : the tail data
 * @param s          : the tail data index
 * @param suffix_idx : pointer to current character index in suffix
 * @param c          : the character to use in walking
 *
 * @return boolean indicating success
 *
 * Walk in the tail data @a t at entry @a s, from given character position
 * @a *suffix_idx, using given character @a c. If the walk is successful,
 * it returns TRUE, and @a *suffix_idx is updated to the next character.
 * Otherwise, it returns FALSE, and @a *suffix_idx is left unchanged.
 */
Bool     tail_walk_char (const Tail      *t,
                         TrieIndex        s,
                         short           *suffix_idx,
                         TrieChar         c);

/**
 * @brief Test walkability in tail with a character
 *
 * @param t          : the tail data
 * @param s          : the tail data index
 * @param suffix_idx : current character index in suffix
 * @param c          : the character to test walkability
 *
 * @return boolean indicating walkability
 *
 * Test if the character @a c can be used to walk from given character 
 * position @a suffix_idx of entry @a s of the tail data @a t.
 */
/*
Bool     tail_is_walkable_char (Tail            *t,
                                TrieIndex        s,
                                short            suffix_idx,
                                const TrieChar   c);
*/
#define  tail_is_walkable_char(t,s,suffix_idx,c) \
    (tail_get_suffix ((t), (s)) [suffix_idx] == (c))

#endif  /* __TAIL_H */

/*
vi:ts=4:ai:expandtab
*/
