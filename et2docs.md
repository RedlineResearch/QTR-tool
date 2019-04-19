# Elephant Tracks 2

## Current Status
ET2/JVM runs correctly (sometimes) and without thrown exceptions on Oracle Java 8 if bytecode verification is disabled,
but it seems to sometimes fail bytecode verification at the moment. Currently we are not exactly sure what is
wrong with it.

## Requirements
   * JNIF, available from [here](https://github.com/ElephantTracksProject/jnif).
   * Java 8
   * For the trace analysis programs:
       * gcc/g++ or clang++ (TODO: Which versions did we use?)
       * cmake 3.9 or greater

## Required JNIF Modification
One must modify the JNIF file `src-libjnif/parser.cpp` and go into the `AttrsParser::parse`
method to prevent the parsing of LineNumberTable, LocalVariableTable and LocalVariableTypeTable, or
use our fork of JNIF.

## Building the *Elephant Tracks 2 agent* with `cmake`
   * [cmake](https://cmake.org/) is many things, but most importantly it's a Makefile generator.
   * Steps to building with `cmake`:
       * Create a `BUILD` directory anywhere.
           * `mkdir /path/to/et2-agent/BUILD`
       * Run `cmake` on `CMakeLists.txt`
           * `cmake /path/to/et2-java/
                  -DLIBJNIF=/path/to/jnif/src-libjnif
                  -DJAVA_HOME=/path/to/java
                  -DBOOST_ROOT=/path/to/boost_1.66`
       * This generates a `Makefile` in `/path/to/BUILD`
           * `cd /path/to/BUILD && make`

## Building the *Elephant Tracks 2 simulator* with `cmake`
   * Steps to building with `cmake`:
       * Create a `BUILD` directory anywhere.
           * `mkdir /path/to/et2-simulator/BUILD`
       * Run `cmake` on `CMakeLists.txt`
           * `cmake /path/to/et2-simulator
       * This generates a `Makefile` in `/path/to/et2-simulator/BUILD`
           * `cd /path/to/et2-simulator/BUILD && make`

## Running tests
   * The tests are run using [pytest](https://docs.pytest.org/en/latest/).
       * See the [et2_tests.py](https://github.com/ElephantTracksProject/et2-java/blob/master/et2_tests.py) for details.
   * Use the following command:
       * `python3.4  -m pytest et2_tests.py --java_path /etc/alternatives/java_sdk_1.8.0/bin/java --agent_path /data/rveroy/pulsrc/et2-java/BUILD  --rootdir .`
       * Add `-v` to the end of the command to get verbose testing.
   * Modify `--java_path` and `--agent_path` to fit your setup.
   * The `--rootdir` option specifies where the pytest cache is placed.

## Building with the Makefile in source (DEPRECATED)
   * Ensure `JAVA_HOME` is set correctly
   * Set the variables in `Makefile.inc`
   * `make`
   * Note: this is deprecated. Please see how to build with cmake.

## Testing (the old way)
   * `make test`
   * `java -agentlib:et2 Hello`
   * `java -agentlib:et2 BinarySearchTree`


