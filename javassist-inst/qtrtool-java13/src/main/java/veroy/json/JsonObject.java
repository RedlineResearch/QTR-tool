package veroy.json;

import java.io.IOException;
import java.io.StringWriter;
import java.io.Writer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class JsonObject extends HashMap implements Map {

	private static final long serialVersionUID = 1L;

	public JsonObject(Map map) {
		super(map);
	}

	public static void writeJsonAsString(Map map, Writer out) throws IOException {
		if (map == null) {
			out.write("null");
			return;
		}
		boolean first = true;
		Iterator iter=map.entrySet().iterator();
        out.write('{');
		while (iter.hasNext()) {
            if (first) {
                first = false;
            } else {
                out.write(',');
            }
			Map.Entry entry=(Map.Entry)iter.next();
            out.write('\"');
            out.write(escape(String.valueOf(entry.getKey())));
            out.write('\"');
            out.write(':');
			JsonValue.writeJsonString(entry.getValue(), out);
		}
		out.write('}');
	}

	public void writeJsonString(Writer out) throws IOException {
		writeJsonString(this, out);
	}

	public static String toJsonString(Map map) {
		final StringWriter writer = new StringWriter();
		try {
			writeJsonString(map, writer);
			return writer.toString();
		} catch (IOException e) {
			// This should never happen with a StringWriter
			throw new IllegalStateException(e);
		}
	}

	public String toJsonString() {
		return toJsonString(this);
	}

	public String toString() {
		return toJsonString();
	}

	public static String toString(String key, Object value) {
        StringBuffer sb = new StringBuffer();
        sb.append('\"');
        if (key == null) {
            sb.append("null");
        } else {
            JsonValue.escape(key, sb);
        }
		sb.append('\"').append(':');
		sb.append(JsonValue.toJsonString(value));

		return sb.toString();
	}

	public static String escape(String s) {
		return JsonValue.escape(s);
	}
}
