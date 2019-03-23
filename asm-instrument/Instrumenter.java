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
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.commons.AdviceAdapter;


public class Instrumenter {

    public static void premain(String args, Instrumentation inst) throws Exception {
        // System.out.println("Loading Agent..");
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
    // TODO: static boolean etProxyLoaded = false;

    public Et2Transformer(PrintWriter pwriter) {
        this.pwriter = pwriter;
    }

    // Map to prevent duplicate loading of classes:
    private final Map<ClassLoader, Set<String>> classMap = new WeakHashMap<ClassLoader, Set<String>>();
    // Elephant Tracks 2 metadata:

    @Override
    public synchronized byte[] transform( ClassLoader loader,
                                          String className,
                                          Class<?> klass,
                                          ProtectionDomain domain,
                                          byte[] klassFileBuffer ) throws IllegalClassFormatException {
        System.err.println("Et2Transformer::transform -> " + className);
        // ASM stuff:
        System.out.println(className + " is about to get loaded by the ClassLoader");
        byte[] barray;
        ClassWriter cwriter = new ClassWriter(ClassWriter.COMPUTE_FRAMES);
        System.err.println("              ::transform - step A");
        ClassReader creader = new ClassReader(klassFileBuffer);
        // TODO: try {
        // TODO:     creader = new ClassReader(new ByteArrayInputStream(klassFileBuffer));
        // TODO: } catch (Exception exc) {
        // TODO:     throw new IllegalClassFormatException(exc.getMessage());
        // TODO: }
        System.err.println("              ::transform - step B");
        ClassVisitor cvisitor = new ClassAdapter(cwriter, this.pwriter);
        System.err.println("              ::transform - step C");
        creader.accept(cvisitor, 0);
        // creader.accept(cwriter, 0);
        System.err.println("              ::transform - step D");
        // creader.accept(cwriter, 0);
        barray = cwriter.toByteArray();
        System.err.println(">>> END transform.");
        // return barray;
        return klassFileBuffer;
    }

    private static byte[] generateETProxyClass() {
        byte[] barray = null;
        ClassWriter cwriter = new ClassWriter(ClassWriter.COMPUTE_FRAMES);
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
        System.err.println("ClassAdapter::<INIT>");
    }

    @Override
    public void visit( int version, int access,
                       String name, String signature,
                       String superName, String[] interfaces) {
        System.err.println("ClassAdapter::visit -> " + name + " - " + superName + " - ");
        this.className = name;
    }

    @Override
    public MethodVisitor visitMethod( final int access,
                                      final String name,
                                      final String desc,
                                      final String signature,
                                      final String[] exceptions ) {
        System.err.println("ClassAdapter::visitMethod : " + this.className + "#" + name + " - " + signature + " => " + ClassAdapter.nextMethodId);
        MethodVisitor mv = cv.visitMethod(access, name, desc, signature, exceptions);

        this.pwriter.println(ClassAdapter.nextMethodId + "," + this.className + "#" + name);
        // 1. Save the method and assign a method id for it.
        // 2. Increment the method id
        // TODO: this.addToMap(name);
        // AdviceAdapter methodVisitor = new ET2MethodAdapter(access, name, desc, mv);
        if (mv == null) {
            System.err.println("----- visitMethod returns NULL!");
            return null;
        } 
        else {
             System.err.println("***** visitMethod create AdviceAdapter");
             // return new ET2MethodAdapter(access, name, desc, mv);
             /*
             MethodVisitor et2Mv = new AdviceAdapter(ASM7, mv, access, name, desc) {
                 @Override
                 protected void onMethodEnter() {
                     // System.err.println("ET2MethodAdapter::onMethodEnter");
                     // TODO: Label label0 = new Label();
                     // TODO: mv.visitLabel(label0);
                     // TODO: mv.visitFieldInsn(GETSTATIC, "java/lang/System", "err", "Ljava/io/PrintStream;");
                     // TODO: mv.visitLdcInsn(">>> CALL " + pubName);
                     // TODO: mv.visitMethodInsn(INVOKEVIRTUAL, "java/io/PrintStream", "println", "(Ljava/lang/String;)V", false);

                     // TODO: mv.visitIntInsn(BIPUSH, 123);
                     // TODO: mv.visitVarInsn(ALOAD, 0);
                     // TODO: mv.visitMethodInsn(INVOKESTATIC, "ETProxy", "onEntry", "(ILjava/lang/Object;)V", false);

                     // System.err.println("RETURN" + name);
                     // TODO mv.visitFieldInsn(Opcodes.GETSTATIC, "java/lang/System", "err", "Ljava/io/PrintStream;");
                     // TODO mv.visitLdcInsn(">>> RETURN " + name);
                     // TODO mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL, "java/io/PrintStream", "println", "(Ljava/lang/String;)V", false);
                 }

                 @Override
                 protected void onMethodExit(int opcode) {
                 }
             };
             */
        }
        return null;
    }

    /*
    protected void addToMap(String methodName) {
        ClassAdapter.methodMap.put(nextMethodId, className + "#" + methodName);
        ClassAdapter.nextMethodId++;
    }
    */
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
