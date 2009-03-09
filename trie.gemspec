# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{trie}
  s.version = "0.1.1"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Tyler McMullen"]
  s.date = %q{2009-03-08}
  s.description = %q{TODO}
  s.email = %q{tyler@scribd.com}
  s.extensions = ["ext/trie/extconf.rb"]
  s.files = ["VERSION.yml", "lib/trie.rb", "spec/test-trie", "spec/test-trie/README", "spec/trie_spec.rb", "ext/libdatrie", "ext/libdatrie/aclocal.m4", "ext/libdatrie/AUTHORS", "ext/libdatrie/ChangeLog", "ext/libdatrie/config.guess", "ext/libdatrie/config.h.in", "ext/libdatrie/config.sub", "ext/libdatrie/configure", "ext/libdatrie/configure.ac", "ext/libdatrie/COPYING", "ext/libdatrie/datrie", "ext/libdatrie/datrie/alpha-map.c", "ext/libdatrie/datrie/alpha-map.h", "ext/libdatrie/datrie/alpha-map.o", "ext/libdatrie/datrie/darray.c", "ext/libdatrie/datrie/darray.h", "ext/libdatrie/datrie/darray.o", "ext/libdatrie/datrie/fileutils.c", "ext/libdatrie/datrie/fileutils.h", "ext/libdatrie/datrie/fileutils.o", "ext/libdatrie/datrie/libdatrie.def", "ext/libdatrie/datrie/Makefile", "ext/libdatrie/datrie/Makefile.am", "ext/libdatrie/datrie/Makefile.in", "ext/libdatrie/datrie/sb-trie.c", "ext/libdatrie/datrie/sb-trie.h", "ext/libdatrie/datrie/sb-trie.o", "ext/libdatrie/datrie/tail.c", "ext/libdatrie/datrie/tail.h", "ext/libdatrie/datrie/tail.o", "ext/libdatrie/datrie/trie-private.h", "ext/libdatrie/datrie/trie.c", "ext/libdatrie/datrie/trie.h", "ext/libdatrie/datrie/trie.o", "ext/libdatrie/datrie/triedefs.h", "ext/libdatrie/datrie/typedefs.h", "ext/libdatrie/datrie.pc.in", "ext/libdatrie/depcomp", "ext/libdatrie/doc", "ext/libdatrie/doc/Doxyfile.in", "ext/libdatrie/doc/Makefile", "ext/libdatrie/doc/Makefile.am", "ext/libdatrie/doc/Makefile.in", "ext/libdatrie/INSTALL", "ext/libdatrie/install-sh", "ext/libdatrie/ltmain.sh", "ext/libdatrie/Makefile", "ext/libdatrie/Makefile.am", "ext/libdatrie/Makefile.in", "ext/libdatrie/man", "ext/libdatrie/man/Makefile", "ext/libdatrie/man/Makefile.am", "ext/libdatrie/man/Makefile.in", "ext/libdatrie/man/trietool.1", "ext/libdatrie/missing", "ext/libdatrie/NEWS", "ext/libdatrie/README", "ext/libdatrie/tools", "ext/libdatrie/tools/Makefile", "ext/libdatrie/tools/Makefile.am", "ext/libdatrie/tools/Makefile.in", "ext/libdatrie/tools/trietool.c", "ext/libdatrie/tools/trietool.o", "ext/trie", "ext/trie/extconf.rb", "ext/trie/trie.c"]
  s.homepage = %q{http://github.com/tyler/trie}
  s.require_paths = ["lib", "ext"]
  s.rubygems_version = %q{1.3.1}
  s.summary = %q{TODO}

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 2

    if Gem::Version.new(Gem::RubyGemsVersion) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
