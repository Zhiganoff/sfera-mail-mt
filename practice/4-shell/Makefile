CXX = g++
CXXFLAGS = -std=c++11 -Wall

all: res_shell

res_shell: my_shell.o
	$(CXX) my_shell.o -o res_shell

my_shell.o: my_shell.cpp
	$(CXX) $(CXXFLAGS) -c my_shell.cpp

clean:
	rm -rf *.o res_shell
