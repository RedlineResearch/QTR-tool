package veroy.research.et2.javassist;

import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.NotFoundException;

public class InstrumentMethod {

    private String targetClass;

    public InstrumentMethod(String targetClass) {
        this.targetClass = targetClass;
    }

    public Class instrumentStart() {
        ClassPool cp = ClassPool.getDefault();
        Class klazz = null;
        CtClass ctKlazz = null;
        try {
            ctKlazz = cp.get(targetClass);
        } catch (NotFoundException exc) {
            System.err.println("Error getting class/method:");
            exc.printStackTrace();
            System.exit(1);
        }

        CtMethod[] methods = ctKlazz.getMethods();
        for (int ind = 0 ; ind < methods.length; ind++) {
            CtMethod method = methods[ind];
            String targetMethod = method.getName();
            try {
                method.insertBefore("{ System.out.println(\"ENTER \" + targetMethod); }");
                method.insertAfter("{ System.out.println\"-> EXIT \" + targetMethod); }");
                klazz = ctKlazz.toClass();
            } catch (CannotCompileException exc) {
                System.err.println("Error compiling new code into class/method:");
                exc.printStackTrace();
                System.exit(2);
            }
        }

        return klazz;
    }
}
