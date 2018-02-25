#ifndef ET2_OBJSIZE_H_
#define ET2_OBJSIZE_H_

#include <jni.h>
#include <jni_md.h>

JNIEXPORT jint JNICALL jniObjectSize(JNIEnv *jni, jclass cls, jobject obj);

#endif
