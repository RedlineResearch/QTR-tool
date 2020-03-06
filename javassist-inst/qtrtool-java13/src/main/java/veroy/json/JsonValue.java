package veroy.json;

import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.io.StringWriter;
import java.io.Writer;
import java.util.Collection;
import java.util.Map;

import org.json.simple.parser.JsonParser;
import org.json.simple.parser.ParseException;


/**
 * @author FangYidong<fangyidong@yahoo.com.cn>
 */
public class JsonValue {
    /*
	public static Object parse(Reader in) {
		try{
			JsonParser parser=new JsonParser();
			return parser.parse(in);
		}
		catch (Exception e) {
			return null;
		}
	}

	public static Object parse(String s) {
		StringReader in=new StringReader(s);
		return parse(in);
	}

	public static Object parseWithException(Reader in) throws IOException, ParseException{
		JsonParser parser=new JsonParser();
		return parser.parse(in);
	}

	public static Object parseWithException(String s) throws ParseException{
		JsonParser parser=new JsonParser();
		return parser.parse(s);
	}
   */

	public static void writeJsonString(Object value, Writer out) throws IOException {
		if (value == null) {
			out.write("null");
			return;
		}

		if (value instanceof String) {
            out.write('\"');
			out.write(escape((String) value));
            out.write('\"');
			return;
		}

		if (value instanceof Double) {
			if (((Double) value).isInfinite() || ((Double) value).isNaN()) {
				out.write("null");
            } else {
				out.write(value.toString());
            }
			return;
		}

		if (value instanceof Float) {
			if (((Float) value).isInfinite() || ((Float) value).isNaN()) {
				out.write("null");
            } else {
				out.write(value.toString());
            }
			return;
		}

		if ((value instanceof Number) || (value instanceof Boolean)) {
			out.write(value.toString());
			return;
		}

		if (value instanceof Map) {
			JsonObject.writeJsonString((Map) value, out);
			return;
		}

		if (value instanceof Collection) {
			JsonArray.writeJsonString((Collection) value, out);
            return;
		}

		if (value instanceof byte[]) {
			JsonArray.writeJsonString((byte[]) value, out);
			return;
		}

		if (value instanceof short[]) {
			JsonArray.writeJsonString((short[]) value, out);
			return;
		}

		if (value instanceof int[]) {
			JsonArray.writeJsonString((int[]) value, out);
			return;
		}

		if (value instanceof long[]) {
			JsonArray.writeJsonString((long[]) value, out);
			return;
		}

		if (value instanceof float[]) {
			JsonArray.writeJsonString((float[]) value, out);
			return;
		}

		if (value instanceof double[]) {
			JsonArray.writeJsonString((double[]) value, out);
			return;
		}

		if (value instanceof boolean[]) {
			JsonArray.writeJsonString((boolean[]) value, out);
			return;
		}

		if (value instanceof char[]) {
			JsonArray.writeJsonString((char[]) value, out);
			return;
		}

		if (value instanceof Object[]) {
			JsonArray.writeJsonString((Object[]) value, out);
			return;
		}

		out.write(value.toString());
	}

	public static String toJsonString(Object value) {
		final StringWriter writer = new StringWriter();

		try{
			JsonValue.writeJsonString(value, writer);
			return writer.toString();
		} catch (IOException e) {
			throw new IllegalStateException(e);
		}
	}

	public static String escape(String s) {
		if (s==null) {
			return null;
        }
        StringBuffer sb = new StringBuffer();
        JsonValue.escape(s, sb);
        return sb.toString();
    }

    static void escape(String s, StringBuffer sb) {
    	final int len = s.length();
		for (int i=0; i<len; i++) {
			char ch = s.charAt(i);
			switch (ch) {
			case '"':
				sb.append("\\\"");
				break;
			case '\\':
				sb.append("\\\\");
				break;
			case '\b':
				sb.append("\\b");
				break;
			case '\f':
				sb.append("\\f");
				break;
			case '\n':
				sb.append("\\n");
				break;
			case '\r':
				sb.append("\\r");
				break;
			case '\t':
				sb.append("\\t");
				break;
			case '/':
				sb.append("\\/");
				break;
			default:
				if ((ch>='\u0000' && ch<='\u001F') || (ch>='\u007F' && ch<='\u009F') || (ch>='\u2000' && ch<='\u20FF')) {
					String ss=Integer.toHexString(ch);
					sb.append("\\u");
					for (int k=0; k<4-ss.length(); k++) {
						sb.append('0');
					}
					sb.append(ss.toUpperCase());
				} else {
					sb.append(ch);
				}
			}
		}
	}
}
