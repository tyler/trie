#include "ruby.h"
#include "trie.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dstring.h"
#include "ruby/encoding.h"

#define GetTrie(obj, trie)                                 \
  do                                                       \
  {                                                        \
    TypedData_Get_Struct((obj), Trie, &trie_type, (trie)); \
    if ((trie) == NULL)                                    \
      raise_null_object("Trie");                           \
  } while (0)

#define GetTrieState(obj, trie_state)                                      \
  do                                                                       \
  {                                                                        \
    TypedData_Get_Struct((obj), TrieState, &trie_node_type, (trie_state)); \
    if ((trie_state) == NULL)                                              \
      raise_null_object("TrieNode");                                       \
  } while (0)

#define GetAlphaMap(obj, alpha_map)                                      \
  do                                                                     \
  {                                                                      \
    TypedData_Get_Struct((obj), AlphaMap, &alpha_map_type, (alpha_map)); \
    if ((alpha_map) == NULL)                                             \
      raise_null_object("AlphaMap");                                     \
  } while (0)

static VALUE cTrie = Qnil;
static VALUE cTrieNode = Qnil;
static VALUE cAlphaMap = Qnil;
static rb_encoding *utf32_encoding = NULL;
static AlphaMap *default_alpha_map = NULL;

static void raise_null_object(const char *className)
{
  rb_raise(rb_eRuntimeError, "%s was not allocated but not initialized", className);
}

static void rb_trie_free(void *data)
{
  if (data)
  {
    trie_free((Trie *)data);
  }
}

static size_t rb_trie_memsize(const void *data)
{
  return data ? trie_get_serialized_size((Trie *)data) : 0;
}

