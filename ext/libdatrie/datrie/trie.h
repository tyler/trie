/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * trie.h - Trie data type and functions
 * Created: 2006-08-11
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#ifndef __TRIE_H
#define __TRIE_H

#include <datrie/triedefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file trie.h
 * @brief Trie data type and functions
 */

/**
 * @brief Trie data type
 */
typedef struct _Trie   Trie;

/**
 * @brief Trie enumeration function
 *
 * @param key  : the key of the entry
 * @param data : the data of the entry
 *
 * @return TRUE to continue enumeration, FALSE to stop
 */
typedef Bool (*TrieEnumFunc) (const TrieChar   *key,
                              TrieData          key_data,
                              void             *user_data);

/**
 * @brief Trie walking state
 */
typedef struct _TrieState TrieState;

/*-----------------------*
 *   GENERAL FUNCTIONS   *
 *-----------------------*/

/**
 * @brief Open a trie
 *
 * @param path : the path that stores the trie files
 * @param name : the name of the trie (not actual file name)
 * @param mode : openning mode, read or write
 *
 * @return a pointer to the openned trie, NULL on failure
 *
 * Open a trie of given name. Note that @a name here does not mean the
 * actual file name. Rather, the file name will be inferred by the name.
 */
Trie *  trie_open (const char *path, const char *name, TrieIOMode mode);

/**
 * @brief Close a trie
 *
 * @param trie: the trie
 *
 * @return 0 on success, non-zero on failure
 *
 * Close the given trie. If @a trie was openned for writing, all pending
 * changes will be saved to file.
 */
int     trie_close (Trie *trie);

/**
 * @brief Save a trie
 *
 * @param trie: the trie
 *
 * @return 0 on success, non-zero on failure
 *
 * If @a trie was openned for writing, save all pending data to file.
 */
int     trie_save (Trie *trie);


/*------------------------------*
 *   GENERAL QUERY OPERATIONS   *
 *------------------------------*/

/**
 * @brief Retrieve an entry from trie
 *
 * @param trie   : the trie
 * @param key    : the key for the entry to retrieve
 * @param o_data : the storage for storing the entry data on return
 *
 * @return boolean value indicating the existence of the entry.
 *
 * Retrieve an entry for the given @a key from @a trie. On return,
 * if @a key is found and @a o_val is not NULL, @a *o_val is set
 * to the data associated to @a key.
 */
Bool    trie_retrieve (Trie *trie, const TrieChar *key, TrieData *o_data);

/**
 * @brief Store a value for  an entry to trie
 *
 * @param trie  : the trie
 * @param key   : the key for the entry to retrieve
 * @param data  : the data associated to the entry
 *
 * @return boolean value indicating the success of the process
 *
 * Store a data @a data for the given @a key in @a trie. If @a key does not 
 * exist in @a trie, it will be appended.
 */
Bool    trie_store (Trie *trie, const TrieChar *key, TrieData data);

/**
 * @brief Delete an entry from trie
 *
 * @param trie  : the trie
 * @param key   : the key for the entry to delete
 *
 * @return boolean value indicating whether the key exists and is removed
 *
 * Delete an entry for the given @a key from @a trie.
 */
Bool    trie_delete (Trie *trie, const TrieChar *key);

/**
 * @brief Enumerate entries in trie
 *
 * @param trie       : the trie
 * @param enum_func  : the callback function to be called on each key
 * @param user_data  : user-supplied data to send as an argument to @a enum_func
 *
 * @return boolean value indicating whether all the keys are visited
 *
 * Enumerate all entries in trie. For each entry, the user-supplied 
 * @a enum_func callback function is called, with the entry key and data.
 * Returning FALSE from such callback will stop enumeration and return FALSE.
 */
Bool    trie_enumerate (Trie *trie, TrieEnumFunc enum_func, void *user_data);


/*-------------------------------*
 *   STEPWISE QUERY OPERATIONS   *
 *-------------------------------*/

/**
 * @brief Get root state of a trie
 *
 * @param trie : the trie
 *
 * @return the root state of the trie
 *
 * Get root state of @a trie, for stepwise walking.
 *
 * The returned state is allocated and must be freed with trie_state_free()
 */
TrieState * trie_root (Trie *trie);


/*----------------*
 *   TRIE STATE   *
 *----------------*/

/**
 * @brief Clone a trie state
 *
 * @param s    : the state to clone
 *
 * @return an duplicated instance of @a s
 *
 * Make a copy of trie state.
 *
 * The returned state is allocated and must be freed with trie_state_free()
 */
TrieState * trie_state_clone (const TrieState *s);

/**
 * @brief Free a trie state
 *
 * @param s    : the state to free
 *
 * Free the trie state.
 */
void      trie_state_free (TrieState *s);

/**
 * @brief Rewind a trie state
 *
 * @param s    : the state to rewind
 *
 * Put the state at root.
 */
void      trie_state_rewind (TrieState *s);

/**
 * @brief Walk the trie from the state
 *
 * @param s    : current state
 * @param c    : key character for walking
 *
 * @return boolean value indicating the success of the walk
 *
 * Walk the trie stepwise, using a given character @a c.
 * On return, the state @a s is updated to the new state if successfully walked.
 */
Bool      trie_state_walk (TrieState *s, TrieChar c);

/**
 * @brief Test walkability of character from state
 *
 * @param s    : the state to check
 * @param c    : the input character
 *
 * @return boolean indicating walkability
 *
 * Test if there is a transition from state @a s with input character @a c.
 */
Bool      trie_state_is_walkable (const TrieState *s, TrieChar c);

/**
 * @brief Check for terminal state
 *
 * @param s    : the state to check
 *
 * @return boolean value indicating whether it is a terminal state
 *
 * Check if the given state is a terminal state. A leaf state is a trie state 
 * that terminates a key, and stores a value associated with the key.
 */
#define   trie_state_is_terminal(s) trie_state_is_walkable((s),TRIE_CHAR_TERM)

/**
 * @brief Check for leaf state
 *
 * @param s    : the state to check
 *
 * @return boolean value indicating whether it is a leaf state
 *
 * Check if the given state is a leaf state. A leaf state is a terminal state 
 * that has no other branch.
 */
Bool      trie_state_is_leaf (const TrieState *s);

/**
 * @brief Get data from leaf state
 *
 * @param s    : the leaf state
 *
 * @return the data associated with the leaf state @a s,
 *         or TRIE_DATA_ERROR if @a s is not a leaf state
 *
 * Get value from a leaf state of trie. Getting the value from leaf state 
 * will result in TRIE_DATA_ERROR.
 */
TrieData trie_state_get_data (const TrieState *s);

#ifdef __cplusplus
}
#endif

#endif  /* __TRIE_H */

/*
vi:ts=4:ai:expandtab
*/
