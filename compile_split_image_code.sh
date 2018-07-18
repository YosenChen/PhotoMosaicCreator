set -x

mv SplitImage.cpp_ SplitImage.cpp
g++ SplitImage.cpp -o split_image -std=c++11 -O3 `pkg-config opencv --cflags --libs`
mv SplitImage.cpp SplitImage.cpp_