static const rb_data_type_t trie_type = {
    .wrap_struct_name = "Trie",
    .function = {
        .dmark = NULL,
        .dfree = rb_trie_free,
        .dsize = rb_trie_memsize,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void rb_trie_node_free(void *data)
{
  if (data)
  {
    trie_state_free((TrieState *)data);
  }
}

static size_t rb_trie_node_memsize(const void *ptr)
{
  // Estimate - libdatrie struct is opaque
  return 64;
}

static const rb_data_type_t trie_node_type = {
    .wrap_struct_name = "TrieNode",
    .function = {
        .dmark = NULL,
        .dfree = rb_trie_node_free,
        .dsize = rb_trie_node_memsize},
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void rb_alpha_map_free(void *data)
{
  if (data)
  {
    alpha_map_free((AlphaMap *)data);
  }
}

static size_t rb_alpha_map_memsize(const void *ptr)
{
  // Estimate - libdatrie struct is opaque
  return 64;
}

static const rb_data_type_t alpha_map_type = {
    .wrap_struct_name = "AlphaMap",
    .function = {
        .dmark = NULL,
        .dfree = rb_alpha_map_free,
        .dsize = rb_alpha_map_memsize,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

/*
 * Fetch UTF-32 encoding for this system.
 */
static rb_encoding *
get_utf32_encoding()
{
  if (utf32_encoding == NULL)
  {
    int i = 1;
    if (*((char *)&i) == 1)
    {
      utf32_encoding = rb_enc_find("UTF-32LE");
    }
    else
    {
      utf32_encoding = rb_enc_find("UTF-32BE");
    }

    if (utf32_encoding == NULL)
    {
      rb_raise(rb_eRuntimeError, "rb_enc_get() failed to get UTF-32 encoding in alpha_char_new_from_rb_value");
    }
  }
  return utf32_encoding;
}

/*
 * Convert Ruby value to AlphaChar string.
 */
static AlphaChar *
alpha_char_new_from_rb_value(VALUE value)
{
  StringValue(value);

  if (rb_enc_str_asciionly_p(value))
  {
    // Fast path for ACSII-only strings
    long len = RSTRING_LEN(value);
    AlphaChar *result = (AlphaChar *)malloc(sizeof(AlphaChar) * (len + 1));
    if (result == NULL)
    {
      rb_raise(rb_eRuntimeError, "malloc() failed in alpha_char_new_from_rb_value");
    }
    char *s = RSTRING_PTR(value);
    for (long i = 0; i < len; i++)
    {
      result[i] = s[i];
    }
    result[len] = 0;
    return result;
  }
  else
  {
    // Convert string to Unicode code points (UTF-32). TODO: Endianness. Detect BE system.
    VALUE utf32_string_value = rb_str_conv_enc(value, NULL, get_utf32_encoding());

    int len = RSTRING_LEN(utf32_string_value);

    AlphaChar *result = (AlphaChar *)malloc(sizeof(AlphaChar) * (len + 1));
    if (result == NULL)
    {
      rb_raise(rb_eRuntimeError, "malloc() failed in alpha_char_new_from_rb_value");
    }
    memcpy(result, RSTRING_PTR(utf32_string_value), len * sizeof(AlphaChar));
    result[len] = 0;
    return result;
  }
}

void alpha_char_free(AlphaChar *s)
{
  if (s != NULL)
  {
    free(s);
  }
}

/*
 * Document-class: AlphaMap
 *
 * Defines mapping between character codes and trie alphabet.
 *
 */

static VALUE
rb_alpha_map_alloc(VALUE klass)
{
  AlphaMap *alpha_map = alpha_map_new();
  if (alpha_map == NULL)
  {
    rb_raise(rb_eRuntimeError, "alpha_map_new() failed in rb_alpha_map_alloc");
  }

  return TypedData_Wrap_Struct(klass, &alpha_map_type, alpha_map);
}

/*
 * call-seq: add_range(begin, end)
 *
 * Adds the given range of characters to the alpha map.
 */
static VALUE
rb_alpha_map_add_range(VALUE self, VALUE begin, VALUE end)
{
  AlphaMap *alpha_map;
  GetAlphaMap(self, alpha_map);

  Check_Type(begin, T_FIXNUM);
  Check_Type(end, T_FIXNUM);

  if (alpha_map_add_range(alpha_map, (AlphaChar)NUM2UINT(begin), (AlphaChar)NUM2UINT(end)) != 0)
  {
    rb_raise(rb_eRuntimeError, "alpha_map_add_range() failed in rb_alpha_map_add_range");
  }

  return Qnil;
}

/*
 * Document-class: Trie
 *
 * A key-value data structure for string keys which is efficient memory usage
 * and fast retrieval time.
 *
 */

static VALUE
rb_trie_alloc(VALUE klass)
{
  return TypedData_Wrap_Struct(klass, &trie_type, NULL);
}

static VALUE
rb_trie_initialize(int argc, VALUE *argv, VALUE self)
{
  AlphaMap *alpha_map = default_alpha_map;
  VALUE alpha_map_value = Qnil;

  if (rb_scan_args(argc, argv, "01", &alpha_map_value) != 0)
  {
    if (!rb_obj_is_kind_of(alpha_map_value, cAlphaMap))
    {
      rb_raise(rb_eTypeError, "Trie#initialize must be passed an AlphaMap");
    }

    GetAlphaMap(alpha_map_value, alpha_map);
  }

  Trie *trie = trie_new(alpha_map);
  if (trie == NULL)
  {
    rb_raise(rb_eRuntimeError, "trie_new() failed in rb_trie_initialize");
  }

  RDATA(self)->data = trie;

  return self;
}

static void raise_ioerror(const char *message)
{
  VALUE rb_eIOError = rb_const_get(rb_cObject, rb_intern("IOError"));
  rb_raise(rb_eIOError, "%s", message);
}

/*
 * call-seq: read(filename_base) -> Trie
 *
 * Returns a new trie with data as read from disk.
 */
static VALUE
rb_trie_read(VALUE klass, VALUE filename)
{
  StringValueCStr(filename);

  Trie *trie = trie_new_from_file(RSTRING_PTR(filename));
  if (trie != NULL)
  {
    return TypedData_Wrap_Struct(klass, &trie_type, trie);
  }
  else
  {
    raise_ioerror("Could not read trie from file in rb_trie_read");
    return Qnil;
  }
}

/*
 * call-seq: has_key?(key) -> true/false
 *
 * Determines whether or not a key exists in the Trie.  Use this if you don't
 * care about the value, as it is marginally faster than Trie#get.
 *
 */
static VALUE
rb_trie_has_key(VALUE self, VALUE key)
{
  Trie *trie;
  GetTrie(self, trie);

  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);
  TrieData data;
  Bool result = trie_retrieve(trie, alpha_key, &data);
  alpha_char_free(alpha_key);

  return result ? Qtrue : Qfalse;
}

/*
 * call-seq: get(key) -> value [key]    -> value
 *
 * Retrieves the value for a particular key (or nil) from the Trie.
 *
 */
static VALUE
rb_trie_get(VALUE self, VALUE key)
{
  VALUE result = Qnil;

  Trie *trie;
  GetTrie(self, trie);

  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);

  TrieData data;
  if (trie_retrieve(trie, alpha_key, &data))
  {
    result = INT2FIX(data);
  }

  alpha_char_free(alpha_key);

  return result;
}

/*
 * call-seq: add(key) add(key,value)
 *
 * Add a key, or a key and value to the Trie.  If you add a key without a value
 * it assumes -1 for the value. If the operation succeeds, returns self.
 * If the key could not be added to the trie, returns nil. Any existing key in
 * the trie is replaced.
 *
 */
static VALUE
rb_trie_add(VALUE self, VALUE args)
{
  Trie *trie;
  GetTrie(self, trie);

  long size = RARRAY_LEN(args);
  if (size < 1 || size > 2)
  {
    rb_raise(rb_eArgError, "wrong number of arguments (given 0, expected 1..2)");
    return Qnil;
  }

  TrieData trie_data_value = TRIE_DATA_ERROR;
  if (size == 2)
  {
    VALUE weight = RARRAY_AREF(args, 1);
    Check_Type(weight, T_FIXNUM);
    trie_data_value = NUM2INT(weight);
  }

  VALUE key = RARRAY_AREF(args, 0);
  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);

  trie_store(trie, alpha_key, trie_data_value);

  alpha_char_free(alpha_key);
  return self;
}

/*
 * call-seq: add?(key) add?(key, value)
 *
 * Add a key, or a key and value to the Trie.  If you add a key without a value
 * it assumes -1 for the value. If the key is added to the trie, returns self.
 * If the key is already in the trie or could not be added to the trie, returns nil.
 *
 */
static VALUE
rb_trie_add_p(VALUE self, VALUE args)
{
  Trie *trie;
  GetTrie(self, trie);

  long size = RARRAY_LEN(args);
  if (size < 1 || size > 2)
  {
    rb_raise(rb_eArgError, "wrong number of arguments (given 0, expected 1..2)");
    return Qnil;
  }

  TrieData trie_data_value = TRIE_DATA_ERROR;
  if (size == 2)
  {
    VALUE weight = RARRAY_AREF(args, 1);
    Check_Type(weight, T_FIXNUM);
    trie_data_value = NUM2INT(weight);
  }

  VALUE key = RARRAY_AREF(args, 0);
  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);

  Bool result = trie_store_if_absent(trie, alpha_key, trie_data_value);

  alpha_char_free(alpha_key);

  return result ? self : Qnil;
}

/*
 * call-seq: concat(keys)
 *
 * Adds an array of keys to the Trie. Each element of the array may be a string, or a sub-array
 * containing a key and a weight. Keys that do not match the trie's alphabet are discarded.
 * Returns self.
 *
 */
static VALUE
rb_trie_concat(VALUE self, VALUE keys)
{
  Trie *trie;
  GetTrie(self, trie);

  Check_Type(keys, T_ARRAY);

  long size = RARRAY_LEN(keys);
  for (long i = 0; i < size; i++)
  {
    VALUE obj = RARRAY_AREF(keys, i);

    if (TYPE(obj) == T_ARRAY)
    {
      VALUE key = RARRAY_AREF(obj, 0);
      VALUE weight = RARRAY_LEN(obj) > 1 ? RARRAY_AREF(obj, 1) : INT2FIX(-1);
      Check_Type(weight, T_FIXNUM);
      AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);
      trie_store(trie, alpha_key, NUM2INT(weight));
      alpha_char_free(alpha_key);
    }
    else
    {
      AlphaChar *alpha_key = alpha_char_new_from_rb_value(obj);
      trie_store(trie, alpha_key, TRIE_DATA_ERROR);
      alpha_char_free(alpha_key);
    }
  }

  return self;
}

