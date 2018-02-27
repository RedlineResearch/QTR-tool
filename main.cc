#include "main.h"
#include "Callbacks.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace std;

bool isReady = false;
MethodTable methodTable;
ClassTable classTable, fieldTable;

jvmtiEnv *jvmti = nullptr;

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
    #ifdef DEBUG
    cerr << "Agent loaded" << endl;
    #endif
    
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

    error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, NULL);
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
    #ifdef DEBUG
    cerr << "Setting callbacks" << endl;
    #endif
    
    jvmtiEventCallbacks callbacks;
    
    (void) memset(&callbacks, 0, sizeof(callbacks));

    callbacks.VMStart = &onVMStart;
    callbacks.MethodEntry = &onMethodEntry;
    // callbacks.MethodExit = &onMethodExit;
    callbacks.VMInit = &onVMInit;

    // we don't start rewriting bytecode until we've hit main
    callbacks.ClassFileLoadHook = &onClassFileLoad;
    callbacks.VMDeath = &flushBuffers;
    
    jvmtiError error = jvmti->SetEventCallbacks(&callbacks, (jint) sizeof(callbacks));
    assert(!error);
}
