#include "InstrumentBytecode.h"
#include <cassert>
#include <iostream>
#include <utility>

using namespace std;
using namespace jnif;

ClassHierarchy classHierarchy;

class ClassNotLoadedException {
public:

	ClassNotLoadedException(const String &className) :
        className(className) {}

	String className;

};

class ClassPath: public IClassPath {
public:
	ClassPath(JNIEnv* jni, jobject loader) :
        jni(jni), loader(loader) {
	}

	String getCommonSuperClass(const String& className1, const String& className2) {

        // cerr << "Finding common super class: " << className1 << " and " << className2 << endl;
        
		if (!isReady) {
			return "java/lang/Object";
		}

		try {
			loadClassIfNotLoaded(className1);
			loadClassIfNotLoaded(className2);

			String sup = className1;
			const String& sub = className2;

			while (!isAssignableFrom(sub, sup)) {
				loadClassIfNotLoaded(sup);
				sup = classHierarchy.getSuperClass(sup);
				if (sup == "0") {
					return "java/lang/Object";
				}

			}

			return sup;
		} catch (const ClassNotLoadedException& e) {
            // cerr << "Not loaded: " << e.className << endl;
			String res = "java/lang/Object";
			return res;
		}
	}

	bool isAssignableFrom(const string& sub, const string& sup) {
		string cls = sub;
		while (cls != "0") {
			if (cls == sup) {
				return true;
			}

			loadClassIfNotLoaded(cls);
			cls = classHierarchy.getSuperClass(cls);
		}

		return false;
	}

	static void initProxyClass(JNIEnv* jni) {
		if (proxyClass == NULL) {
			proxyClass = jni->FindClass("ETProxy");

            assert(proxyClass);

			getResourceId = jni->GetStaticMethodID(proxyClass, "getResource",
                                                   "(Ljava/lang/String;Ljava/lang/ClassLoader;)[B");

            assert(getResourceId);
            
			proxyClass = (jclass) jni->NewGlobalRef(proxyClass);

            assert(proxyClass);
		}
	}

private:

	void loadClassIfNotLoaded(const String& className) {
		if (!classHierarchy.isDefined(className)) {
			loadClassAsResource(className);
		}
	}

	void loadClassAsResource(const String& className) {

		ClassPath::initProxyClass(jni);

		jstring targetName = jni->NewStringUTF(className.c_str());

		jobject res = jni->CallStaticObjectMethod(proxyClass, getResourceId,
                                                  targetName, loader);

		if (res == NULL) {
			throw ClassNotLoadedException(className);
		}

		jsize len = jni->GetArrayLength((jarray) res);

		u1* bytes = (u1*) jni->GetByteArrayElements((jbyteArray) res, NULL);

		ClassFile cf(bytes, len);

		jni->ReleaseByteArrayElements((jbyteArray) res, (jbyte*) bytes,
                                      JNI_ABORT);
		jni->DeleteLocalRef(res);
		jni->DeleteLocalRef(targetName);

		classHierarchy.addClass(cf);
	}

	//const char* className;
	JNIEnv* jni;
	jobject loader;

	static jclass proxyClass;
	static jmethodID getResourceId;
};

jclass ClassPath::proxyClass = NULL;
jmethodID ClassPath::getResourceId = NULL;

static void instrumentMethodEntry(ClassFile &cf, Method *method, ConstIndex &methodIDIndex, ConstIndex &instrMethod);
static void instrumentMethodExit(ClassFile &cf, Method *method, ConstIndex &methodIDIndex, ConstIndex &instrMethod);
static void instrumentObjectAlloc(ClassFile &cf, Method *method, ConstIndex &instrMethod);
static void instrumentObjectArrayAlloc(ClassFile &cf, Method *method, ConstIndex &instrMethod);
static void instrumentPrimitiveArrayAlloc(ClassFile &cf, Method *method, ConstIndex &instrMethod);
static void instrumentMultiArrayAlloc(ClassFile &cf, Method *method, ConstIndex &instrMethod);
static void instrumentPutField(ClassFile &cf, Method *method, ConstIndex &instrMethod);

enum PrimitiveType {
    BYTE = 8,
    CHAR = 5,
    DOUBLE = 7,
    FLOAT = 6,
    INT = 10,
    LONG = 11,
    SHORT = 9,
    BOOLEAN = 4
};

