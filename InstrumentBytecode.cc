#include "InstrumentBytecode.h"
#include <cassert>
#include <iostream>
#include <utility>

using namespace std;
using namespace jnif;

void instrumentClass(jvmtiEnv *jvmti, JNIEnv *jni, u1 *class_data,
                     jint class_data_len, jint *new_class_data_len,
                     u1 **new_class_data)
{
    static int methodIDCounter = 0;
    
    ClassFile cf(class_data, class_data_len);
    
    ConstIndex instrumentFlagClass = cf.addClass("InstrumentFlag");
    ConstIndex proxyClass = cf.addClass("ETProxy");
    
    // public static void onEntry(long methodID);
    ConstIndex onEntryMethod = cf.addMethodRef(proxyClass, "onEntry", "(I)V");
    
    for (Method *method : cf.methods) {
        // Record the method
        methodTable.insert(make_pair(methodIDCounter, make_pair(string(cf.getThisClassName()),
                                                                method->getName())));
        int methodID = methodIDCounter;
        methodIDCounter++;

        // Inject call to onEntry
        // But be careful not to instrument methods with no bytecode
        // e.g., abstract methods and JNI native methods
        // Also, instrumenting constructors look buggy so let's not do it for the moment
        if (method->hasCode() && !method->isInit()) {
            InstList &instList = method->instList();
            ConstIndex methodIDIndex = cf.addInteger(methodID);
            
            Inst *p = *instList.begin();
            // Stack: ...
            instList.addLdc(OPCODE_ldc_w, methodIDIndex, p);
            // Stack: ... | methodIDIndex
            instList.addInvoke(OPCODE_invokestatic, onEntryMethod, p);
            // Stack: ...instrumentMethodEntry(methodID);
        }
    }
    
    *new_class_data_len = (jint) cf.computeSize();
    jvmtiError error = jvmti->Allocate((jlong) *new_class_data_len, new_class_data);
    assert(!error);
    
    cf.write(*new_class_data, *new_class_data_len);
}
