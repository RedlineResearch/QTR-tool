public class ShutdownRunnable implements Runnable {

    public void run() {
        System.err.print("SHUTDOWN HOOK:");
        ETProxy.flushBuffer();
        System.err.println("...DONE.");
    }
};


