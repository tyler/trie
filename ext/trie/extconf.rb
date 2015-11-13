require 'mkmf'
require 'fileutils'

datrie_dir = File.join(File.dirname(__FILE__), '..', 'libdatrie-0.2.9')
datrie_build_dir = File.join(datrie_dir, 'build')

Dir.chdir(datrie_dir) do
  system "./configure", "--enable-shared", "--prefix=#{datrie_build_dir}"
  system "make clean"
  system "make"
  system "make install"
end

datrie_include_dir = File.join(datrie_build_dir, "include")
datrie_lib_dir = File.join(datrie_build_dir, "lib")

$CFLAGS << " -I#{datrie_include_dir}"
$LIBS << " #{datrie_lib_dir}/libdatrie.a"

create_makefile 'trie'

