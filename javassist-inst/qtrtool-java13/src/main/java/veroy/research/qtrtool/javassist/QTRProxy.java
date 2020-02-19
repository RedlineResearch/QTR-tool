package veroy.research.qtrtool.javassist;

import java.io.PrintWriter;
import java.lang.Runtime;
import java.lang.Thread;
import java.lang.instrument.Instrumentation;
import java.lang.reflect.Array;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Collections;
import java.util.concurrent.locks.ReentrantLock;
// TODO: import org.apache.commons.collections4.map.LRUMap;
import org.apache.commons.lang3.tuple.Pair;
// TODO: import org.apache.log4j.Logger;


public class QTRProxy {

    public static PrintWriter traceWriter = null;
    public static Instrumentation inst;

    // Thread local boolean w/ default value false
    public static final InstrumentFlag inInstrumentMethod = new InstrumentFlag();
    private static ReentrantLock mx = new ReentrantLock();

    // TODO: private static Logger qtrtoolLogger = Logger.getLogger(QTRProxy.class);

    // Buffers:
    private static final int BUFMAX = 10000;
    private static final int[] eventTypeBuffer = new int[BUFMAX+1];
    private static final int[] firstBuffer = new int[BUFMAX+1];
    private static final int[] secondBuffer = new int[BUFMAX+1];
    private static final int[] thirdBuffer = new int[BUFMAX+1];
    private static final long[] fourthBuffer = new long[BUFMAX+1];
    private static final int[] fifthBuffer = new int[BUFMAX+1];

    private static final long[] timestampBuffer = new long[BUFMAX+1];
    private static final int[] threadIDBuffer = new int[BUFMAX+1];

    private static final String[] dimsBuffer = new String[BUFMAX+1];

    public static final Map<Integer, Pair<Long, Integer>> witnessMap = Collections.synchronizedMap(new HashMap<Integer, Pair<Long, Integer>>());
    public static final Map<Integer, Integer> threadIdMap = Collections.synchronizedMap(new HashMap<Long, Integer>());
    private static AtomicInteger nextThreadId = new AtomicInteger(1);

    private static AtomicInteger ptr = new AtomicInteger();

    // TRACING EVENTS
    final static int METHOD_ENTRY_EVENT = 1;
    final static int METHOD_EXIT_EVENT = 2;
    final static int OBJECT_ALLOCATION_EVENT = 3;
    final static int ARRAY_ALLOC_EVENT = 4;
    final static int ARRAY_2D_ALLOC_EVENT = 6;
    final static int PUT_FIELD_EVENT = 7;
    final static int GET_FIELD_EVENT = 8;


    public static void debugCall(String message) {
        System.err.println("XXX: " + message);
    }

    public static void onEntry(int methodId, Object receiver) {
        long timestamp = System.nanoTime();
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }
        mx.lock();
        try {
            synchronized(ptr) {
                if (ptr.get() >= BUFMAX) {
                    flushBuffer();
                    assert(ptr.get() == 0);
                }
                int currPtr = ptr.getAndIncrement();
                firstBuffer[currPtr] = methodId;
                secondBuffer[currPtr] = (receiver == null) ? 0 : System.identityHashCode(receiver);
                eventTypeBuffer[currPtr] = METHOD_ENTRY_EVENT;
                timestampBuffer[currPtr] = timestamp; // TODO: Not really useful
                threadIDBuffer[currPtr] = getThreadId(System.identityHashCode(Thread.currentThread()));
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
        mx.lock();
        try {
            synchronized(ptr) {
                if (ptr.get() >= BUFMAX) {
                    flushBuffer();
                    assert(ptr.get() == 0);
                }
                int currPtr = ptr.getAndIncrement();
                firstBuffer[currPtr] = methodId;
                eventTypeBuffer[currPtr] = METHOD_EXIT_EVENT;
                timestampBuffer[currPtr] = timestamp;
                threadIDBuffer[currPtr] = getThreadId(System.identityHashCode(Thread.currentThread()));
            }
        } finally {
            mx.unlock();
        }
        inInstrumentMethod.set(false);
    }

    public static void onObjectAlloc(Object obj, int allocdClassID, int allocSiteID) {
        long timestamp = System.nanoTime();
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }
        mx.lock();
        try {
            synchronized(ptr) {
                if (ptr.get() >= BUFMAX) {
                    flushBuffer();
                    assert(ptr.get() == 0);
                }
                int currPtr = ptr.getAndIncrement();
                firstBuffer[currPtr] = System.identityHashCode(obj);
                eventTypeBuffer[currPtr] = OBJECT_ALLOCATION_EVENT;
                secondBuffer[currPtr] = allocdClassID;
                thirdBuffer[currPtr] = allocSiteID;
                fourthBuffer[currPtr] = inst.getObjectSize(obj);
                timestampBuffer[currPtr] = timestamp;
                threadIDBuffer[currPtr] = getThreadId(System.identityHashCode(Thread.currentThread()));
            }
        } finally {
            mx.unlock();
        }
        inInstrumentMethod.set(false);
    }