/*
 * call-seq: delete(key)
 *
 * Delete a key from the Trie and returns self.
 *
 */
static VALUE
rb_trie_delete(VALUE self, VALUE key)
{
  Trie *trie;
  GetTrie(self, trie);

  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);
  trie_delete(trie, alpha_key);
  alpha_char_free(alpha_key);

  return self;
}

/*
 * call-seq: delete?(key)
 *
 * Deletes a key from the Trie and returns self. If a key was not deleted, returns nil.
 *
 */
static VALUE
rb_trie_delete_p(VALUE self, VALUE key)
{
  Trie *trie;
  GetTrie(self, trie);

  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);
  Bool result = trie_delete(trie, alpha_key);

  alpha_char_free(alpha_key);

  return result ? self : Qnil;
}

static VALUE
alpha_char_to_rb_str(AlphaChar *alpha_char)
{
  // Convert AlphaChar* to a Ruby UTF-32 string
  VALUE utf32_value = rb_enc_str_new((const char *)alpha_char, sizeof(AlphaChar) * alpha_char_strlen(alpha_char), get_utf32_encoding());

  // Convert Ruby UTF-32 string to UTF-8
  return rb_str_conv_enc(utf32_value, NULL, rb_utf8_encoding());
}

static Bool
traverse(TrieState *state, AlphaChar *prefix)
{
  while (*prefix)
  {
    if (!trie_state_is_walkable(state, *prefix))
    {
      return FALSE;
    }
    trie_state_walk(state, *prefix++);
  }
  return TRUE;
}

