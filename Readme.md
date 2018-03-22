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
   * JNIF, available from [here](https://gitlab.com/acuarica/jnif).
   * Oracle JDK 8 or IBM J9 JDK.
   * Linux. Only tested with RHEL and Ubuntu; not tested with other distributions or 
     operating systems.

## Building
   * Ensure `JAVA_HOME` is set correctly
   * Set the variables in `Makefile.inc`
   * `make`

## Testing
   * `make test`
   * `java -agentlib:et2 Hello`
   * `java -agentlib:et2 BinarySearchTree`

## Current Status
ET2/JVM runs correctly (sometimes) and without thrown exceptions on Oracle Java 8 if bytecode verification is disabled, 
but it seems to sometimes fail bytecode verification at the moment. Currently we are not exactly sure what is 
wrong with it.
