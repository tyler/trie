#include "ruby.h"
#include <datrie/sb-trie.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

VALUE cTrie, cTrieNode;

static TrieChar* stringToTrieChar(VALUE string) {
    StringValue(string);
    return (TrieChar*) RSTRING(string)->ptr;
}

static void trie_free(SBTrie *sb_trie) {
    if(sb_trie)
	sb_trie_close(sb_trie);
}

static VALUE trie_alloc(VALUE klass) {
    SBTrie *sb_trie;
    VALUE obj;
    
    obj = Data_Wrap_Struct(klass, 0, trie_free, sb_trie);
    rb_iv_set(obj, "@open", Qfalse);

    return obj;
}

static VALUE trie_initialize(VALUE self, VALUE path) {
    SBTrie *sb_trie;

    char *cpath = RSTRING(path)->ptr;
    char *full_path = (char*)malloc(strlen(cpath) + 10);
    sprintf(full_path, "%s/trie.sbm", cpath);

    FILE *file;

    file = fopen (full_path, "r");
    if (!file) {
        file = fopen (full_path, "w+");
	fprintf(file,"[00,FF]\n");
    }
    fclose(file);
    free (full_path);

    // replace the pretend SBTrie created in alloc with a real one
    RDATA(self)->data = sb_trie_open(cpath, "trie",
				     TRIE_IO_READ | TRIE_IO_WRITE | TRIE_IO_CREATE);

    rb_iv_set(self, "@open", Qtrue);
    rb_iv_set(self, "@path", path);
    return self;
}

static VALUE trie_close(VALUE self) {
    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    rb_iv_set(self, "@open", Qfalse);

    return self;
}

static VALUE trie_has_key(VALUE self, VALUE key) {
    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    const TrieChar *sb_key = (const TrieChar *)RSTRING(key)->ptr;

    if(sb_trie_retrieve(sb_trie, sb_key, NULL))
	return Qtrue;
    else
	return Qnil;
}

static VALUE trie_get(VALUE self, VALUE key) {
    SBTrie *sb_trie;
    TrieData trie_data;
    Data_Get_Struct(self, SBTrie, sb_trie);

    const TrieChar *sb_key = stringToTrieChar(key);

    if(sb_trie_retrieve(sb_trie, sb_key, &trie_data)) {
	return INT2FIX(trie_data);
    } else
	return Qnil;
}

static VALUE trie_add(VALUE self, VALUE args) {
    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    long size = RARRAY(args)->len;
    if(size < 1 || size > 2)
	return Qnil;

    VALUE key;
    key = RARRAY(args)->ptr[0];
    TrieData value = size == 2 ? NUM2INT(RARRAY(args)->ptr[1]) : TRIE_DATA_ERROR;
    
    const TrieChar *sb_key = stringToTrieChar(key);
    
    if(sb_trie_store(sb_trie, sb_key, value))
	return Qtrue;
    else
	return Qnil;
}

static VALUE trie_delete(VALUE self, VALUE key) {
    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    const TrieChar *sb_key = stringToTrieChar(key);

    if(sb_trie_delete(sb_trie, sb_key))
	return Qtrue;
    else
	return Qnil;
}

static VALUE trie_save(VALUE self) {
    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);
    
    if(sb_trie_save(sb_trie) == -1)
	return Qnil;
    else
	return Qtrue;
}

static VALUE walk_all_paths(VALUE children, SBTrieState *state, char *prefix) {
    int c;
    for(c = 1; c < TRIE_CHAR_MAX; c++) {
	if(sb_trie_state_is_walkable(state,c)) {
	    SBTrieState *next_state = sb_trie_state_clone(state);
	    sb_trie_state_walk(next_state, (TrieChar)c);

	    char *word = (char*)malloc(strlen(prefix) + 2);
	    strcat(strcpy(word, prefix), (char*)&c);

	    if(sb_trie_state_is_terminal(next_state))
		rb_ary_push(children, rb_str_new2(word));

	    walk_all_paths(children, next_state, word);

	    sb_trie_state_free(next_state);
	}
    }
}

