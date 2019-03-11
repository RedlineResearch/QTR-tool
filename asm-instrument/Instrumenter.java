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
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;

public class Instrumenter {

    public static void premain(String args, Instrumentation inst) throws Exception {
        System.out.println("Loading Agent..");
        inst.addTransformer(new MyTransformer());
        URL[] urls = new URL[] { new File("./ETProxy.java").toURI().toURL() };
        URLClassLoader classLoader = new URLClassLoader( urls,
                                                         Instrumenter.class.getClassLoader() );
        // TODO: Class classToLoad = Class.forName("com.MyClass", true, child);
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

    Instrumentation inst;

    @Override
    public byte[] transform( ClassLoader loader,
                             String className,
                             Class<?> klass,
                             ProtectionDomain domain,
                             byte[] klassFileBuffer ) throws IllegalClassFormatException {
        // ASM stuff:
        System.out.println(className + " is about to get loaded by the ClassLoader");
        byte[] barray;
        ClassWriter cwriter = new ClassWriter(ClassWriter.COMPUTE_FRAMES);
        ClassReader creader = new ClassReader(klassFileBuffer);
        ClassVisitor cvisitor = new ClassAdapter(cwriter);
        creader.accept(cvisitor, 0);
        barray = cwriter.toByteArray();

        System.err.println(">>> END transforom.");
        return barray;
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
