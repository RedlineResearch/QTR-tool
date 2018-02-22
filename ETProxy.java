import java.util.concurrent.atomic.AtomicInteger;
import java.util.Arrays;
import java.io.PrintWriter;

public class ETProxy
{
    
    private static String logFilePath = "methodTimestamp.log";

    // Thread local boolean w/ default value false
    private static final InstrumentFlag inInstrumentMethod = new InstrumentFlag();

    private static long[] timestampBuffer = new long[1000];
    private static long[] threadIDBuffer = new long[1000];

    // TRACING EVENTS
    // Method entry = 1, method exit = 2, object allocation = 3
    // object array allocation = 4, primitive array allocation = 5,
    // 2D array allocation = 6, put field = 7
    // WITNESSES
    // get field = 8
    // We should have used enums, but it breaks things
    private static int[] eventTypeBuffer = new int[1000];
    
    private static int[] firstBuffer = new int[1000];
    private static int[] secondBuffer = new int[1000];
    private static int[] thirdBuffer = new int[1000];
    private static int[] fourthBuffer = new int[1000];
    private static AtomicInteger ptr = new AtomicInteger();

    // Adapted from JNIF test code
    private static byte[] getResourceEx(String className, ClassLoader loader) {
		java.io.InputStream is;

		try {
			is = loader.getResourceAsStream(className + ".class");
		} catch (Throwable e) {
			// System.err.println("Error: getResource for " + className);
			// System.err.println("Error: loader: " + loader);
			// e.printStackTrace();
			return null;
		}

		java.io.ByteArrayOutputStream os = new java.io.ByteArrayOutputStream();
		try {
			byte[] buffer = new byte[0xFFFF];

			for (int len; (len = is.read(buffer)) != -1;) {
				os.write(buffer, 0, len);
			}

			os.flush();

			return os.toByteArray();
		} catch (Throwable e) {
			// System.err.println("Error: getResource for " + className);
			// System.err.println("Error: loader: " + loader);
			// System.err.println("Error: is: " + is);
			// e.printStackTrace();
			return null;
		}
	}

    public static byte[] getResource(String className, ClassLoader loader) {
		byte[] res = null;

        // System.err.println(className + " requested");
        
		if (loader != null) {
			res = getResourceEx(className, loader);
		}

		if (res == null) {
			ClassLoader cl = ClassLoader.getSystemClassLoader();
			res = getResourceEx(className, cl);
		}

		return res;
	}

    // public static native void _onMain();
    
    public static void onEntry(int methodID, Object receiver)
    {
        long timestamp = System.nanoTime();
        
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }

        if (ptr.get() < 1000) {
            // wait on ptr to prevent overflow
            int currPtr = ptr.getAndIncrement();
            firstBuffer[currPtr] = methodID;
            secondBuffer[currPtr] = System.identityHashCode(receiver);
            eventTypeBuffer[currPtr] = 1;
            timestampBuffer[currPtr] = timestamp;
            threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
        } else {
            synchronized(ptr) {
                if (ptr.get() >= 1000) {
                    flushBuffer();
                }
            }
        }
        // System.out.println("Method ID: " + methodID);

