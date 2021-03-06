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
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;
import java.util.concurrent.ConcurrentHashMap;

import javassist.ByteArrayClassPath;
import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.Modifier;

import veroy.research.qtrtool.javassist.QtrToolTransformer;
import veroy.research.qtrtool.javassist.QTRProxy;

public class DynamicInstrumenter {

    final static ConcurrentHashMap<String, Boolean> doneClasses = new ConcurrentHashMap<>();

    public static void premain(final String args, final Instrumentation inst) throws Exception {
        System.out.println("Loading Agent..");
        assert (inst.isRetransformClassesSupported());
        final PrintWriter methodsWriter = new PrintWriter(new FileOutputStream(new File("methods.list")), true);
        final PrintWriter fieldsWriter = new PrintWriter(new FileOutputStream(new File("fields.list")), true);
        final PrintWriter classWriter = new PrintWriter(new FileOutputStream(new File("class.list")), true);
        final PrintWriter witnessWriter = new PrintWriter(new FileOutputStream(new File("witness.list")), true);
        MethodInstrumenter.setPrintWriters(methodsWriter, fieldsWriter, classWriter);
        final PrintWriter traceWriter = new PrintWriter(new FileOutputStream(new File("trace")), true);
        QTRProxy.traceWriter = traceWriter;
        QTRProxy.inst = inst;
        ClassPool.doPruning = false;
        ClassPool.releaseUnmodifiedClassFile = false;
        final ClassPool cp = ClassPool.getDefault();
        cp.get("veroy.research.qtrtool.javassist.QTRProxy");
        final QtrToolTransformer optimus = new QtrToolTransformer(traceWriter);
        inst.addTransformer(optimus);
        inst.setNativeMethodPrefix(optimus, MethodInstrumenter.nativePrefix);
        Runtime.getRuntime().addShutdownHook(new Thread() {
            public void run() {
                // DEBUG: System.err.println("SHUTDOWN running.");
                inst.removeTransformer(optimus);
                QTRProxy.inInstrumentMethod.set(true);
                QTRProxy.flushBuffer();
                MethodInstrumenter.writeMapsToFile(witnessWriter);
            }
        });
        // Get all loaded classes:
        final Class[] classes = inst.getAllLoadedClasses();
        final List<Class> candidates = new ArrayList<>();
        int i = 0;
        for (final Class klass : classes) {
            final String klassName = klass.getName();
            if (inst.isModifiableClass(klass) && !DynamicInstrumenter.shouldIgnore(klassName)) {
                // DEBUG: System.err.println("     Adding " + klassName + ": " + inst.isModifiableClass(klass));
                if (!doneClasses.contains(klassName)) {
                    candidates.add(klass);
                }
            }
            // DEBUG: else {
            // DEBUG:     System.err.println("<<<-Unmodifiable: " + klassName + ".");
            // DEBUG: }
        }
        if (candidates.size() > 0) {
            final Class[] candidatesArr = new Class[candidates.size()];
            for (i = 0; i < candidates.size(); i++) {
                candidatesArr[i] = candidates.get(i);
            }
            inst.retransformClasses(candidatesArr);
        }
    }

    private static void writeMethodList() {
        // for (Entry.)
    }

    protected static boolean shouldIgnore(final String className) {
        return ( (className == null) ||
                 (className.indexOf("QTRProxy") >= 0) ||
                 (className.indexOf("javassist") == 0) ||
                 (className.indexOf("veroy.research.qtrtool.javassist") == 0) ||
                 (className.indexOf("java.lang") == 0) );
                 /*
                 (className.indexOf("java.lang.invoke.LambdaForm") == 0) ||
                 (className.indexOf("jdk.internal") == 0) ||
                 (className.indexOf("sun.reflect") == 0) ||
                 (className.indexOf("java.lang.J9VMInternals") == 0) ||
                 (className.indexOf("com.ibm.oti") == 0) ||
                 (className.indexOf("java.util.Locale") == 0) ||
                 (className.indexOf("sun.security.jca") == 0)
                 );
                 */
    }
}