static VALUE trie_children(VALUE self, VALUE prefix) {
    if(NIL_P(prefix))
	return rb_ary_new();

    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    const TrieChar *sb_prefix = stringToTrieChar(prefix);
    
    VALUE children = rb_ary_new();

    SBTrieState *state = sb_trie_root(sb_trie);
    
    TrieChar *iterator = (TrieChar*)sb_prefix;
    while(*iterator != '\0') {
	if(!sb_trie_state_is_walkable(state, *iterator))
	    return rb_ary_new();
	sb_trie_state_walk(state, *iterator);
	iterator++;
    }

    if(sb_trie_state_is_terminal(state))
	rb_ary_push(children, prefix);

    walk_all_paths(children, state, (char*)sb_prefix);

    sb_trie_state_free(state);
    return children;
}
static VALUE walk_all_paths_with_values(VALUE children, SBTrieState *state, char *prefix) {
    int c;
    for(c = 1; c < TRIE_CHAR_MAX; c++) {
	if(sb_trie_state_is_walkable(state,c)) {
	    SBTrieState *next_state = sb_trie_state_clone(state);
	    sb_trie_state_walk(next_state, (TrieChar)c);

	    char *word = (char*)malloc(strlen(prefix) + 2);
	    strcat(strcpy(word, prefix), (char*)&c);

	    if(sb_trie_state_is_terminal(next_state)) {
		SBTrieState *end_state = sb_trie_state_clone(next_state);
		sb_trie_state_walk(end_state, '\0');

		VALUE tuple = rb_ary_new();
		rb_ary_push(tuple, rb_str_new2(word));
		TrieData trie_data = sb_trie_state_get_data(end_state);
		rb_ary_push(tuple, INT2FIX(trie_data));
		rb_ary_push(children, tuple);

		sb_trie_state_free(end_state);
	    }

	    walk_all_paths_with_values(children, next_state, word);

	    sb_trie_state_free(next_state);
	}
    }
}

static VALUE trie_children_with_values(VALUE self, VALUE prefix) {
    if(NIL_P(prefix))
	return rb_ary_new();

    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    const TrieChar *sb_prefix = stringToTrieChar(prefix);
    
    VALUE children = rb_ary_new();

    SBTrieState *state = sb_trie_root(sb_trie);
    
    TrieChar *iterator = (TrieChar*)sb_prefix;
    while(*iterator != '\0') {
	if(!sb_trie_state_is_walkable(state, *iterator))
	    return rb_ary_new();
	sb_trie_state_walk(state, *iterator);
	iterator++;
    }

    if(sb_trie_state_is_terminal(state)) {
	SBTrieState *end_state = sb_trie_state_clone(state);
	sb_trie_state_walk(end_state, '\0');

	VALUE tuple = rb_ary_new();
	rb_ary_push(tuple, prefix);
	TrieData trie_data = sb_trie_state_get_data(end_state);
	rb_ary_push(tuple, INT2FIX(trie_data));
	rb_ary_push(children, tuple);

	sb_trie_state_free(end_state);
    }

    walk_all_paths_with_values(children, state, (char*)sb_prefix);

    sb_trie_state_free(state);
    return children;
}

static VALUE trie_walk_to_terminal(VALUE self, VALUE args) {
    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    long size = RARRAY(args)->len;
    if(size < 1 || size > 2)
	return Qnil;

    VALUE rb_path;
    rb_path = RARRAY(args)->ptr[0];
    short include_value = size == 2 ? RTEST(RARRAY(args)->ptr[1]) : 0;
    
    SBTrieState *state = sb_trie_root(sb_trie);
    
    char *path = RSTRING(rb_path)->ptr;

    TrieChar *iterator = (TrieChar*)path;
    while(*iterator != '\0') {
	if(sb_trie_state_is_terminal(state)) {
	    int word_length = (int)iterator - (int)path;
	    char *word = (char*)malloc(word_length + 1);
	    strncpy(word, path, word_length);
	    word[word_length] = '\0';
	    VALUE rb_word = rb_str_new2((const char*)word);

	    if(include_value) {
		sb_trie_state_walk(state, (TrieChar)'\0');

		VALUE return_ary = rb_ary_new();
		rb_ary_push(return_ary, rb_word);
		TrieData trie_data = sb_trie_state_get_data(state);
		rb_ary_push(return_ary, INT2FIX(trie_data));
		return return_ary;
	    } else
		return rb_word;
	}

	if(!sb_trie_state_is_walkable(state, *iterator))
	   return Qnil;

	sb_trie_state_walk(state, *iterator);
	iterator++;
    }

    sb_trie_state_free(state);

    return Qnil;
}

static VALUE trie_get_path(VALUE self) {
    return rb_iv_get(self, "@path");
}

static void trie_node_free(SBTrieState *state) {
    if(state)
	sb_trie_state_free(state);
}

static VALUE trie_node_alloc(VALUE klass) {
    SBTrieState *state;
    VALUE obj;
    
    obj = Data_Wrap_Struct(klass, 0, trie_node_free, state);

    return obj;
}

