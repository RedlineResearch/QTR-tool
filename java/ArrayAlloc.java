// New call test
class ArrayAlloc {
    private static final int TOTAL = 1000;

    public static void main(String args[]) {
        int[] array = new int[123];
        for (int i = 0; i < 123; i++) {
            array[i] = i;
        }
    }
}
