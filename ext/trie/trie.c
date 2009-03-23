#include "ruby.h"
#include <datrie/trie.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ctype.h>
#include <locale.h>
#include <langinfo.h>
#include <iconv.h>


typedef struct _AlphaRange {
    struct _AlphaRange *next;

    AlphaChar           begin;
    AlphaChar           end;
} AlphaRange;

typedef struct _AlphaMap {
    AlphaRange     *first_range;
} AlphaMap;

typedef struct _DArray DArray;
typedef struct _Tail Tail;

typedef struct _Trie {
    AlphaMap   *alpha_map;
    DArray     *da;
    Tail       *tail;

    Bool        is_dirty;
} Trie;

typedef struct _TrieState {
    const Trie *trie;
    TrieIndex   index;
    short       suffix_idx;
    short       is_suffix;
} TrieState;

typedef struct _RbTrie {
	const Trie *trie;
	iconv_t     to_alpha_conv;
    iconv_t     from_alpha_conv;
} RbTrie;

typedef struct _RbTrieState {
	const TrieState *state;
	const RbTrie *rb_trie;
} RbTrieState;

VALUE cTrie, cTrieNode;

#define ALPHA_ENC   "UCS-4LE"
#define N_ELEMENTS(a)   (sizeof(a)/sizeof((a)[0]))

static uint32 alpha_map_next(AlphaMap *alpha_map, AlphaChar alpha_char) {
	AlphaRange *range;
	for(range = alpha_map->first_range; range; range = range->next) {
		if(range->begin <= alpha_char && range->end > alpha_char)
			return alpha_char + 1;
		if(range->end == alpha_char && range->next)
			return range->next->begin;
	}
	return 0;
}
static uint32 alpha_map_first(AlphaMap *alpha_map) {
	return alpha_map->first_range->begin;
}



static size_t
to_alpha (RbTrie *rb_trie, const char *in, AlphaChar *out, size_t out_size)
{
    char   *in_p = (char *) in;
    char   *out_p = (char *) out;
    size_t  in_left = strlen (in);
    size_t  out_left = out_size * sizeof (AlphaChar);
    size_t  res;

    /* convert to UCS-4LE */
    res = iconv (rb_trie->to_alpha_conv, (char **) &in_p, &in_left,
                 &out_p, &out_left);

    if (res < 0)
        return res;

    /* convert UCS-4LE to AlphaChar string */
    res = 0;
    for (in_p = (char *) out; res < out_size && in_p + 3 < out_p; in_p += 4) {
        out[res++] = in_p[0]
                     | (in_p[1] << 8)
                     | (in_p[2] << 16)
                     | (in_p[3] << 24);
    }
    if (res < out_size) {
        out[res] = 0;
    }

    return res;
}

static size_t
from_alpha (RbTrie *rb_trie, const AlphaChar *in, char *out, size_t out_size)
{
    size_t  in_left = alpha_char_strlen (in) * sizeof (AlphaChar);
    size_t  res;

    /* convert AlphaChar to UCS-4LE */
    for (res = 0; in[res]; res++) {
        unsigned char  b[4];

        b[0] = in[res] & 0xff;
        b[1] = (in[res] >> 8) & 0xff;
        b[2] = (in[res] >> 16) & 0xff;
        b[3] = (in[res] >> 24) & 0xff;

        memcpy ((char *) &in[res], b, 4);
    }

    /* convert UCS-4LE to locale codeset */
    res = iconv (rb_trie->from_alpha_conv, (char **) &in, &in_left,
                 &out, &out_size);
    *out = 0;

    return res;
}



static void rb_trie_free(RbTrie *trie) {
	trie_free((Trie*)trie->trie);
    iconv_close (trie->to_alpha_conv);
    iconv_close (trie->from_alpha_conv);}

static VALUE rb_trie_alloc(VALUE klass) {
    VALUE obj;
    obj = Data_Wrap_Struct(klass, 0, rb_trie_free, NULL);
    rb_iv_set(obj, "@open", Qfalse);
    return obj;
}

RbTrie* new_rb_trie() {
	RbTrie *rb_trie = (RbTrie*) malloc(sizeof(RbTrie));

	AlphaMap *alpha_map = alpha_map_new();
	alpha_map_add_range(alpha_map, 1, 0xFF);

	rb_trie->trie = trie_new(alpha_map);
	
	char *prev_locale;
    char *locale_codeset;

    prev_locale = setlocale (LC_CTYPE, "");
    locale_codeset = nl_langinfo (CODESET);
    setlocale (LC_CTYPE, prev_locale);

    rb_trie->to_alpha_conv = iconv_open (ALPHA_ENC, locale_codeset);
    rb_trie->from_alpha_conv = iconv_open (locale_codeset, ALPHA_ENC);

	return rb_trie;
}

