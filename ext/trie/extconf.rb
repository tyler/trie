require 'mkmf'

$srcs = [
  "trie_ext.c",
  "libdatrie/datrie/alpha-map.c",
  "libdatrie/datrie/darray.c",
  "libdatrie/datrie/dstring.c",
  "libdatrie/datrie/fileutils.c",
  "libdatrie/datrie/tail.c",
  "libdatrie/dastrie/trie.c",
  "libdatrie/dastrie/trie-string.c"
]

$INCFLAGS << " -I$(srcdir)/libdatrie -I$(srcdir)/libdatrie/datrie"
$VPATH << "$(srcdir)/libdatrie/datrie"

create_makefile 'trie/trie'
