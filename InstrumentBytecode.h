#ifndef ET2_INSTRUMENTBYTECODE_H_
#define ET2_INSTRUMENTBYTECODE_H_

#include <jvmti.h>
#include <jni.h>
#include <jnif.hpp>

void instrumentClass(jvmtiEnv *jvmti, JNIEnv *jni, jnif::u1 *class_data,
                     jint class_data_len, jint *new_class_data_len,
                     jnif::u1 **new_class_data);

#endif
