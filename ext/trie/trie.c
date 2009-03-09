#include "ruby.h"
#include <datrie/sb-trie.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static TrieChar* stringToTrieChar(VALUE string) {
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

static VALUE trie_add(VALUE self, VALUE key) {
    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    const TrieChar *sb_key = stringToTrieChar(key);
    
    if(sb_trie_store(sb_trie, sb_key, TRIE_DATA_ERROR))
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

static VALUE walk_all_paths(VALUE children, SBTrieState *state, char *prefix) {
    int c;
    for(c = 1; c < TRIE_CHAR_MAX; c++) {
	if(sb_trie_state_is_walkable(state,c)) {
	    SBTrieState *next_state = sb_trie_state_clone(state);
	    sb_trie_state_walk(next_state, (TrieChar)c);

	    char *word = (char*) malloc(strlen(prefix) + 2);
	    strcat(strcpy(word, prefix), (char*)&c);

	    if(sb_trie_state_is_terminal(next_state))
		rb_ary_push(children, rb_str_new2(word));

	    walk_all_paths(children, next_state, word);

	    sb_trie_state_free(next_state);
	}
    }
}

static VALUE trie_children(VALUE self, VALUE prefix) {
    SBTrie *sb_trie;
    Data_Get_Struct(self, SBTrie, sb_trie);

    const TrieChar *sb_prefix = stringToTrieChar(prefix);
    
    VALUE children = rb_ary_new();

    SBTrieState *state = sb_trie_root(sb_trie);
    
    TrieChar *iterator = (TrieChar*)sb_prefix;
    while(*iterator != '\0') {
	if(!sb_trie_state_is_walkable(state, *iterator))
	   return Qnil;
	sb_trie_state_walk(state, *iterator);
	iterator++;
    }

    if(sb_trie_state_is_terminal(state))
	rb_ary_push(children, prefix);

    walk_all_paths(children, state, (char*)sb_prefix);

    sb_trie_state_free(state);
    return children;
}

static VALUE trie_get_path(VALUE self) {
    return rb_iv_get(self, "@path");
}
 
VALUE cTrie;

void Init_trie() {
    cTrie = rb_define_class("Trie", rb_cObject);
    rb_define_alloc_func(cTrie, trie_alloc);
    rb_define_method(cTrie, "initialize", trie_initialize, 1);
    rb_define_method(cTrie, "path", trie_get_path, 0);
    rb_define_method(cTrie, "has_key?", trie_has_key, 1);
    rb_define_method(cTrie, "get", trie_get, 1);
    rb_define_method(cTrie, "add", trie_add, 1);
    rb_define_method(cTrie, "delete", trie_delete, 1);
    rb_define_method(cTrie, "close", trie_close, 0);
    rb_define_method(cTrie, "children", trie_children, 1);
}
