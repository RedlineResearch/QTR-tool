package veroy.research.et2.javassist;

import java.io.InputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import javassist.ByteArrayClassPath;
import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.LoaderClassPath;
import javassist.Modifier;
import javassist.expr.ExprEditor;
import javassist.expr.MethodCall;

public class MethodInstrumenter {

    private static AtomicInteger nextMethodId = new AtomicInteger(1);
    private static ConcurrentHashMap<String, Integer> methodIdMap = new ConcurrentHashMap();

    private static AtomicBoolean mainInstrumentedFlag = new AtomicBoolean(false);
    private InputStream instream;
    private String newName;
    private PrintWriter mwriter;

    public MethodInstrumenter(InputStream instream, String newName, PrintWriter methodsWriter) {
        this.instream = instream;
        this.newName = newName;
        this.mwriter = methodsWriter;
    }

    protected int getMethodId(String className, String methodName) {
        String key = className + "#" + methodName;
        methodIdMap.computeIfAbsent(key, k -> nextMethodId.getAndAdd(1));
        Integer result = methodIdMap.get(key);
        return ((result != null) ? (Integer) result : -1);
    }

    public CtClass instrumentMethods(ClassLoader loader) throws CannotCompileException {
        ClassPool cp = ClassPool.getDefault();
        cp.insertClassPath(new LoaderClassPath(loader));
        // cp.importPackage("");
        CtClass ctKlazz = null;
        try {
            ctKlazz = cp.makeClass(instream);
        } catch (IOException exc) {
            System.err.println("IO error getting class/method:");
            exc.printStackTrace();
            System.exit(1);
        }

        CtMethod[] methods = ctKlazz.getMethods();
        String className = ctKlazz.getName();
        for (int ind = 0 ; ind < methods.length; ind++) {
            CtMethod method = methods[ind];
            String methodName = method.getName();
            int modifiers = method.getModifiers();
            if (shouldIgnore(modifiers, methodName)) {
                continue;
            }
            // DEBUG: System.err.println("XXX: " + className + " # " + methodName);
            // Edit method body to instrument events:
            // TODO: method.instrument( new ExprEditor() {
            // TODO:     public void edit(MethodCall m) throws CannotCompileException {
            // TODO:         // TODO: m.replace("{ veroy.research.et2.javassist.ETProxy.onObjectAlloc(); $_ = $proceed($$); }");
            // TODO:         m.replace("{  $_ = $proceed($$); }");
            // TODO:     }
            // TODO: });
            // Insert ENTRY and EXIT events:
            int methodId = getMethodId(className, methodName);
            try {
                if (Modifier.isStatic(modifiers)) {
                    if (mainInstrumentedFlag.get() || !methodName.equals("main")) {
                        method.insertBefore("{ veroy.research.et2.javassist.ETProxy.onEntry(" + methodId + ", (Object) null); }");
                    } else {
                        // main function:
                        System.err.println("XXX: MAIN => " + className + " # " + methodName);
                        method.insertBefore("{ veroy.research.et2.javassist.ETProxy.onEntry(" + methodId + ", (Object) null); }");
                        mainInstrumentedFlag.set(true);
                    }
                    // TODO: if (methodName.indexOf("main") 
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
