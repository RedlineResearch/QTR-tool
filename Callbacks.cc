#include "Callbacks.h"
#include <cassert>
#include <iostream>

using namespace std;

void JNICALL onMethodEntry(jvmtiEnv *jvmti, JNIEnv *jni, jthread th, jmethodID method)
{
    if (!is_ready) {
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
    if (!is_ready) {
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

void JNICALL onVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread th)
{
    cerr << "READY!" << endl; 
    is_ready = true;
}
