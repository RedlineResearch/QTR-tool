import java.util.*;
import java.nio.file.*;

public class BF {

    private class InterpreterError extends Exception {
        public InterpreterError(String message) {
            super(message);
        }
    }

    private final int INIT_CAP = 1024;
    
    private int programPtr;
    private char[] program;
    
    private int dataPtr;
    private ArrayList<Byte> data;

    public BF(String programText) {
        programPtr = 0;
        program = programText.toCharArray();
        dataPtr = 0;
        data = new ArrayList<Byte>(INIT_CAP);

        for (int i = 0; i < INIT_CAP; i++) {
            data.add((byte)0);
        }
    }

    public boolean interp() throws Exception {
        if (programPtr == program.length) {
            return false;
        }
        switch (program[programPtr]) {
        case '>':
            dataPtr++;
            break;
        case '<':
            if (dataPtr == 0) {
                throw new InterpreterError("Data pointer can't be negative");
            }
            dataPtr--;
            break;
        case '+':
            byte v = data.get(dataPtr);            
            data.set(dataPtr, ++v);
            break;
        case '-':
            v = data.get(dataPtr);
            data.set(dataPtr, --v);
            break;
        case '.':
            v = data.get(dataPtr);
            System.out.print((char) v);
            break;
        case ',':
            int in = System.in.read();
            if (in == -1) {
                throw new InterpreterError("Invalid input from console");
            }
            data.set(dataPtr, (byte) in);
            break;
        case '[':
            if (data.get(dataPtr) != 0) {
                break;
            } else {
                Deque<Character> stack = new ArrayDeque<Character>();

                do {
                    if (programPtr >= program.length) {
                        throw new InterpreterError("Program pointer went out of bounds");
                    } else if (program[programPtr] == '[') {
                        stack.push('[');
                    } else if (program[programPtr] == ']') {
                        stack.pop();
                    }
                    programPtr++;
                } while (!stack.isEmpty());
                return true;
            }
        case ']':
            if (data.get(dataPtr) == 0) {
                break;
            } else {
                Deque<Character> stack = new ArrayDeque<Character>();

                do {
                    if (programPtr < 0) {
                        throw new InterpreterError("Program pointer went out of bounds");
                    } else if (program[programPtr] == ']') {
                        stack.push(']');
                    } else if (program[programPtr] == '[') {
                        stack.pop();
                    }
                    programPtr--;
                } while (!stack.isEmpty());

                programPtr++;
            }
            break;
        default:
            break;
        }
        programPtr++;

        return true;
    }

    public static void main(String[] args) {
        String path = args[0];
        String programText;

        try {
            programText = new String(Files.readAllBytes(Paths.get(path)));
        } catch (Exception e) {
            System.err.println("Program file not found");
            return;
        }

        BF interpreter = new BF(programText);
        try {
            while (interpreter.interp()) {
                continue;
            }
        } catch (Exception e) {
            System.err.println(e);
        }
    }
}
