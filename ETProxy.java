public class ETProxy
{
    private static final InstrumentFlag inInstrumentMethod = new InstrumentFlag();
    
    public static void onEntry(int methodID)
    {
        if (inInstrumentMethod.get()) {
            return;
        } else {
            inInstrumentMethod.set(true);
        }
        
        System.out.println("Method ID: " + methodID);

        inInstrumentMethod.set(false);
    }
}
