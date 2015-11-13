#include "ruby.h"
#include "datrie/trie.h"
#include "datrie/alpha-map.h"
#include "datrie/triedefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

VALUE cTrie, cTrieNode;

/*
 * Document-class: Trie
 * 
 * A key-value data structure for string keys which is efficient memory usage and fast retrieval time.
 *
 */

static VALUE rb_trie_alloc(VALUE klass) {
	VALUE obj;
    AlphaMap * alpha_map;

    alpha_map = alpha_map_new();
    alpha_map_add_range(alpha_map, 0, 255);
    
	obj = Data_Wrap_Struct(klass, 0, trie_free, trie_new(alpha_map));

    alpha_map_free(alpha_map);
    
	return obj;
}

void raise_ioerror(const char * message) {
    VALUE rb_eIOError = rb_const_get(rb_cObject, rb_intern("IOError"));
    rb_raise(rb_eIOError, "%s", message);
}

/*
 * call-seq:
 *   read(filename_base) -> Trie
 *
 * Returns a new trie with data as read from disk.
 */
static VALUE rb_trie_read(VALUE self, VALUE filename) {
  VALUE obj;
  Trie *trie;

  Data_Get_Struct(self, Trie, trie);
  if (trie != NULL) {
    trie_free(trie);
  }
  
  trie = trie_new_from_file(RSTRING_PTR(filename));
  obj = Data_Wrap_Struct(self, 0, trie_free, trie);

  return obj;
}

/*
 * call-seq:
 *   has_key?(key) -> true/false
 *
 * Determines whether or not a key exists in the Trie.  Use this if you don't care about the value, as it
 * is marginally faster than Trie#get.
 *
 */
static VALUE rb_trie_has_key(VALUE self, VALUE key) {
    Trie *trie;

	StringValue(key);

    Data_Get_Struct(self, Trie, trie);

    if(trie_retrieve(trie, (AlphaChar *)RSTRING_PTR(key), NULL))
		return Qtrue;
    else
		return Qnil;
}

/*
 * call-seq:
 *   get(key) -> value
 *   [key]    -> value
 *
 * Retrieves the value for a particular key (or nil) from the Trie.
 *
 */
static VALUE rb_trie_get(VALUE self, VALUE key) {
    Trie *trie;
	TrieData data;

	StringValue(key);
    Data_Get_Struct(self, Trie, trie);

    if(trie_retrieve(trie, (AlphaChar *)RSTRING_PTR(key), &data))
		return (VALUE)data;
    else
		return Qnil;
}

/*
 * call-seq:
 *   add(key)
 *   add(key,value)
 *
 * Add a key, or a key and value to the Trie.  If you add a key without a value it assumes true for the value. 
 *
 */
static VALUE rb_trie_add(VALUE self, VALUE args) {
	Trie *trie;
    int size;
    VALUE key;
    TrieData value;
    
    Data_Get_Struct(self, Trie, trie);

    size = RARRAY_LEN(args);
    if(size < 1 || size > 2)
		return Qnil;

    key = RARRAY_PTR(args)[0];
	StringValue(key);

    value = size == 2 ? RARRAY_PTR(args)[1] : TRIE_DATA_ERROR;
    
    if(trie_store(trie, (AlphaChar *)RSTRING_PTR(key), value))
		return Qtrue;
    else
		return Qnil;
}

/*
 * call-seq:
 *   delete(key)
 *
 * Delete a key from the Trie.  Returns true if it deleted a key, nil otherwise.
 *
 */
static VALUE rb_trie_delete(VALUE self, VALUE key) {
	Trie *trie;
	StringValue(key);

    Data_Get_Struct(self, Trie, trie);

    if(trie_delete(trie, (AlphaChar *)RSTRING_PTR(key)))
		return Qtrue;
    else
		return Qnil;
}

