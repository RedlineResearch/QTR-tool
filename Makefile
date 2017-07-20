include Makefile.inc

INC_PATH += -I$(JNIF_INC_PATH)
LD_PATH  += -L$(JNIF_LIB_PATH)

LIBS = -ljnif
CXXFLAGS = -std=c++11 -g -O0 -fPIC

et2: libet2.so ETProxy.class

libet2.so: main.o Callbacks.o InstrumentBytecode.o
	$(CXX) $(LD_PATH) -shared $^ $(LIBS) -o $@

main.o: main.cc
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c $^
Callbacks.o: Callbacks.cc
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c $^
InstrumentBytecode.o: InstrumentBytecode.cc
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c $^

ETProxy.class: ETProxy.java
	$(JAVAC) $^

clean:
	rm -f libet2.so *o *class *~
