system('cd ext/libdatrie && ./configure && make')

require 'mkmf'

unless find_library('datrie',nil,'ext/libdatrie')
  puts 'Need libdatrie.'
  exit
end

create_makefile 'trie'