void instrumentClass(jvmtiEnv *jvmti, JNIEnv *jni, jobject loader,
                     u1 *class_data, jint class_data_len,
                     jint *new_class_data_len, u1 **new_class_data)
{
    static int methodIDCounter = 1;
    
    ClassFile cf(class_data, class_data_len);

    cerr << "Loading: " << cf.getThisClassName() << endl;

    classHierarchy.addClass(cf);
    classTable.mapOrFind(string(cf.getThisClassName()));
    
    ConstIndex instrumentFlagClass = cf.addClass("InstrumentFlag");
    ConstIndex proxyClass = cf.addClass("ETProxy");
    
    // public static void onEntry(int methodID);
    ConstIndex onEntryMethod = cf.addMethodRef(proxyClass, "onEntry", "(I)V");

    // public static void onExit(int methodID);
    ConstIndex onExitMethod = cf.addMethodRef(proxyClass, "onExit", "(I)V");

    // public static void onObjectAlloc(int allocdClassID, Object allocdObject);
    ConstIndex onAllocMethod = cf.addMethodRef(proxyClass, "onObjectAlloc", "(ILjava/lang/Object;)V");

    // public static void onObjectArrayAlloc(int allocdClassID, int size, Object[] allocdArray);
    ConstIndex onAllocObjArrayMethod = cf.addMethodRef(proxyClass, "onObjectArrayAlloc", "(II[Ljava/lang/Object;)V");

    // exploiting the fact that arrays are objects
    // public static void on(Type)ArrayAlloc(int atype, int size, Object allocdArray);
    ConstIndex onAllocPrimArrayMethod = cf.addMethodRef(proxyClass, "onPrimitiveArrayAlloc", "(IILjava/lang/Object;)V");

    // public static void on2DArrayAlloc(int size1, int size2, Object allocdArray, int arrayClassID)
    ConstIndex onAlloc2DArrayMethod = cf.addMethodRef(proxyClass, "on2DArrayAlloc", "(IILjava/lang/Object;I)V");
    
    // public static void onPutField(Object tgtObject, Object srcObject, int fieldID);
    ConstIndex onPutFieldMethod = cf.addMethodRef(proxyClass, "onPutField","(Ljava/lang/Object;Ljava/lang/Object;I)V");

    for (Method *method : cf.methods) {
        // Record the method
        methodTable.insert(make_pair(methodIDCounter, make_pair(string(cf.getThisClassName()),
                                                                method->getName())));
        int methodID = methodIDCounter;
        methodIDCounter++;

        if (method->hasCode()) {
            ConstIndex methodIDIndex = cf.addInteger(methodID);
            instrumentMethodEntry(cf, method, methodIDIndex, onEntryMethod);
            instrumentMethodExit(cf, method, methodIDIndex, onExitMethod);
            instrumentObjectAlloc(cf, method, onAllocMethod);
            instrumentObjectArrayAlloc(cf, method, onAllocObjArrayMethod);
            instrumentPrimitiveArrayAlloc(cf, method, onAllocPrimArrayMethod);
            instrumentMultiArrayAlloc(cf, method, onAlloc2DArrayMethod);
            instrumentPutField(cf, method, onPutFieldMethod);
        }
    }

    ClassPath cp(jni, loader);
    // TestClassPath cp;
    
    cf.computeFrames(&cp);
    *new_class_data_len = (jint) cf.computeSize();
    jvmtiError error = jvmti->Allocate((jlong) *new_class_data_len, new_class_data);
    assert(!error);
    
    cf.write(*new_class_data, *new_class_data_len);
}

static void instrumentPutField(ClassFile &cf, Method *method, ConstIndex &instrMethod)
{
    InstList &instList = method->instList();

    for (Inst *inst : instList) {
        if (inst->opcode == OPCODE_putfield) {
            ConstIndex targetFieldRefIndex = inst->field()->fieldRefIndex;
            string targetClassName, targetFieldName, fieldDesc;
            cf.getFieldRef(targetFieldRefIndex, &targetClassName, &targetFieldName, &fieldDesc);
            
            // Only worry about objects for now
            if (fieldDesc[0] == 'L') {
                // Foo.bar becomes Foo/bar
                string fieldString = targetClassName + "/" + targetFieldName;
                int targetFieldID = fieldTable.mapOrFind(fieldString);

                // Stack: ... | tgtObjectRef | srcObjectRef
                instList.addZero(OPCODE_dup2, inst);
                // Stack: ... | tgtObjectRef | srcObjectRef | tgtObjectRef | srcObjectRef

                ConstIndex fieldIDIndex = cf.addInteger(targetFieldID);
                instList.addLdc(OPCODE_ldc_w, fieldIDIndex, inst);
                // Stack: ... | tgtObjectRef | srcObjectRef | tgtObjectRef | srcObjectRef | fieldID

                instList.addInvoke(OPCODE_invokestatic, instrMethod, inst);
                // Stack: ... | tgtObjectRef | srcObjectRef
                // Stack preserved
            }
        }
    }
}

static void instrumentMethodEntry(ClassFile &cf, Method *method, ConstIndex &methodIDIndex,
                                  ConstIndex &instrMethod)
{
    InstList &instList = method->instList();
            
    Inst *p = *instList.begin();
    // Stack: ...
    instList.addLdc(OPCODE_ldc_w, methodIDIndex, p);
    // Stack: ... | methodIDIndex
    instList.addInvoke(OPCODE_invokestatic, instrMethod, p);
    // Stack: ...
    // Stack preserved
}

static void instrumentMethodExit(ClassFile &cf, Method *method, ConstIndex &methodIDIndex,
                                 ConstIndex &instrMethod)
{
    InstList &instList = method->instList();

    for (Inst *inst : instList) {
        if (inst->isExit()) {
            instList.addLdc(OPCODE_ldc_w, methodIDIndex, inst);
            instList.addInvoke(OPCODE_invokestatic, instrMethod, inst);
        }
    }
}

