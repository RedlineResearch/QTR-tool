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

import java.io.File;
import java.lang.Exception;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;

public class Instrumenter {

    public static void premain(String args, Instrumentation inst) throws Exception {
        System.out.println("Loading Agent..");
        inst.addTransformer(new MyTransformer());
        //  URL[] urls = new URL[] { new File("./ETProxy.class").toURI().toURL() };
        //  URLClassLoader classLoader = new URLClassLoader( urls,
        //                                                   Instrumenter.class.getClassLoader() );
        //  Class proxyClass = Class.forName("ETProxy", true, classLoader);
        // TODO: Method method = classToLoad.getDeclaredMethod("myMethod");
        // TODO: Object instance = classToLoad.newInstance();
        // TODO: Object result = method.invoke(instance);
        /*
        ClassDefinition classDef = new ClassDefinition(klass, barray);
        try {
            inst.redefineClasses(classDef);
            System.err.println("DEBUG: " + className + " done.");
        } catch (Exception exc) {
            System.err.println("Class not found? => " + exc.getMessage());
        }
        */
    }
}

class MyTransformer implements ClassFileTransformer {

    private final Map<ClassLoader, Set<String>> classMap = new WeakHashMap<ClassLoader, Set<String>>();

    @Override
    public byte[] transform( ClassLoader loader,
                             String className,
                             Class<?> klass,
                             ProtectionDomain domain,
                             byte[] klassFileBuffer ) throws IllegalClassFormatException {
        // Ignore the ET2 Proxy class:
        add(loader, className);
        if (shouldIgnore(className)) {
            System.out.println(">>> " + className + " will not be instrumented.");
            return klassFileBuffer;
        }
        /*
        try {
            // Getting the 'findLoadedClass method to call later:
            Method findMethod = ClassLoader.class.getDeclaredMethod("findLoadedClass", new Class[] { String.class });
            findMethod.setAccessible(true);
            ClassLoader cloader = ClassLoader.getSystemClassLoader();
            Object testObj = findMethod.invoke(cloader, className);
            if (testObj != null) {
                return null;
            }
        } catch (Exception exc) {
            throw new IllegalClassFormatException(exc.getMessage());
        }
        */
        if (this.isLoaded(className, klass.getClassLoader())) {
            return klassFileBuffer;
        }
        // ASM stuff:
        System.out.println(className + " is about to get loaded by the ClassLoader");
        byte[] barray;
        ClassWriter cwriter = new ClassWriter(ClassWriter.COMPUTE_FRAMES);
        ClassReader creader = new ClassReader(klassFileBuffer);
        ClassVisitor cvisitor = new ClassAdapter(cwriter);
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
                 (className.indexOf("sun/") == 0) // sun.* classes
                 // (className.indexOf("$") >= 0) // inner classes
                 );
    }
}

class ClassAdapter extends ClassVisitor implements Opcodes {

    public ClassAdapter(final ClassVisitor cv) {
        super(ASM7, cv);
    }

    @Override
    public MethodVisitor visitMethod( final int access,
                                      final String name,
                                      final String desc,
                                      final String signature,
                                      final String[] exceptions ) {
        System.err.println("MethodVisitor::visitMethod -> " + name + " - " + signature);
        MethodVisitor mv = cv.visitMethod(access, name, desc, signature, exceptions);
        // TODO: return ((mv == null) ? null
        // TODO:                      : new MethodAdapter(mv));
        if (mv == null) {
            System.err.println("----- visitMethod returns NULL!");
            return null;
        } else {
            return new MethodAdapter(mv);
        }
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
