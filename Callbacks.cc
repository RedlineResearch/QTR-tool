#include "Callbacks.h"
#include "InstrumentBytecode.h"
#include "ETProxy_class.h"
#include "InstrumentFlag_class.h"
#include "ObjectSize.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <jnif.hpp>

using namespace std;

// void JNICALL onMethodEntry(jvmtiEnv *jvmti, JNIEnv *jni, jthread th, jmethodID method)
// {
//     unsigned char *name;
//     // not safe, but I think no one has a 201 byte class name!
//     jvmtiError error = jvmti->Allocate(200, &name);
//     assert(!error);
    
//     error = jvmti->GetMethodName(method, (char**) &name, nullptr, nullptr);
//     assert(!error);

//     // do this until we get to main
//     if (string((char*) name) == "main") {
//         // cerr << "yay main" << endl;
//         jvmtiEventCallbacks callbacks;
    
//         (void) memset(&callbacks, 0, sizeof(callbacks));

//         callbacks.VMStart = &loadProxyClass;
//         // disable method entry callback from now on
//         // callbacks.MethodEntry = &onMethodEntry;
//         // callbacks.MethodExit = &onMethodExit;
//         callbacks.VMInit = &onVMInit;
//         callbacks.ClassFileLoadHook = &onClassFileLoad;
//         callbacks.VMDeath = &flushBuffers;
    
//         jvmtiError error = jvmti->SetEventCallbacks(&callbacks, (jint) sizeof(callbacks));
//         assert(!error);
//     }
    
//     error = jvmti->Deallocate(name);
//     assert(!error);
// }

void JNICALL onVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread th)
{
    isReady = true;
}

void JNICALL onVMStart(jvmtiEnv *jvmti, JNIEnv *jni)
{
    // cerr << "VM Starting..." << endl;
    
    jni->DefineClass("InstrumentFlag", NULL, (jbyte *) InstrumentFlag_class, (jsize) InstrumentFlag_class_len);
    jclass etProxyClass = jni->DefineClass("ETProxy", NULL, (jbyte *) ETProxy_class, (jsize) ETProxy_class_len);
    
    JNINativeMethod jniMethod;

    jniMethod.name = (char *) "getObjectSize";
    jniMethod.signature = (char *) "(Ljava/lang/Object;)I";
    jniMethod.fnPtr = (void *) jniObjectSize;

    // cerr << "hello?" << endl;
    
    jint suc = jni->RegisterNatives(etProxyClass, &jniMethod, 1);
    assert(suc == 0);

    // cerr << "Register natives seem successful" << endl;
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
    if (!isReady || string(class_name).find("java/util") == 0) { return; }
    
    if (string(class_name).find("sun") == 0) {
        return;
    }

    try {
        instrumentClass(jvmti, jni, loader, (unsigned char *) class_data, class_data_len,
                        new_class_data_len, new_class_data);
    } catch (jnif::Exception &e) {
        cerr << e.message << endl;
        throw;
    }
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