static void instrumentObjectAlloc(ClassFile &cf, Method *method, ConstIndex &instrMethod)
{
    InstList &instList = method->instList();
    
    for (Inst *inst : instList) {
        if (inst->opcode == OPCODE_new) {
            // Stack: ...
            Inst *p = inst->next;
            // Stack: ... | objRef
            
            instList.addZero(OPCODE_dup, p);
            // Stack: ... | objRef | objRef
            
            ConstIndex allocatedClassIndex = inst->type()->classIndex;
            int classID = classTable.mapOrFind(string(cf.getClassName(allocatedClassIndex)));            
            ConstIndex classIDIndex = cf.addInteger(classID);
            
            instList.addLdc(OPCODE_ldc_w, classIDIndex, p);
            // Stack: ... | objRef | objRef | classID

            instList.addZero(OPCODE_swap, p);
            // Stack: ... | objRef | classID | objRef
            
            instList.addInvoke(OPCODE_invokestatic, instrMethod, p);
            // Stack: ... | objRef
            // Stack preserved
        }
    }
}

static void instrumentObjectArrayAlloc(ClassFile &cf, Method *method, ConstIndex &instrMethod)
{
    InstList &instList = method->instList();
    
    for (Inst *inst : instList) {
        if (inst->opcode == OPCODE_anewarray) {
            // Stack: ... | size

            ConstIndex allocatedClassIndex = inst->type()->classIndex;
            int classID = classTable.mapOrFind(string(cf.getClassName(allocatedClassIndex)));            
            ConstIndex classIDIndex = cf.addInteger(classID);

            instList.addLdc(OPCODE_ldc_w, classIDIndex, inst);
            // Stack: ... | size | classID

            instList.addZero(OPCODE_swap, inst);
            // Stack: ... | classID | size
            
            instList.addZero(OPCODE_dup, inst);
            // Stack: ... | classID | size | size
            
            Inst *p = inst->next;
            // Stack: ... | classID | size | arrayRef
            
            instList.addZero(OPCODE_dup_x2, p);
            // Stack: ... | arrayRef | classID | size | arrayRef
            
            instList.addInvoke(OPCODE_invokestatic, instrMethod, p);
            // Stack: ... | arrayRef
            // Stack preserved
        }
    }   
}

static void instrumentPrimitiveArrayAlloc(ClassFile &cf, Method *method, ConstIndex &instrMethod)
{
    InstList &instList = method->instList();
    
    for (Inst *inst : instList) {
        if (inst->opcode == OPCODE_newarray) {
            // Stack: ... | size

            u1 atype = inst->newarray()->atype;
            instList.addBiPush(atype, inst);
            // Stack: ... | size | atype

            instList.addZero(OPCODE_swap, inst);
            // Stack: ... | atype | size
            
            instList.addZero(OPCODE_dup, inst);
            // Stack: ... | atype | size | size
            
            Inst *p = inst->next;
            // Stack: ... | atype | size | arrayRef
            
            instList.addZero(OPCODE_dup_x2, p);
            // Stack: ... | arrayRef | atype | size | arrayRef

            // Yes, arrays are objects!
            instList.addInvoke(OPCODE_invokestatic, instrMethod, p);
            // Stack: ... | arrayRef
            // Stack preserved
        }
    }   
}

static void instrumentMultiArrayAlloc(ClassFile &cf, Method *method, ConstIndex &instrMethod)
{
    InstList &instList = method->instList();
    
    for (Inst *inst : instList) {
        if (inst->opcode == OPCODE_multianewarray) {
            // Stack: ... | size1 | size2 | ... | sizeN

            ConstIndex allocatedClassIndex = inst->multiarray()->classIndex;
            int classID = classTable.mapOrFind(string(cf.getClassName(allocatedClassIndex)));            
            ConstIndex classIDIndex = cf.addInteger(classID);

            u1 dims = inst->multiarray()->dims;

            if (dims == 2) {
                // Stack: ... | size1 | size2
                instList.addZero(OPCODE_dup2, inst);
                // Stack: ... | size1 | size2 | size1 | size2

                Inst *p = inst->next;
                // Stack: ... | size1 | size2 | arrayRef

                instList.addZero(OPCODE_dup_x2, p);
                // Stack: ... | arrayRef | size1 | size2 | arrayRef
                
                instList.addLdc(OPCODE_ldc_w, classIDIndex, p);
                // Stack: ... | arrayRef | size1 | size2 | arrayRef | arrayClassID

                instList.addInvoke(OPCODE_invokestatic, instrMethod, p);
                // Stack: ... | arrayRef
                // Stack preserved
                
            } else { // I don't know how to do arrays with more dimensions yet
                cerr << "Large array of size " << dims << "allocated, "
                     << "array type of " << cf.getClassName(allocatedClassIndex)
                     << ". Not instrumented" << endl;
            }
        }
    }
}
