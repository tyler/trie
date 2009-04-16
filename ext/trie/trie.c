#include "ruby.h"
#include "trie.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

VALUE cTrie, cTrieNode;

static VALUE rb_trie_alloc(VALUE klass) {
	VALUE obj;
	obj = Data_Wrap_Struct(klass, 0, trie_free, trie_new());
	return obj;
}

static VALUE rb_trie_has_key(VALUE self, VALUE key) {
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);

    if(trie_retrieve(trie, (TrieChar*)RSTRING(key)->ptr, NULL))
		return Qtrue;
    else
		return Qnil;
}

static VALUE rb_trie_get(VALUE self, VALUE key) {
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);

	TrieData data;
    if(trie_retrieve(trie, (TrieChar*)RSTRING(key)->ptr, &data))
		return INT2FIX(data);
    else
		return Qnil;
}

static VALUE rb_trie_add(VALUE self, VALUE args) {
	Trie *trie;
    Data_Get_Struct(self, Trie, trie);

    int size = RARRAY(args)->len;
    if(size < 1 || size > 2)
		return Qnil;

    VALUE key;
    key = RARRAY(args)->ptr[0];
    TrieData value = size == 2 ? NUM2INT(RARRAY(args)->ptr[1]) : TRIE_DATA_ERROR;
    
    if(trie_store(trie, (TrieChar*)RSTRING(key)->ptr, value))
		return Qtrue;
    else
		return Qnil;
}

static VALUE rb_trie_delete(VALUE self, VALUE key) {
	Trie *trie;
    Data_Get_Struct(self, Trie, trie);

    if(trie_delete(trie, (TrieChar*)RSTRING(key)->ptr))
		return Qtrue;
    else
		return Qnil;
}

char* append_char(char* existing, int size, char c) {
	char *new = (char*) malloc(size + 2);
	memcpy(new, existing, size);
	new[size] = c;
	new[size + 1] = 0;
	return new;
}

static VALUE walk_all_paths(Trie *trie, VALUE children, TrieState *state, char *prefix, int prefix_size) {
	int c;
    for(c = 1; c < 256; c++) {
		if(trie_state_is_walkable(state,c)) {
			TrieState *next_state = trie_state_clone(state);
			trie_state_walk(next_state, c);

			prefix[prefix_size] = c;
			prefix[prefix_size + 1] = 0;

			if(trie_state_is_terminal(next_state)) {
				char *word = (char*) malloc(prefix_size + 2);
				memcpy(word, prefix, prefix_size + 2);
				rb_ary_push(children, rb_str_new2(word));
			}

			walk_all_paths(trie, children, next_state, prefix, prefix_size + 1);
			
			prefix[prefix_size] = 0;
			trie_state_free(next_state);
		}
    }
}

static VALUE rb_trie_children(VALUE self, VALUE prefix) {
    if(NIL_P(prefix))
		return rb_ary_new();

    Trie *trie;
    Data_Get_Struct(self, Trie, trie);

	int prefix_size = RSTRING(prefix)->len;
    TrieState *state = trie_root(trie);
    VALUE children = rb_ary_new();
	TrieChar *char_prefix = (TrieChar*)RSTRING(prefix)->ptr;
    
    const TrieChar *iterator = char_prefix;
    while(*iterator != 0) {
		if(!trie_state_is_walkable(state, *iterator))
			return children;
		trie_state_walk(state, *iterator);
		iterator++;
    }

    if(trie_state_is_terminal(state))
		rb_ary_push(children, prefix);
	
	char prefix_buffer[1024];
	memcpy(prefix_buffer, char_prefix, prefix_size);
	prefix_buffer[prefix_size] = 0;

    walk_all_paths(trie, children, state, prefix_buffer, prefix_size);

    trie_state_free(state);
    return children;
}


