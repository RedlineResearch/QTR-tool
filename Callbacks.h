#ifndef ET2_CALLBACKS_H_
#define ET2_CALLBACKS_H_

#include <jvmti.h>
#include <jni.h>

extern bool is_ready;

void JNICALL onMethodEntry(jvmtiEnv *jvmti, JNIEnv *jni, jthread th, jmethodID method);
void JNICALL onMethodExit(jvmtiEnv *jvmti, JNIEnv *jni, jthread th, jmethodID method,
                          jboolean was_popped_by_exception, jvalue return_value);
void JNICALL onVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread th);

#endif
