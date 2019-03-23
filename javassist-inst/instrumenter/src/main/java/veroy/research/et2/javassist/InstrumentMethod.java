package veroy.research.et2.javassist;

import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.NotFoundException;

public class InstrumentMethod {

    private String targetClass;
    private String targetMethod;

    public InstrumentMethod(String targetClass,String targetMethod) {
        this.targetClass = targetClass;
        this.targetMethod = targetMethod;
    }

    public Class instrumentStart() {
        ClassPool cp = ClassPool.getDefault();
        Class klazz = null;
        CtClass ctKlazz = null;
        CtMethod method = null;
        try {
            ctKlazz = cp.get(targetClass);
            method = ctKlazz.getDeclaredMethod(targetMethod);
        } catch (NotFoundException exc) {
            System.err.println("Error getting class/method:");
            exc.printStackTrace();
            System.exit(1);
        }

        try {
            method.insertBefore("{ System.out.println(\"\"); }");
            method.insertAfter("{ System.out.println\"\"); }");
            klazz = ctKlazz.toClass();
        } catch (CannotCompileException exc) {
            System.err.println("Error compiling new code into class/method:");
            exc.printStackTrace();
            System.exit(2);
        }

        return klazz;
    }
}
