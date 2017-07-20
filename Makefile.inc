# Use javac at JAVA_HOME and g++ by default
# ET2 is only tested with OpenJDK 8 and g++
# but clang++ should work in principle
JAVAC = $(JAVA_HOME)/bin/javac
CXX  = g++

# Necessary Java libraries
INC_PATH  = -I$(JAVA_HOME)/include

JNIF_INC_PATH = /home/xuanrui/jnif/src
JNIF_LIB_PATH = /home/xuanrui/jnif/build
