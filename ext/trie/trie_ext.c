#include "ruby.h"
#include "trie.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dstring.h"
#include "ruby/encoding.h"

static VALUE cTrie, cTrieNode, cAlphaMap;
static rb_encoding *utf32_encoding = NULL;
static VALUE default_alpha_map;

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
      rb_raise(rb_eRuntimeError, "Error: rb_enc_get() failed to get UTF-32 encoding in alpha_char_new_from_rb_value\n");
    }
  }
  return utf32_encoding;
}

/*
 * Convert Ruby string to AlphaChar string. The VALUE string_value is expected
 * to be of string type, e.g. coerced with StringValue().
 */
static AlphaChar *
alpha_char_new_from_rb_value(VALUE string_value)
{
  Check_Type(string_value, T_STRING);

  if (rb_enc_str_asciionly_p(string_value))
  {
    // Fast path for ACSII-only strings
    int len = RSTRING_LEN(string_value);
    AlphaChar *result = (AlphaChar *)malloc(sizeof(AlphaChar) * (len + 1));
    if (result == NULL)
    {
      rb_raise(rb_eRuntimeError, "Error: malloc() failed in alpha_char_new_from_rb_value\n");
    }
    char *s = RSTRING_PTR(string_value);
    for (int i = 0; i < len; i++)
    {
      result[i] = s[i];
    }
    result[len] = 0;
    return result;
  }
  else
  {
    // Convert string to Unicode code points (UTF-32). TODO: Endianness. Detect BE system.
    VALUE utf32_string_value = rb_str_conv_enc(string_value, NULL, get_utf32_encoding());

    int len = RSTRING_LEN(utf32_string_value);

    AlphaChar *result = (AlphaChar *)malloc(sizeof(AlphaChar) * (len + 1));
    if (result == NULL)
    {
      rb_raise(rb_eRuntimeError, "Error: malloc() failed in alpha_char_new_from_rb_value\n");
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
 * Data structure for holding the current word, reallocating only when
 * necessary.
 */
#define MIN_WORD_BUFFER_CAPACITY 256

typedef struct
{
  AlphaChar *buffer;
  int capacity;
} WordBuffer;

static void init_word_buffer(WordBuffer *word_buffer)
{
  word_buffer->buffer = NULL;
  word_buffer->capacity = 0;
}

static void
free_word_buffer(WordBuffer *word_buffer)
{
  if (word_buffer->buffer)
  {
    free(word_buffer->buffer);
    word_buffer->buffer = NULL;
  }
  word_buffer->capacity = 0;
}

static AlphaChar *
word_buffer_ptr(WordBuffer *word_buffer)
{
  return word_buffer->buffer;
}

static void
copy_string_to_word_buffer(WordBuffer *word_buffer, char *word_start, int length)
{
  if (word_buffer->buffer == NULL || word_buffer->capacity < length)
  {
    free_word_buffer(word_buffer);
    word_buffer->capacity = length > MIN_WORD_BUFFER_CAPACITY ? length : MIN_WORD_BUFFER_CAPACITY;
    word_buffer->buffer = (AlphaChar *)malloc(sizeof(AlphaChar) * (word_buffer->capacity + 1));
    if (word_buffer->buffer == NULL)
    {
      rb_raise(rb_eRuntimeError, "Error: malloc() failed in copy_string_to_word_buffer\n");
    }
  }
  // Note: This code only supports ASCII characters, not full Unicode.
  AlphaChar *ptr = word_buffer->buffer;
  for (int i = 0; i < length; i++)
  {
    ptr[i] = word_start[i];
  }
  ptr[length] = 0;
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
    rb_raise(rb_eRuntimeError, "Error: alpha_map_new() failed in rb_alpha_map_alloc\n");
  }

  VALUE obj = Data_Wrap_Struct(klass, 0, alpha_map_free, alpha_map);
  rb_iv_set(obj, "@ranges", rb_ary_new());
  return obj;
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
  Data_Get_Struct(self, AlphaMap, alpha_map);

  Check_Type(begin, T_FIXNUM);
  Check_Type(end, T_FIXNUM);

  if (alpha_map_add_range(alpha_map, (AlphaChar)NUM2INT(begin), (AlphaChar)NUM2INT(end)) != 0)
  {
    rb_raise(rb_eRuntimeError, "Error: alpha_map_add_range() failed in rb_alpha_map_add_range\n");
  }

  VALUE ranges = rb_iv_get(self, "@ranges");
  Check_Type(ranges, T_ARRAY);

  VALUE range = rb_ary_new();
  rb_ary_push(range, begin);
  rb_ary_push(range, end);

  rb_ary_push(ranges, range);

  return Qnil;
}

/*
 * call-seq: ranges -> array
 *
 * Returns an array containing all ranges that have been added to this alpha map
 * using add_range. Each range is a tuple [begin, end].
 *
 */
static VALUE
rb_alpha_map_get_ranges(VALUE self)
{
  return rb_iv_get(self, "@ranges");
}

/*
 * Document-class: Trie
 *
 * A key-value data structure for string keys which is efficient memory usage
 * and fast retrieval time.
 *
 */

static VALUE
rb_trie_initialize(VALUE self, VALUE alpha_map_value)
{
  rb_iv_set(self, "@alpha_map", alpha_map_value);
  return self;
}

static VALUE
rb_trie_new(int argc, VALUE *argv, VALUE klass)
{
  VALUE alpha_map_value;

  if (rb_scan_args(argc, argv, "01", &alpha_map_value) == 0)
  {
    alpha_map_value = default_alpha_map;
  }

  if (!rb_obj_is_kind_of(alpha_map_value, cAlphaMap))
  {
    rb_raise(rb_eRuntimeError, "Error: Trie.new must be passed an AlphaMap\n");
  }

  AlphaMap *alpha_map;
  Data_Get_Struct(alpha_map_value, AlphaMap, alpha_map);

  Trie *trie = trie_new(alpha_map);
  if (trie == NULL)
  {
    rb_raise(rb_eRuntimeError, "Error: trie_new() failed in rb_trie_new\n");
  }

  VALUE self = Data_Wrap_Struct(klass, 0, trie_free, trie);

  VALUE init_argv[1];
  init_argv[0] = alpha_map_value;
  rb_obj_call_init(self, 1, init_argv);

  return self;
}

static void raise_ioerror(const char * message)
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
rb_trie_read(VALUE self, VALUE filename)
{
  StringValueCStr(filename);

  Trie *trie = trie_new_from_file(RSTRING_PTR(filename));
  if (trie != NULL)
  {
    return Data_Wrap_Struct(self, 0, trie_free, trie);
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
  StringValue(key);

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);
  TrieData data;
  Bool result = trie_retrieve(trie, alpha_key, &data);
  alpha_char_free(alpha_key);

  return result ? Qtrue : Qnil;
}

static inline Bool
is_word_char_class(char ch)
{
  // Equivalent of "\w" character class in Ruby regex:[A - Za - z0 - 9 _]
  return isalnum(ch) || ch == '_';
}

/*
 * call-seq: text_has_keys?(text) -> true/false
 *
 * Scans all words in the given string, delimited by word boundary characters (\W),
 * and returns true if any of the words are found in the trie.
 *
 */
static VALUE
rb_trie_text_has_keys(VALUE self, VALUE text)
{
  StringValue(text);
  VALUE result = Qnil;

  if (!rb_enc_str_asciionly_p(text))
  {
    rb_raise(rb_eRuntimeError, "Error: rb_trie_text_has_keys supports only ASCII strings\n");
  }

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  char *s = RSTRING_PTR(text);
  int len = RSTRING_LEN(text);

  WordBuffer word_buffer;
  init_word_buffer(&word_buffer);

  int i = 0;
  while (i < len)
  {
    // Skip non-word characters
    while (i < len && !is_word_char_class(s[i]))
    {
      i++;
    }

    // Find bounds of word
    int word_start = i;
    while (i < len && is_word_char_class(s[i]))
    {
      i++;
    }
    int word_end = i;

    int word_len = word_end - word_start;
    if (word_len > 0)
    {
      copy_string_to_word_buffer(&word_buffer, s + word_start, word_len);
      TrieData data;
      if (trie_retrieve(trie, word_buffer_ptr(&word_buffer), &data))
      {
        result = Qtrue;
        break;
      }
    }
  }

  free_word_buffer(&word_buffer);
  return result;
}

/*
 * call-seq: tags_has_keys?(text) -> true/false
 *
 * Scans all tags in the given string, delimited by whitespace characters,
 * and returns true if any of the tags are found in the trie.
 *
 */
static VALUE
rb_trie_tags_has_keys(VALUE self, VALUE tags)
{
  VALUE result = Qnil;
  StringValue(tags);

  if (!rb_enc_str_asciionly_p(tags))
  {
    rb_raise(rb_eRuntimeError, "Error: rb_trie_tags_has_keys supports only ASCII strings\n");
  }

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  char *s = RSTRING_PTR(tags);
  int len = RSTRING_LEN(tags);

  WordBuffer word_buffer;
  init_word_buffer(&word_buffer);

  int i = 0;
  while (i < len)
  {
    // Skip spaces
    while (i < len && isspace(s[i]))
    {
      i++;
    }

    // Find bounds of word
    int word_start = i;
    while (i < len && !isspace(s[i]))
    {
      i++;
    }
    int word_end = i;

    int word_len = word_end - word_start;
    if (word_len > 0)
    {
      copy_string_to_word_buffer(&word_buffer, s + word_start, word_len);
      TrieData data;
      if (trie_retrieve(trie, word_buffer_ptr(&word_buffer), &data))
      {
        result = Qtrue;
        break;
      }
    }
  }

  free_word_buffer(&word_buffer);
  return result;
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

  StringValue(key);

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  TrieData data;

  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);

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
 * it assumes true for the value.
 *
 */
static VALUE
rb_trie_add(VALUE self, VALUE args)
{
  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  int size = RARRAY_LEN(args);
  if (size < 1 || size > 2)
  {
    return Qnil;
  }
  VALUE key;
  key = RARRAY_PTR(args)[0];
  StringValue(key);

  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);
  TrieData trie_data_value = TRIE_DATA_ERROR;

  if (size == 2)
  {
    VALUE weight = RARRAY_PTR(args)[1];
    Check_Type(weight, T_FIXNUM);
    trie_data_value = NUM2INT(weight);
  }

  Bool result = trie_store(trie, alpha_key, trie_data_value);

  alpha_char_free(alpha_key);
  return result ? Qtrue : Qnil;
}

/*
 * call-seq: add_if_absent(key) add_if_absent(key, value)
 *
 * Add a key, or a key and value to the Trie.  If you add a key without a value
 * it assumes true for the value.
 *
 */
static VALUE
rb_trie_add_if_absent(VALUE self, VALUE args)
{
  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  int size = RARRAY_LEN(args);
  if (size < 1 || size > 2)
  {
    return Qnil;
  }
  VALUE key;
  key = RARRAY_PTR(args)[0];
  StringValue(key);

  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);
  TrieData trie_data_value = TRIE_DATA_ERROR;

  if (size == 2)
  {
    VALUE weight = RARRAY_PTR(args)[1];
    Check_Type(weight, T_FIXNUM);
    trie_data_value = NUM2INT(weight);
  }

  Bool result = trie_store_if_absent(trie, alpha_key, trie_data_value);

  alpha_char_free(alpha_key);
  return result ? Qtrue : Qnil;
}

