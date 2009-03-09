#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libdatrie, 0.1.3, thep@linux.thai.net)
AC_CONFIG_SRCDIR([datrie/trie.h])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

# Library versioning
# Library code modified:                    REVISION++
# Interfaces added:              CURRENT++  REVISION=0  AGE++
# Interfaces changed/removed:    CURRENT++  REVISION=0  AGE=0
LT_CURRENT=0
LT_REVISION=3
LT_AGE=0
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h stdio.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

dnl Disable doc generation with doxygen option
AC_ARG_ENABLE(doxygen-doc,
              [AC_HELP_STRING([--disable-doxygen-doc],
                              [disable document generation with doxygen])],
              , enable_doxygen_doc="yes")

if test "x$enable_doxygen_doc" = "xyes"; then
  AC_CHECK_PROG(DOXYGEN,doxygen,doxygen,no)
  if test "x$DOXYGEN" = "xno"; then
    enable_doxygen_doc="no"
  fi
fi

dnl where to install the doxygen-generated HTML doc
AC_ARG_WITH(html-docdir,
            [AC_HELP_STRING([--with-html-docdir=DIR],
                            [where to install the doxyten-generated HTML doc [PREFIX/share/doc/datrie/html]])],
            [htmldocdir="$withval"], [htmldocdir=\$\{prefix\}/share/doc/datrie/html])
AC_SUBST(htmldocdir)

AM_CONDITIONAL(ENABLE_DOXYGEN_DOC,test "x$enable_doxygen_doc" = "xyes")

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile
                 datrie.pc
                 datrie/Makefile
                 tools/Makefile
                 man/Makefile
                 doc/Makefile
                 doc/Doxyfile])
AC_OUTPUT