static VALUE trie_root(VALUE self) {
    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    VALUE trie_node = trie_node_alloc(cTrieNode);

    // replace the pretend SBTrieState created in TrieNode's alloc with a real one
    RDATA(trie_node)->data = sb_trie_root(sb_trie);
    
    rb_iv_set(trie_node, "@state", Qnil);
    rb_iv_set(trie_node, "@full_state", rb_str_new2(""));
    return trie_node;
}

static VALUE trie_node_get_state(VALUE self) {
    return rb_iv_get(self, "@state");
}
static VALUE trie_node_get_full_state(VALUE self) {
    return rb_iv_get(self, "@full_state");
}

static VALUE trie_node_walk_bang(VALUE self, VALUE rchar) {
    SBTrieState *state;
    Data_Get_Struct(self, SBTrieState, state);

    if(RSTRING(rchar)->len != 1)
	return Qnil;

    char ch = RSTRING(rchar)->ptr[0];
    int result = sb_trie_state_walk(state, (TrieChar)ch);
    
    if(result) {
	rb_iv_set(self, "@state", rchar);
	VALUE full_state = rb_iv_get(self, "@full_state");
	rb_str_append(full_state, rchar);
	rb_iv_set(self, "@full_state", full_state);
	return self;
    } else
	return Qnil;
}

static VALUE trie_node_value(VALUE self) {
    SBTrieState *state, *dup;
    Data_Get_Struct(self, SBTrieState, state);
    
    dup = sb_trie_state_clone(state);

    sb_trie_state_walk(dup, (TrieChar)'\0');
    TrieData trie_data = sb_trie_state_get_data(dup);
    sb_trie_state_free(dup);

    return TRIE_DATA_ERROR == trie_data ? Qnil : INT2FIX(trie_data);
}

static VALUE trie_node_terminal(VALUE self) {
    SBTrieState *state;
    Data_Get_Struct(self, SBTrieState, state);
    
    return sb_trie_state_is_terminal(state) ? Qtrue : Qnil;
}

static VALUE trie_node_leaf(VALUE self) {
    SBTrieState *state;
    Data_Get_Struct(self, SBTrieState, state);
    
    return sb_trie_state_is_leaf(state) ? Qtrue : Qnil;
}

static VALUE trie_node_clone(VALUE self) {
    SBTrieState *state;
    Data_Get_Struct(self, SBTrieState, state);
    
    VALUE new_node = trie_node_alloc(cTrieNode);
    RDATA(new_node)->data = sb_trie_state_clone(state);
    
    rb_iv_set(new_node, "@state", rb_iv_get(self, "@state"));
    rb_iv_set(new_node, "@full_state", rb_iv_get(self, "@full_state"));

    return new_node;
}

 
void Init_trie() {
    cTrie = rb_define_class("Trie", rb_cObject);
    rb_define_alloc_func(cTrie, trie_alloc);
    rb_define_method(cTrie, "initialize", trie_initialize, 1);
    rb_define_method(cTrie, "path", trie_get_path, 0);
    rb_define_method(cTrie, "has_key?", trie_has_key, 1);
    rb_define_method(cTrie, "get", trie_get, 1);
    rb_define_method(cTrie, "add", trie_add, -2);
    rb_define_method(cTrie, "delete", trie_delete, 1);
    rb_define_method(cTrie, "close", trie_close, 0);
    rb_define_method(cTrie, "children", trie_children, 1);
    rb_define_method(cTrie, "children_with_values", trie_children_with_values, 1);
    rb_define_method(cTrie, "walk_to_terminal", trie_walk_to_terminal, -2);
    rb_define_method(cTrie, "root", trie_root, 0);
    rb_define_method(cTrie, "save", trie_save, 0);

    cTrieNode = rb_define_class("TrieNode", rb_cObject);
    rb_define_alloc_func(cTrieNode, trie_node_alloc);
    rb_define_method(cTrieNode, "state", trie_node_get_state, 0);
    rb_define_method(cTrieNode, "full_state", trie_node_get_full_state, 0);
    rb_define_method(cTrieNode, "walk!", trie_node_walk_bang, 1);
    rb_define_method(cTrieNode, "value", trie_node_value, 0);
    rb_define_method(cTrieNode, "terminal?", trie_node_terminal, 0);
    rb_define_method(cTrieNode, "leaf?", trie_node_leaf, 0);
    rb_define_method(cTrieNode, "clone", trie_node_clone, 0);
}
