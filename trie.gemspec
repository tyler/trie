# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{trie}
  s.version = "0.3.5"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Tyler McMullen"]
  s.date = %q{2009-05-12}
  s.description = %q{Ruby Trie based on libdatrie.}
  s.email = %q{tyler@scribd.com}
  s.extensions = ["ext/trie/extconf.rb"]
  s.files = ["README.textile", "VERSION.yml", "lib/trie.rb", "spec/trie_spec.rb", "ext/trie", "ext/trie/darray.c", "ext/trie/darray.h", "ext/trie/extconf.rb", "ext/trie/fileutils.c", "ext/trie/fileutils.h", "ext/trie/Makefile", "ext/trie/tail.c", "ext/trie/tail.h", "ext/trie/trie-private.c", "ext/trie/trie-private.h", "ext/trie/trie.c", "ext/trie/trie.h", "ext/trie/triedefs.h", "ext/trie/typedefs.h"]
  s.has_rdoc = true
  s.homepage = %q{http://github.com/tyler/trie}
  s.rdoc_options = ["--title", "Trie", "--line-numbers", "--op", "rdoc", "--main", "ext/trie/trie.c", "README"]
  s.require_paths = ["ext", "lib"]
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
