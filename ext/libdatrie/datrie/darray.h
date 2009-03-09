/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * darray.h - Double-array trie structure
 * Created: 2006-08-11
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#ifndef __DARRAY_H
#define __DARRAY_H

#include "triedefs.h"

/**
 * @file darray.h
 * @brief Double-array trie structure
 */

/**
 * @brief Double-array structure type
 */
typedef struct _DArray  DArray;

/**
 * @brief Double-array entry enumeration function
 *
 * @param key       : the key of the entry, up to @a sep_node
 * @param sep_node  : the separate node of the entry
 * @param user_data : user-supplied data
 *
 * @return TRUE to continue enumeration, FALSE to stop
 */
typedef Bool (*DAEnumFunc) (const TrieChar   *key,
                            TrieIndex         sep_node,
                            void             *user_data);


/**
 * @brief Open double-array from file
 *
 * @param path : the path that stores the double-array files
 * @param name : the name of the double-array (not actual file name)
 * @param mode : openning mode, read or write
 *
 * @return a pointer to the openned double-array, NULL on failure
 *
 * Open a double-array structure of given name. Note that @a name here does 
 * not mean the actual file name. Rather, the file name will be inferred by 
 * the name.
 */
DArray * da_open (const char *path, const char *name, TrieIOMode mode);

/**
 * @brief Close double-array data
 *
 * @param d : the double-array data
 *
 * @return 0 on success, non-zero on failure
 *
 * Close the given double-array data. If @a d was openned for writing, all 
 * pending changes will be saved to file.
 */
int      da_close (DArray *d);

/**
 * @brief Save double-array data
 *
 * @param d : the double-array data
 *
 * @return 0 on success, non-zero on failure
 *
 * If @a double-array data was openned for writing, save all pending changes 
 * to file.
 */
int      da_save (DArray *d);


/**
 * @brief Get root state
 *
 * @param d     : the double-array data
 *
 * @return root state of the @a index set, or TRIE_INDEX_ERROR on failure
 *
 * Get root state for stepwise walking.
 */
TrieIndex  da_get_root (const DArray *d);


/**
 * @brief Get BASE cell
 *
 * @param d : the double-array data
 * @param s : the double-array state to get data
 *
 * @return the BASE cell value for the given state
 *
 * Get BASE cell value for the given state.
 */
TrieIndex  da_get_base (const DArray *d, TrieIndex s);

/**
 * @brief Get CHECK cell
 *
 * @param d : the double-array data
 * @param s : the double-array state to get data
 *
 * @return the CHECK cell value for the given state
 *
 * Get CHECK cell value for the given state.
 */
TrieIndex  da_get_check (const DArray *d, TrieIndex s);


/**
 * @brief Set BASE cell
 *
 * @param d   : the double-array data
 * @param s   : the double-array state to get data
 * @param val : the value to set
 *
 * Set BASE cell for the given state to the given value.
 */
void       da_set_base (DArray *d, TrieIndex s, TrieIndex val);

/**
 * @brief Set CHECK cell
 *
 * @param d   : the double-array data
 * @param s   : the double-array state to get data
 * @param val : the value to set
 *
 * Set CHECK cell for the given state to the given value.
 */
void       da_set_check (DArray *d, TrieIndex s, TrieIndex val);

/**
 * @brief Walk in double-array structure
 *
 * @param d : the double-array structure
 * @param s : current state
 * @param c : the input character
 *
 * @return boolean indicating success
 *
 * Walk the double-array trie from state @a *s, using input character @a c.
 * If there exists an edge from @a *s with arc labeled @a c, this function
 * returns TRUE and @a *s is updated to the new state. Otherwise, it returns
 * FALSE and @a *s is left unchanged.
 */
Bool       da_walk (DArray *d, TrieIndex *s, TrieChar c);

/**
 * @brief Test walkability in double-array structure
 *
 * @param d : the double-array structure
 * @param s : current state
 * @param c : the input character
 *
 * @return boolean indicating walkability
 *
 * Test if there is a transition from state @a s with input character @a c.
 */
/*
Bool       da_is_walkable (DArray *d, TrieIndex s, TrieChar c);
*/
#define    da_is_walkable(d,s,c) \
    (da_get_check ((d), da_get_base ((d), (s)) + (c)) == (s))

/**
 * @brief Insert a branch from trie node
 *
 * @param d : the double-array structure
 * @param s : the state to add branch to
 * @param c : the character for the branch label
 *
 * @return the index of the new node
 *
 * Insert a new arc labelled with character @a c from the trie node 
 * represented by index @a s in double-array structure @a d.
 * Note that it assumes that no such arc exists before inserting.
 */
TrieIndex  da_insert_branch (DArray *d, TrieIndex s, TrieChar c);

/**
 * @brief Prune the single branch
 *
 * @param d : the double-array structure
 * @param s : the dangling state to prune off
 *
 * Prune off a non-separate path up from the final state @a s.
 * If @a s still has some children states, it does nothing. Otherwise, 
 * it deletes the node and all its parents which become non-separate.
 */
void       da_prune (DArray *d, TrieIndex s);

/**
 * @brief Prune the single branch up to given parent
 *
 * @param d : the double-array structure
 * @param p : the parent up to which to be pruned
 * @param s : the dangling state to prune off
 *
 * Prune off a non-separate path up from the final state @a s to the
 * given parent @a p. The prunning stop when either the parent @a p
 * is met, or a first non-separate node is found.
 */
void       da_prune_upto (DArray *d, TrieIndex p, TrieIndex s);

/**
 * @brief Enumerate entries stored in double-array structure
 *
 * @param d          : the double-array structure
 * @param enum_func  : the callback function to be called on each separate node
 * @param user_data  : user-supplied data to send as an argument to @a enum_func
 *
 * @return boolean value indicating whether all the keys are visited
 *
 * Enumerate all keys stored in double-array structure. For each entry, the 
 * user-supplied @a enum_func callback function is called, with the entry key,
 * the separate node, and user-supplied data. Returning FALSE from such
 * callback will stop enumeration and return FALSE.
 */
Bool    da_enumerate (DArray *d, DAEnumFunc enum_func, void *user_data);

#endif  /* __DARRAY_H */

/*
vi:ts=4:ai:expandtab
*/
