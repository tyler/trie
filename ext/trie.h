#include "darray.h"
#include "tail.h"

typedef struct _Trie {
    DArray     *da;
    Tail       *tail;
} Trie;

typedef struct _TrieState {
    const Trie *trie;       /**< the corresponding trie */
    TrieIndex   index;      /**< index in double-array/tail structures */
    short       suffix_idx; /**< suffix character offset, if in suffix */
    short       is_suffix;  /**< whether it is currently in suffix part */
} TrieState;


#define trie_da_is_separate(da,s)      (da_get_base ((da), (s)) < 0)
#define trie_da_get_tail_index(da,s)   (-da_get_base ((da), (s)))
#define trie_da_set_tail_index(da,s,v) (da_set_base ((da), (s), -(v)))
#define trie_state_is_terminal(s) trie_state_is_walkable((s),TRIE_CHAR_TERM)


Trie* trie_new();
void trie_free(Trie *trie);
static Bool trie_branch_in_branch (Trie *trie, TrieIndex sep_node, const TrieChar *suffix, TrieData data);
static Bool trie_branch_in_tail(Trie *trie, TrieIndex sep_node, const TrieChar *suffix, TrieData data);
Bool trie_store (Trie *trie, const TrieChar *key, TrieData data);
Bool trie_retrieve (const Trie *trie, const TrieChar *key, TrieData *o_data);
Bool trie_delete (Trie *trie, const TrieChar *key);
TrieState * trie_root (const Trie *trie);
static TrieState * trie_state_new (const Trie *trie, TrieIndex index, short suffix_idx, short is_suffix);
TrieState * trie_state_clone (const TrieState *s);
void trie_state_free (TrieState *s);
void trie_state_rewind (TrieState *s);
Bool trie_state_walk (TrieState *s, TrieChar c);
Bool trie_state_is_walkable (const TrieState *s, TrieChar c);
Bool trie_state_is_leaf (const TrieState *s);
TrieData trie_state_get_data (const TrieState *s);


