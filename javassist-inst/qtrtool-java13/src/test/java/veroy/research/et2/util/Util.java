package veroy.research.util;

public class Util {
    public static void startSecondJVM(Class classWithMain) throws Exception {
        String separator = System.getProperty("file.separator");
        String classpath = System.getProperty("java.class.path");
        String path = System.getProperty("java.home") + separator + "bin" + separator + "java";
        ProcessBuilder processBuilder = new ProcessBuilder( path,
                                                            "-cp", classpath, 
                                                            classWithMain.getName());
                                                            // TODO: add some program to instrument here with the correct JVM arguments.
        Process process = processBuilder.start();
        process.waitFor();
    }
} 