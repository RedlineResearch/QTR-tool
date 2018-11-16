// Method test
class Methods {
    private static int x;
    public static void main(String args[]) {
        x = 1;
    }

    private static void Method1() {
        Method2();
    }

    private static void Method2() {
        Method3();
    }

    private static void Method3() {
        x = 42;
    }
}