static VALUE rb_trie_initialize(VALUE self, VALUE args) {
    RDATA(self)->data = new_rb_trie();

    rb_iv_set(self, "@open", Qtrue);
    return self;
}

static VALUE rb_trie_save(VALUE self) {
	return Qnil;
}

static VALUE rb_trie_has_key(VALUE self, VALUE key) {
    RbTrie *trie;
    Data_Get_Struct(self, RbTrie, trie);

	AlphaChar  key_alpha[256];
	to_alpha (trie, RSTRING(key)->ptr, key_alpha, N_ELEMENTS (key_alpha));

    if(trie_retrieve(trie->trie, key_alpha, NULL))
		return Qtrue;
    else
		return Qnil;
}

static VALUE rb_trie_get(VALUE self, VALUE key) {
    RbTrie *trie;
    Data_Get_Struct(self, RbTrie, trie);

	AlphaChar  key_alpha[256];
	to_alpha (trie, RSTRING(key)->ptr, key_alpha, N_ELEMENTS (key_alpha));

	TrieData data;
    if(trie_retrieve(trie->trie, key_alpha, &data))
		return INT2FIX(data);
    else
		return Qnil;
}

static VALUE rb_trie_add(VALUE self, VALUE args) {
	RbTrie *trie;
    Data_Get_Struct(self, RbTrie, trie);

    int size = RARRAY(args)->len;
    if(size < 1 || size > 2)
		return Qnil;

    VALUE key;
    key = RARRAY(args)->ptr[0];
    TrieData value = size == 2 ? NUM2INT(RARRAY(args)->ptr[1]) : TRIE_DATA_ERROR;
    
	AlphaChar  key_alpha[256];
	to_alpha (trie, RSTRING(key)->ptr, key_alpha, N_ELEMENTS (key_alpha));

    if(trie_store((Trie*)trie->trie, key_alpha, value))
		return Qtrue;
    else
		return Qnil;
}

static VALUE rb_trie_delete(VALUE self, VALUE key) {
	RbTrie *trie;
    Data_Get_Struct(self, RbTrie, trie);

    AlphaChar alpha_key[RSTRING(key)->len * 4];
	to_alpha(trie, RSTRING(key)->ptr, alpha_key, N_ELEMENTS(alpha_key));

    if(trie_delete((Trie*)trie->trie, alpha_key))
		return Qtrue;
    else
		return Qnil;
}

AlphaChar* alpha_push_char(AlphaChar *left, int left_size, AlphaChar right) {
	AlphaChar *out = (AlphaChar*) malloc(left_size * 4 + 5);
	memcpy(out, left, left_size * 4);
	out[left_size] = right;
	out[left_size + 1] = 0;

	return out;
}

static VALUE walk_all_paths(RbTrie *rb_trie, VALUE children, TrieState *state, AlphaChar *prefix, int prefix_size) {
	AlphaChar c;
	AlphaMap *alpha_map = state->trie->alpha_map;

    for(c = alpha_map_first(alpha_map); c; c = alpha_map_next(alpha_map, c)) {
		if(trie_state_is_walkable(state,c)) {
			TrieState *next_state = trie_state_clone(state);
			trie_state_walk(next_state, c);

			AlphaChar *word = alpha_push_char(prefix, prefix_size, c);

			if(trie_state_is_terminal(next_state)) {
				char key_locale[1024];
				from_alpha (rb_trie, word, key_locale, N_ELEMENTS (key_locale));
				rb_ary_push(children, rb_str_new2(key_locale));
			}

			walk_all_paths(rb_trie, children, next_state, word, prefix_size + 1);
			
			trie_state_free(next_state);
		}
    }
}

