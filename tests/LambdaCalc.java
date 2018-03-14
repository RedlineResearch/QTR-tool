import java.util.*;

public class LambdaCalc
{

    private static Var v(String x) { return new Var(x); }
    private static Apply ap(Term m, Term n) { return new Apply(m, n); }
    private static Lam l(String x, Term e) { return new Lam(x, e); }
    
    
    public static void main(String[] args) {
        // \x . (x x) (\y. y) (\z . z)
        // --> (\y . y) (\y . y) (\z . z)
        // --> (\y . y) (\z . z)
        // --> (\z . z)
        Term t = ap(ap(l("x", ap(v("x"), v("x"))), l("y", v("y"))), l("z", v("z")));
        try {
            while (t != null) {
                System.out.println(t);
                t = t.reduce();
            }
        } catch (Exception e) {
            System.out.println(t + " cannot be further reduced");
            System.err.println(e);
        }
                
    }
}

class EvalError extends Exception {
    public EvalError(String message) {
        super(message);
    }

}

// represents lambda terms using named terms
abstract class Term
{
    private static int counter = 0;

    public abstract Term copy() throws EvalError;
    
    public abstract Set<String> freeVars();
    public abstract boolean isVal();
    public abstract boolean reducible();
    public abstract Term reduce() throws EvalError;

    // t[x -> e]
    public abstract Term subst(String x, Term e) throws EvalError;
    public abstract Term rename(String x, String y) throws EvalError;

    public static String freshName(String base) {
        String name = base + counter;
        counter++;
        return name;
    }
}

// x
class Var extends Term
{
    public String name;

    public Var(String x) {
        this.name = x;
    }

    public Var(Term t) throws EvalError {
        if (!(t instanceof Var)) {
            throw new EvalError("Cannot coerce term " + t + " to a variable form");
        } else {
            Var tV = (Var) t;
            this.name = tV.name;
        }
    }

    public Term copy() throws EvalError { return new Var(this); }
    
    public boolean isVal() { return false; }
    public boolean reducible() { return false; }
    
    public Term reduce() {
        return null;
    }

    public Term subst(String x, Term e) throws EvalError {
        if (name.equals(x)) {
            return e.copy();
        } else {
            return new Var(this);
        }
    }

    public Term rename(String x, String y) throws EvalError {
        if (name.equals(x)) {
            return new Var(y);
        } else {
            return new Var(this);
        }
    }

    public Set<String> freeVars() {
        HashSet<String> hs = new HashSet<>();
        hs.add(name);
        return hs;
    }
    
    @Override
    public String toString() {
        return name;
    }

    
}

// M N
class Apply extends Term
{
    Term m, n;
    
    public Apply(Term m, Term n) {
        this.m = m;
        this.n = n;
    }

    public Term copy() { return new Apply(this.m, this.n); }

    public Term reduce() throws EvalError{
        if (m.isVal()) {
            if (n.reducible()) {
                // n --> n'
                // m n --> m n'
                return new Apply(m, n.reduce());
            } else if (m.reducible()) {
                return new Apply(m.reduce(), n);
            } else {
                // [m = (\x . e)] n --> n[x->e]
                // where m.body = e
                Lam mL = (Lam) m;
                return mL.body.subst(mL.name, n);
            }
        } else {
            if (m.reducible()) {
                // m --> m'
                // m n --> m' n
                return new Apply(m.reduce(), n);
            } else {
                throw new EvalError(m + " is not a valid lambda abstraction form");
            }
        }
    }

    public Term subst(String x, Term e) throws EvalError {
        return new Apply(m.subst(x, e), n.subst(x, e));
    }

    public Term rename(String x, String y) throws EvalError {
        return new Apply(m.rename(x, y), n.rename(x, y));
    }

    public Set<String> freeVars() {
        Set<String> fv = m.freeVars();
        fv.addAll(n.freeVars());
        return fv;
    }
    
    public boolean isVal() { return false; }
    public boolean reducible() { return true; }
    
    @Override
    public String toString() {
        return "(" + m + ")" + " (" + n + ")";
    }
}

// \x . M
class Lam extends Term
{
    public String name;
    public Term body;
    
    public Lam(String x, Term m) {
        this.name = x;
        this.body = m;
    }

    public Lam(Lam l) {
        this.name = l.name;
        this.body = l.body;
    }

    public Term copy() { return new Lam(this); }

    public boolean isVal() { return true; }
    public boolean reducible() { return false; }

    public Term reduce() {
        return null;
    }

    public Term subst(String x, Term e) throws EvalError {
        if (name.equals(x)) {
            return new Lam(this);
        } else if (!e.freeVars().contains(name)) {
            return new Lam(name, body.subst(x, e));
        } else {
            Term renamed = this.rename(x, Term.freshName(x));
            return renamed.subst(x, e);
        }
    }

    public Term rename(String x, String y) throws EvalError {
        if (name.equals(x)) {
            return new Lam(y, body.rename(x, y));
        } else {
            return new Lam(name, body.rename(x, y));
        }
    }
    
    public Set<String> freeVars() {
        Set<String> fv =  body.freeVars();
        fv.remove(name);
        return fv;
    }
    
    @Override
    public String toString() {
        return "\\" + name + " . " + body;
    }
}
