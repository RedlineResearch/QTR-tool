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
`java -agentlib:et2 [Class]`, all other Java options (including `-jar`) work as usual.

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
   * `java -agentlib:et2 Test`

## Current Status
ET2/JVM runs correctly and without thrown exceptions on the Oracle and IBM J9 JDKs, but 
throws a `ClassCircularityError ` on OpenJDK. This exception is not yet known to cause 
any runtime errors, but it is probably a better idea to use Oracle JDK instead of OpenJDK 
(since both run the HotSpot JVM anyway).