        inInstrumentMethod.set(false);
    }

    public static void onExit(int methodID)
    {
        long timestamp = System.nanoTime();
        
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }

        if (ptr.get() < 1000) {
            // wait on ptr to prevent overflow
            int currPtr = ptr.getAndIncrement();
            firstBuffer[currPtr] = methodID;
            eventTypeBuffer[currPtr] = 2;
            timestampBuffer[currPtr] = timestamp;
            threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
        } else {
            synchronized(ptr) {
                if (ptr.get() >= 1000) {
                    flushBuffer();
                }
            }
        }
        // System.out.println("Method ID: " + methodID);

        inInstrumentMethod.set(false);
    }
    
    public static void onObjectAlloc(int allocdClassID, Object allocdObject)
    {
        long timestamp = System.nanoTime();
        
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }

        if (ptr.get() < 1000) {
            // wait on ptr to prevent overflow
            int currPtr = ptr.getAndIncrement();
            firstBuffer[currPtr] = System.identityHashCode(allocdObject);
            eventTypeBuffer[currPtr] = 3;
            secondBuffer[currPtr] = allocdClassID;
            timestampBuffer[currPtr] = timestamp;
            threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
        } else {
            synchronized(ptr) {
                if (ptr.get() >= 1000) {
                    flushBuffer();
                }
            }
        }
        
        inInstrumentMethod.set(false);
    }

    public static void onPutField(Object tgtObject, Object srcObject, int fieldID)
    {
        long timestamp = System.nanoTime();
        
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }

        if (ptr.get() < 1000) {
            // wait on ptr to prevent overflow
            int currPtr = ptr.getAndIncrement();
            firstBuffer[currPtr] = System.identityHashCode(tgtObject);
            eventTypeBuffer[currPtr] = 7;
            secondBuffer[currPtr] = fieldID;
            timestampBuffer[currPtr] = timestamp;
            thirdBuffer[currPtr] = System.identityHashCode(srcObject);
            threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
        } else {
            synchronized(ptr) {
                if (ptr.get() >= 1000) {
                    flushBuffer();
                }
            }
        }
        
        inInstrumentMethod.set(false);
    }

    public static void onPrimitiveArrayAlloc(int atype, int size, Object allocdArray)
    {
        long timestamp = System.nanoTime();
        
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }

        if (ptr.get() < 1000) {
            // wait on ptr to prevent overflow
            int currPtr = ptr.getAndIncrement();
            firstBuffer[currPtr] = System.identityHashCode(allocdArray);
            eventTypeBuffer[currPtr] = 5;
            secondBuffer[currPtr] = atype;
            thirdBuffer[currPtr] = size;
            timestampBuffer[currPtr] = timestamp;
            threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
        } else {
            synchronized(ptr) {
                if (ptr.get() >= 1000) {
                    flushBuffer();
                }
            }
        }
        
        inInstrumentMethod.set(false);
    }

    public static void onObjectArrayAlloc(int allocdClassID, int size, Object[] allocdArray)
    {
        long timestamp = System.nanoTime();
        
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }

        if (ptr.get() < 1000) {
            // wait on ptr to prevent overflow
            int currPtr = ptr.getAndIncrement();
            firstBuffer[currPtr] = System.identityHashCode(allocdArray);
            eventTypeBuffer[currPtr] = 4;
            secondBuffer[currPtr] = allocdClassID;
            thirdBuffer[currPtr] = size;
            timestampBuffer[currPtr] = timestamp;
            threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
        } else {
            synchronized(ptr) {
                if (ptr.get() >= 1000) {
                    flushBuffer();
                }
            }
        }
        
        inInstrumentMethod.set(false);
    }

    public static void on2DArrayAlloc(int size1, int size2, Object allocdArray, int arrayClassID)
    {
        long timestamp = System.nanoTime();
        
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }

        if (ptr.get() < 1000) {
            // wait on ptr to prevent overflow
            int currPtr = ptr.getAndIncrement();
            firstBuffer[currPtr] = System.identityHashCode(allocdArray);
            eventTypeBuffer[currPtr] = 6;
            secondBuffer[currPtr] = arrayClassID;
            thirdBuffer[currPtr] = size1;
            fourthBuffer[currPtr] = size2;
            timestampBuffer[currPtr] = timestamp;
            threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
        } else {
            synchronized(ptr) {
                if (ptr.get() >= 1000) {
                    flushBuffer();
                }
            }
        }
        
        inInstrumentMethod.set(false);
    }

    public static void witnessObjectAlive(Object aliveObject, int classID)
    {
        long timestamp = System.nanoTime();
        
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }

        if (ptr.get() < 1000) {
            // wait on ptr to prevent overflow
            int currPtr = ptr.getAndIncrement();
            firstBuffer[currPtr] = System.identityHashCode(aliveObject);
            eventTypeBuffer[currPtr] = 8;
            secondBuffer[currPtr] = classID;
            timestampBuffer[currPtr] = timestamp;
            threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
        } else {
            synchronized(ptr) {
                if (ptr.get() >= 1000) {
                    flushBuffer();
                }
            }
        }
        
        inInstrumentMethod.set(false);
    }
    
    public static void flushBuffer()
    {
        for (int i = 0; i < 1000; i++) {
            switch(eventTypeBuffer[i]) {
            case 1: // method entry
                // M <method-id> <receiver-object-id> <thread-id>
                System.out.println("M " + firstBuffer[i] + " " + secondBuffer[i] + " " + threadIDBuffer[i]);
                break;
            case 2: // method exit
                // 1/2, methodID, timestamp
                System.out.println("E " + firstBuffer[i] + " " + threadIDBuffer[i]);
                break;
            case 3: // object allocation
                // A <object-id> <size> <type-id> <site-id> <length (0)> <thread-id>
                System.out.println("A " + firstBuffer[i] + " " + 0 + " " +
                                   secondBuffer[i] + " " + 0 + "  " + 0 + threadIDBuffer[i]);
                break;
            case 4: // object array allocation
            case 5: // primitive array allocation
                // 4/5, arrayHash, classID/atype, size, timestamp
                System.out.println("AA " + firstBuffer[i] + " " +
                                   secondBuffer[i] + " " + thirdBuffer[i] + " " + threadIDBuffer[i]);
                break;
            case 6: // 2D array allocation
                // 6, arrayHash, arrayClassID, size1, size2, timestamp
                System.out.println("AA2 " + firstBuffer[i] + " " +
                                   secondBuffer[i] + " " + thirdBuffer[i] + " " +
                                   fourthBuffer[i] + " " + threadIDBuffer[i]);
                break;
            case 7: // object update
                // 7, targetObjectHash, fieldID, srcObjectHash, timestamp
                // U <old-tgt-obj-id> <obj-id> <new-tgt-obj-id> <field-id> <thread-id>
                System.out.println("U " + firstBuffer[i] + " " + secondBuffer[i] + " " +
                                   thirdBuffer[i] + " " + threadIDBuffer[i]);
                break;
            case 8: // witness with get field
                // 8, aliveObjectHash, classID, timestamp
                System.out.println("W" + " " + firstBuffer[i] + " " + secondBuffer[i] + " " +
                                   threadIDBuffer[i]);
                break;
            }
        }
        ptr.set(0);
    }
    
}
