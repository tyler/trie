/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * sb-trie.h - Single-byte domain trie front end
 * Created: 2006-08-19
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#ifndef __SB_TRIE_H
#define __SB_TRIE_H

#include <datrie/triedefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sb-trie.h
 * @brief Trie data type and functions
 */

/**
 * @brief Single-byte domain trie data type
 */
typedef struct _SBTrie   SBTrie;

/**
 * @brief Single-byte character type
 */
typedef unsigned char    SBChar;

/**
 * @brief Single-byte domain trie enumeration function
 *
 * @param key  : the key of the entry
 * @param data : the data of the entry
 *
 * @return TRUE to continue enumeration, FALSE to stop
 */
typedef Bool (*SBTrieEnumFunc) (const SBChar   *key,
                                TrieData        key_data,
                                void           *user_data);

/**
 * @brief Single-byte domain trie walking state
 */
typedef struct _SBTrieState SBTrieState;

/*-----------------------*
 *   GENERAL FUNCTIONS   *
 *-----------------------*/

/**
 * @brief Open a single-byte domain trie
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
SBTrie * sb_trie_open (const char *path, const char *name, TrieIOMode mode);

/**
 * @brief Close a single-byte domain trie
 *
 * @param sb_trie: the single-byte domain trie
 *
 * @return 0 on success, non-zero on failure
 *
 * Close the given trie. If @a trie was openned for writing, all pending
 * changes will be saved to file.
 */
int      sb_trie_close (SBTrie *sb_trie);

/**
 * @brief Save a single-byte domain trie
 *
 * @param sb_trie: the single-byte domain trie
 *
 * @return 0 on success, non-zero on failure
 *
 * If @a trie was openned for writing, save all pending data to file.
 */
int      sb_trie_save (SBTrie *sb_trie);


/*------------------------------*
 *   GENERAL QUERY OPERATIONS   *
 *------------------------------*/

/**
 * @brief Retrieve an entry from single-byte domain trie
 *
 * @param sb_trie : the single-byte domain trie
 * @param key     : the key for the entry to retrieve
 * @param o_data  : the storage for storing the entry data on return
 *
 * @return boolean value indicating the existence of the entry.
 *
 * Retrieve an entry for the given @a key from @a trie. On return,
 * if @a key is found and @a o_val is not NULL, @a *o_val is set
 * to the data associated to @a key.
 */
Bool     sb_trie_retrieve (SBTrie       *sb_trie,
                           const SBChar *key,
                           TrieData     *o_data);

/**
 * @brief Store a value for an entry to single-byte domain trie
 *
 * @param sb_trie : the single-byte domain trie
 * @param key     : the key for the entry to retrieve
 * @param data    : the data associated to the entry
 *
 * @return boolean value indicating the success of the process
 *
 * Store a data @a data for the given @a key in @a trie. If @a key does not 
 * exist in @a trie, it will be appended.
 */
Bool     sb_trie_store (SBTrie *sb_trie, const SBChar *key, TrieData data);

/**
 * @brief Delete an entry from single-byte domain trie
 *
 * @param sb_trie : the single-byte domain trie
 * @param key     : the key for the entry to delete
 *
 * @return boolean value indicating whether the key exists and is removed
 *
 * Delete an entry for the given @a key from @a trie.
 */
Bool     sb_trie_delete (SBTrie *sb_trie, const SBChar *key);

/**
 * @brief Enumerate entries in single-byte domain trie
 *
 * @param sb_trie    : the single-byte domain trie
 * @param enum_func  : the callback function to be called on each key
 * @param user_data  : user-supplied data to send as an argument to @a enum_func
 *
 * @return boolean value indicating whether all the keys are visited
 *
 * Enumerate all entries in trie. For each entry, the user-supplied 
 * @a enum_func callback function is called, with the entry key and data.
 * Returning FALSE from such callback will stop enumeration and return FALSE.
 */
Bool     sb_trie_enumerate (SBTrie         *sb_trie,
                            SBTrieEnumFunc  enum_func,
                            void           *user_data);


/*-------------------------------*
 *   STEPWISE QUERY OPERATIONS   *
 *-------------------------------*/

/**
 * @brief Get root state of a single-byte domain trie
 *
 * @param sb_trie : the single-byte domain trie
 *
 * @return the root state of the trie
 *
 * Get root state of @a trie, for stepwise walking.
 *
 * The returned state is allocated and must be freed with trie_state_free()
 */
SBTrieState * sb_trie_root (SBTrie *sb_trie);


/*----------------*
 *   TRIE STATE   *
 *----------------*/

/**
 * @brief Clone a single-byte domain trie state
 *
 * @param s    : the state to clone
 *
 * @return an duplicated instance of @a s
 *
 * Make a copy of trie state.
 *
 * The returned state is allocated and must be freed with trie_state_free()
 */
SBTrieState * sb_trie_state_clone (const SBTrieState *s);

/**
 * @brief Free a single-byte trie state
 *
 * @param s    : the state to free
 *
 * Free the trie state.
 */
void     sb_trie_state_free (SBTrieState *s);

/**
 * @brief Rewind a single-byte trie state
 *
 * @param s    : the state to rewind
 *
 * Put the state at root.
 */
void     sb_trie_state_rewind (SBTrieState *s);

/**
 * @brief Walk the single-byte domain trie from the state
 *
 * @param s    : current state
 * @param c    : key character for walking
 *
 * @return boolean value indicating the success of the walk
 *
 * Walk the trie stepwise, using a given character @a c.
 * On return, the state @a s is updated to the new state if successfully walked.
 */
Bool     sb_trie_state_walk (SBTrieState *s, SBChar c);

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
Bool     sb_trie_state_is_walkable (const SBTrieState *s, SBChar c);

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
Bool     sb_trie_state_is_terminal (const SBTrieState *s);

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
Bool     sb_trie_state_is_leaf (const SBTrieState *s);

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
TrieData sb_trie_state_get_data (const SBTrieState *s);

#ifdef __cplusplus
}
#endif

#endif  /* __SB_TRIE_H */

/*
vi:ts=4:ai:expandtab
*/
