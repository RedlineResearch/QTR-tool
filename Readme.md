# Elephant Tracks 2 (JVM Frontend)

A rewritten version of the garbage collection tracing tool for Java programs, 
Elephant Tracks, developed by the RedLine Research Group @ Tufts University 
Department of Computer Science.

## Improvements
ET2 is written from scratch in order to improve the performance of Elephant Tracks 
and for compatibility with the HotSpot JVM. Instead of using the ASM library for 
instrumentation (and forking Java processes to do so) and relying on the JNI to 
write instrumentation methods, ET2 uses the native instrumentation library 
[JNIF](http://sape.inf.usi.ch/jnif) and write all instrumentation methods in Java.

Moreover, instead of creating and tracing object graphs at runtime as the current 
implementation of Elephant Tracks does, ET2 generates data that allows the object 
graphs to be generated post-mortem.

The short-term performance goal is a 1/3 performance boost compared to the current 
implementation of Elephant Tracks. In the long term, we aspire to increase the 
performance of Elephant Tracks by 7-10 times.

## Using ET2
`java -agentlib:et2 [Class]`, all other Java options (including `-jar`) 
*should* work as usual.

## Required JNIF Modification
One must modify the JNIF file `src-libjnif/parser.cpp` and go into the `AttrsParser::parse` 
method to prevent the parsing of LineNumberTable, LocalVariableTable and LocalVariableTypeTable, or 
use our fork of JNIF.

## Requirements
   * gcc
   * JNIF, available from [here](https://github.com/ElephantTracksProject/jnif).
   * Oracle JDK 8 or IBM J9 JDK.
   * Linux. Only tested with RHEL and Ubuntu; not tested with other distributions or 
     operating systems.
   * cmake 3.9 or greater

## Building with cmake
   * `cmake` is many things, but most importantly it's a Makefile generator.
   * Steps to building with `cmake`:
       * Create a `BUILD` directory anywhere.
           * `mkdir /path/to/BUILD`
       * Run `cmake` on `CMakeLists.txt`
           * `cmake /path/to/et2-java/ 
                  -DLIBJNIF=/path/to/jnif/src-libjnif 
                  -DJAVA_HOME=/path/to/java 
                  -DBOOST_ROOT=/path/to/boost_1.66`
       * This generates a `Makefile` in `/path/to/BUILD`
           * `cd /path/to/BUILD && make`

## Running tests
   * The tests are run using [pytest](https://docs.pytest.org/en/latest/).
   * Use the following command:
       * `python3.4  -m pytest et2_tests.py --java_path /etc/alternatives/java_sdk_1.8.0/bin/java --agent_path /data/rveroy/pulsrc/et2-java/BUILD  --rootdir .`
   * Modify `--java_path` and `--agent_path` to fit your setup.

## Building with the Makefile in source (DEPRECATED)
   * Ensure `JAVA_HOME` is set correctly
   * Set the variables in `Makefile.inc`
   * `make`
   * Note: this is deprecated. Please see how to build with cmake.

## Testing
   * `make test`
   * `java -agentlib:et2 Hello`
   * `java -agentlib:et2 BinarySearchTree`

## Current Status
ET2/JVM runs correctly (sometimes) and without thrown exceptions on Oracle Java 8 if bytecode verification is disabled, 
but it seems to sometimes fail bytecode verification at the moment. Currently we are not exactly sure what is 
wrong with it.

## Licensing
See the "LICENSE" file.
