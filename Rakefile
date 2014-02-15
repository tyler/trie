require 'rake'
require 'rdoc/task'

begin
  require 'jeweler' # gem install technicalpickles-jeweler
  Jeweler::Tasks.new do |s|
    s.name = "fast_trie"
    s.email = "tyler@scribd.com"
    s.homepage = "http://github.com/tyler/trie"
    s.description = "Ruby Trie based on libdatrie."
    s.summary = s.description
    s.authors = ["Tyler McMullen"]
    s.extensions = ['ext/trie/extconf.rb']
    s.require_paths = ['ext']
    s.files = FileList["[A-Z]*.*", "{spec,ext}/**/*"]
    s.has_rdoc = true
    s.rdoc_options = ['--title', 'Trie', '--line-numbers', '--op', 'rdoc', '--main', 'ext/trie/trie.c', 'README']
  end
end

begin
  require 'spec/rake/spectask'
  Spec::Rake::SpecTask.new do |t|
    t.spec_files = 'spec/**/*_spec.rb'
  end
rescue LoadError
end

Rake::RDocTask.new do |rdoc|
  rdoc.rdoc_dir = 'rdoc'
  rdoc.title    = 'Trie'
  rdoc.options << '--line-numbers' << '--inline-source'
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('ext/trie/trie.c')
end

task :clean do
  sh 'rm -fv ext/*.{o,bundle} ext/trie/*.{o,bundle} ext/Makefile ext/trie/Makefile'
end

task :default => :spec
