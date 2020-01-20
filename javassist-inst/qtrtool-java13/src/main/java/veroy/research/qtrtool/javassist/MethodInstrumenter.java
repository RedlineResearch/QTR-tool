package veroy.research.qtrtool.javassist;

import java.io.InputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Map;
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
import javassist.expr.NewArray;
import javassist.expr.NewExpr;
import org.apache.commons.lang3.tuple.Pair;

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
    private static PrintWriter methodsWriter;
    private static PrintWriter fieldsWriter;
    private static PrintWriter classWriter;

    public MethodInstrumenter(InputStream instream, String newName) {
        this.instream = instream;
        this.newName = newName;
        ClassPool cp = ClassPool.getDefault();
        // TODO: cp.importPackage("org.apache.log4j");
    }

    public static void setPrintWriters( PrintWriter methodsWriter,
                                        PrintWriter fieldsWriter,
                                        PrintWriter classWriter) {
        MethodInstrumenter.methodsWriter = methodsWriter;
        MethodInstrumenter.fieldsWriter = fieldsWriter;
        MethodInstrumenter.classWriter = classWriter;
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
                                expr.replace( "{ $_ = $proceed($$); veroy.research.qtrtool.javassist.QTRProxy.onObjectAlloc($_, " +
                                              classId + ", " + allocSiteId + "); }");
                            }

                            // Instrument new array expressions:
                            public void edit(NewArray expr) throws CannotCompileException {
                                try {
                                    final int allocSiteId = getAllocSiteId(className, expr.indexOfBytecode());
                                    expr.replace( "{ $_ = $proceed($$); veroy.research.qtrtool.javassist.QTRProxy.onArrayAlloc( $_," +
                                                  classId + ", " + allocSiteId + ", " + generateNewArrayReplacement(expr) + "); }");
                                } catch (CannotCompileException exc) {
                                    System.err.println("Unable on to compile call to onArrayAlloc - " + exc.getMessage());
                                    throw exc;
                                }
                            }

                            // Instrument field updates:
                            public void edit(FieldAccess expr) throws CannotCompileException {
                                try {
                                    final String fieldName = expr.getField().getName();
                                    if (expr.isWriter()) {
                                        expr.replace( "{ veroy.research.qtrtool.javassist.QTRProxy.onPutField($1, $0, " + getFieldId(className, fieldName) + "); $_ = $proceed($$); }" );
                                    } else {
                                        expr.replace( "{ veroy.research.qtrtool.javassist.QTRProxy.witnessObjectAliveVer2($0, " + classId + "); $_ = $proceed($$); }" );
                                    }
                                } catch (NotFoundException exc) {
                                }
                            }

                        }
                );
                // Insert ENTRY and EXIT events:
                try {
                    if (Modifier.isStatic(modifiers)) {
                        method.insertBefore("{ veroy.research.qtrtool.javassist.QTRProxy.onEntry(" + methodId + ", (Object) null); }");
                    } else {
                        method.insertBefore("{ veroy.research.qtrtool.javassist.QTRProxy.onEntry(" + methodId + ", (Object) this); }");
                    }
                } catch (CannotCompileException exc) {
                    System.err.println("Error compiling 'insertBefore' code into class/method: " + methodName + " ==? " + methodName.equals("equals"));
                    exc.printStackTrace();
                    throw exc;
                }
                try {
                    method.insertAfter("{ veroy.research.qtrtool.javassist.QTRProxy.onExit(" + methodId + "); }");
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

    protected String generateNewArrayReplacement(NewArray expr) {
        String result = "new int[]{";
        int numDims = expr.getCreatedDimensions();
        result = result + "$1";
        for (int ind = 1; ind < numDims; ind++) {
            result = result + ", $" + (ind + 1);
        }
        result = result + "}";
        return result;
    }

    protected boolean shouldIgnore(int modifiers, String methodName) {
        return (Modifier.isNative(modifiers) ||
                Modifier.isAbstract(modifiers) ||
                methodName.equals("equals") ||
                methodName.equals("finalize") ||
                methodName.equals("toString") ||
                methodName.equals("wait"));
    }

    public static void writeMapsToFile(PrintWriter witnessWriter) {
        writeMap(methodIdMap, methodsWriter);
        writeMap(fieldIdMap, fieldsWriter);
        writeMap(classIdMap, classWriter);
        writeWitnessMap(QTRProxy.witnessMap, witnessWriter);
    }

    public static void writeMap(Map<String, Integer> map, PrintWriter writer) {
        for (Map.Entry<String, Integer> entry : map.entrySet()) {
            String key = (String) entry.getKey();
            Integer value = (Integer) entry.getValue();
            writer.println(key + "," + value.toString());
        }
    }

    public static void writeWitnessMap(Map<Integer, Pair<Long, Integer>> map, PrintWriter writer) {
        QTRProxy.inInstrumentMethod.set(true);
        for (Map.Entry<Integer, Pair<Long, Integer>> entry : map.entrySet()) {
            Integer key = (Integer) entry.getKey();
            Pair<Long, Integer> value = (Pair<Long, Integer>) entry.getValue();
            Long timestamp = (Long) value.getLeft();
            Integer classId  = (Integer) value.getRight();
            writer.println(key + "," + timestamp.toString() + "," + classId.toString());
        }
    }
}
