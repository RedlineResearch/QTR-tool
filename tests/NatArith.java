public class NatArith {

    public static void main(String[] args) {
        Nat m = Nat.fromInt(1921);
        System.out.println(m);
        Nat n = Nat.fromInt(3385);
        System.out.println(n);
        
        System.out.println("1921 + 3385 = " + m.plus(n));
    } 
}

abstract class Nat {
    public static Nat fromInt(int i) {
        if (i == 0) {
            return new Zero();
        } else {
            return new Succ(i-1);
        }
    }
    
    public Nat succ() { return new Succ(this); }
    public abstract Nat plus(Nat n);
    public abstract Nat minus(Nat n);
    public abstract int toInt();

    @Override
    public String toString() {
        return Integer.toString(this.toInt());
    }
}

class Zero extends Nat {   
    public Zero() { return; }
    public Nat plus(Nat n) { return n; }
    public Nat minus(Nat n) { return this; }
    public int toInt() { return 0; }
}

class Succ extends Nat {
    final private Nat prev;
    
    public Succ(Nat n) {
        prev = n;
    }

    public Succ(int i) {
        assert i >= 0;
        if (i == 0) {
            prev = new Zero();
        } else {
            prev = new Succ(i-1);
        }
    }

    public Nat getPrev() {
        return prev;
    }
    
    public Nat plus(Nat n) {
        if (n instanceof Zero) {
            return this;
        } else {
            Succ ns = (Succ) n;
            return this.succ().plus(ns.getPrev());
        }
    }

    public Nat minus(Nat n) {
        if (n instanceof Zero) {
            return n;
        } else {
            Succ ns = (Succ) n;
            return this.getPrev().minus(ns.getPrev());
        }
    }

    public int toInt() {
        return 1 + this.getPrev().toInt();
    }
}
