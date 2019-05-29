package veroy.research.qtrtool.javassist;

public class InstrumentFlag extends ThreadLocal<Boolean>
{
    @Override protected Boolean initialValue() {
        return Boolean.FALSE;
    }
}
