echo "Removing class files:"
rm -vf ClassAdapter.class Instrumenter.class MethodAdapter.class Et2Transformer.class instrumenter.jar
echo "Compiling..."
javac -cp ASM/asm-7.1.jar:ASM/asm-commons-7.1.jar  Instrumenter.java
echo "Creating jar..."
jar cvmf  MANIFEST.MF instrumenter.jar  ClassAdapter.class Instrumenter.class ETProxy.class MethodAdapter.class Et2Transformer.class
echo "DONE."