static VALUE rb_trie_children(VALUE self, VALUE prefix) {
    if(NIL_P(prefix))
		return rb_ary_new();

    RbTrie *trie;
    Data_Get_Struct(self, RbTrie, trie);

	int prefix_size = RSTRING(prefix)->len;
    AlphaChar alpha_prefix[RSTRING(prefix)->len * 4];
	to_alpha(trie, RSTRING(prefix)->ptr, alpha_prefix, N_ELEMENTS(alpha_prefix));
    
    VALUE children = rb_ary_new();

    TrieState *state = trie_root(trie->trie);
    
    const AlphaChar *iterator = alpha_prefix;
    while(*iterator != 0) {
		if(!trie_state_is_walkable(state, *iterator))
			return children;
		trie_state_walk(state, *iterator);
		iterator++;
    }

    if(trie_state_is_terminal(state))
		rb_ary_push(children, prefix);

    walk_all_paths(trie, children, state, alpha_prefix, prefix_size);

    trie_state_free(state);
    return children;
}


static VALUE 
walk_all_paths_with_values(RbTrie *rb_trie, VALUE children, TrieState *state, AlphaChar *prefix, int prefix_size) {
	AlphaChar c;
	AlphaMap *alpha_map = state->trie->alpha_map;

    for(c = alpha_map_first(alpha_map); c; c = alpha_map_next(alpha_map, c)) {
		if(trie_state_is_walkable(state,c)) {
			TrieState *next_state = trie_state_clone(state);
			trie_state_walk(next_state, c);

			AlphaChar *word = alpha_push_char(prefix, prefix_size, c);

			if(trie_state_is_terminal(next_state)) {
				TrieState *end_state = trie_state_clone(next_state);
				trie_state_walk(end_state, '\0');
 
				VALUE tuple = rb_ary_new();

				char key_locale[1024];
				from_alpha (rb_trie, word, key_locale, N_ELEMENTS (key_locale));
				rb_ary_push(tuple, rb_str_new2(key_locale));

				TrieData trie_data = trie_state_get_data(end_state);
				rb_ary_push(tuple, INT2FIX(trie_data));
				rb_ary_push(children, tuple);
 
				trie_state_free(end_state);
			}

			walk_all_paths_with_values(rb_trie, children, next_state, word, prefix_size + 1);
			
			trie_state_free(next_state);
		}
    }
}




static VALUE rb_trie_children_with_values(VALUE self, VALUE prefix) {
    if(NIL_P(prefix))
		return rb_ary_new();

    RbTrie *trie;
    Data_Get_Struct(self, RbTrie, trie);

	int prefix_size = RSTRING(prefix)->len;
    AlphaChar alpha_prefix[RSTRING(prefix)->len * 4];
	to_alpha(trie, RSTRING(prefix)->ptr, alpha_prefix, N_ELEMENTS(alpha_prefix));
    
    VALUE children = rb_ary_new();

    TrieState *state = trie_root(trie->trie);
    
    const AlphaChar *iterator = alpha_prefix;
    while(*iterator != 0) {
		if(!trie_state_is_walkable(state, *iterator))
			return rb_ary_new();
		trie_state_walk(state, *iterator);
		iterator++;
    }

    if(trie_state_is_terminal(state)) {
		TrieState *end_state = trie_state_clone(state);
		trie_state_walk(end_state, '\0');

		VALUE tuple = rb_ary_new();
		rb_ary_push(tuple, prefix);
		TrieData trie_data = trie_state_get_data(end_state);
		rb_ary_push(tuple, INT2FIX(trie_data));
		rb_ary_push(children, tuple);

		trie_state_free(end_state);
    }

    walk_all_paths_with_values(trie, children, state, alpha_prefix, prefix_size);

    trie_state_free(state);
    return children;
}

static VALUE rb_trie_get_path(VALUE self) {
    return rb_iv_get(self, "@path");
}

static void rb_trie_node_free(RbTrieState *state) {
    if(state)
		trie_state_free((TrieState*)state->state);
}

static VALUE rb_trie_node_alloc(VALUE klass) {
    RbTrieState *state;
    VALUE obj;
    
    obj = Data_Wrap_Struct(klass, 0, rb_trie_node_free, state);

    return obj;
}

static VALUE rb_trie_root(VALUE self) {
    RbTrie *trie;
    Data_Get_Struct(self, RbTrie, trie);

    VALUE trie_node = rb_trie_node_alloc(cTrieNode);

	RbTrieState *state = (RbTrieState*) malloc(sizeof(RbTrieState));
	state->rb_trie = trie;
	state->state = trie_root(trie->trie);
	RDATA(trie_node)->data = state;
    
    rb_iv_set(trie_node, "@state", Qnil);
    rb_iv_set(trie_node, "@full_state", rb_str_new2(""));
    return trie_node;
}

