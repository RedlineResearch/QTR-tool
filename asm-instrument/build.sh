echo "Removing class files:"
rm -vf ClassAdapter.class Instrumenter.class MethodAdapter.class MyTransformer.class instrumenter.jar
echo "Compiling..."
javac -cp ASM/asm-7.1.jar    Instrumenter.java
echo "Creating jar..."
jar cvmf  MANIFEST.MF instrumenter.jar  ClassAdapter.class Instrumenter.class MethodAdapter.class MyTransformer.class
echo "DONE."
