echo "Removing class files:"
rm -vf ClassAdapter.class Instrumenter.class Et2Transformer.class ETProxy.class instrumenter.jar
echo "Compiling..."
javac ETProxy.java
javac -cp asm-7.1.jar:asm-commons-7.1.jar  Instrumenter.java
echo "Creating jar..."
jar cvmf  MANIFEST.MF instrumenter.jar  ClassAdapter.class Instrumenter.class ETProxy.class Et2Transformer.class ETProxy.class
echo "DONE."
