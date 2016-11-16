CXX = g++
CXXFLAGS = -std=c++11 -Wall

all: res_shell

res_shell: working.o
	$(CXX) working.o -o res_shell

working.o: working.cpp
	$(CXX) $(CXXFLAGS) -c working.cpp

clean:
	rm -rf *.o res_shell
