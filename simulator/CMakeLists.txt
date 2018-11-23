CMAKE_MINIMUM_REQUIRED(VERSION 3.9)
PROJECT(et2-simulator)

# Where is the Boost library?
if(NOT DEFINED BOOST_ROOT)
    MESSAGE(FATAL_ERROR "Please supply the boost root path via -DBOOST_ROOT=/path/to/boost")
ENDIF()

# Find C++ Boost library:
FIND_PACKAGE(Boost)
IF (NOT ${Boost_FOUND})
    MESSAGE(FATAL_ERROR "Can't find Boost library.")
ENDIF()
IF ((${Boost_MINOR_VERSION} LESS 66) AND (${Boost_MAJOR_VERSION} EQUAL 1))
    MESSAGE(WARNING "Found Boost version ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}. ET2 was tested with
    1.60. Continuing anyway.")
ENDIF()


INCLUDE_DIRECTORIES(${etanalyzer-cpp_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

EXECUTE_PROCESS( COMMAND sh ${CMAKE_SOURCE_DIR}/create-version-cpp.sh
                 OUTPUT_FILE ${CMAKE_SOURCE_DIR}/version.cpp )

SET(SOURCES_SIM simulator.cpp execution.cpp heap.cpp classinfo.cpp tokenizer.cpp
    analyze.cpp version.cpp)

# The file(GLOB...) allows for wildcard additions if needed:
# file(GLOB SOURCES "src/*.cpp")

# Set C++ 11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -Werror")

# TODO:
#   Add dependencies on header files. Otherwise, the simulator might not be
#   rebuilt when header files are modified? TODO
# TODO:

# Main static simulator
ADD_EXECUTABLE(simulator ${SOURCES_SIM})
# TARGET_COMPILE_DEFINITIONS(simulator
#                            PRIVATE _SIZE_DEBUG)
# Non-static simulator
# TODO: Add a build option to enable static building when needed
#       Need to change the SET to the TARGET_COMPILE_OPTIONS
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++11 -g -Werror -static")
# ADD_EXECUTABLE(simulator-type1 ${SOURCES_SIM})
# TARGET_COMPILE_OPTIONS(simulator-dyn
#                        PRIVATE static)

ADD_EXECUTABLE(simulator-type1 ${SOURCES_SIM})
TARGET_COMPILE_DEFINITIONS(simulator-type1 
                           PRIVATE ENABLE_TYPE1)

# DEBUG simulator executable
# ADD_EXECUTABLE(simulator-debug ${SOURCES_SIM})
# TARGET_COMPILE_DEFINITIONS(simulator-debug
#                            PRIVATE DEBUG_SPECJBB)
