import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.PrintWriter;
import java.lang.Exception;
import java.lang.instrument.ClassDefinition;
import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.IllegalClassFormatException;
import java.lang.instrument.Instrumentation;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.URL;
import java.net.URLClassLoader;
import java.security.ProtectionDomain;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;
import java.util.function.IntSupplier;

import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.commons.AnalyzerAdapter;


public class Instrumenter {

    public static void premain(String args, Instrumentation inst) throws Exception {
        System.out.println("Loading Agent..");
        PrintWriter pwriter = new PrintWriter(new FileOutputStream( new File("methods.list") ), true);
        Et2Transformer optimus = new Et2Transformer(pwriter);
        inst.addTransformer(optimus);
        // TODO: URL[] urls = new URL[] { new File("./ETProxy.class").toURI().toURL() };
        // TODO: URLClassLoader classLoader = new URLClassLoader( urls,
        // TODO:                                                  Instrumenter.class.getClassLoader() );
        // TODO: Class proxyClass = Class.forName("ETProxy", true, classLoader);
        // TODO: Method method = classToLoad.getDeclaredMethod("myMethod");
        // TODO: Object instance = classToLoad.newInstance();
        // TODO: Object result = method.invoke(instance);
    }

    private static void writeMethodList() {
        // for (Entry.)
    }
}

class Et2Transformer implements ClassFileTransformer {

    private final PrintWriter pwriter;

    public Et2Transformer(PrintWriter pwriter) {
        this.pwriter = pwriter;
    }

    // Map to prevent duplicate loading of classes:
    private final Map<ClassLoader, Set<String>> classMap = new WeakHashMap<ClassLoader, Set<String>>();
    // Elephant Tracks 2 metadata:

    @Override
    public byte[] transform( ClassLoader loader,
                             String className,
                             Class<?> klass,
                             ProtectionDomain domain,
                             byte[] klassFileBuffer ) throws IllegalClassFormatException {
        // Ignore the ET2 Proxy class:
        System.err.println("Et2Transformer::transform -> " + className);
        // ASM stuff:
        System.out.println(className + " is about to get loaded by the ClassLoader");
        byte[] barray;
        ClassWriter cwriter = new ClassWriter(ClassWriter.COMPUTE_FRAMES | ClassWriter.COMPUTE_MAXS);
        ClassReader creader;
        try {
            creader = new ClassReader(new ByteArrayInputStream(klassFileBuffer));
        } catch (Exception exc) {
            throw new IllegalClassFormatException(exc.getMessage());
        }
        // AnalyzerAdapter adapter = new AnalyzerAdapter(className, access, String name, String descriptor, MethodVisitor methodVisitor)
        ClassVisitor cvisitor = new ClassAdapter(cwriter, this.pwriter);
        // TODO: AnalyzerAdapter canalyzer = new AnalyzerAdapter();
        // synchronized (this) {
        // }
        // synchronized(creader) {
            creader.accept(cvisitor, ClassReader.EXPAND_FRAMES);
            // creader.accept(cwriter, 0);
            barray = cwriter.toByteArray();
        // }
        System.err.println(">>> END transform.");
        return barray;
    }

}

class ClassAdapter extends ClassVisitor implements Opcodes {

    public static Integer nextMethodId = 1;
    public final static Map<Integer, String> methodMap = new HashMap<>();
    protected String className;
    protected PrintWriter pwriter;

    public ClassAdapter(ClassVisitor cv, PrintWriter pwriter) {
        super(ASM7, cv);
        this.className = "__NONE__";
        this.pwriter = pwriter;
    }

    @Override
    public void visit( int version, int access,
                       String name, String signature,
                       String superName, String[] interfaces) {
        System.err.println("ClassAdapter::visit -> " + name + " - " + superName + " - ");
        this.className = name;
        this.cv.visit(version, access, name, signature, superName, interfaces);
    }

    @Override
    public MethodVisitor visitMethod( final int access,
                                      final String name,
                                      final String desc,
                                      final String signature,
                                      final String[] exceptions ) {
        // System.err.println("ClassAdapter::visitMethod : " + this.className + "#" + name + " - " + signature + " => " + ClassAdapter.nextMethodId);

        this.pwriter.println(ClassAdapter.nextMethodId + "," + this.className + "#" + name);
        MethodVisitor mv = cv.visitMethod(access, name, desc, signature, exceptions);
        // TODO:
        // 1. Save the method and assign a method id for it.
        // 2. Increment the method id
        this.addToMap(name);
        // if ((access & Modifier.NATIVE) != 0) {
        //     System.err.println(">>> Method " + name + " is native, so IGNORING.");
        //     return null;
        // } else if (mv == null) {
        if (mv == null) {
            System.err.println("----- visitMethod returns NULL!");
            return null;
        } else {
            return new MethodAdapter(mv);
            // TODO: return new MethodAdapter( this.className, access, name, desc, mv );
        }
    }

    protected void addToMap(String methodName) {
        ClassAdapter.methodMap.put(nextMethodId, className + "#" + methodName);
        ClassAdapter.nextMethodId++;
    }
}

class MethodAdapter extends MethodVisitor implements Opcodes {
// TODO: class MethodAdapter extends AnalyzerAdapter implements Opcodes {

    private int maxStack = 0;

    public MethodAdapter(final MethodVisitor mv) {
    // TODO: public MethodAdapter(String owner, int access, String name, String descriptor, MethodVisitor mv) {
        super(ASM7, mv);
        // TODO: super(owner, access, name, descriptor, mv);
    }

    @Override
    public void visitMethodInsn(int opcode, String owner, String name, String desc, boolean itf) {
        // TODO: Change this to do tracing.
        System.err.println("MethodAdapter::visitMethodInsn -> " + owner + "#" + name + " - " + desc);
        mv.visitFieldInsn(GETSTATIC, "java/lang/System", "err", "Ljava/io/PrintStream;");
        mv.visitLdcInsn(">>> CALL " + name);
        mv.visitMethodInsn(INVOKEVIRTUAL, "java/io/PrintStream", "println", "(Ljava/lang/String;)V", false);

        // do call
        mv.visitMethodInsn(opcode, owner, name, desc, itf);

        // System.err.println("RETURN" + name);
        mv.visitFieldInsn(Opcodes.GETSTATIC, "java/lang/System", "err", "Ljava/io/PrintStream;");
        mv.visitLdcInsn(">>> RETURN " + name);
        mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL, "java/io/PrintStream", "println", "(Ljava/lang/String;)V", false);

        this.maxStack = 4;
    }

    @Override
    public void visitMaxs(int maxStack, int maxLocals) {
        super.visitMaxs(5, 0);
    }

    // TODO: @Override
    // TODO: public void visitEnd() {
    // TODO: }
}

class MyInteger {
    private int value;
    public MyInteger(int value) {
        this.value = value;
    }

    public void increment() {
        this.value++;
    }

    public int getThenIncrement() {
        return this.value;
    }

    public void setValue(int value) {
        this.value = value;
    }
}
