include Makefile.inc

INC_PATH += -I$(JNIF_INC_PATH)
LD_PATH  += -L$(JNIF_LIB_PATH)

LIBS = -ljnif

MODE = production
CXXFLAGS = -std=c++11 -fPIC

ifeq ($(MODE), debug)
	CXXFLAGS += -g -O0 -DDEBUG
else
	CXXFLAGS += -O2
endif

et2: libet2.so libobjectsize.so

libet2.so: main.o Callbacks.o InstrumentBytecode.o ClassTable.o ObjectSize.o
	$(CXX) $(LD_PATH) -shared $^ $(LIBS) -o $@

main.o: main.cc
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c $^
Callbacks.o: Callbacks.cc ETProxy_class.h InstrumentFlag_class.h
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c $^
InstrumentBytecode.o: InstrumentBytecode.cc
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c $^
ClassTable.o: ClassTable.cc
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c $^
ObjectSize.o: ObjectSize.cc
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c $^


ETProxy.class: ETProxy.java
	$(JAVAC) $^
InstrumentFlag.class: InstrumentFlag.java
	$(JAVAC) $^
ETProxy_class.h: ETProxy.class
	xxd -i $^ $@
InstrumentFlag_class.h: InstrumentFlag.class
	xxd -i $^ $@

libobjectsize.so: ObjectSize.cc ObjectSize.h
	$(CXX) $(INC_PATH) $(CXXFLAGS) -shared $^ -o $@

test: Hello.class BinarySearchTree.class FunctionalCounter.class

Hello.class: tests/Hello.java
	$(JAVAC) -g -d . $^
BinarySearchTree.class: tests/BinarySearchTree.java
	$(JAVAC) -g -d . $^
FunctionalCounter.class: tests/FunctionalCounter.scala
	$(SCALAC) -g:source -d . $^

clean:
	rm -f libet2.so *o *class *class.h *gch *~ *log
