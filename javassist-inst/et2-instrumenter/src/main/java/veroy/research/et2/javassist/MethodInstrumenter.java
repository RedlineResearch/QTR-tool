package veroy.research.et2.javassist;

import java.io.InputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import javassist.ByteArrayClassPath;
import javassist.CannotCompileException;
import javassist.NotFoundException;
import javassist.ClassPool;
import javassist.CtBehavior;
import javassist.CtClass;
import javassist.CtConstructor;
import javassist.CtMethod;
import javassist.LoaderClassPath;
import javassist.Modifier;
import javassist.expr.ExprEditor;
import javassist.expr.FieldAccess;
import javassist.expr.NewExpr;

public class MethodInstrumenter {

    private static AtomicInteger nextAllocSiteId = new AtomicInteger(1);
    private static AtomicInteger nextClassId = new AtomicInteger(1);
    private static AtomicInteger nextFieldId = new AtomicInteger(1);
    private static AtomicInteger nextMethodId = new AtomicInteger(1);
    private static ConcurrentHashMap<String, Integer> methodIdMap = new ConcurrentHashMap();
    private static ConcurrentHashMap<String, Integer> classIdMap = new ConcurrentHashMap();
    private static ConcurrentHashMap<String, Integer> fieldIdMap = new ConcurrentHashMap();
    private static ConcurrentHashMap<String, Integer> allocSiteIdMap = new ConcurrentHashMap();

    private static AtomicBoolean mainInstrumentedFlag = new AtomicBoolean(false);
    private InputStream instream;
    private String newName;
    private PrintWriter mwriter;

    public MethodInstrumenter(InputStream instream, String newName, PrintWriter methodsWriter) {
        this.instream = instream;
        this.newName = newName;
        this.mwriter = methodsWriter;

        ClassPool cp = ClassPool.getDefault();
        // TODO: cp.importPackage("org.apache.log4j");
    }

    protected int getAllocSiteId(String className, Integer byteCodeIndex) {
        String key = className + "#" + byteCodeIndex.toString();
        allocSiteIdMap.computeIfAbsent(key, k -> nextAllocSiteId.getAndAdd(1));
        Integer result = allocSiteIdMap.get(key);
        return ((result != null) ? (Integer) result : -1);
    }

    protected int getClassId(String className) {
        classIdMap.computeIfAbsent(className, k -> nextClassId.getAndAdd(1));
        Integer result = classIdMap.get(className);
        return ((result != null) ? (Integer) result : -1);
    }

    protected int getMethodId(String className, String methodName) {
        String key = className + "#" + methodName;
        methodIdMap.computeIfAbsent(key, k -> nextMethodId.getAndAdd(1));
        Integer result = methodIdMap.get(key);
        return ((result != null) ? (Integer) result : -1);
    }

    protected int getFieldId(String className, String fieldName) {
        String key = className + "#" + fieldName;
        fieldIdMap.computeIfAbsent(key, k -> nextFieldId.getAndAdd(1));
        Integer result = fieldIdMap.get(key);
        return ((result != null) ? (Integer) result : -1);
    }

    public CtClass instrumentMethods(ClassLoader loader) throws CannotCompileException {
        ClassPool cp = ClassPool.getDefault();
        cp.insertClassPath(new LoaderClassPath(loader));
        CtClass ctKlazz = null;
        try {
            ctKlazz = cp.makeClass(instream);
        } catch (IOException exc) {
            System.err.println("IO error getting class/method:");
            exc.printStackTrace();
            System.exit(1);
        }

        final String className = ctKlazz.getName();

        // Methods:
        // CtMethod[] methods = ctKlazz.getMethods();
        CtBehavior[] methods = ctKlazz.getDeclaredBehaviors();
        for (int ind = 0 ; ind < methods.length; ind++) {
            final CtBehavior method = methods[ind];
            final String methodName = method.getName();
            final int modifiers = method.getModifiers();
            final int methodId = getMethodId(className, methodName);
            final int classId = getClassId(className);

            if (shouldIgnore(modifiers, methodName)) {
                continue;
            }
            if (method instanceof CtMethod) {
                method.instrument(
                        new ExprEditor() {
                            // Instrument new expressions:
                            public void edit(NewExpr expr) throws CannotCompileException {
                                final int allocSiteId = getAllocSiteId(className, expr.indexOfBytecode());
                                expr.replace( "{ $_ = $proceed($$); veroy.research.et2.javassist.ETProxy.onObjectAlloc($_, " +
                                              classId + ", " + allocSiteId + "); }");
                            }

                            public void edit(FieldAccess expr) throws CannotCompileException {
                                try {
                                    final String fieldName = expr.getField().getName();
                                    if (expr.isWriter()) {
                                        expr.replace( "{ veroy.research.et2.javassist.ETProxy.onPutField($1, $0, " + getFieldId(className, fieldName) + "); $_ = $proceed($$); }" );
                                    }
                                } catch (NotFoundException exc) {
                                }
                            }
                        }
                );
                // Insert ENTRY and EXIT events:
                try {
                    if (Modifier.isStatic(modifiers)) {
                        method.insertBefore("{ veroy.research.et2.javassist.ETProxy.onEntry(" + methodId + ", (Object) null); }");
                    } else {
                        method.insertBefore("{ veroy.research.et2.javassist.ETProxy.onEntry(" + methodId + ", (Object) this); }");
                    }
                } catch (CannotCompileException exc) {
                    System.err.println("Error compiling 'insertBefore' code into class/method: " + methodName + " ==? " + methodName.equals("equals"));
                    exc.printStackTrace();
                    throw exc;
                }
                try {
                    method.insertAfter("{ veroy.research.et2.javassist.ETProxy.onExit(" + methodId + "); }");
                } catch (CannotCompileException exc) {
                    System.err.println("Error compiling 'insertAfter' code into class/method: " + methodName);
                    exc.printStackTrace();
                    throw exc;
                }
            }
        }
        ctKlazz.setName(newName);

        return ctKlazz;
    }

    protected boolean shouldIgnore(int modifiers, String methodName) {
        return (Modifier.isNative(modifiers) ||
                Modifier.isAbstract(modifiers) ||
                methodName.equals("equals") ||
                methodName.equals("finalize") ||
                methodName.equals("toString") ||
                methodName.equals("wait"));
    }
}
