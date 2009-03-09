system('cd ext/libdatrie && ./configure && make')

require 'mkmf'
dir_config 'trie'

unless find_library('datrie',nil,'ext/libdatrie')
  puts 'Need libdatrie.'
  exit
end

create_makefile 'trie'

