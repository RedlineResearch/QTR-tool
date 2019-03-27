package veroy.research.et2.javassist;

import java.lang.Runtime;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Arrays;
import java.lang.Thread;
import java.io.PrintWriter;
import java.util.concurrent.locks.ReentrantLock;

public class ETProxy {
    
    // TODO: private static boolean atMain = false;

    // Thread local boolean w/ default value false
    private static final InstrumentFlag inInstrumentMethod = new InstrumentFlag();

    /*
    private static final int BUFMAX = 1000;

    private static long[] timestampBuffer = new long[BUFMAX];
    private static long[] threadIDBuffer = new long[BUFMAX];

    // TRACING EVENTS
    // Method entry = 1,
    // method exit = 2,
    // object allocation = 3
    // object array allocation = 4
    // 2D array allocation = 6,
    // put field = 7
    // get field = 8

    private static int[] eventTypeBuffer = new int[BUFMAX];
    
    private static int[] firstBuffer = new int[BUFMAX];
    private static int[] secondBuffer = new int[BUFMAX];
    private static int[] thirdBuffer = new int[BUFMAX];
    private static int[] fourthBuffer = new int[BUFMAX];
    private static int[] fifthBuffer = new int[BUFMAX];
    private static AtomicInteger ptr = new AtomicInteger();
    private static ReentrantLock mx = new ReentrantLock();
    private static PrintWriter pw;


    static {
        try {
            pw = new PrintWriter("trace");
        } catch (Exception e) {
            System.err.println("FNF");
        }
    }

    */

    
    // I hope no one ever creates a 2 gigabyte object
    // TODO: private static native int getObjectSize(Object obj);
    
    public static void onEntry(int methodID, Object receiver)
    {
        // TODO: if (!atMain) {
        // TODO:     return;
        // TODO: }
        
        // TODO: long timestamp = System.nanoTime();
        // TODO: 
        // TODO: if (inInstrumentMethod.get()) {
        // TODO:     return;
        // TODO: } else {
        // TODO:     inInstrumentMethod.set(true);
        // TODO: }

        // TODO: mx.lock();
        // TODO: try {
        // TODO:     while (true) {
        // TODO:         synchronized(ptr) {
        // TODO:             if (ptr.get() < BUFMAX) {
        // TODO:                 // wait on ptr to prevent overflow
        // TODO:                 int currPtr = ptr.getAndIncrement();
        // TODO:                 firstBuffer[currPtr] = methodID;
        // TODO:                 secondBuffer[currPtr] = (receiver == null) ? 0 : System.identityHashCode(receiver);
        // TODO:                 eventTypeBuffer[currPtr] = 1;
        // TODO:                 timestampBuffer[currPtr] = timestamp;
        // TODO:                 threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
        // TODO:                 break;
        // TODO:             } else {
        // TODO:                 flushBuffer();
        // TODO:             }
        // TODO:         }
        // TODO:     }
        // TODO:     // System.out.println("Method ID: " + methodID);
        // TODO: } finally {
        // TODO:     mx.unlock();
        // TODO: }

        // TODO: inInstrumentMethod.set(false);
    }

