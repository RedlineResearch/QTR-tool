# Elephant Tracks 2

A rewritten version of the garbage collection tracing tool for Java programs, 
Elephant Tracks, developed at the RedLine Research Group @ Tufts University 
Department of Computer Science.

## Improvements
ET2 is written from scratch in order to improve the performance 
of Elephant Tracks and to support the HotSpot JVM. Instead of using the ASM 
library for instrumentation (and forking Java processes to ) and relying on the 
JNI to write instrumentation methods, ET2 uses the native instrumentation library 
[JNIF](http://sape.inf.usi.ch/jnif) and write all instrumentation methods in Java.

## Requirements
   * gcc 5.4.0
   * JNIF, available from [here](https://bitbucket.org/acuarica/jnif).
   * OpenJDK 8 with HotSpot. Not tested with other JVMs or other versions of OpenJDK.
   * Linux. Only tested with RHEL and Ubuntu; not tested with other distributions or 
     operating systems.

## Build
   * Set `JAVA_HOME` correctly
   * Run the configure script `./config`
   * `make`

## Authors
   * Xuanrui Qi ([xqi01@cs.tufts.edu](mailto:xqi01@cs.tufts.edu))
   * Samuel Guyer ([sguyer@cs.tufts.edu](mailto:sguyer@cs.tufts.edu))

## Affiliation
   RedLine Research Group, Department of Computer Science, Tufts University.
