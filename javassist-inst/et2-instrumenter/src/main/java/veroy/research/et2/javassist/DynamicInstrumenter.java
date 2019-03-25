package veroy.research.et2.javassist;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.lang.Exception;
import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.IllegalClassFormatException;
import java.lang.instrument.Instrumentation;
import java.lang.reflect.Method;
import java.security.ProtectionDomain;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;

import javassist.ByteArrayClassPath;
import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.Modifier;


public class DynamicInstrumenter {

    public static void premain(String args, Instrumentation inst) throws Exception {
        System.out.println("Loading Agent..");
        PrintWriter pwriter = new PrintWriter(new FileOutputStream( new File("methods.list") ), true);
        Et2Transformer optimus = new Et2Transformer(pwriter);
        inst.addTransformer(optimus);
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
        if (shouldIgnore(className)) {
            System.err.println(">>> END IGNORE: " + className);
            return klassFileBuffer;
        }
        // Javassist stuff:
        System.out.println(className + " is about to get loaded by the ClassLoader");
        ByteArrayInputStream istream = new ByteArrayInputStream(klassFileBuffer);
        InstrumentMethods instMeth = new InstrumentMethods(istream, className);
        CtClass klazz = instMeth.instrumentStart();
        System.err.println(">>> END transform.");
        try {
            byte[] barray = klazz.toBytecode();
            return barray;
        } catch (CannotCompileException | IOException exc) {
            System.err.println("Error converting class[ " + className + " ] into bytecode.");
            exc.printStackTrace();
            return klassFileBuffer;
        }
    }

    protected boolean shouldIgnore(String className) {
        return (className.indexOf("java/lang") == 0);
    }
}
