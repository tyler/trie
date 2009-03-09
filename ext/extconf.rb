require 'mkmf'

unless find_library('datrie',nil,'/usr/local/lib','/usr/lib')
  puts 'Need libdatrie.'
  exit
end

create_makefile 'trie'

