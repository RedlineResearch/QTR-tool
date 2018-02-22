#include "Callbacks.h"
#include "InstrumentBytecode.h"
#include "ETProxy_class.h"
#include "InstrumentFlag_class.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>

using namespace std;

void JNICALL onVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread th)
{
    // cerr << "READY!" << endl; 
    isReady = true;
    return;
}

void JNICALL loadProxyClass(jvmtiEnv *jvmti, JNIEnv *jni)
{
    jni->DefineClass("InstrumentFlag", NULL, (jbyte *) InstrumentFlag_class, (jsize) InstrumentFlag_class_len);
    jni->DefineClass("ETProxy", NULL, (jbyte *) ETProxy_class, (jsize) ETProxy_class_len);
}

void JNICALL onClassFileLoad(jvmtiEnv *jvmti,
                             JNIEnv *jni,
                             jclass class_being_redefined,
                             jobject loader,
                             const char *class_name,
                             jobject protection_domain,
                             jint class_data_len,
                             const unsigned char *class_data,
                             jint *new_class_data_len,
                             unsigned char **new_class_data)
{
    if (!isReady) {
        return;
    }
    
    instrumentClass(jvmti, jni, loader, (unsigned char *) class_data, class_data_len,
                    new_class_data_len, new_class_data); 
}

static void flushMethodTable(MethodTable &methodTable, ofstream &methodDataF)
{    
    for (auto it = methodTable.begin(); it != methodTable.end(); ++it) {
        long methodID = it->first;
        string className = (it->second).first;
        string methodName = (it->second).second;

        methodDataF << methodID << ", " << className << ", " << methodName << endl;
    }
}


void JNICALL flushBuffers(jvmtiEnv *jvmti, JNIEnv *jni)
{
    // Flush the method call timestamp buffer first
    jclass proxyClass = jni->FindClass("ETProxy");
    jmethodID flushBuffer = jni->GetStaticMethodID(proxyClass, "flushBuffer", "()V");
    jni->CallStaticVoidMethod(proxyClass, flushBuffer);

    ofstream methodDataF, classDataF, fieldDataF;
    methodDataF.open("methods.list");
    classDataF.open("classes.list");
    fieldDataF.open("fields.list");
    
    // Then flush the rest of the timestamp table
    flushMethodTable(methodTable, methodDataF);
    classTable.dumpTable(classDataF);
    fieldTable.dumpTable(fieldDataF);

    methodDataF.close();
    classDataF.close();
    fieldDataF.close();
}
