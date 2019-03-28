package veroy.research.et2.javassist;

import java.io.InputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import javassist.ByteArrayClassPath;
import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.LoaderClassPath;
import javassist.Modifier;

public class InstrumentMethods {

    private static AtomicInteger nextMethodId = new AtomicInteger(1);
    private static ConcurrentHashMap<String, Integer> methodIdMap = new ConcurrentHashMap();

    private InputStream instream;
    private String newName;
    private PrintWriter mwriter;

    public InstrumentMethods(InputStream instream, String newName, PrintWriter methodsWriter) {
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

    public CtClass instrumentStart(ClassLoader loader) throws CannotCompileException {
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

        CtMethod[] methods = ctKlazz.getMethods();
        String className = ctKlazz.getName();
        for (int ind = 0 ; ind < methods.length; ind++) {
            CtMethod method = methods[ind];
            String targetMethod = method.getName();
            int modifiers = method.getModifiers();
            if (shouldIgnore(modifiers)) {
                continue;
            }
            String methodName = method.getName();
            int methodId = getMethodId(className, methodName);
            try {
                if (Modifier.isStatic(modifiers)) {
                    method.insertBefore("{ veroy.research.et2.javassist.ETProxy.onEntry(" + methodId + ", (Object) null); }");
                } else {
                    method.insertBefore("{ veroy.research.et2.javassist.ETProxy.onEntry(" + nextMethodId.getAndAdd(1) + ", (Object) this); }");
                }
                // TODO: method.insertAfter("{ System.out.println(\"-> EXIT " + methodName + "\"); }");
                // + targetMethod); }");
            } catch (CannotCompileException exc) {
                System.err.println("Error compiling new code into class/method:");
                exc.printStackTrace();
                throw exc;
            }
        }
        ctKlazz.setName(newName);

        return ctKlazz;
    }

    protected boolean shouldIgnore(int modifiers) {
        return (Modifier.isNative(modifiers) ||
                Modifier.isAbstract(modifiers));
    }
}
