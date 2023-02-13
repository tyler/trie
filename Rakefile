# encoding: utf-8

require 'bundler'
require 'rubygems'

begin
  Bundler.setup(:default, :development)
rescue Bundler::BundlerError => e
  $stderr.puts e.message
  $stderr.puts "Run `bundle install` to install missing gems"
  exit e.status_code
end
require 'rake'

require 'jeweler'

jeweler_tasks = Jeweler::Tasks.new do |gem|
  gem.name = "fast_trie"
  gem.email = "tyler@scribd.com"
  gem.licenses = ["MIT", "LGPL-2.1"]
  gem.homepage = "http://github.com/tyler/trie"
  gem.description = "Ruby Trie abstract data type based on libdatrie."
  gem.summary = "Ruby Trie"
  gem.authors = ["Tyler McMullen", "Matt Hickford"]
  gem.extensions = ['ext/trie/extconf.rb']
  gem.require_paths = ['ext']
  gem.files = FileList["[A-Z]*.*", "{spec,ext}/**/*"]
  gem.files.exclude '*.gem'
  gem.files.exclude '*.bundle'
  gem.rdoc_options = ['--title', 'Trie', '--line-numbers', '--op', 'rdoc', '--main', 'ext/trie/trie_ext.c', 'README']
end
Jeweler::RubygemsDotOrgTasks.new

$gemspec         = jeweler_tasks.gemspec
$gemspec.version = jeweler_tasks.jeweler.version

require 'rake/extensiontask'
Rake::ExtensionTask.new('trie', $gemspec)
CLEAN.include 'lib/**/*.so'

require 'rspec/core/rake_task'
RSpec::Core::RakeTask.new

require 'rdoc/task'
Rake::RDocTask.new do |rdoc|
  rdoc.rdoc_dir = 'rdoc'
  rdoc.title    = 'Trie'
  rdoc.options << '--line-numbers' << '--inline-source'
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('ext/trie/trie_ext.c')
end

task :default => [:compile, :spec]
