#include "Callbacks.h"
#include "InstrumentBytecode.h"
#include "ETProxy_class.h"
#include "InstrumentFlag_class.h"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace std;

void JNICALL onVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread th)
{
    cerr << "READY!" << endl; 
    isReady = true;
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
    if (!isReady || strcmp(class_name, "java/util/jar/JarFile") == 0) {
        cerr << "Not instrumenting: " << class_name << endl;
        return;
    }
    
    instrumentClass(jvmti, jni, loader, (unsigned char *) class_data, class_data_len,
                    new_class_data_len, new_class_data); 
}

void JNICALL onMethodEntry(jvmtiEnv *jvmti, JNIEnv *jni, jthread th, jmethodID method)
{
    if (!isReady) {
        return;
    }
    
    (void) th;
    (void) jni;



    char *name_ptr = NULL;
    char *signature_ptr = NULL;

    jvmti->GetMethodName(method, &name_ptr, &signature_ptr, NULL);
    assert(name_ptr);
    assert(signature_ptr);
    
    cerr << "Method entered: " << name_ptr << " / signature: " << signature_ptr << endl;

    jvmti->Deallocate((unsigned char *) name_ptr);
    jvmti->Deallocate((unsigned char *) signature_ptr);
}

void JNICALL onMethodExit(jvmtiEnv *jvmti, JNIEnv *jni, jthread th, jmethodID method,
                          jboolean was_popped_by_exception, jvalue return_value)
{
    if (!isReady) {
        return;
    }
    
    (void) th;
    (void) return_value;

    if (was_popped_by_exception) {
        cerr << "POPPED BY EXCEPTION ";
    }

    char *name_ptr = NULL;

    jvmti->GetMethodName(method, &name_ptr, NULL, NULL);
    assert(name_ptr);
    
    cerr << "Method exited: " << name_ptr << endl;

    jvmti->Deallocate((unsigned char *) name_ptr);
}

static void flushMethodTable(MethodTable methodTable)
{
    cout << endl;
    
    for (auto it = methodTable.begin(); it != methodTable.end(); ++it) {
        long methodID = it->first;
        string className = (it->second).first;
        string methodName = (it->second).second;

        cout << methodID << ", " << className << ", " << methodName << endl;
    }
}


void JNICALL flushBuffers(jvmtiEnv *jvmti, JNIEnv *jni)
{
    // Flush the method call timestamp buffer first
    jclass proxyClass = jni->FindClass("ETProxy");
    jmethodID flushBuffer = jni->GetStaticMethodID(proxyClass, "flushBuffer", "()V");
    jni->CallStaticVoidMethod(proxyClass, flushBuffer);

    // Then flush the rest of the timestamp table
    flushMethodTable(methodTable);
    classTable.dumpTable();
    fieldTable.dumpTable();
}