    /*
    public static void onExit(int methodID)
    {
        if (!atMain) {
            return;
        }
        
        long timestamp = System.nanoTime();
        
        // TODO: if (inInstrumentMethod.get()) {
        // TODO:     return;
        // TODO: } else {
        // TODO:     inInstrumentMethod.set(true);
        // TODO: }

        mx.lock();
        try {
            while (true) {
                synchronized(ptr) {
                    if (ptr.get() < BUFMAX) {
                        // wait on ptr to prevent overflow
                        int currPtr = ptr.getAndIncrement();
                        firstBuffer[currPtr] = methodID;
                        eventTypeBuffer[currPtr] = 2;
                        timestampBuffer[currPtr] = timestamp;
                        threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
                        break;
                    } else {
                        flushBuffer();
                    }
                }
            }
        } finally {
            mx.unlock();
        }
        // System.out.println("Method ID: " + methodID);

        // TODO: inInstrumentMethod.set(false);
    }
    
    public static void onObjectAlloc(Object allocdObject, int allocdClassID, int allocSiteID)
    {
        if (!atMain) {
            return;
        }
        long timestamp = System.nanoTime();
        // TODO: if (inInstrumentMethod.get()) {
        // TODO:     return;
        // TODO: } else {
        // TODO:     inInstrumentMethod.set(true);
        // TODO: }
        mx.lock();
        try {
            while (true) {
                synchronized(ptr) {
                    if (ptr.get() < BUFMAX) {
                        // wait on ptr to prevent overflow
                        int currPtr = ptr.getAndIncrement();
                        firstBuffer[currPtr] = System.identityHashCode(allocdObject);
                        eventTypeBuffer[currPtr] = 3;
                        secondBuffer[currPtr] = allocdClassID;
                        thirdBuffer[currPtr] = allocSiteID;
                        // I hope no one ever wants a 2 gigabyte (shallow size!) object
                        // some problem here...
                        // System.err.println("Class ID: " + allocdClassID);
                        fourthBuffer[currPtr] = (int) getObjectSize(allocdObject);
                        timestampBuffer[currPtr] = timestamp;
                        threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
                        break;
                    } else {
                        flushBuffer();
                    }
                }
            }
        } finally {
            mx.unlock();
        }
        // TODO: inInstrumentMethod.set(false);
    }

    public static void onPutField(Object tgtObject, Object srcObject, int fieldID)
    {

        if (!atMain) {
            return;
        }
        
        long timestamp = System.nanoTime();
        
        // TODO: if (inInstrumentMethod.get()) {
        // TODO:     return;
        // TODO: } else {
        // TODO:     inInstrumentMethod.set(true);
        // TODO: }

        mx.lock();
        try {
            while (true) {
                synchronized(ptr) {
                    if (ptr.get() < BUFMAX) {
                        // wait on ptr to prevent overflow
                        int currPtr = ptr.getAndIncrement();
                        firstBuffer[currPtr] = System.identityHashCode(tgtObject);
                        eventTypeBuffer[currPtr] = 7;
                        secondBuffer[currPtr] = fieldID;
                        timestampBuffer[currPtr] = timestamp;
                        thirdBuffer[currPtr] = System.identityHashCode(srcObject);
                        threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
                        break;
                    } else {
                        flushBuffer();
                    }
                }
            }
        } finally {
            mx.unlock();
        }
        
        // TODO: inInstrumentMethod.set(false);
    }

    public static void onArrayAlloc( int allocdClassID,
                                     int size,
                                     Object allocdArray,
                                     int allocSiteID )
    {

        if (!atMain) {
            return;
        }
        
        long timestamp = System.nanoTime();
        
        // TODO: if (inInstrumentMethod.get()) {
        // TODO:     return;
        // TODO: } else {
        // TODO:     inInstrumentMethod.set(true);
        // TODO: }

        mx.lock();

        try {
            while (true) {
                synchronized(ptr) {
                    if (ptr.get() < BUFMAX) {
                        // wait on ptr to prevent overflow
                        int currPtr = ptr.getAndIncrement();
                        firstBuffer[currPtr] = System.identityHashCode(allocdArray);
                        eventTypeBuffer[currPtr] = 4;
                        secondBuffer[currPtr] = allocdClassID;
                        thirdBuffer[currPtr] = size;
                        fourthBuffer[currPtr] = allocSiteID;
                        fifthBuffer[currPtr] = getObjectSize(allocdArray);
                        timestampBuffer[currPtr] = timestamp;
                        threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
                        break;
                    } else {
                        flushBuffer();
                    }
                }
            }
        } finally {
            mx.unlock();
        }
        
        // TODO: inInstrumentMethod.set(false);
    }

    public static void onMultiArrayAlloc( int dims,
                                          int allocdClassID,
                                          Object[] allocdArray,
                                          int allocSiteID )
    {
        if (!atMain) {
            return;
        }
        
        long timestamp = System.nanoTime();
        
        // TODO: if (inInstrumentMethod.get()) {
        // TODO:     return;
        // TODO: } else {
        // TODO:     inInstrumentMethod.set(true);
        // TODO: }

        mx.lock();

        try {
            while (true) {
                if (ptr.get() < BUFMAX) {
                    // wait on ptr to prevent overflow
                    int currPtr;
                    synchronized(ptr) {
                        currPtr = ptr.getAndIncrement();
                    }
                    firstBuffer[currPtr] = System.identityHashCode(allocdArray);
                    eventTypeBuffer[currPtr] = 6;
                    secondBuffer[currPtr] = allocdClassID;
                    thirdBuffer[currPtr] = allocdArray.length;
                    fourthBuffer[currPtr] = allocSiteID;
                    fifthBuffer[currPtr] = getObjectSize(allocdArray);
                    timestampBuffer[currPtr] = timestamp;
                    threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());

                    if (dims > 2) {
                        for (int i = 0; i < allocdArray.length; ++i) {
                            onMultiArrayAlloc(dims - 1, allocdClassID, (Object[]) allocdArray[i], allocSiteID);
                        }
                    } else { // dims == 2
                        for (int i = 0; i < allocdArray.length; ++i) {
                            onArrayAlloc(allocdClassID, 0, allocdArray[i], allocSiteID);
                        }
                    }
                    break;
                } else {
                    synchronized(ptr) {
                        if (ptr.get() >= BUFMAX) {
                            flushBuffer();
                        }
                    }
                }
            }
        } finally {
            mx.unlock();
        }
        
        // TODO: inInstrumentMethod.set(false);
    }

    public static void witnessObjectAlive( Object aliveObject,
                                           int classID )
    {
        if (!atMain) {
            return;
        }
        
        long timestamp = System.nanoTime();
        
        // TODO: if (inInstrumentMethod.get()) {
        // TODO:     return;
        // TODO: } else {
        // TODO:     inInstrumentMethod.set(true);
        // TODO: }

        mx.lock();

        try {
            while (true) {
                if (ptr.get() < BUFMAX) {
                    // wait on ptr to prevent overflow
                    int currPtr;
                    synchronized(ptr) {
                        currPtr = ptr.getAndIncrement();
                    }

                    // System.err.println("Seems like problem is here...");
                    // System.err.println("Class being instrumented here (ID): " + classID);

                    firstBuffer[currPtr] = System.identityHashCode(aliveObject);

                    // System.err.println("gets here");
                    eventTypeBuffer[currPtr] = 8;
                    secondBuffer[currPtr] = classID;
                    timestampBuffer[currPtr] = timestamp;

                    threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
                    break;
                } else {
                    synchronized(ptr) {
                        if (ptr.get() >= BUFMAX) {
                            flushBuffer();
                        }
                    }
                }
            }
        } finally {
            mx.unlock();
        }
        
        // TODO: inInstrumentMethod.set(false);
    }
    
    public static void onInvokeMethod(Object allocdObject, int allocdClassID, int allocSiteID)
    {
        if (!atMain) {
            return;
        }
        long timestamp = System.nanoTime();
        // TODO: if (inInstrumentMethod.get()) {
        // TODO:     return;
        // TODO: } else {
        // TODO:     inInstrumentMethod.set(true);
        // TODO: }
        mx.lock();
        try {
            while (true) {
                if (ptr.get() < BUFMAX) {
                    // wait on ptr to prevent overflow
                    int currPtr;
                    synchronized(ptr) {
                        currPtr = ptr.getAndIncrement();
                    }
                    firstBuffer[currPtr] = System.identityHashCode(allocdObject);
                    eventTypeBuffer[currPtr] = 3;
                    secondBuffer[currPtr] = allocdClassID;
                    thirdBuffer[currPtr] = allocSiteID;
                    // I hope no one ever wants a 2 gigabyte (shallow size!) object
                    // some problem here...
                    // System.err.println("Class ID: " + allocdClassID);
                    fourthBuffer[currPtr] = (int) getObjectSize(allocdObject);
                    timestampBuffer[currPtr] = timestamp;
                    threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
                    break;
                } else {
                    synchronized(ptr) {
                        if (ptr.get() >= BUFMAX) {
                            flushBuffer();
                        }
                    }
                }
            }
        } finally {
            mx.unlock();
        }
        // TODO: inInstrumentMethod.set(false);
    }
    */