/*
 * call-seq: concat(keys)
 *
 * Adds an array of keys to the Trie. Each element of the array may be a string, or a sub-array
 * containing a key and a weight.
 *
 */
static VALUE
rb_trie_concat(VALUE self, VALUE keys)
{
  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  Check_Type(keys, T_ARRAY);

  int size = RARRAY_LEN(keys);
  for (int i=0; i<size; i++)
  {
    VALUE obj = RARRAY_PTR(keys)[i];

    if (TYPE(obj) == T_ARRAY)
    {
      VALUE key = RARRAY_PTR(obj)[0];
      VALUE weight = RARRAY_LEN(obj) > 1 ? RARRAY_PTR(obj)[1] : INT2FIX(-1);
      StringValue(key);
      Check_Type(weight, T_FIXNUM);
      AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);
      trie_store(trie, alpha_key, NUM2INT(weight));
      alpha_char_free(alpha_key);
    }
    else
    {
      StringValue(obj);
      AlphaChar *alpha_key = alpha_char_new_from_rb_value(obj);
      trie_store(trie, alpha_key, TRIE_DATA_ERROR);
      alpha_char_free(alpha_key);
    }
  }

  return Qnil;
}

/*
 * call-seq: add_text(text, delimiters)
 *
 * Scans for words in the given string, delimited by any of the characters
 * in the given delimiters string. All words are added to the trie.
 *
 */
