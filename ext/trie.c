#include "ruby.h"

static VALUE trie_initialize(VALUE self) {
    return self;
}
 
VALUE cTrie;

void Init_trie() {
    cTrie = rb_define_class("Trie", rb_cObject);
    rb_define_method(cTrie, "initialize", trie_initialize, 0);
}
