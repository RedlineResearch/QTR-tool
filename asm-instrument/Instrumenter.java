import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.lang.instrument.ClassDefinition;
import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.IllegalClassFormatException;
import java.lang.instrument.Instrumentation;
import java.security.ProtectionDomain;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.lang.Exception;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;
import java.util.function.IntSupplier;

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
        this.add(loader, className);
        if (shouldIgnore(className)) {
            System.out.println(">>> " + className + " will not be instrumented.");
            return null;
        }
        // ASM stuff:
        System.out.println(className + " is about to get loaded by the ClassLoader");
        byte[] barray;
        ClassWriter cwriter = new ClassWriter(ClassWriter.COMPUTE_FRAMES);
        ClassReader creader;
        try {
            creader = new ClassReader(new ByteArrayInputStream(klassFileBuffer));
        } catch (Exception exc) {
            throw new IllegalClassFormatException(exc.getMessage());
        }
        ClassVisitor cvisitor = new ClassAdapter(cwriter, this.pwriter); 
        creader.accept(cvisitor, 0);
        barray = cwriter.toByteArray();

        System.err.println(">>> END transform.");
        return barray;
    }


    private void add(Class<?> clazz) {
        add(clazz.getClassLoader(), clazz.getName());
    }

    private void add(ClassLoader loader, String className) {
        synchronized (classMap) {
            System.out.println(">>> add: loaded " + className);
            Set<String> set = classMap.get(loader);
            if (set == null) {
                set = new HashSet<String>();
                classMap.put(loader, set);
            }
            set.add(className);
        }
    }

    private boolean isLoaded(String className, ClassLoader loader) {
        synchronized (classMap) {
            Set<String> set = classMap.get(loader);
            if (set == null) {
                return false;
            }
            return set.contains(className);
        }
    }

    public boolean isClassLoaded(String className, ClassLoader loader) {
        if ((loader == null) || (className == null)) {
            throw new IllegalArgumentException();
        }
        while (loader != null) {
            if (this.isLoaded(className, loader)) {
                return true;
            }
            loader = loader.getParent();
        }
        return false;
    }


    public boolean shouldIgnore(String className) {
        return ( className.equals("ETProxy") ||
                 (className.indexOf("java/lang") == 0) || // core java.lang.* classes
                 (className.indexOf("java/io") == 0) || // java.io.* classes
                 (className.indexOf("sun/") == 0) || // sun.* classes
                 (className.indexOf("$") >= 0) // inner classes
                 );
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
        System.err.println("ClassAdapter::visitMethod : " + this.className + "#" + name + " - " + signature + " => " + ClassAdapter.nextMethodId);
        
        this.pwriter.println(ClassAdapter.nextMethodId + "," + this.className + "#" + name);
        MethodVisitor mv = cv.visitMethod(access, name, desc, signature, exceptions);
        // TODO:
        // 1. Save the method and assign a method id for it.
        // 2. Increment the method id
        this.addToMap(name, desc);
        if (mv == null) {
            System.err.println("----- visitMethod returns NULL!");
            return null;
        } else {
            return new MethodAdapter(mv);
        }
    }

    protected void addToMap(String methodName, String desc) {
        ClassAdapter.methodMap.put(nextMethodId, className + "#" + methodName + "#" + desc);
        ClassAdapter.nextMethodId++;
    }
}

class MethodAdapter extends MethodVisitor implements Opcodes {

    public MethodAdapter(final MethodVisitor mv) {
        super(ASM7, mv);
    }

    @Override
    public void visitMethodInsn(int opcode, String owner, String name, String desc, boolean itf) {
        System.err.println("MethodAdapter::visitMethodInsn -> " + name + " - " + desc);
        // System.err.println("CALL" + name);
        mv.visitFieldInsn(GETSTATIC, "java/lang/System", "err", "Ljava/io/PrintStream;");
        mv.visitLdcInsn("CALL " + name);
        mv.visitMethodInsn(INVOKEVIRTUAL, "java/io/PrintStream", "println", "(Ljava/lang/String;)V", false);

        // do call
        mv.visitMethodInsn(opcode, owner, name, desc, itf);

        // System.err.println("RETURN" + name);
        mv.visitFieldInsn(Opcodes.GETSTATIC, "java/lang/System", "err", "Ljava/io/PrintStream;");
        mv.visitLdcInsn("RETURN " + name);
        mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL, "java/io/PrintStream", "println", "(Ljava/lang/String;)V", false);
    }
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
