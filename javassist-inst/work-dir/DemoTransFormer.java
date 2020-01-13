public class DemoTransformer implements ClassFileTransformer {
    /** The internal form class name of the class to transform */
    protected String className;
    /** The class loader of the class */
    protected ClassLoader classLoader;
    /**
     * Creates a new DemoTransformer
     * @param className The binary class name of the class to transform
     * @param classLoader The class loader of the class
     */
    public DemoTransformer(String className, ClassLoader classLoader) {
        this.className = className.replace('.', '/');
        this.classLoader = classLoader;
    }

    /**
     * {@inheritDoc}
     * @see java.lang.instrument.ClassFileTransformer#transform(java.lang.ClassLoader, java.lang.String, java.lang.Class, java.security.ProtectionDomain, byte[])
     */
    @Override
    public byte[] transform(ClassLoader loader, String className,
            Class<?> classBeingRedefined, ProtectionDomain protectionDomain,
            byte[] classfileBuffer) throws IllegalClassFormatException {
        if(className.equals(this.className) && loader.equals(classLoader)) {
            return new ModifyMethodTest(classfileBuffer).modiySleepMethod();
        }
        return classfileBuffer;
    }

}