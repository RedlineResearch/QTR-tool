// New call test
class LotsOfAllocs {
    private static final int TOTAL = 1000;

    public static void main(String args[]) {
        FooClass lastFoo = null;
        for (int i = 0; i < TOTAL; i++) {
            FooClass foo = new FooClass();
            foo.setNext(lastFoo);
        }
    }
}
