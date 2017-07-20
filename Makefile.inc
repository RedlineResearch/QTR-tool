# If you want to use a different JAVA_HOME only for this
# build, e.g. a custom OpenJDK you built, then fill in
# the following line. Otherwise comment it.
JAVA_HOME = /home/xuanrui/jdk


# Use javac at JAVA_HOME and g++ by default
# ET2 is only tested with OpenJDK 8 and g++
# but clang++ should work in principle
JAVAC = $(JAVA_HOME)/bin/javac
CXX   = g++

# Necessary Java libraries
INC_PATH  = -I$(JAVA_HOME)/include

JNIF_INC_PATH = /home/xuanrui/jnif/src
JNIF_LIB_PATH = /home/xuanrui/jnif/build
