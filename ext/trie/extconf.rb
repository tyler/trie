require 'mkmf'
dir_config 'trie'

unless find_library('datrie',nil, '/usr/local/lib', '/usr/lib')
  puts 'Need libdatrie.'
  exit
end

unless find_library('iconv',nil, '/usr/local/lib', '/usr/lib')
  puts 'Need iconv.'
  exit
end

create_makefile 'trie'