/*
 * call-seq: children(prefix) -> [ key, ... ]
 *
 * Finds all keys in the Trie beginning with the given prefix.
 *
 */
static VALUE
rb_trie_children(VALUE self, VALUE prefix)
{
  TrieIterator *iterator = NULL;
  TrieState *state = NULL;
  AlphaChar *prefix_alpha_char = NULL;
  VALUE children = rb_ary_new();

  if (NIL_P(prefix))
  {
    goto exit_gracefully;
  }
  prefix_alpha_char = alpha_char_new_from_rb_value(prefix);

  Trie *trie;
  GetTrie(self, trie);

  state = trie_root(trie);

  if (!traverse(state, prefix_alpha_char))
  {
    goto exit_gracefully;
  }

  iterator = trie_iterator_new(state);
  if (iterator == NULL)
  {
    goto exit_gracefully;
  }

  while (trie_iterator_next(iterator))
  {
    AlphaChar *key = trie_iterator_get_key(iterator);
    if (key != NULL)
    {
      rb_ary_push(children, rb_str_append(rb_str_dup(prefix), alpha_char_to_rb_str(key)));
      free(key);
    }
  }

exit_gracefully:
  if (state != NULL)
  {
    trie_state_free(state);
    state = NULL;
  }

  if (iterator != NULL)
  {
    trie_iterator_free(iterator);
    iterator = NULL;
  }

  alpha_char_free(prefix_alpha_char);

  return children;
}

/*
 * call-seq: has_children(prefix) -> true/false
 *
 * Returns true if any keys in the Trie begin with the given prefix.
 *
 */
