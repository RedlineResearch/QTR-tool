package veroy.research.qtrtool.javassist;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.IllegalClassFormatException;
import java.security.ProtectionDomain;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;

import javassist.CannotCompileException;
import javassist.CtClass;

class QtrToolTransformer implements ClassFileTransformer {

    private final PrintWriter traceWriter;

    public QtrToolTransformer(final PrintWriter traceWriter) {
        this.traceWriter = traceWriter;
    }

    // Map to prevent duplicate loading of classes:
    private final Map<ClassLoader, Set<String>> classMap = new WeakHashMap<ClassLoader, Set<String>>();
    // Elephant Tracks 2 metadata:

    @Override
    public byte[] transform(final ClassLoader loader, final String className, final Class<?> klass,
            final ProtectionDomain domain, final byte[] klassFileBuffer) throws IllegalClassFormatException {
        if (this.shouldIgnore(className)) {
            return klassFileBuffer;
        }
        // Javassist stuff:
        // DEBUG: System.err.println(className + " is about to get loaded by the ClassLoader");
        final ByteArrayInputStream istream = new ByteArrayInputStream(klassFileBuffer);
        final MethodInstrumenter instMeth = new MethodInstrumenter(istream, className);
        CtClass klazz = null;
        try {
            // DEBUG: System.err.println(" -- Instrumenting: " + className);
            klazz = instMeth.instrumentMethods(loader);
        } catch (final CannotCompileException exc) {
            return klassFileBuffer;
        }
        try {
            final byte[] barray = klazz.toBytecode();
            return barray;
        } catch (CannotCompileException | IOException exc) {
            System.err.println("Error converting class[ " + className + " ] into bytecode.");
            exc.printStackTrace();
            return klassFileBuffer;
        }
    }

    // TODO: The classes are the same but the representation sometimes uses '.' or a '/'
    protected boolean shouldIgnore(final String className) {
        return ( (className == null) ||
                 (className.indexOf("QTRProxy") >= 0) ||
                 (className.indexOf("javassist") == 0) ||
                 (className.indexOf("veroy/research/qtrtool/javassist") == 0) ); // ||
                 /*
                 (className.indexOf("java/lang/invoke/LambdaForm") == 0) ||
                 (className.indexOf("jdk/internal") == 0) ||
                 (className.indexOf("sun/reflect") == 0) ||
                 (className.indexOf("java/lang/J9VMInternals") == 0) ||
                 (className.indexOf("com/ibm/oti") == 0) ||
                 (className.indexOf("java/util/Locale") == 0) ||
                 (className.indexOf("sun/security/jca") == 0)
                 );
                 */
    }
}
