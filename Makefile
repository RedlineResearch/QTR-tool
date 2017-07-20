CXX  = g++
INC  = -I/home/xuanrui/jdk/include -I/home/xuanrui/jnif/src
LD   = -L/home/xuanrui/jnif/build
LIBS = -ljnif
CXXFLAGS = -std=c++11 -g -O0 -fPIC

JAVAC = /home/xuanrui/jdk/bin/javac

et2: libet2.so ETProxy.class

libet2.so: main.o Callbacks.o InstrumentBytecode.o
	$(CXX) $(LD) -shared $^ $(LIBS) -o $@

main.o: main.cc
	$(CXX) $(INC) $(CXXFLAGS) -c $^
Callbacks.o: Callbacks.cc
	$(CXX) $(INC) $(CXXFLAGS) -c $^
InstrumentBytecode.o: InstrumentBytecode.cc
	$(CXX) $(INC) $(CXXFLAGS) -c $^

ETProxy.class: ETProxy.java
	$(JAVAC) $^

clean:
	rm -f libet2.so *o *class *~
