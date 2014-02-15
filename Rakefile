# encoding: utf-8

require 'rubygems'
require 'bundler'
begin
  Bundler.setup(:default, :development)
rescue Bundler::BundlerError => e
  $stderr.puts e.message
  $stderr.puts "Run `bundle install` to install missing gems"
  exit e.status_code
end
require 'rake'

require 'jeweler'

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
Jeweler::RubygemsDotOrgTasks.new

require 'rake/extensiontask'

Rake::ExtensionTask.new('trie', $gemspec) do |ext|
end

CLEAN.include 'lib/**/*.so'

require 'rake/testtask'
Rake::TestTask.new(:test) do |test|
  test.libs = ['ext', 'spec']
  test.pattern = 'spec/**/*_spec.rb'
  test.verbose = true
end

require 'rdoc/task'
Rake::RDocTask.new do |rdoc|
  rdoc.rdoc_dir = 'rdoc'
  rdoc.title    = 'Trie'
  rdoc.options << '--line-numbers' << '--inline-source'
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('ext/trie/trie.c')
end

task :default => :test
