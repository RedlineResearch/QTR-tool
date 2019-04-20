# Elephant Tracks 3

A rewritten version of the garbage collection tracing tool for Java programs,
Elephant Tracks, developed by the RedLine Research Group @ Tufts University
Department of Computer Science.  ET3 is written from scratch in order to
improve the performance of Elephant Tracks and for compatibility with the
HotSpot JVM.

### How we got here
* [Elephant Tracks 1](http://www.cs.tufts.edu/research/redline/elephantTracks/) is a JVMTI
  agent written in C++. There were compatibility issues as ET1 could only run with Java 1.6.
  This limited the number of applications it could run with.
* Elephant Tracks 2 - An early prototype for ET2 was created by the native instrumentation library
  [JNIF](http://sape.inf.usi.ch/jnif) by [Ray Qi](https://www.xuanruiqi.com/) was up and running
  but we couldn't successfully run the [DaCapo benchmarks](http://dacapobench.sourceforge.net/)
  with it.
* Elephant Tracks 3 (the latest version) uses Javassist which uses the [ASM library](https://asm.ow2.io/)
  underneath.

## Improvements
Instead of using the [JNIF] library for instrumentation through the JVMTI interface,
ET3 uses the [Javassist library](http://www.javassist.org/) through the [Java agent instrumentation](https://docs.oracle.com/javase/8/docs/api/java/lang/instrument/package-summary.html). Javassist is itself based on on the [ASM bytecode rewriting library](https://asm.ow2.io/).
The instrumentation methods in ET2 that were written in Java are reused for ET3 with some modifications.

The important idea behind ET2 and ET3 is that instead of creating and tracing object graphs at runtime
(as ET1 does), ET3 generates data that allows the object graphs to be generated offline after the
program ends.

## Using ET3

## Requirements
* [Java 8](https://openjdk.java.net/install/)
* [Javassist](http://www.javassist.org/) 3.25.0-GA
* For the trace analysis programs:
   * gcc/g++ or clang++ (TODO: Which versions did we use?)
   * [cmake 3.9 or greater](https://cmake.org/download/)

### Build using Maven
* `cd javassist-inst/et2-instrumenter`
* `mvn clean compile package`
* Look for the jar file `instrumenter-1.0-SNAPSHOT-jar-with-dependencies.jar` and copy to working directory.

### Running your jar with the ET3 Java agent
* `java -javaagent:./instrumenter-1.0-SNAPSHOT-jar-with-dependencies.jar  -jar YouProgram.jar`
* To run a selected DaCapo benchmark:
    * `java -javaagent:./instrumenter-1.0-SNAPSHOT-jar-with-dependencies.jar  -jar dacapo-9.12-bach.jar --no-validation -t 8 avrora`
        * Earlier prototypes of ET3 had issues when running with largish values for
          thread number (`-t`). Currently the issues seem to have disappeared but
          are not guaranteed to be bug-free. Please let us know if you run into any
          problems running with large number of threads.
        * The `--no-validation` is currently required as DaCapo complains loudly if left out. This is because we do bytecode rewriting which causes DaCapo to declare our run as invalid.

### Current Status

* ET3 can run the DaCapo benchmarks if run with the `--no-validation` option.

## Licensing
See the "LICENSE" file.
