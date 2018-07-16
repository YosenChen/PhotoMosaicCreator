CXX ?= g++

# the following line can compile successfully with a main function in MontageImage.cpp
#	g++ -std=c++11 -o MontageImage MontageImage.cpp `pkg-config opencv --cflags --libs`
#	for example: int main(void) { return 0; }

#CXXFLAGS += -c -std=c++11 -O3 -I ../../Downloads/boost/boost_1_62_0 -Wall $(shell pkg-config --cflags opencv)

CXXFLAGS += -std=c++11
OBJ_FILES := $(patsubst ./%.cpp, ./%.o, $(wildcard ./*.cpp))

all: mosaic_creator

./%.o: ./%.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

mosaic_creator: $(OBJ_FILES)
	g++ -o $@ $^ `pkg-config opencv --cflags --libs`

clean:
	find . -name \*.o -type f -delete
	rm mosaic_creator

# LDFLAGS += $(shell pkg-config --libs --static opencv)
# opencv_knn_svr_dm: opencv_knn_svr_dm.o; $(CXX) $< -o $@ $(LDFLAGS)
# demo: demo.o; $(CXX) $< -o $@ $(LDFLAGS)
# ./%.o: ./%.cpp
# 	g++ -c -o $@ $<
# clean: ; rm -f opencv_knn_svr_dm.o opencv_knn_svr_dm demo.o demo
