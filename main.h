#ifndef ET2_MAIN_H
#define ET2_MAIN_H

#include <jvmti.h>
#include <jni.h>
#include <map>
#include <utility>

typedef std::map<long, std::pair<const std::string, const std::string> > MethodTable;

extern bool isReady;
extern MethodTable methodTable;

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved);
void setCallbacks(jvmtiEnv *jvmti);
void setCapabilities(jvmtiEnv *jvmti);
void setNotificationMode(jvmtiEnv *jvmti);

#endif
