public class TransformerService implements TransformerServiceMBean {
    /** The JVM's instrumentation instance */
    protected final Instrumentation instrumentation;

    /**
     * Creates a new TransformerService
     * @param instrumentation  The JVM's instrumentation instance 
     */
    public TransformerService(Instrumentation instrumentation) {
        this.instrumentation = instrumentation;
    }

    /**
     * {@inheritDoc}
     * @see com.heliosapm.shorthandexamples.TransformerServiceMBean#transformClass(java.lang.String)
     */
    @Override
    public void transformClass(String className) {
        Class<?> targetClazz = null;
        ClassLoader targetClassLoader = null;
        // first see if we can locate the class through normal means
        try {
            targetClazz = Class.forName(className);
            targetClassLoader = targetClazz.getClassLoader();
            transform(targetClazz, targetClassLoader);
            return;
        } catch (Exception ex) { /* Nope */ }
        // now try the hard/slow way
        for(Class<?> clazz: instrumentation.getAllLoadedClasses()) {
            if(clazz.getName().equals(className)) {
                targetClazz = clazz;
                targetClassLoader = targetClazz.getClassLoader();
                transform(targetClazz, targetClassLoader);
                return;             
            }
        }
        throw new RuntimeException("Failed to locate class [" + className + "]");
    }

    /**
     * Registers a transformer and executes the transform
     * @param clazz The class to transform
     * @param classLoader The classloader the class was loaded from
     */
    protected void transform(Class<?> clazz, ClassLoader classLoader) {
        DemoTransformer dt = new DemoTransformer(clazz.getName(), classLoader);
        instrumentation.addTransformer(dt, true);
        try {
            instrumentation.retransformClasses(clazz);
        } catch (Exception ex) {
            throw new RuntimeException("Failed to transform [" + clazz.getName() + "]", ex);
        } finally {
            instrumentation.removeTransformer(dt);
        }       
    }
}