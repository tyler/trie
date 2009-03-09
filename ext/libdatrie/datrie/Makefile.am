pkgincludedir = $(includedir)/datrie

pkginclude_HEADERS =	\
	typedefs.h	\
	triedefs.h	\
	trie.h		\
	sb-trie.h

EXTRA_DIST = libdatrie.def

INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES = libdatrie.la

libdatrie_la_SOURCES =	\
	typedefs.h	\
	triedefs.h	\
	trie-private.h	\
	fileutils.h	\
	fileutils.c	\
	darray.h	\
	darray.c	\
	tail.h		\
	tail.c		\
	trie.h		\
	trie.c		\
	alpha-map.h	\
	alpha-map.c	\
	sb-trie.h	\
	sb-trie.c
libdatrie_la_LDFLAGS =	-no-undefined \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-export-symbols $(srcdir)/libdatrie.def
libdatrie_la_DEPENDENCIES = libdatrie.def

