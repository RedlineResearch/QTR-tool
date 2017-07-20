#include "InstrumentBytecode.h"
#include <cassert>

using namespace std;
using namespace jnif;

void instrumentClass(jvmtiEnv *jvmti, JNIEnv *jni, u1 *class_data,
                     jint class_data_len, jint *new_class_data_len,
                     u1 **new_class_data)
{   
    ClassFile cf(class_data, class_data_len);

    ConstIndex proxyClass = cf.addClass("ETProxy");

    // public static void onEntry(String className, String methodName);
    ConstIndex onEntryMethod = cf.addMethodRef(proxyClass, "onEntry",
                                               "(Ljava/lang/String;Ljava/lang/String;)V");
    
    ConstIndex classNameIdx = cf.addStringFromClass(cf.thisClassIndex);
    for (Method *method : cf.methods) {
        // Inject call to onEntry
        InstList &instList = method->instList();
        ConstIndex methodNameIndex = cf.addString(method->nameIndex);

        Inst *p = *instList.begin();

        // Stack: ...
        instList.addLdc(OPCODE_ldc_w, classNameIdx, p);
        // Stack: ... | className
        instList.addLdc(OPCODE_ldc_w, methodNameIndex, p);
        // Stack: ... | className | methodName
        instList.addInvoke(OPCODE_invokestatic, onEntryMethod, p);
        // Stack: ...instrumentMethodEntry(cf, method);
    }
    
    *new_class_data_len = (jint) cf.computeSize();
    jvmtiError error = jvmti->Allocate((jlong) *new_class_data_len, new_class_data);
    assert(!error);
    
    cf.write(*new_class_data, *new_class_data_len);
}
