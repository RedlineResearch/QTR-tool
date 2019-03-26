package veroy.research.et2.javassist;

import java.io.InputStream;
import java.io.IOException;
import javassist.ByteArrayClassPath;
import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.Modifier;

public class InstrumentMethods {

    private InputStream instream;
    private String newName;

    public InstrumentMethods(InputStream instream, String newName) {
        this.instream = instream;
        this.newName = newName;
    }

    public CtClass instrumentStart() throws CannotCompileException {
        ClassPool cp = ClassPool.getDefault();
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
            if (shouldIgnore(method)) {
                continue;
            }
            String methodName = method.getName();
            try {
                method.insertBefore("{ System.out.println(\"ENTER " + methodName + "\"); }");
                // + targetMethod); }");
                method.insertAfter("{ System.out.println(\"-> EXIT " + methodName + "\"); }");
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

    protected boolean shouldIgnore(CtMethod method) {
        int modifiers = method.getModifiers();
        return (Modifier.isNative(modifiers) ||
                Modifier.isAbstract(modifiers));
    }
}
