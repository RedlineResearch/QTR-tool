#include "main.h"
#include "Callbacks.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace std;

bool isReady = false;
MethodTable methodTable;

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
    // cerr << "Instrumenting..." << endl;
    
    jvmtiEnv *jvmti;
    jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_2);

    setCapabilities(jvmti);
    setNotificationMode(jvmti);
    setCallbacks(jvmti);
    return JNI_OK;
}

void setNotificationMode(jvmtiEnv *jvmti)
{

    jvmtiError error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, NULL);
    assert(!error);
    
    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);
    assert(!error);
    
    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);
    assert(!error);

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, NULL);
    assert(!error);

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
    assert(!error);
}

void setCapabilities(jvmtiEnv *jvmti)
{
    jvmtiCapabilities capabilities;
    
    (void) memset(&capabilities, 0, sizeof(capabilities));
    capabilities.can_generate_method_entry_events = 1;
    capabilities.can_generate_method_exit_events = 1;
    capabilities.can_generate_all_class_hook_events	= 1;
    
    jvmtiError error = jvmti->AddCapabilities(&capabilities);
    assert(!error);
}

void setCallbacks(jvmtiEnv *jvmti)
{
    jvmtiEventCallbacks callbacks;
    
    (void) memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMStart = &loadProxyClass;
    // callbacks.MethodEntry = &onMethodEntry;
    // callbacks.MethodExit = &onMethodExit;
    callbacks.VMInit = &onVMInit;
    callbacks.ClassFileLoadHook = &onClassFileLoad;
    
    jvmtiError error = jvmti->SetEventCallbacks(&callbacks, (jint) sizeof(callbacks));
    assert(!error);
}
