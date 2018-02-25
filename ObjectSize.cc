#include <iostream>
#include <jni.h>
#include <jni_md.h>
#include <jvmti.h>
#include "main.h"

JNIEXPORT jint JNICALL jniObjectSize(JNIEnv *jni, jclass cls, jobject obj)
{
    // std::cerr << "in jni" << std::endl;
    jlong objSize;
    jvmti->GetObjectSize(obj, &objSize);
    return objSize;
}
