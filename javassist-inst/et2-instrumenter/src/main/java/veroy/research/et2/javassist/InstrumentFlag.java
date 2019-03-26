package veroy.research.et2.javassist;

public class InstrumentFlag extends ThreadLocal<Boolean>
{
    @Override protected Boolean initialValue() {
        return Boolean.FALSE;
    }
}
