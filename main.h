#ifndef ET2_MAIN_H
#define ET2_MAIN_H

#include "ClassTable.h"
#include <jvmti.h>
#include <jni.h>
#include <unordered_map>
#include <utility>

typedef std::unordered_map<long, std::pair<const std::string, const std::string> > MethodTable;

extern MethodTable methodTable;
extern ClassTable classTable;
extern ClassTable fieldTable;
extern bool isReady;

extern jvmtiEnv *jvmti;

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved);
void setCallbacks(jvmtiEnv *jvmti);
void setCapabilities(jvmtiEnv *jvmti);
void setNotificationMode(jvmtiEnv *jvmti);

#endif
