set -x

mv TestImageFileManager.cpp_ TestImageFileManager.cpp
g++ TestImageFileManager.cpp -o TestImageFileManager -std=c++11 -O3
mv TestImageFileManager.cpp TestImageFileManager.cpp_


