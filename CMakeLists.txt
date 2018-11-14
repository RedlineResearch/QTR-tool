CMAKE_MINIMUM_REQUIRED(VERSION 3.9)
PROJECT(et2-java
    VERSION 1.0
    DESCRIPTION "Elephant Tracks The Sequel")

# Set C++ 11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check first if JAVA_HOME has been set through the command line.
#       Then only assign if not.
if(NOT DEFINED JAVA_HOME)
    # This is defined on my server. Not sure if this is a Linux convention.
    SET(JAVA_HOME /etc/alternatives/java_sdk_1.8.0/)
ENDIF()
FIND_PACKAGE(Java 1.8)
FIND_PACKAGE(JNI)

# Where is the libjnif?
if(NOT DEFINED LIBJNIF)
    MESSAGE(FATAL_ERROR "Please supply the libjnif path via -DLIBJNIF=/path/to/libjnif")
ENDIF()

# Bring the headers.
INCLUDE_DIRECTORIES(
    ${JNI_INCLUDE_DIRS}
    ${LIBJNIF}/src-libjnif
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

LINK_DIRECTORIES(${LIBJNIF}/build)

# Find the xxd binary:
FIND_PROGRAM(XXD xxd)
IF(NOT XXD)
    MESSAGE(FATAL_ERROR "xxd not found.")
ENDIF()

# ET2 library
ADD_CUSTOM_TARGET(
    InstrumentFlag_class.h
    COMMAND ${Java_JAVAC_EXECUTABLE} -d ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/InstrumentFlag.java
    COMMAND cd ${CMAKE_BINARY_DIR} && ${XXD} -i InstrumentFlag.class ${CMAKE_BINARY_DIR}/InstrumentFlag_class.h 
    DEPENDS ${CMAKE_SOURCE_DIR}/InstrumentFlag.java
    COMMENT "Generating InstrumentFlag_class.h"
)

ADD_CUSTOM_TARGET(
    ETProxy_class.h
    COMMAND ${Java_JAVAC_EXECUTABLE} -classpath ${CMAKE_SOURCE_DIR} -d ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/ETProxy.java
    COMMAND cd ${CMAKE_BINARY_DIR} && ${XXD} -i ETProxy.class ${CMAKE_BINARY_DIR}/ETProxy_class.h 
    DEPENDS ${CMAKE_SOURCE_DIR}/ETProxy.java
    COMMENT "Generating ETProxy_class.h"
)

ADD_LIBRARY(et2 SHARED
    main.cc
    Callbacks.cc
    InstrumentBytecode.cc
    ClassTable.cc
    ObjectSize.cc
)

TARGET_LINK_LIBRARIES(et2 jnif)

ADD_DEPENDENCIES(et2
    InstrumentFlag_class.h
    ETProxy_class.h
)

# objectsize library
ADD_LIBRARY(objectsize SHARED
    ObjectSize.cc
)

# C++ compile flags
SET(CUSTOM_CXX_FLAGS "-fPIC -O2")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CUSTOM_CXX_FLAGS}")
