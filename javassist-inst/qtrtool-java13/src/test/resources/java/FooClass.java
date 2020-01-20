// FooClass 
class FooClass {
    private int x;
    private float y;
    private FooClass next;

    public FooClass() {
        this.x = 42;
        this.y = 14.0f;
        this.next = null;
    }

    public void setX(int newX) {
        this.x = newX;
    }

    public void setY(float newY) {
        this.y = newY;
    }

    public void setNext(FooClass nextFoo) {
        this.next = nextFoo;
    }

    public FooClass getNext() {
        return this.next;
    }
}
