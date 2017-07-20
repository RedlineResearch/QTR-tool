public class ETProxy
{
    private static boolean inInstrumentMethod = false;

    public static void onEntry(String className, String methodName)
    {
        if (inInstrumentMethod) {
            return;
        } else {
            inInstrumentMethod = true;
        }
        
        System.out.println("Class: " + className);
        System.out.println("Mehod: " + methodName);

        inInstrumentMethod = false;
    }
}