static VALUE walk_all_paths_with_values(Trie *trie, VALUE children, TrieState *state, char *prefix, int prefix_size) {
	int c;
    for(c = 1; c < 256; c++) {
		if(trie_state_is_walkable(state,c)) {
			TrieState *next_state = trie_state_clone(state);
			trie_state_walk(next_state, c);

			prefix[prefix_size] = c;
			prefix[prefix_size + 1] = 0;

			if(trie_state_is_terminal(next_state)) {
				TrieState *end_state = trie_state_clone(next_state);
				trie_state_walk(end_state, '\0');
 
				char *word = (char*) malloc(prefix_size + 2);
				memcpy(word, prefix, prefix_size + 2);

				VALUE tuple = rb_ary_new();
				rb_ary_push(tuple, rb_str_new2(word));

				TrieData trie_data = trie_state_get_data(end_state);
				rb_ary_push(tuple, INT2FIX(trie_data));
				rb_ary_push(children, tuple);
 
				trie_state_free(end_state);
			}

			walk_all_paths_with_values(trie, children, next_state, prefix, prefix_size + 1);
			
			prefix[prefix_size] = 0;
			trie_state_free(next_state);
		}
    }
}




static VALUE rb_trie_children_with_values(VALUE self, VALUE prefix) {
    if(NIL_P(prefix))
		return rb_ary_new();

    Trie *trie;
    Data_Get_Struct(self, Trie, trie);

	int prefix_size = RSTRING(prefix)->len;
    TrieChar *char_prefix = (TrieChar*)RSTRING(prefix)->ptr;
    
    VALUE children = rb_ary_new();

    TrieState *state = trie_root(trie);
    
    const TrieChar *iterator = char_prefix;
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

	char prefix_buffer[1024];
	memcpy(prefix_buffer, char_prefix, prefix_size);
	prefix_buffer[prefix_size] = 0;

    walk_all_paths_with_values(trie, children, state, prefix_buffer, prefix_size);

    trie_state_free(state);
    return children;
}

static VALUE rb_trie_node_alloc(VALUE klass) {
    VALUE obj;
    obj = Data_Wrap_Struct(klass, 0, trie_state_free, NULL);
    return obj;
}

static VALUE rb_trie_root(VALUE self) {
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);

    VALUE trie_node = rb_trie_node_alloc(cTrieNode);

	TrieState *state = trie_root(trie);
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
    TrieState *state;
    Data_Get_Struct(self, TrieState, state);

    if(RSTRING(rchar)->len != 1)
		return Qnil;

    Bool result = trie_state_walk(state, *RSTRING(rchar)->ptr);
    
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
    TrieState *state;
	TrieState *dup;
    Data_Get_Struct(self, TrieState, state);
    
    dup = trie_state_clone(state);

    trie_state_walk(dup, 0);
    TrieData trie_data = trie_state_get_data(dup);
    trie_state_free(dup);

    return TRIE_DATA_ERROR == trie_data ? Qnil : INT2FIX(trie_data);
}

static VALUE rb_trie_node_terminal(VALUE self) {
    TrieState *state;
    Data_Get_Struct(self, TrieState, state);
    
    return trie_state_is_terminal(state) ? Qtrue : Qnil;
}

static VALUE rb_trie_node_leaf(VALUE self) {
    TrieState *state;
    Data_Get_Struct(self, TrieState, state);
    
    return trie_state_is_leaf(state) ? Qtrue : Qnil;
}

static VALUE rb_trie_node_clone(VALUE self) {
    TrieState *state;
    Data_Get_Struct(self, TrieState, state);
    
    VALUE new_node = rb_trie_node_alloc(cTrieNode);

	TrieState *new_state = trie_state_clone(state);

    RDATA(new_node)->data = new_state;
    
    rb_iv_set(new_node, "@state", rb_iv_get(self, "@state"));
    rb_iv_set(new_node, "@full_state", rb_iv_get(self, "@full_state"));

    return new_node;
}

 
void Init_trie() {
    cTrie = rb_define_class("Trie", rb_cObject);
    rb_define_alloc_func(cTrie, rb_trie_alloc);
    //rb_define_method(cTrie, "initialize", rb_trie_initialize, -2);
    //rb_define_method(cTrie, "path", rb_trie_get_path, 0);
    rb_define_method(cTrie, "has_key?", rb_trie_has_key, 1);
    rb_define_method(cTrie, "get", rb_trie_get, 1);
    rb_define_method(cTrie, "add", rb_trie_add, -2);
    rb_define_method(cTrie, "delete", rb_trie_delete, 1);
    rb_define_method(cTrie, "children", rb_trie_children, 1);
    rb_define_method(cTrie, "children_with_values", rb_trie_children_with_values, 1);
    rb_define_method(cTrie, "root", rb_trie_root, 0);
    //rb_define_method(cTrie, "save", rb_trie_save, 0);

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
