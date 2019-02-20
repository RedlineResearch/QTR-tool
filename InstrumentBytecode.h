#ifndef ET2_INSTRUMENTBYTECODE_H_
#define ET2_INSTRUMENTBYTECODE_H_

#include "ClassTable.h"
#include "class_list.hpp"
#include <jvmti.h>
#include <jni.h>
#include <jnif.hpp>
#include <unordered_map>
#include <utility>

typedef std::unordered_map<long, std::pair<const std::string, const std::string> > MethodTable;

extern bool isReady;
extern MethodTable methodTable;
extern ClassTable classTable;
extern ClassTable fieldTable;

void instrumentClass(jvmtiEnv *jvmti, JNIEnv *jni, jobject loader,
                     jnif::u1 *class_data, jint class_data_len,
                     jint *new_class_data_len, jnif::u1 **new_class_data);


#endif
