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

jeweler_tasks = Jeweler::Tasks.new do |s|
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
  rdoc.rdoc_files.include('ext/trie/trie.c')
end

task :default => :spec