static void walk_all_paths(Trie *trie, VALUE children, TrieState *state, char *prefix, int prefix_size) {
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


static Bool traverse(TrieState *state, AlphaChar *char_prefix) {
	const AlphaChar *iterator = char_prefix;
	while(*iterator != 0) {
		if(!trie_state_is_walkable(state, *iterator))
			return FALSE;
		trie_state_walk(state, *iterator);
		iterator++;
	}
	return TRUE;
}


/*
 * call-seq:
 *   children(prefix) -> [ key, ... ]
 *
 * Finds all keys in the Trie beginning with the given prefix. 
 *
 */
static VALUE rb_trie_children(VALUE self, VALUE prefix) {
    Trie *trie;
	int prefix_size;
    TrieState *state;
    VALUE children;
	AlphaChar *char_prefix;
	char prefix_buffer[1024];

    if(NIL_P(prefix))
		return rb_ary_new();

	StringValue(prefix);

    Data_Get_Struct(self, Trie, trie);

	prefix_size = RSTRING_LEN(prefix);
    state = trie_root(trie);
    children = rb_ary_new();
	char_prefix = (AlphaChar*)RSTRING_PTR(prefix);
    
    if(!traverse(state, char_prefix)) {
    	return children;
    }

    if(trie_state_is_terminal(state))
		rb_ary_push(children, prefix);
	
	memcpy(prefix_buffer, char_prefix, prefix_size);
	prefix_buffer[prefix_size] = 0;

    walk_all_paths(trie, children, state, prefix_buffer, prefix_size);

    trie_state_free(state);
    return children;
}

static Bool walk_all_paths_until_first_terminal(Trie *trie, TrieState *state, char *prefix, int prefix_size) {
	int c;
	Bool ret = FALSE;
    for(c = 1; c < 256; c++) {
		if(trie_state_is_walkable(state,c)) {
			TrieState *next_state = trie_state_clone(state);
			trie_state_walk(next_state, c);

			prefix[prefix_size] = c;
			prefix[prefix_size + 1] = 0;

			if(trie_state_is_terminal(next_state)) {
				return TRUE;
			}

			ret = walk_all_paths_until_first_terminal(trie, next_state, prefix, prefix_size + 1);

			prefix[prefix_size] = 0;
			trie_state_free(next_state);

			if (ret == TRUE) {
				return ret;
			}
		}
    }

    return ret;
}

static VALUE rb_trie_has_children(VALUE self, VALUE prefix) {
    Trie *trie;
	int prefix_size;
    TrieState *state;
	AlphaChar *char_prefix;
	char prefix_buffer[1024];
    Bool ret;

    if(NIL_P(prefix))
		return rb_ary_new();

	StringValue(prefix);

    Data_Get_Struct(self, Trie, trie);

	prefix_size = RSTRING_LEN(prefix);
    state = trie_root(trie);
	char_prefix = (AlphaChar*)RSTRING_PTR(prefix);

    if(!traverse(state, char_prefix)) {
		return Qfalse;
	}

    if(trie_state_is_terminal(state))
        return Qtrue;

	memcpy(prefix_buffer, char_prefix, prefix_size);
	prefix_buffer[prefix_size] = 0;

    ret = walk_all_paths_until_first_terminal(trie, state, prefix_buffer, prefix_size);

    trie_state_free(state);
    return ret == TRUE ? Qtrue : Qfalse;
}

static void walk_all_paths_with_values(Trie *trie, VALUE children, TrieState *state, char *prefix, int prefix_size) {
	int c;
    TrieState *next_state, *end_state;
    char *word;
    VALUE tuple;
    TrieData trie_data;
    
    for(c = 1; c < 256; c++) {
		if(trie_state_is_walkable(state,c)) {
			next_state = trie_state_clone(state);
			trie_state_walk(next_state, c);

			prefix[prefix_size] = c;
			prefix[prefix_size + 1] = 0;

			if(trie_state_is_terminal(next_state)) {
				end_state = trie_state_clone(next_state);
				trie_state_walk(end_state, '\0');
 
				word = (char*) malloc(prefix_size + 2);
				memcpy(word, prefix, prefix_size + 2);

				tuple = rb_ary_new();
				rb_ary_push(tuple, rb_str_new2(word));

				trie_data = trie_state_get_data(end_state);
				rb_ary_push(tuple, (VALUE)trie_data);
				rb_ary_push(children, tuple);
 
				trie_state_free(end_state);
			}

			walk_all_paths_with_values(trie, children, next_state, prefix, prefix_size + 1);
			
			prefix[prefix_size] = 0;
			trie_state_free(next_state);
		}
    }
}

/*
 * call-seq:
 *   children_with_values(key) -> [ [key,value], ... ]
 *
 * Finds all keys with their respective values in the Trie beginning with the given prefix. 
 * 
 */
static VALUE rb_trie_children_with_values(VALUE self, VALUE prefix) {
    Trie *trie;
	int prefix_size;
    AlphaChar *char_prefix;
    VALUE children, tuple;
    TrieState *state, *end_state;
    TrieData trie_data;
	char prefix_buffer[1024];
    
    if(NIL_P(prefix))
		return rb_ary_new();

	StringValue(prefix);

    Data_Get_Struct(self, Trie, trie);

	prefix_size = RSTRING_LEN(prefix);
    char_prefix = (AlphaChar*)RSTRING_PTR(prefix);
    
    children = rb_ary_new();

    state = trie_root(trie);
    
    if(!traverse(state, char_prefix)) {
		return children;
	}

    if(trie_state_is_terminal(state)) {
		end_state = trie_state_clone(state);
		trie_state_walk(end_state, '\0');

		tuple = rb_ary_new();
		rb_ary_push(tuple, prefix);
		trie_data = trie_state_get_data(end_state);
		rb_ary_push(tuple, (VALUE)trie_data);
		rb_ary_push(children, tuple);

		trie_state_free(end_state);
    }

	memcpy(prefix_buffer, char_prefix, prefix_size);
	prefix_buffer[prefix_size] = 0;

    walk_all_paths_with_values(trie, children, state, prefix_buffer, prefix_size);

    trie_state_free(state);
    return children;
}

static VALUE rb_trie_node_alloc(VALUE klass);

/*
 * call-seq:
 *   root -> TrieNode
 *
 * Returns a TrieNode representing the root of the Trie.
 *
 */
static VALUE rb_trie_root(VALUE self) {
    Trie *trie;
    VALUE trie_node;
	TrieState *state;
    
    Data_Get_Struct(self, Trie, trie);

    trie_node = rb_trie_node_alloc(cTrieNode);

	state = trie_root(trie);
	RDATA(trie_node)->data = state;
    
    rb_iv_set(trie_node, "@state", Qnil);
    rb_iv_set(trie_node, "@full_state", rb_str_new2(""));
    return trie_node;
}


/*
 * Document-class: TrieNode
 * 
 * Represents a single node in the Trie. It can be used as a cursor to walk around the Trie.
 * You can grab a TrieNode for the root of the Trie by using Trie#root.
 *
 */

static VALUE rb_trie_node_alloc(VALUE klass) {
    VALUE obj;
    obj = Data_Wrap_Struct(klass, 0, trie_state_free, NULL);
    return obj;
}

/* nodoc */
static VALUE rb_trie_node_initialize_copy(VALUE self, VALUE from) {
    VALUE state, full_state;
    
	RDATA(self)->data = trie_state_clone(RDATA(from)->data);
    
    state = rb_iv_get(from, "@state");
    rb_iv_set(self, "@state", state == Qnil ? Qnil : rb_str_dup(state));

    full_state = rb_iv_get(from, "@full_state");
    rb_iv_set(self, "@full_state", full_state == Qnil ? Qnil : rb_str_dup(full_state));

    return self;
}

/*
 * call-seq:
 *   state -> single character
 *
 * Returns the letter that the TrieNode instance points to. So, if the node is pointing at the "e" in "monkeys", the state is "e".
 *
 */
static VALUE rb_trie_node_get_state(VALUE self) {
    return rb_iv_get(self, "@state");
}

/*
 * call-seq:
 *   full_state -> string
 *
 * Returns the full string from the root of the Trie up to this node.  So if the node pointing at the "e" in "monkeys",
 * the full_state is "monke".
 *
 */
static VALUE rb_trie_node_get_full_state(VALUE self) {
    return rb_iv_get(self, "@full_state");
}

/*
 * call-seq:
 *   walk!(letter) -> TrieNode
 *
 * Tries to walk down a particular branch of the Trie.  It modifies the node it is called on.
 *
 */
static VALUE rb_trie_node_walk_bang(VALUE self, VALUE rchar) {
    TrieState *state;
    Bool result;
    VALUE full_state;

	StringValue(rchar);

    Data_Get_Struct(self, TrieState, state);

    if(RSTRING_LEN(rchar) != 1)
		return Qnil;

    result = trie_state_walk(state, *RSTRING_PTR(rchar));
    
    if(result) {
		rb_iv_set(self, "@state", rchar);
		full_state = rb_iv_get(self, "@full_state");
		rb_str_append(full_state, rchar);
		rb_iv_set(self, "@full_state", full_state);
		return self;
    } else
		return Qnil;
}

/*
 * call-seq:
 *   walk(letter) -> TrieNode
 *
 * Tries to walk down a particular branch of the Trie.  It clones the node it is called on and 
 * walks with that one, leaving the original unchanged.
 *
 */
static VALUE rb_trie_node_walk(VALUE self, VALUE rchar) {
	VALUE new_node, full_state;
    TrieState *state;
    Bool result;

	StringValue(rchar);

	new_node = rb_funcall(self, rb_intern("dup"), 0);

    Data_Get_Struct(new_node, TrieState, state);

    if(RSTRING_LEN(rchar) != 1)
		return Qnil;

    result = trie_state_walk(state, *RSTRING_PTR(rchar));
    
    if(result) {
		rb_iv_set(new_node, "@state", rchar);
		full_state = rb_iv_get(new_node, "@full_state");
		rb_str_append(full_state, rchar);
		rb_iv_set(new_node, "@full_state", full_state);
		return new_node;
    } else
		return Qnil;
}

/*
 * call-seq:
 *   value
 *
 * Attempts to get the value at this node of the Trie.  This only works if the node is a terminal 
 * (i.e. end of a key), otherwise it returns nil.
 *
 */
static VALUE rb_trie_node_value(VALUE self) {
    TrieState *state, *dup;
    TrieData trie_data;
    Data_Get_Struct(self, TrieState, state);
    
    dup = trie_state_clone(state);

    trie_state_walk(dup, 0);
    trie_data = trie_state_get_data(dup);
    trie_state_free(dup);

    return TRIE_DATA_ERROR == trie_data ? Qnil : (VALUE)trie_data;
}

/*
 * call-seq:
 *   terminal? -> true/false
 *
 * Returns true if this node is at the end of a key.  So if you have two keys in your Trie, "he" and
 * "hello", and you walk all the way to the end of "hello", the "e" and the "o" will return true for terminal?.
 *
 */
static VALUE rb_trie_node_terminal(VALUE self) {
    TrieState *state;
    Data_Get_Struct(self, TrieState, state);
    
    return trie_state_is_terminal(state) ? Qtrue : Qnil;
}

/*
 * call-seq:
 *   leaf? -> true/false
 *
 * Returns true if there are no branches at this node.
 */
static VALUE rb_trie_node_leaf(VALUE self) {
    TrieState *state;
    Data_Get_Struct(self, TrieState, state);
    
    return trie_state_is_leaf(state) ? Qtrue : Qnil;
}

/*
 * call-seq:
 *   save(filename_base) -> true
 *
 * Saves the trie data to two files, filename_base.da and filename_base.tail.
 * Returns true if saving was successful.
 */
static VALUE rb_trie_save(VALUE self, VALUE filename_base) {
    /*
  VALUE da_filename, tail_filename;
  Trie *trie;
  FILE *da_file, *tail_file;

  rb_str_concat(da_filename, rb_str_new2(".da"));
  StringValue(da_filename);
    
  tail_filename = rb_str_dup(filename_base);
  rb_str_concat(tail_filename, rb_str_new2(".tail"));
  StringValue(tail_filename);

  Data_Get_Struct(self, Trie, trie);

  da_file = fopen(RSTRING_PTR(da_filename), "w");
  if (da_file == NULL)
    raise_ioerror("Error opening .da file for writing.");
  if (da_write(trie->da, da_file) != 0)
    raise_ioerror("Error writing DArray data.");
  fclose(da_file);

  tail_file = fopen(RSTRING_PTR(tail_filename), "w");
  if (tail_file == NULL)
    raise_ioerror("Error opening .tail file for writing.");
  if (tail_write(trie->tail, tail_file) != 0)
    raise_ioerror("Error writing Tail data.");
  fclose(tail_file);
    */

  return Qtrue;
}

 
void Init_trie() {
    cTrie = rb_define_class("Trie", rb_cObject);
    rb_define_alloc_func(cTrie, rb_trie_alloc);
    rb_define_module_function(cTrie, "read", rb_trie_read, 1);
    rb_define_method(cTrie, "has_key?", rb_trie_has_key, 1);
    rb_define_method(cTrie, "get", rb_trie_get, 1);
    rb_define_method(cTrie, "add", rb_trie_add, -2);
    rb_define_method(cTrie, "delete", rb_trie_delete, 1);
    rb_define_method(cTrie, "children", rb_trie_children, 1);
    rb_define_method(cTrie, "children_with_values", rb_trie_children_with_values, 1);
    rb_define_method(cTrie, "has_children?", rb_trie_has_children, 1);
    rb_define_method(cTrie, "root", rb_trie_root, 0);
    rb_define_method(cTrie, "save", rb_trie_save, 1);

    cTrieNode = rb_define_class("TrieNode", rb_cObject);
    rb_define_alloc_func(cTrieNode, rb_trie_node_alloc);
    rb_define_method(cTrieNode, "initialize_copy", rb_trie_node_initialize_copy, 1);
    rb_define_method(cTrieNode, "state", rb_trie_node_get_state, 0);
    rb_define_method(cTrieNode, "full_state", rb_trie_node_get_full_state, 0);
    rb_define_method(cTrieNode, "walk!", rb_trie_node_walk_bang, 1);
    rb_define_method(cTrieNode, "walk", rb_trie_node_walk, 1);
    rb_define_method(cTrieNode, "value", rb_trie_node_value, 0);
    rb_define_method(cTrieNode, "terminal?", rb_trie_node_terminal, 0);
    rb_define_method(cTrieNode, "leaf?", rb_trie_node_leaf, 0);
}
