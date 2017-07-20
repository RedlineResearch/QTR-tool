#ifndef ET2_MAIN_H
#define ET2_MAIN_H

#include <jvmti.h>
#include <jni.h>

extern bool isReady;

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved);
void setCallbacks(jvmtiEnv *jvmti);
void setCapabilities(jvmtiEnv *jvmti);
void setNotificationMode(jvmtiEnv *jvmti);

#endif