static VALUE
rb_trie_has_children(VALUE self, VALUE prefix)
{
  TrieIterator *iterator = NULL;
  TrieState *state = NULL;
  AlphaChar *prefix_alpha_char = NULL;
  Bool ret = FALSE;

  if (NIL_P(prefix))
  {
    goto exit_gracefully;
  }
  prefix_alpha_char = alpha_char_new_from_rb_value(prefix);

  Trie *trie;
  GetTrie(self, trie);

  state = trie_root(trie);

  if (!traverse(state, prefix_alpha_char))
  {
    goto exit_gracefully;
  }

  iterator = trie_iterator_new(state);
  if (iterator == NULL)
  {
    goto exit_gracefully;
  }

  if (trie_iterator_next(iterator))
  {
    ret = TRUE;
  }

exit_gracefully:
  if (state != NULL)
  {
    trie_state_free(state);
    state = NULL;
  }

  if (iterator != NULL)
  {
    trie_iterator_free(iterator);
    iterator = NULL;
  }

  alpha_char_free(prefix_alpha_char);

  return ret ? Qtrue : Qfalse;
}

/*
 * call-seq: children_with_values(key) -> [ [key,value], ... ]
 *
 * Finds all keys with their respective values in the Trie beginning with the
 * given prefix.
 *
 */
static VALUE
rb_trie_children_with_values(VALUE self, VALUE prefix)
{
  TrieIterator *iterator = NULL;
  TrieState *state = NULL;
  AlphaChar *prefix_alpha_char = NULL;
  VALUE children = rb_ary_new();

  if (NIL_P(prefix))
  {
    goto exit_gracefully;
  }
  prefix_alpha_char = alpha_char_new_from_rb_value(prefix);

  Trie *trie;
  GetTrie(self, trie);

  state = trie_root(trie);

  if (!traverse(state, prefix_alpha_char))
  {
    goto exit_gracefully;
  }

  iterator = trie_iterator_new(state);
  if (iterator == NULL)
  {
    goto exit_gracefully;
  }

  while (trie_iterator_next(iterator))
  {
    AlphaChar *key = trie_iterator_get_key(iterator);
    if (key != NULL)
    {
      TrieData trie_data = trie_iterator_get_data(iterator);

      VALUE tuple = rb_ary_new();
      rb_ary_push(tuple, rb_str_append(rb_str_dup(prefix), alpha_char_to_rb_str(key)));
      rb_ary_push(tuple, INT2FIX(trie_data));
      rb_ary_push(children, tuple);

      free(key);
    }
  }

exit_gracefully:
  if (state != NULL)
  {
    trie_state_free(state);
    state = NULL;
  }

  if (iterator != NULL)
  {
    trie_iterator_free(iterator);
    iterator = NULL;
  }

  alpha_char_free(prefix_alpha_char);

  return children;
}

static VALUE rb_trie_node_alloc(VALUE klass);

/*
 * call-seq: root -> TrieNode
 *
 * Returns a TrieNode representing the root of the Trie.
 *
 */
