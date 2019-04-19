package veroy.research.et2.javassist;

import org.apache.commons.io.IOUtils;
import java.io.InputStream;
import java.io.IOException;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.nio.file.Path;
import java.nio.file.Paths;
import javassist.ByteArrayClassPath;
import javassist.CannotCompileException;
import javassist.CtClass;
import javassist.NotFoundException;

public class Instrumenter {

    public static void main(String[] args) {
        System.out.println("Starting Instrumenter...");
        if (args.length != 3) {
            System.out.println("Usage: java -jar instrumenter-1.0-SNAPSHOT.jar <Target.class> <Path> <full/Klass/Name>");
            System.exit(1);
        }
        // Get input class from args
        String argKlass = args[0];
        String argPath = args[1];
        String fullKlass = args[2];
        Path tgtPath = Paths.get(argPath, argKlass);
        System.out.println("Instrumenting " + argKlass + "...");
        InputStream fistream = null;
        try {
            fistream = new FileInputStream(argKlass);
        } catch (FileNotFoundException exc) {
            System.err.println("File not found: " + argKlass);
            exc.printStackTrace();
            System.exit(1);
        }
        // byte[] barray = IOUtils.toByteArray(fistream);
        // ByteArrayClassPath klassBuf = new ByteArrayClassPath(fullKlass, barray);
        String newName = fullKlass + "INST";
        CtClass newKlazz = instrumentClass(fistream, newName);
        System.out.println("New class name: " + newKlazz.getName());
        try {
            newKlazz.writeFile(".");
        } catch (CannotCompileException exc) {
            System.err.println("Cannot compile when writing out: " + newName);
            exc.printStackTrace();
            System.exit(1);
        } catch (IOException exc) {
            System.err.println("IO error when writing out: " + newName);
            exc.printStackTrace();
            System.exit(2);
        }
        System.out.println("Finished writing: " + newKlazz.getName());
    }

    protected static CtClass instrumentClass(InputStream klassBuf, String newName) {
        InstrumentMethods instr = new InstrumentMethods(klassBuf, newName);

        CtClass newKlazz = instr.instrumentStart();
        return newKlazz;
    }

}
