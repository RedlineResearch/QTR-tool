#ifndef ET2_CALLBACKS_H_
#define ET2_CALLBACKS_H_

#include <jvmti.h>
#include <jni.h>

extern bool isReady;
void JNICALL onClassFileLoad(jvmtiEnv *jvmti,
                             JNIEnv *jni,
                             jclass class_being_redefined,
                             jobject loader,
                             const char *class_name,
                             jobject protection_domain,
                             jint class_data_len,
                             const unsigned char *class_data,
                             jint *new_class_data_len,
                             unsigned char **new_class_data);
void JNICALL onMethodEntry(jvmtiEnv *jvmti, JNIEnv *jni, jthread th, jmethodID method);
void JNICALL onMethodExit(jvmtiEnv *jvmti, JNIEnv *jni, jthread th, jmethodID method,
                          jboolean was_popped_by_exception, jvalue return_value);
void JNICALL onVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread th);

#endif