static VALUE
rb_trie_root(VALUE self)
{
  Trie *trie;
  GetTrie(self, trie);

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
 * Represents a single node in the Trie. It can be used as a cursor to walk
 * around the Trie. You can grab a TrieNode for the root of the Trie by using
 * Trie#root.
 *
 */

static VALUE
rb_trie_node_alloc(VALUE klass)
{
  return TypedData_Wrap_Struct(klass, &trie_node_type, NULL);
}

/* nodoc */
static VALUE
rb_trie_node_initialize_copy(VALUE self, VALUE from)
{
  RDATA(self)->data = trie_state_clone(RDATA(from)->data);

  VALUE state = rb_iv_get(from, "@state");
  rb_iv_set(self, "@state", state == Qnil ? Qnil : rb_str_dup(state));

  VALUE full_state = rb_iv_get(from, "@full_state");
  rb_iv_set(self, "@full_state", full_state == Qnil ? Qnil : rb_str_dup(full_state));

  return self;
}

/*
 * call-seq: state -> single character
 *
 * Returns the letter that the TrieNode instance points to. So, if the node is
 * pointing at the "e" in "monkeys", the state is "e".
 *
 */
static VALUE
rb_trie_node_get_state(VALUE self)
{
  return rb_iv_get(self, "@state");
}

/*
 * call-seq: full_state -> string
 *
 * Returns the full string from the root of the Trie up to this node.  So if the
 * node pointing at the "e" in "monkeys", the full_state is "monke".
 *
 */
static VALUE
rb_trie_node_get_full_state(VALUE self)
{
  return rb_iv_get(self, "@full_state");
}

/*
 * call-seq: walk!(letter) -> TrieNode
 *
 * Tries to walk down a particular branch of the Trie.  It modifies the node it
 * is called on.
 *
 */
static VALUE
rb_trie_node_walk_bang(VALUE self, VALUE rchar)
{
  VALUE result = Qnil;

  StringValue(rchar);
  AlphaChar *alpha_char = alpha_char_new_from_rb_value(rchar);

  TrieState *state;
  GetTrieState(self, state);

  if (alpha_char_strlen(alpha_char) == 1 && trie_state_walk(state, *alpha_char))
  {
    rb_iv_set(self, "@state", rchar);
    VALUE full_state = rb_iv_get(self, "@full_state");
    rb_str_append(full_state, rchar);
    rb_iv_set(self, "@full_state", full_state);
    result = self;
  }

  alpha_char_free(alpha_char);
  return result;
}

/*
 * call-seq: walk(letter) -> TrieNode
 *
 * Tries to walk down a particular branch of the Trie.  It clones the node it is
 * called on and walks with that one, leaving the original unchanged.
 *
 */
static VALUE
rb_trie_node_walk(VALUE self, VALUE rchar)
{
  VALUE result = Qnil;

  StringValue(rchar);
  AlphaChar *alpha_char = alpha_char_new_from_rb_value(rchar);

  VALUE new_node = rb_funcall(self, rb_intern("dup"), 0);

  TrieState *state;
  GetTrieState(self, state);

  if (alpha_char_strlen(alpha_char) == 1 && trie_state_walk(state, *alpha_char))
  {
    rb_iv_set(new_node, "@state", rchar);
    VALUE full_state = rb_iv_get(new_node, "@full_state");
    rb_str_append(full_state, rchar);
    rb_iv_set(new_node, "@full_state", full_state);
    result = new_node;
  }

  alpha_char_free(alpha_char);
  return result;
}

/*
 * call-seq: value
 *
 * Attempts to get the value at this node of the Trie.  This only works if the
 * node is a terminal (i.e. end of a key), otherwise it returns nil.
 *
 */
static VALUE
rb_trie_node_value(VALUE self)
{
  TrieState *state;
  GetTrieState(self, state);

  if (!trie_state_is_terminal(state))
  {
    return Qnil;
  }

  TrieState *dup = trie_state_clone(state);

  trie_state_walk(dup, 0);
  TrieData trie_data = trie_state_get_data(dup);
  trie_state_free(dup);

  return INT2FIX(trie_data);
}

/*
 * call-seq: terminal? -> true/false
 *
 * Returns true if this node is at the end of a key.  So if you have two keys in
 * your Trie, "he" and "hello", and you walk all the way to the end of
 * "hello", the "e" and the "o" will return true for terminal?.
 *
 */
static VALUE
rb_trie_node_terminal(VALUE self)
{
  TrieState *state;
  GetTrieState(self, state);

  return trie_state_is_terminal(state) ? Qtrue : Qfalse;
}

/*
 * call-seq: leaf? -> true/false
 *
 * Returns true if there are no branches at this node.
 */
static VALUE
rb_trie_node_leaf(VALUE self)
{
  TrieState *state;
  GetTrieState(self, state);

  return trie_state_is_leaf(state) ? Qtrue : Qfalse;
}

/*
 * call-seq: save(filename_base) -> true
 *
 * Saves the trie data to a file. Returns true if saving was successful.
 */
static VALUE
rb_trie_save(VALUE self, VALUE filename)
{
  StringValueCStr(filename);

  Trie *trie;
  GetTrie(self, trie);

  int res = trie_save(trie, RSTRING_PTR(filename));
  return res == 0 ? Qtrue : Qnil;
}

/*
 * call-seq: marshal_dump() -> marshaled data dump
 *
 * Serializes the Trie for use in Marshal serialization. Returns the
 * serialized representation of the Trie.
 */
static VALUE
rb_trie_marshal_dump(VALUE self)
{
  Trie *trie;
  GetTrie(self, trie);

  size_t size = trie_get_serialized_size(trie);
  uint8 *ptr = (uint8 *)malloc(size);
  if (ptr == NULL)
  {
    rb_raise(rb_eRuntimeError, "malloc() failed in rb_trie_marshal_dump()");
  }

  trie_serialize(trie, ptr);

  VALUE hash = rb_hash_new();
  rb_hash_aset(hash, ID2SYM(rb_intern("data")), rb_str_new((char *)ptr, size));

  free(ptr);

  return hash;
}

static VALUE
rb_trie_marshal_load(VALUE self, VALUE hash)
{
  Check_Type(hash, T_HASH);

  VALUE data = rb_hash_aref(hash, ID2SYM(rb_intern("data")));
  StringValue(data);

  FILE *fp = fmemopen(RSTRING_PTR(data), RSTRING_LEN(data), "r");
  if (fp == NULL)
  {
    rb_raise(rb_eRuntimeError, "fmemopen() failed in rb_trie_marshal_load()");
  }

  Trie *new_trie = trie_fread(fp);
  fclose(fp);

  if (new_trie == NULL)
  {
    rb_raise(rb_eRuntimeError, "trie_fread() failed in rb_trie_marshal_load()");
  }

  if (RDATA(self)->data != NULL)
  {
    // marshal_load has been called on an already-initialized Trie... free it.
    trie_free((Trie *)RDATA(self)->data);
  }

  RDATA(self)->data = new_trie;
  return Qnil;
}

void Init_trie()
{
  cTrie = rb_define_class("Trie", rb_cObject);
  rb_define_alloc_func(cTrie, rb_trie_alloc);
  rb_define_method(cTrie, "initialize", rb_trie_initialize, -1);
  rb_define_module_function(cTrie, "read", rb_trie_read, 1);
  rb_define_method(cTrie, "has_key?", rb_trie_has_key, 1);
  rb_define_method(cTrie, "get", rb_trie_get, 1);
  rb_define_method(cTrie, "add", rb_trie_add, -2);
  rb_define_method(cTrie, "add?", rb_trie_add_p, -2);
  rb_define_method(cTrie, "concat", rb_trie_concat, 1);
  rb_define_method(cTrie, "delete", rb_trie_delete, 1);
  rb_define_method(cTrie, "delete?", rb_trie_delete_p, 1);
  rb_define_method(cTrie, "children", rb_trie_children, 1);
  rb_define_method(cTrie, "children_with_values", rb_trie_children_with_values, 1);
  rb_define_method(cTrie, "has_children?", rb_trie_has_children, 1);
  rb_define_method(cTrie, "root", rb_trie_root, 0);
  rb_define_method(cTrie, "save", rb_trie_save, 1);
  rb_define_method(cTrie, "marshal_load", rb_trie_marshal_load, 1);
  rb_define_method(cTrie, "marshal_dump", rb_trie_marshal_dump, 0);

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

  cAlphaMap = rb_define_class("AlphaMap", rb_cObject);
  rb_define_alloc_func(cAlphaMap, rb_alpha_map_alloc);
  rb_define_method(cAlphaMap, "add_range", rb_alpha_map_add_range, 2);

  default_alpha_map = alpha_map_new();
  alpha_map_add_range(default_alpha_map, 0, 255);
}