    // ET1 looked like this:
    //       U <old-target> <object> <new-target> <field> <thread>
    // QTR is now:
    //       U  srcObjectHash targetObjectHash fieldId threadId?
    //       0         1           2              3       4
    public static void onPutField(Object tgtObject, Object object, int fieldId)
    {
        long timestamp = System.nanoTime();
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }
        mx.lock();
        try {
            synchronized(ptr) {
                if (ptr.get() >= BUFMAX) {
                    flushBuffer();
                    assert(ptr.get() == 0);
                }
                int currPtr = ptr.getAndIncrement();
                eventTypeBuffer[currPtr] = 7;
                firstBuffer[currPtr] = System.identityHashCode(tgtObject);
                secondBuffer[currPtr] = fieldId;
                thirdBuffer[currPtr] = System.identityHashCode(object);
                timestampBuffer[currPtr] = timestamp;
                threadIDBuffer[currPtr] = getThreadId(System.identityHashCode(Thread.currentThread()));
            }
        } finally {
            mx.unlock();
        }
        inInstrumentMethod.set(false);
    }

    public static void saveDimsToBuffer(int currPtr, int[] dims) {
        String[] dimsStr = Arrays.stream(dims)
                                 .mapToObj(String::valueOf)
                                 .toArray(String[]::new);
        dimsBuffer[currPtr] = Integer.toString(dims.length) + "," + String.join(",", dimsStr);
    }

    public static void onArrayAlloc( Object arrayObj,
                                     int typeId,
                                     int allocSiteId,
                                     int[] dims )
    {
        long timestamp = System.nanoTime();
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }

        mx.lock();
        try {
            synchronized(ptr) {
                if (ptr.get() >= BUFMAX) {
                    flushBuffer();
                    assert(ptr.get() == 0);
                }
                int currPtr = ptr.getAndIncrement();
                eventTypeBuffer[currPtr] = ARRAY_ALLOC_EVENT;
                firstBuffer[currPtr] = System.identityHashCode(arrayObj);
                secondBuffer[currPtr] = typeId;
                try {
                    thirdBuffer[currPtr] = Array.getLength(arrayObj);
                } catch (IllegalArgumentException exc) {
                    thirdBuffer[currPtr] = 0;
                }
                fourthBuffer[currPtr] = inst.getObjectSize(arrayObj);
                fifthBuffer[currPtr] = allocSiteId;
                timestampBuffer[currPtr] = timestamp;
                threadIDBuffer[currPtr] = getThreadId(System.identityHashCode(Thread.currentThread()));
                saveDimsToBuffer(currPtr, dims);
            }
        } finally {
            mx.unlock();
        }
        inInstrumentMethod.set(false);
    }

    public static void witnessObjectAliveVer2( Object aliveObject,
                                               int classId ) {
        long timestamp = System.nanoTime();
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }
        mx.lock();
        try {
            witnessMap.put(System.identityHashCode(aliveObject), Pair.of(timestamp, classId));
            // eventTypeBuffer[currPtr] = 8;
            // TODO: threadIDBuffer[currPtr] = getThreadId(System.identityHashCode(Thread.currentThread()));
            // TODO: Override 'remoteLRU' method to save the timestamp.
        } finally {
            mx.unlock();
        }
        inInstrumentMethod.set(false);
    }
    /*
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
                    eventTypeBuffer[currPtr] = ARRAY_2D_ALLOC_EVENT;
                    secondBuffer[currPtr] = allocdClassID;
                    thirdBuffer[currPtr] = allocdArray.length;
                    // TODO: fourthBuffer[currPtr] = allocSiteID;
                    fifthBuffer[currPtr] = getObjectSize(allocdArray);
                    timestampBuffer[currPtr] = timestamp;
                    threadIDBuffer[currPtr] = getThreadId(System.identityHashCode(Thread.currentThread()));

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
                    threadIDBuffer[currPtr] = getThreadId(System.identityHashCode(Thread.currentThread()));
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
            int bufSize = Math.min(ptr.get(), BUFMAX);
            for (int i = 0; i < bufSize; i++) {
                switch (eventTypeBuffer[i]) {
                    case METHOD_ENTRY_EVENT: // method entry
                        // M <method-id> <receiver-object-id> <thread-id>
                        traceWriter.println( "M " +
                                             firstBuffer[i] + " " +
                                             secondBuffer[i] + " " +
                                             threadIDBuffer[i] );
                        // TODO: qtrtoolLogger.info( "M " +
                        // TODO:                 firstBuffer[i] + " " +
                        // TODO:                 secondBuffer[i] + " " +
                        // TODO:                 threadIDBuffer[i] );
                        break;
                    case METHOD_EXIT_EVENT: // method exit
                        // E <method-id> <thread-id>
                        traceWriter.println( "E " +
                                             firstBuffer[i] + " " +
                                             threadIDBuffer[i] );
                        break;
                    case OBJECT_ALLOCATION_EVENT: // object allocation
                        // N <object-id> <size> <type-id> <site-id> <length (0)> <thread-id>
                        // 1st buffer = object ID (hash)
                        // 2nd buffer = class ID
                        // 3rd buffer = allocation site (method ID)
                        traceWriter.println( "N " +
                                             firstBuffer[i] + " " +
                                             fourthBuffer[i] + " " +
                                             secondBuffer[i] + " " +
                                             thirdBuffer[i] + " "
                                             + 0 + " " // Always zero because this isn't an array.
                                             + threadIDBuffer[i] );
                        break;
                    case ARRAY_ALLOC_EVENT: // object array allocation
                    case 5: // primitive array allocation
                        // 5 now removed so nothing should come out of it
                        // A <object-id> <size> <type-id> <site-id> <length> <thread-id>
                        traceWriter.println( "A " +
                                             firstBuffer[i] + " " + // objectId
                                             fourthBuffer[i] + " " + // size
                                             secondBuffer[i] + " " + // typedId
                                             fifthBuffer[i] + " " + // siteId
                                             thirdBuffer[i] + " " + // length
                                             threadIDBuffer[i] + " " + // threadId
                                             dimsBuffer[i] ); // dimensions
                        break;
                    case ARRAY_2D_ALLOC_EVENT: // 2D array allocation
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
                    case PUT_FIELD_EVENT: // object update
                        // TODO: Conflicting documention: 2018-1112
                        // 7, targetObjectHash, fieldID, srcObjectHash, timestamp
                        // U <obj-id> <new-tgt-obj-id> <field-id> <thread-id>
                        traceWriter.println( "U " +
                                             thirdBuffer[i] + " " + // objId
                                             firstBuffer[i] + " " + // newTgtObjId
                                             secondBuffer[i] + " " + // fieldId
                                             threadIDBuffer[i] ); // threadId
                        break;
                    case GET_FIELD_EVENT: // witness with get field
                        // 8, aliveObjectHash, classID, timestamp
                        traceWriter.println( "W" + " " +
                                             firstBuffer[i] + " " +
                                             secondBuffer[i] + " " +
                                             threadIDBuffer[i] );
                        break;
                    default:
                        traceWriter.format( "Unexpected event %d: [%d, %d] thread: %d",
                                            eventTypeBuffer[i],
                                            firstBuffer[i],
                                            secondBuffer[i],
                                            threadIDBuffer[i] );
                        // TODO: throw new IllegalStateException("Unexpected event: " + eventTypeBuffer[i]);
                }
                dimsBuffer[i] = "";
            }
            ptr.set(0);
        } finally {
            mx.unlock();
        }
    }

    protected static int getThreadId(int objHashCode) {
        if (threadIdMap.containsKey(objHashCode)) {
            return threadIdMap.get(objHashCode);
        } else {
            int currThreadId = netxtThreadId.getAndIncrement();
            threadIdMap.put(objHashCode, currThreadId);
            return currThreadId;
        }
    }
}

