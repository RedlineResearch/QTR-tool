CXX = g++
INC = -I/home/xuanrui/jdk/include
CXXFLAGS = -g -O0 -fPIC

et2: libet2.so

libet2.so: main.o Callbacks.o
	g++ -shared -o $@ $^

main.o: main.cc
	$(CXX) $(INC) $(CXXFLAGS) -c  $^
Callbacks.o: Callbacks.cc
	$(CXX) $(INC) $(CXXFLAGS) -c $^

clean:
	rm -f libet2.so *o *~
