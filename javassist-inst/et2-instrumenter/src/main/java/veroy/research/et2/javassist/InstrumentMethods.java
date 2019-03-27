package veroy.research.et2.javassist;

import java.io.InputStream;
import java.io.IOException;
import javassist.ByteArrayClassPath;
import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.LoaderClassPath;
import javassist.Modifier;

public class InstrumentMethods {

    private InputStream instream;
    private String newName;

    public InstrumentMethods(InputStream instream, String newName) {
        this.instream = instream;
        this.newName = newName;
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
        for (int ind = 0 ; ind < methods.length; ind++) {
            CtMethod method = methods[ind];
            String targetMethod = method.getName();
            int modifiers = method.getModifiers();
            if (shouldIgnore(modifiers)) {
                continue;
            }
            String methodName = method.getName();
            try {
                // method.insertBefore("{ System.out.println(\"ENTER " + methodName + "\"); }");
                if (Modifier.isStatic(modifiers)) {
                    method.insertBefore("{ veroy.research.et2.javassist.ETProxy.onEntry(123, (Object) null); }");
                } else {
                    method.insertBefore("{ veroy.research.et2.javassist.ETProxy.onEntry(123, (Object) this); }");
                }
                // + targetMethod); }");
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
