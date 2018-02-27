# If you want to use a different JAVA_HOME only for this
# build, e.g. a custom OpenJDK you built, then fill in
# the following line. Otherwise comment it.
# JAVA_HOME = /home/xuanrui/jdk


# Use javac at JAVA_HOME and g++ by default
# ET2 is only tested with OpenJDK 8 and g++
# but clang++ should work in principle
# Set NOSCALA to true if you don't have Scala
JAVAC  = javac
CXX    = g++
JAVAH  = javah
ifneq ($(NOSCALA), true)
	SCALAC = scalac
endif

# Necessary Java libraries
INC_PATH  = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux

JNIF_INC_PATH = /home/xuanrui/jnif/src-libjnif
JNIF_LIB_PATH = /home/xuanrui/jnif/build