    /*
    public static void flushBuffer()
    {
        mx.lock();
        try {
            for (int i = 0; i < BUFMAX; i++) {
                switch(eventTypeBuffer[i]) {
                    case 1: // method entry
                        // M <method-id> <receiver-object-id> <thread-id>
                        pw.println( "M " +
                                    firstBuffer[i] + " " +
                                    secondBuffer[i] + " " +
                                    threadIDBuffer[i] );
                        break;
                    case 2: // method exit
                        // E <method-id> <thread-id>
                        pw.println( "E " +
                                    firstBuffer[i] + " " +
                                    threadIDBuffer[i] );
                        break;
                    case 3: // object allocation
                        // N <object-id> <size> <type-id> <site-id> <length (0)> <thread-id>
                        // 1st buffer = object ID (hash)
                        // 2nd buffer = class ID
                        // 3rd buffer = allocation site (method ID)
                        pw.println( "N " +
                                    firstBuffer[i] + " " +
                                    fourthBuffer[i] + " " +
                                    secondBuffer[i] + " " +
                                    thirdBuffer[i] + " "
                                    + 0 + " "
                                    + threadIDBuffer[i] );
                        break;
                    case 4: // object array allocation
                    case 5: // primitive array allocation
                        // 5 now removed so nothing should come out of it
                        // A <object-id> <size> <type-id> <site-id> <length> <thread-id>
                        pw.println( "A " +
                                    firstBuffer[i] + " " +
                                    fifthBuffer[i] + " " +
                                    secondBuffer[i] + " " +
                                    fourthBuffer[i] + " " +
                                    thirdBuffer[i] + " " +
                                    threadIDBuffer[i] );
                        break;
                    case 6: // 2D array allocation
                        // TODO: Conflicting documention: 2018-1112
                        // 6, arrayHash, arrayClassID, size1, size2, timestamp
                        // A <object-id> <size> <type-id> <site-id> <length> <thread-id>
                        pw.println( "A " +
                                    firstBuffer[i] + " " +
                                    fifthBuffer[i] + " " +
                                    secondBuffer[i] + " " +
                                    fourthBuffer[i] + " " +
                                    thirdBuffer[i] + " " +
                                    threadIDBuffer[i] );
                        break;
                    case 7: // object update
                        // TODO: Conflicting documention: 2018-1112
                        // 7, targetObjectHash, fieldID, srcObjectHash, timestamp
                        // U <old-tgt-obj-id> <obj-id> <new-tgt-obj-id> <field-id> <thread-id>
                        pw.println( "U " +
                                    firstBuffer[i] + " " +
                                    secondBuffer[i] + " " +
                                    thirdBuffer[i] + " " +
                                    threadIDBuffer[i] );
                        break;
                    case 8: // witness with get field
                        // 8, aliveObjectHash, classID, timestamp
                        pw.println( "W" + " " +
                                    firstBuffer[i] + " " +
                                    secondBuffer[i] + " " +
                                    threadIDBuffer[i] );
                        break;
                }
            }
            ptr.set(0);
        } finally {
            mx.unlock();
        }
    }
*/
    
}
