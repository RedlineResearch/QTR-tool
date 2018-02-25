#ifndef ET2_CALLBACKS_H_
#define ET2_CALLBACKS_H_

#include "ClassTable.h"
#include <jvmti.h>
#include <jni.h>
#include <unordered_map>

typedef std::unordered_map<long, std::pair<const std::string, const std::string> > MethodTable;

extern bool isReady;
extern MethodTable methodTable;
extern ClassTable classTable, fieldTable;

void JNICALL onVMStart(jvmtiEnv *jvmti, JNIEnv *jni);
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
void JNICALL flushBuffers(jvmtiEnv *jvmti, JNIEnv *jni);

#endif
