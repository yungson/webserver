src=$(wildcard ./src/*/*.cpp ./src/*.cpp)
header=$(wildcard ./src/*/*.h )
CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g

objs=$(patsubst %.cpp, %.o, $(src))

target=server

$(target):$(objs) $(header)
	$(CXX) $(objs) -o $(target) -pthread -lmysqlclient

%.o:%.cpp
	$(CXX) $(CFLAGS) -c $< -o $@ -pthread -lmysqlclient

.PHONY:clean
clean:
	rm $(objs) -f
