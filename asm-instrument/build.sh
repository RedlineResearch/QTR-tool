javac -cp ASM/asm-7.1.jar    Instrumenter.java
jar cvmf  MANIFEST.MF instrumenter.jar  ClassAdapter.class Instrumenter.class MethodAdapter.class MyTransfomer.class
