set -x

mv dirent_simple_ls.cpp_ dirent_simple_ls.cpp
g++ dirent_simple_ls.cpp -o dirent_simple_ls -std=c++11 -O3
mv dirent_simple_ls.cpp dirent_simple_ls.cpp_


