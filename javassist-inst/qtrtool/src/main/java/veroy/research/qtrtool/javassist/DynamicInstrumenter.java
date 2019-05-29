package veroy.research.qtrtool.javassist;

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

import veroy.research.qtrtool.javassist.ETProxy;

public class DynamicInstrumenter {

    public static void premain(String args, Instrumentation inst) throws Exception {
        System.out.println("Loading Agent..");
        final PrintWriter methodsWriter = new PrintWriter(new FileOutputStream( new File("methods.list") ), true);
        final PrintWriter fieldsWriter = new PrintWriter(new FileOutputStream( new File("fields.list") ), true);
        final PrintWriter classWriter = new PrintWriter(new FileOutputStream( new File("class.list") ), true);
        final PrintWriter witnessWriter = new PrintWriter(new FileOutputStream( new File("witness.list") ), true);
        MethodInstrumenter.setPrintWriters( methodsWriter,
                                            fieldsWriter,
                                            classWriter );
        PrintWriter traceWriter = new PrintWriter(new FileOutputStream( new File("trace") ), true);
        ETProxy.traceWriter = traceWriter;
        ETProxy.inst = inst;
        ClassPool.doPruning = true;
        ClassPool.releaseUnmodifiedClassFile = true;
        ClassPool cp = ClassPool.getDefault();
        cp.get("veroy.research.qtrtool.javassist.ETProxy");
        final QtrToolTransformer optimus = new QtrToolTransformer(traceWriter);
        inst.addTransformer(optimus);
        Runtime.getRuntime()
               .addShutdownHook( new Thread() { 
                                     public void run() {
                                         System.err.println("SHUTDOWN running.");
                                         inst.removeTransformer(optimus);
                                         ETProxy.inInstrumentMethod.set(true);
                                         MethodInstrumenter.writeMapsToFile(witnessWriter);
                                     }
               });
    }

    private static void writeMethodList() {
        // for (Entry.)
    }
}

class QtrToolTransformer implements ClassFileTransformer {

    private final PrintWriter traceWriter;

    public QtrToolTransformer(PrintWriter traceWriter) {
        this.traceWriter = traceWriter;
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
        if (shouldIgnore(className)) {
            return klassFileBuffer;
        }
        // Javassist stuff:
        // System.err.println(className + " is about to get loaded by the ClassLoader");
        ByteArrayInputStream istream = new ByteArrayInputStream(klassFileBuffer);
        MethodInstrumenter instMeth = new MethodInstrumenter(istream, className);
        CtClass klazz = null;
        try {
            klazz = instMeth.instrumentMethods(loader);
        } catch (CannotCompileException exc) {
            return klassFileBuffer;
        }
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
        return ( (className.indexOf("ETProxy") >= 0) ||
                 (className.indexOf("java/lang") == 0) );
        // TODO: (className.indexOf("ClassLoader") >= 0) ||
    }
}
