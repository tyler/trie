#include "ruby.h"
#include "trie.h"
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
	obj = Data_Wrap_Struct(klass, 0, trie_free, trie_new());
	return obj;
}

void raise_ioerror(const char * message) {
    //VALUE rb_eIOError = rb_const_get(rb_cObject, rb_intern("IOError"));
    rb_raise(rb_eIOError, message);
}

/*
 * call-seq:
 *   read(filename_base) -> Trie
 *
 * Returns a new trie with data as read from disk.
 */
static VALUE rb_trie_read(VALUE self, VALUE filename_base) {
  VALUE da_filename = rb_str_dup(filename_base);
  rb_str_concat(da_filename, rb_str_new2(".da"));
  StringValue(da_filename);
    
  VALUE tail_filename = rb_str_dup(filename_base);
  rb_str_concat(tail_filename, rb_str_new2(".tail"));
  StringValue(tail_filename);

  Trie *trie = trie_new();

  VALUE obj;
  obj = Data_Wrap_Struct(self, 0, trie_free, trie);

  DArray *old_da = trie->da;
  Tail *old_tail = trie->tail;

  FILE *da_file = fopen(RSTRING_PTR(da_filename), "r");
  if (da_file == NULL)
    raise_ioerror("Error reading .da file.");

  trie->da = da_read(da_file);
  fclose(da_file);

  FILE *tail_file = fopen(RSTRING_PTR(tail_filename), "r");
  if (tail_file == NULL)
    raise_ioerror("Error reading .tail file.");

  trie->tail = tail_read(tail_file);
  fclose(tail_file);

  da_free(old_da);
  tail_free(old_tail);

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
	StringValue(key);

    Trie *trie;
    Data_Get_Struct(self, Trie, trie);

    if(trie_has_key(trie, (TrieChar*)RSTRING_PTR(key)))
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
	StringValue(key);

    Trie *trie;
    Data_Get_Struct(self, Trie, trie);

	TrieData data;
    if(trie_retrieve(trie, (TrieChar*)RSTRING_PTR(key), &data))
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
    Data_Get_Struct(self, Trie, trie);

    int size = RARRAY_LEN(args);
    if(size < 1 || size > 2)
		return Qnil;

    VALUE key;
    key = RARRAY_PTR(args)[0];
	StringValue(key);

    TrieData value = size == 2 ? RARRAY_PTR(args)[1] : TRIE_DATA_ERROR;
    
    if(trie_store(trie, (TrieChar*)RSTRING_PTR(key), value))
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
	StringValue(key);

	Trie *trie;
    Data_Get_Struct(self, Trie, trie);

    if(trie_delete(trie, (TrieChar*)RSTRING_PTR(key)))
		return Qtrue;
    else
		return Qnil;
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

/*
 * call-seq:
 *   children(prefix) -> [ key, ... ]
 *
 * Finds all keys in the Trie beginning with the given prefix. 
 *
 */
static VALUE rb_trie_children(VALUE self, VALUE prefix) {
    if(NIL_P(prefix))
		return rb_ary_new();

	StringValue(prefix);

    Trie *trie;
    Data_Get_Struct(self, Trie, trie);

	int prefix_size = RSTRING_LEN(prefix);
    TrieState *state = trie_root(trie);
    VALUE children = rb_ary_new();
	TrieChar *char_prefix = (TrieChar*)RSTRING_PTR(prefix);
    
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
    if(NIL_P(prefix))
		return rb_ary_new();

	StringValue(prefix);

    Trie *trie;
    Data_Get_Struct(self, Trie, trie);

	int prefix_size = RSTRING_LEN(prefix);
    TrieChar *char_prefix = (TrieChar*)RSTRING_PTR(prefix);
    
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
		rb_ary_push(tuple, (VALUE)trie_data);
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
    Data_Get_Struct(self, Trie, trie);

    VALUE trie_node = rb_trie_node_alloc(cTrieNode);

	TrieState *state = trie_root(trie);
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
	RDATA(self)->data = trie_state_clone(RDATA(from)->data);
    
    VALUE state = rb_iv_get(from, "@state");
    rb_iv_set(self, "@state", state == Qnil ? Qnil : rb_str_dup(state));

    VALUE full_state = rb_iv_get(from, "@full_state");
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
	StringValue(rchar);

    TrieState *state;
    Data_Get_Struct(self, TrieState, state);

    if(RSTRING_LEN(rchar) != 1)
		return Qnil;

    Bool result = trie_state_walk(state, *RSTRING_PTR(rchar));
    
    if(result) {
		rb_iv_set(self, "@state", rchar);
		VALUE full_state = rb_iv_get(self, "@full_state");
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
	StringValue(rchar);

	VALUE new_node = rb_funcall(self, rb_intern("dup"), 0);

    TrieState *state;
    Data_Get_Struct(new_node, TrieState, state);

    if(RSTRING_LEN(rchar) != 1)
		return Qnil;

    Bool result = trie_state_walk(state, *RSTRING_PTR(rchar));
    
    if(result) {
		rb_iv_set(new_node, "@state", rchar);
		VALUE full_state = rb_iv_get(new_node, "@full_state");
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
    TrieState *state;
	TrieState *dup;
    Data_Get_Struct(self, TrieState, state);
    
    dup = trie_state_clone(state);

    trie_state_walk(dup, 0);
    TrieData trie_data = trie_state_get_data(dup);
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
  VALUE da_filename = rb_str_dup(filename_base);
  rb_str_concat(da_filename, rb_str_new2(".da"));
  StringValue(da_filename);
    
  VALUE tail_filename = rb_str_dup(filename_base);
  rb_str_concat(tail_filename, rb_str_new2(".tail"));
  StringValue(tail_filename);

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  FILE *da_file = fopen(RSTRING_PTR(da_filename), "w");
  if (da_file == NULL)
    raise_ioerror("Error opening .da file for writing.");
  if (da_write(trie->da, da_file) != 0)
    raise_ioerror("Error writing DArray data.");
  fclose(da_file);

  FILE *tail_file = fopen(RSTRING_PTR(tail_filename), "w");
  if (tail_file == NULL)
    raise_ioerror("Error opening .tail file for writing.");
  if (tail_write(trie->tail, tail_file) != 0)
    raise_ioerror("Error writing Tail data.");
  fclose(tail_file);

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
