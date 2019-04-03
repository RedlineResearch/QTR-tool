package veroy.research.et2.javassist;

import java.lang.Runtime;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Arrays;
import java.lang.Thread;
import java.io.PrintWriter;
import java.util.concurrent.locks.ReentrantLock;

public class ETProxy {
    
    public static PrintWriter traceWriter = null;

    // Thread local boolean w/ default value false
    private static final InstrumentFlag inInstrumentMethod = new InstrumentFlag();
    private static ReentrantLock mx = new ReentrantLock();


    // Buffers:
    private static final int BUFMAX = 10000;
    private static int[] eventTypeBuffer = new int[BUFMAX+1];
    private static int[] firstBuffer = new int[BUFMAX+1];
    private static int[] secondBuffer = new int[BUFMAX+1];
    private static int[] thirdBuffer = new int[BUFMAX+1];
    private static int[] fourthBuffer = new int[BUFMAX+1];
    private static int[] fifthBuffer = new int[BUFMAX+1];

    private static long[] timestampBuffer = new long[BUFMAX];
    private static long[] threadIDBuffer = new long[BUFMAX];

    private static AtomicInteger ptr = new AtomicInteger();
    /*


    // TRACING EVENTS
    // Method entry = 1,
    // method exit = 2,
    // object allocation = 3
    // object array allocation = 4
    // 2D array allocation = 6,
    // put field = 7
    // get field = 8

    private static PrintWriter traceWriter;


    static {
        try {
            traceWriter = new PrintWriter("trace");
        } catch (Exception e) {
            System.err.println("FNF");
        }
    }

    */

    
    // I hope no one ever creates a 2 gigabyte object
    // TODO: private static native int getObjectSize(Object obj);
    
    public static void onEntry(int methodId, Object receiver)
    {
        long timestamp = System.nanoTime();
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }
        try {
            mx.lock();
            synchronized(ptr) {
                if (ptr.get() >= BUFMAX) {
                    flushBuffer();
                    assert(ptr.get() == 0);
                }
                // wait on ptr to prevent overflow
                int currPtr = ptr.getAndIncrement();
                firstBuffer[currPtr] = methodId;
                secondBuffer[currPtr] = (receiver == null) ? 0 : System.identityHashCode(receiver);
                eventTypeBuffer[currPtr] = 1; // TODO: Make into constants.
                timestampBuffer[currPtr] = timestamp; // TODO: Not really useful
                threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
            }
        } finally {
            mx.unlock();
        }
        inInstrumentMethod.set(false);
    }

    public static void onExit(int methodId)
    {
        long timestamp = System.nanoTime();
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }
        try {
            mx.lock();
            synchronized(ptr) {
                if (ptr.get() >= BUFMAX) {
                    flushBuffer();
                    assert(ptr.get() == 0);
                }
                int currPtr = ptr.getAndIncrement();
                firstBuffer[currPtr] = methodId;
                eventTypeBuffer[currPtr] = 2;
                timestampBuffer[currPtr] = timestamp;
                threadIDBuffer[currPtr] = System.identityHashCode(Thread.currentThread());
            }
        } finally {
            mx.unlock();
        }
        inInstrumentMethod.set(false);
    }
    /*
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

    public static void flushBuffer()
    {

        try {
            mx.lock();
            int bufSize = Math.max(ptr.get(), BUFMAX);
            for (int i = 0; i < bufSize; i++) {
                switch(eventTypeBuffer[i]) {
                    case 1: // method entry
                        // M <method-id> <receiver-object-id> <thread-id>
                        traceWriter.println( "M " +
                                    firstBuffer[i] + " " +
                                    secondBuffer[i] + " " +
                                    threadIDBuffer[i] );
                        break;
                    case 2: // method exit
                        // E <method-id> <thread-id>
                        traceWriter.println( "E " +
                                    firstBuffer[i] + " " +
                                    threadIDBuffer[i] );
                        break;
                    case 3: // object allocation
                        // N <object-id> <size> <type-id> <site-id> <length (0)> <thread-id>
                        // 1st buffer = object ID (hash)
                        // 2nd buffer = class ID
                        // 3rd buffer = allocation site (method ID)
                        traceWriter.println( "N " +
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
                        traceWriter.println( "A " +
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
                        traceWriter.println( "A " +
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
                        traceWriter.println( "U " +
                                    firstBuffer[i] + " " +
                                    secondBuffer[i] + " " +
                                    thirdBuffer[i] + " " +
                                    threadIDBuffer[i] );
                        break;
                    case 8: // witness with get field
                        // 8, aliveObjectHash, classID, timestamp
                        traceWriter.println( "W" + " " +
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
    
}