static VALUE
rb_trie_add_text(VALUE self, VALUE text, VALUE delimiters)
{
  StringValue(text);
  StringValueCStr(delimiters); // StringValueCStr used so we can use strchr()

  if (!rb_enc_str_asciionly_p(text) || !rb_enc_str_asciionly_p(delimiters))
  {
    rb_raise(rb_eRuntimeError, "Error: rb_trie_add_text supports only ASCII strings\n");
  }

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  char *s = RSTRING_PTR(text);
  int len = RSTRING_LEN(text);
  char *delim = RSTRING_PTR(delimiters);

  WordBuffer word_buffer;
  init_word_buffer(&word_buffer);

  int i = 0;
  while (i < len)
  {
    // Skip any delimiters
    while (i < len && strchr(delim, s[i]) != NULL)
    {
      i++;
    }

    // Find bounds of word
    int word_start = i;
    while (i < len && strchr(delim, s[i]) == NULL)
    {
      i++;
    }
    int word_end = i;

    int word_len = word_end - word_start;
    if (word_len > 0)
    {
      copy_string_to_word_buffer(&word_buffer, s + word_start, word_len);
      trie_store(trie, word_buffer_ptr(&word_buffer), TRIE_DATA_ERROR);
    }
  }

  free_word_buffer(&word_buffer);
  return Qnil;
}