static VALUE rb_trie_node_get_state(VALUE self) {
    return rb_iv_get(self, "@state");
}
static VALUE rb_trie_node_get_full_state(VALUE self) {
    return rb_iv_get(self, "@full_state");
}

static VALUE rb_trie_node_walk_bang(VALUE self, VALUE rchar) {
    RbTrieState *state;
    Data_Get_Struct(self, RbTrieState, state);

    if(RSTRING(rchar)->len != 1)
		return Qnil;

    AlphaChar alpha_char[4];
	to_alpha((RbTrie*)state->rb_trie, RSTRING(rchar)->ptr, alpha_char, N_ELEMENTS(alpha_char));

    int result = trie_state_walk((TrieState*)state->state, *alpha_char);
    
    if(result) {
		rb_iv_set(self, "@state", rchar);
		VALUE full_state = rb_iv_get(self, "@full_state");
		rb_str_append(full_state, rchar);
		rb_iv_set(self, "@full_state", full_state);
		return self;
    } else
		return Qnil;
}

static VALUE rb_trie_node_value(VALUE self) {
    RbTrieState *state;
	TrieState *dup;
    Data_Get_Struct(self, RbTrieState, state);
    
    dup = trie_state_clone(state->state);

    trie_state_walk(dup, 0);
    TrieData trie_data = trie_state_get_data(dup);
    trie_state_free(dup);

    return TRIE_DATA_ERROR == trie_data ? Qnil : INT2FIX(trie_data);
}

static VALUE rb_trie_node_terminal(VALUE self) {
    RbTrieState *state;
    Data_Get_Struct(self, RbTrieState, state);
    
    return trie_state_is_terminal(state->state) ? Qtrue : Qnil;
}

static VALUE rb_trie_node_leaf(VALUE self) {
    RbTrieState *state;
    Data_Get_Struct(self, RbTrieState, state);
    
    return trie_state_is_leaf(state->state) ? Qtrue : Qnil;
}

static VALUE rb_trie_node_clone(VALUE self) {
    RbTrieState *state;
    Data_Get_Struct(self, RbTrieState, state);
    
    VALUE new_node = rb_trie_node_alloc(cTrieNode);

	RbTrieState *new_rb_state = (RbTrieState *) malloc(sizeof(RbTrieState));
	TrieState *new_state = trie_state_clone(state->state);
	new_rb_state->state = new_state;
	new_rb_state->rb_trie = state->rb_trie;

    RDATA(new_node)->data = new_rb_state;
    
    rb_iv_set(new_node, "@state", rb_iv_get(self, "@state"));
    rb_iv_set(new_node, "@full_state", rb_iv_get(self, "@full_state"));

    return new_node;
}

 
void Init_trie() {
    cTrie = rb_define_class("Trie", rb_cObject);
    rb_define_alloc_func(cTrie, rb_trie_alloc);
    rb_define_method(cTrie, "initialize", rb_trie_initialize, -2);
    rb_define_method(cTrie, "path", rb_trie_get_path, 0);
    rb_define_method(cTrie, "has_key?", rb_trie_has_key, 1);
    rb_define_method(cTrie, "get", rb_trie_get, 1);
    rb_define_method(cTrie, "add", rb_trie_add, -2);
    rb_define_method(cTrie, "delete", rb_trie_delete, 1);
    rb_define_method(cTrie, "children", rb_trie_children, 1);
    rb_define_method(cTrie, "children_with_values", rb_trie_children_with_values, 1);
    rb_define_method(cTrie, "root", rb_trie_root, 0);
    rb_define_method(cTrie, "save", rb_trie_save, 0);

    cTrieNode = rb_define_class("TrieNode", rb_cObject);
    rb_define_alloc_func(cTrieNode, rb_trie_node_alloc);
    rb_define_method(cTrieNode, "state", rb_trie_node_get_state, 0);
    rb_define_method(cTrieNode, "full_state", rb_trie_node_get_full_state, 0);
    rb_define_method(cTrieNode, "walk!", rb_trie_node_walk_bang, 1);
    rb_define_method(cTrieNode, "value", rb_trie_node_value, 0);
    rb_define_method(cTrieNode, "terminal?", rb_trie_node_terminal, 0);
    rb_define_method(cTrieNode, "leaf?", rb_trie_node_leaf, 0);
    rb_define_method(cTrieNode, "clone", rb_trie_node_clone, 0);
}
