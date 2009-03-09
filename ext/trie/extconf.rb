path_to_libdatrie = "#{File.dirname(__FILE__)}/../libdatrie"
system("cd #{path_to_libdatrie} && ./configure && make")

require 'mkmf'
dir_config 'trie'

unless find_library('datrie',nil, path_to_libdatrie)
  puts 'Need libdatrie.'
  exit
end

create_makefile 'trie'

