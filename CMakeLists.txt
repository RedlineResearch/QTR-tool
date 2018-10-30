CMAKE_MINIMUM_REQUIRED(VERSION 3.9)
PROJECT(et2-java
    VERSION 1.0
    DESCRIPTION "Elephant Tracks The Sequel")

# Set C++ 11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# TODO: Check first if JAVA_HOME has been set through the command line.
#       Then only assign if not.
SET(JAVA_HOME /etc/alternatives/java_sdk_1.8.0/)
FIND_PACKAGE(Java 1.8)
# TODO: MESSAGE(STATUS ${Java_FOUND})
FIND_PACKAGE(JNI)
# TODO: sample code
FOREACH(DIR ${JNI_INCLUDE_DIRS})
    MESSAGE(STATUS ${DIR})
ENDFOREACH()
# Bring the headers.
# INCLUDE_DIRECTORIES(${JNI_INCLUDE_DIRS} "/data/rveroy/pulsrc/jnif-fork/src-libjnif)
INCLUDE_DIRECTORIES(
    ${JNI_INCLUDE_DIRS}
    /data/rveroy/pulsrc/jnif-fork/src-libjnif
)

# ENABLE_TESTING()
# FIND_PACKAGE( GTest Required )

# INCLUDE_DIRECTORIES($(et2-java_SOURCE_DIR))
# 
ADD_LIBRARY(et2 STATIC
    main.cc
    Callbacks.cc
    InstrumentBytecode.cc
    ClassTable.cc
    ObjectSize.cc
)
# 
# ADD_LIBRARY(objectsize STATIC
#     ObjectSize.cc
# )
# 
# SET_TARGET_PROPERTIES(et2 PROPERTIES VERSION ${PROJECT_VERSION})
# SET_TARGET_PROPERTIES(et2 PROPERTIES SOVERSION 1)
# 
# SET_TARGET_PROPERTIES(et2 PROPERTIES PUBLIC_HEADER include/mylib.h)
# 