/*
 * call-seq: delete(key)
 *
 * Delete a key from the Trie.  Returns true if it deleted a key, nil otherwise.
 *
 */
static VALUE
rb_trie_delete(VALUE self, VALUE key)
{
  StringValue(key);

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

  AlphaChar *alpha_key = alpha_char_new_from_rb_value(key);
  Bool result = trie_delete(trie, alpha_key);

  alpha_char_free(alpha_key);

  return result ? Qtrue : Qnil;
}

static VALUE
alpha_char_to_rb_str(AlphaChar *alpha_char)
{
  // Convert AlphaChar* to a Ruby UTF-32 string
  VALUE utf32_value = rb_enc_str_new((const char*)alpha_char, sizeof(AlphaChar) * alpha_char_strlen(alpha_char), get_utf32_encoding());

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
  StringValue(prefix);
  prefix_alpha_char = alpha_char_new_from_rb_value(prefix);

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

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
  StringValue(prefix);
  prefix_alpha_char = alpha_char_new_from_rb_value(prefix);

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

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
  StringValue(prefix);
  prefix_alpha_char = alpha_char_new_from_rb_value(prefix);

  Trie *trie;
  Data_Get_Struct(self, Trie, trie);

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
 * Represents a single node in the Trie. It can be used as a cursor to walk
 * around the Trie. You can grab a TrieNode for the root of the Trie by using
 * Trie#root.
 *
 */

static VALUE
rb_trie_node_alloc(VALUE klass)
{
  return Data_Wrap_Struct(klass, 0, trie_state_free, NULL);
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
  Data_Get_Struct(self, TrieState, state);

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
  Data_Get_Struct(new_node, TrieState, state);

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
  Data_Get_Struct(self, TrieState, state);

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
  Data_Get_Struct(self, TrieState, state);

  return trie_state_is_terminal(state) ? Qtrue : Qnil;
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
  Data_Get_Struct(self, TrieState, state);

  return trie_state_is_leaf(state) ? Qtrue : Qnil;
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
  Data_Get_Struct(self, Trie, trie);

  int res = trie_save(trie, RSTRING_PTR(filename));
  return res == 0 ? Qtrue : Qnil;
}

void Init_trie()
{
  cTrie = rb_define_class("Trie", rb_cObject);

  rb_define_singleton_method(cTrie, "new", rb_trie_new, -1);
  rb_define_method(cTrie, "initialize", rb_trie_initialize, 1);
  rb_define_module_function(cTrie, "read", rb_trie_read, 1);
  rb_define_method(cTrie, "has_key?", rb_trie_has_key, 1);
  rb_define_method(cTrie, "text_has_keys?", rb_trie_text_has_keys, 1);
  rb_define_method(cTrie, "tags_has_keys?", rb_trie_tags_has_keys, 1);
  rb_define_method(cTrie, "add_text", rb_trie_add_text, 2);
  rb_define_method(cTrie, "get", rb_trie_get, 1);
  rb_define_method(cTrie, "add", rb_trie_add, -2);
  rb_define_method(cTrie, "add_if_absent", rb_trie_add_if_absent, -2);
  rb_define_method(cTrie, "concat", rb_trie_concat, 1);
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

  cAlphaMap = rb_define_class("AlphaMap", rb_cObject);
  rb_define_alloc_func(cAlphaMap, rb_alpha_map_alloc);
  rb_define_method(cAlphaMap, "add_range", rb_alpha_map_add_range, 2);
  rb_define_method(cAlphaMap, "ranges", rb_alpha_map_get_ranges, 0);

  default_alpha_map = rb_alpha_map_alloc(cAlphaMap);
  rb_gc_register_mark_object(default_alpha_map);
  rb_alpha_map_add_range(default_alpha_map, INT2FIX(0), INT2FIX(255));
}
