#include "InstrumentBytecode.h"
// #include <ClassHierarchy.hpp>
#include <cassert>
#include <iostream>
#include <utility>

using namespace std;
using namespace jnif;
using namespace jnif::model;

ClassHierarchy classHierarchy;

class ClassNotLoadedException {
public:

	ClassNotLoadedException(const string &className) :
        className(className) {}

	string className;

};

class ClassPath: public IClassPath {
public:
	ClassPath(JNIEnv* jni, jobject loader) :
        thisJni(jni), loader(loader)
    {
        if (loader != nullptr) {
            inLivePhase = true;
        }
    }

    string getCommonSuperClass(const string& className1, const string& className2) {

        // cerr << "Finding common super class: " << className1 << " and " << className2 << endl;
        
		if (!inLivePhase) {
			return "java/lang/Object";
		}

		try {
			loadClassIfNotLoaded(className1);
			loadClassIfNotLoaded(className2);

			string sup = className1;
			const string& sub = className2;

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
			string res = "java/lang/Object";
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

	static void initProxyClass(JNIEnv* theJni) {
		if (proxyClass == NULL) {
			proxyClass = theJni->FindClass("ETProxy");

            if (!proxyClass) {
                cerr << "ETProxy not found" << endl;
            }
            
            assert(proxyClass);

			getResourceId = theJni->GetStaticMethodID(proxyClass, "getResource",
                                                   "(Ljava/lang/String;Ljava/lang/ClassLoader;)[B");

            assert(getResourceId);
            
			proxyClass = (jclass) theJni->NewGlobalRef(proxyClass);

            assert(proxyClass);
		}
	}

private:

	void loadClassIfNotLoaded(const string& className) {
		if (!classHierarchy.isDefined(className)) {
			loadClassAsResource(className);
		}
	}

	void loadClassAsResource(const string& className) {

        // cerr << "in class: " << className << endl;
        
		ClassPath::initProxyClass(thisJni);

		jstring targetName = thisJni->NewStringUTF(className.c_str());

		jobject res = thisJni->CallStaticObjectMethod(proxyClass, getResourceId,
                                                  targetName, loader);

		if (res == NULL) {
			throw ClassNotLoadedException(className);
		}

		jsize len = thisJni->GetArrayLength((jarray) res);

		u1* bytes = (u1*) thisJni->GetByteArrayElements((jbyteArray) res, NULL);

        parser::ClassFileParser cf(bytes, len);

		thisJni->ReleaseByteArrayElements((jbyteArray) res, (jbyte*) bytes,
                                      JNI_ABORT);
		thisJni->DeleteLocalRef(res);
		thisJni->DeleteLocalRef(targetName);

		classHierarchy.addClass(cf);
	}

	//const char* className;
	JNIEnv* thisJni;
	jobject loader;

	static jclass proxyClass;
	static jmethodID getResourceId;

    bool inLivePhase = false;
};

jclass ClassPath::proxyClass = NULL;
jmethodID ClassPath::getResourceId = NULL;

int lastNewSiteID = 0;

static void instrumentMethodEntry(ClassFile &cf, Method &method, ConstPool::Index &methodIDIndex, ConstPool::Index &instrMethod,
                                  ConstPool::Index &witnessMethod);
static void instrumentMethodExit(ClassFile &cf, Method &method, ConstPool::Index &methodIDIndex, ConstPool::Index &instrMethod);
static void instrumentObjectAlloc(ClassFile &cf, Method &method, ConstPool::Index &instrMethod);
static void instrumentObjectArrayAlloc(ClassFile &cf, Method &method, ConstPool::Index &instrMethod);
static void instrumentPrimitiveArrayAlloc(ClassFile &cf, Method &method, ConstPool::Index &instrMethod);
static void instrumentMultiArrayAlloc(ClassFile &cf, Method &method, ConstPool::Index &instrMethod);
static void instrumentPutField(ClassFile &cf, Method &method, ConstPool::Index &instrMethod);
static void witnessGetField(ClassFile &cf, Method &method, ConstPool::Index &instrMethod);

void instrumentClass(jvmtiEnv *jvmti, JNIEnv *currJni, jobject loader,
                     u1 *class_data, jint class_data_len,
                     jint *new_class_data_len, u1 **new_class_data)
{
    static int methodIDCounter = 1;
    
    parser::ClassFileParser cf(class_data, class_data_len);

    // if (string(cf.getThisClassName()).find("sun") == 0) {
    //     return;
    // }

    // cerr << "Loading: " << cf.getThisClassName() << endl;

    classHierarchy.addClass(cf);
    classTable.mapOrFind(string(cf.getThisClassName()));
    
    ConstPool::Index instrumentFlagClass = cf.addClass("InstrumentFlag");
    ConstPool::Index proxyClass = cf.addClass("ETProxy");
    
    // public static void onEntry(int methodID, Object receiver);
    ConstPool::Index onEntryMethod = cf.addMethodRef(proxyClass, "onEntry", "(ILjava/lang/Object;)V");

    // public static void onExit(int methodID);
    ConstPool::Index onExitMethod = cf.addMethodRef(proxyClass, "onExit", "(I)V");

    // public static native void onMain();
    // ConstPool::Index onMainMethod = cf.addMethodRef(proxyClass, "_onMain", "()V");

    // public static void onObjectAlloc(int allocdClassID, Object allocdObject, int allocSiteID);
    ConstPool::Index onAllocMethod = cf.addMethodRef(proxyClass, "onObjectAlloc", "(ILjava/lang/Object;I)V");

    // pitfall: allocdArray is not always Object[]!
    // public static void onObjectArrayAlloc(int allocdClassID, int size, Object allocdArray, int allocSiteID);
    ConstPool::Index onAllocArrayMethod = cf.addMethodRef(proxyClass, "onArrayAlloc", "(IILjava/lang/Object;I)V");

    // ... but allocdArray is always Object[] here
    // public static void onMultiArrayAlloc(int dims, int allocdClassID, Object[] allocdArray, int allocSiteID);
    ConstPool::Index onAllocMultiArrayMethod = cf.addMethodRef(proxyClass, "onMultiArrayAlloc", "(IILjava/lang/Object;I)V");
    
    // public static void onPutField(Object tgtObject, Object srcObject, int fieldID);
    ConstPool::Index onPutFieldMethod = cf.addMethodRef(proxyClass, "onPutField","(Ljava/lang/Object;Ljava/lang/Object;I)V");
    
    // public static void witnessObjectAlive(Object aliveObject, int classID);
    ConstPool::Index witnessObjectAliveMethod = cf.addMethodRef(proxyClass, "witnessObjectAlive", "(Ljava/lang/Object;I)V");
    
    for (Method &method : cf.methods) {
        // Record the method
        methodTable.insert(make_pair(methodIDCounter, make_pair(string(cf.getThisClassName()),
                                                                method.getName())));
        int methodID = methodIDCounter;
        methodIDCounter++;

        if (method.hasCode()) {
            ConstPool::Index methodIDIndex = cf.addInteger(methodID);
            instrumentMethodEntry(cf, method, methodIDIndex, onEntryMethod, witnessObjectAliveMethod);
            instrumentMethodExit(cf, method, methodIDIndex, onExitMethod);
            instrumentObjectAlloc(cf, method, onAllocMethod);
            instrumentObjectArrayAlloc(cf, method, onAllocArrayMethod);
            instrumentPrimitiveArrayAlloc(cf, method, onAllocArrayMethod);
            instrumentMultiArrayAlloc(cf, method, onAllocMultiArrayMethod);
            instrumentPutField(cf, method, onPutFieldMethod);
            witnessGetField(cf, method, witnessObjectAliveMethod);
        }
    }

    // assert(currJni);
    // assert(loader);
    ClassPath cp(currJni, loader);
    // TestClassPath cp;
    
    cf.computeFrames(&cp);
    *new_class_data_len = (jint) cf.computeSize();
    jvmtiError error = jvmti->Allocate((jlong) *new_class_data_len, new_class_data);
    assert(!error);
    
    cf.write(*new_class_data, *new_class_data_len);
}

inline static void instrumentPutField(ClassFile &cf, Method &method, ConstPool::Index &instrMethod)
{
    InstList &instList = method.instList();

    for (Inst *inst : instList) {
        if (inst->opcode == Opcode::putfield) {
            ConstPool::Index targetFieldRefIndex = inst->field()->fieldRefIndex;
            string targetClassName, targetFieldName, fieldDesc;
            cf.getFieldRef(targetFieldRefIndex, &targetClassName, &targetFieldName, &fieldDesc);
            
            // Only worry about objects for now
            if (fieldDesc[0] == 'L') {
                // Foo.bar becomes Foo/bar
                string fieldString = targetClassName + "/" + targetFieldName;
                int targetFieldID = fieldTable.mapOrFind(fieldString);

                // Stack: ... | tgtObjectRef | srcObjectRef
                instList.addZero(Opcode::dup2, inst);
                // Stack: ... | tgtObjectRef | srcObjectRef | tgtObjectRef | srcObjectRef

                ConstPool::Index fieldIDIndex = cf.addInteger(targetFieldID);
                instList.addLdc(Opcode::ldc_w, fieldIDIndex, inst);
                // Stack: ... | tgtObjectRef | srcObjectRef | tgtObjectRef | srcObjectRef | fieldID

                instList.addInvoke(Opcode::invokestatic, instrMethod, inst);
                // Stack: ... | tgtObjectRef | srcObjectRef
                // Stack preserved
            }
        }
    }
}

inline static void instrumentMethodEntry(ClassFile &cf, Method &method, ConstPool::Index &methodIDIndex,
                                         ConstPool::Index &instrMethod, ConstPool::Index &witnessMethod)
{
    InstList &instList = method.instList();
            
    Inst *p = *instList.begin(); 

    // doesn't quite work yet, because...circular reasoning
    // if (method.isMain()) {
    //     instList.addInvoke(Opcode::invokestatic, onMainMethod, p);
    // }
    
    if (method.isStatic()) {
        // Stack: ...
        instList.addLdc(Opcode::ldc_w, methodIDIndex, p);
        // Stack: ... | methodIDIndex
        instList.addZero(Opcode::aconst_null, p);
        // Stack: ... | methodIDIndex | null
        instList.addInvoke(Opcode::invokestatic, instrMethod, p);
        // Stack: ...
        // Stack preserved
    } else {
        // Stack: ...
        instList.addLdc(Opcode::ldc_w, methodIDIndex, p);
        // Stack: ... | methodIDIndex
        instList.addZero(Opcode::aload_0, p);
        // Stack: ... | methodIDIndex | thisObject
        instList.addInvoke(Opcode::invokestatic, instrMethod, p);
        // Stack: ...
        // Stack preserved
    }

    /* Can't instrument certain built-in classes 
       also, one can't instrument in an initializer 
       because an uninitialized thing isn't an object */
    if (string(cf.getThisClassName()).find("java") != 0
        && string(cf.getThisClassName()).find("sun") != 0
        && string(method.getName()) != "<init>") {
        // Stack: ...
        instList.addZero(Opcode::aload_0, p);
        // Stack: ... | thisObject
        // cerr << "Debug: " << cf.getThisClassName() << endl;
        int classID = classTable.mapOrFind(string(cf.getThisClassName()));
        ConstPool::Index classIDIndex = cf.addInteger(classID);
        instList.addLdc(Opcode::ldc_w, classIDIndex, p);
        // Stack: ... | thisObject | thisClassID
        instList.addInvoke(Opcode::invokestatic, witnessMethod, p);
    }
}

inline static void instrumentMethodExit(ClassFile &cf, Method &method, ConstPool::Index &methodIDIndex,
                                        ConstPool::Index &instrMethod)
{
    InstList &instList = method.instList();

    for (Inst *inst : instList) {
        if (inst->isExit()) {
            instList.addLdc(Opcode::ldc_w, methodIDIndex, inst);
            instList.addInvoke(Opcode::invokestatic, instrMethod, inst);
        }
    }
}

static void instrumentObjectAlloc(ClassFile &cf, Method &method, ConstPool::Index &instrMethod)
{
    
    InstList &instList = method.instList();
    
    for (Inst *inst : instList) {
        if (inst->opcode == Opcode::NEW) {
            // cerr << "object alloc" << endl;
            
            // Stack: ...
            Inst *p = inst->next;
            // Stack: ... | objRef
            
            instList.addZero(Opcode::dup, p);
            // Stack: ... | objRef | objRef
            
            ConstPool::Index allocatedClassIndex = inst->type()->classIndex;
            int classID = classTable.mapOrFind(string(cf.getClassName(allocatedClassIndex)));            
            ConstPool::Index classIDIndex = cf.addInteger(classID);
            
            instList.addLdc(Opcode::ldc_w, classIDIndex, p);
            // Stack: ... | objRef | objRef | classID

            instList.addZero(Opcode::swap, p);
            // Stack: ... | objRef | classID | objRef
            
            ConstPool::Index allocSiteIndex = cf.addInteger(++lastNewSiteID);
            instList.addLdc(Opcode::ldc_w, allocSiteIndex, p);
            // Stack: ... | objRef | classID | objRef | allocSiteID
            
            instList.addInvoke(Opcode::invokestatic, instrMethod, p);
            // Stack: ... | objRef
            // Stack preserved
        }
    }
}

inline static void instrumentObjectArrayAlloc(ClassFile &cf, Method &method, ConstPool::Index &instrMethod)
{
    
    
    InstList &instList = method.instList();
    
    for (Inst *inst : instList) {
        if (inst->opcode == Opcode::anewarray) {
            // cerr << "object array alloc" << endl;
            
            // Stack: ... | size

            ConstPool::Index allocatedClassIndex = inst->type()->classIndex;

            // cerr << "allocd class: " << string(cf.getClassName(allocatedClassIndex)) << endl;
            
            int classID = classTable.mapOrFind(string(cf.getClassName(allocatedClassIndex)));            
            ConstPool::Index classIDIndex = cf.addInteger(classID);

            // cerr << "check 1" << endl;
            
            instList.addLdc(Opcode::ldc_w, classIDIndex, inst);
            // Stack: ... | size | classID

            instList.addZero(Opcode::swap, inst);
            // Stack: ... | classID | size
            
            instList.addZero(Opcode::dup, inst);
            // Stack: ... | classID | size | size
            
            Inst *p = inst->next;
            // Stack: ... | classID | size | arrayRef

            // cerr << "check 2" << endl;
            
            instList.addZero(Opcode::dup_x2, p);
            // Stack: ... | arrayRef | classID | size | arrayRef

            ConstPool::Index allocSiteIndex = cf.addInteger(++lastNewSiteID);
            instList.addLdc(Opcode::ldc_w, allocSiteIndex, p);
            // Stack: ... | arrayRef | classID | size | arrayRef | allocSiteID
            
            instList.addInvoke(Opcode::invokestatic, instrMethod, p);

            // cerr << "check 3" << endl;
            
            // Stack: ... | arrayRef
            // Stack preserved
        }
    }   
}

inline static void instrumentPrimitiveArrayAlloc(ClassFile &cf, Method &method, ConstPool::Index &instrMethod)
{    
    InstList &instList = method.instList();
    
    for (Inst *inst : instList) {
        if (inst->opcode == Opcode::newarray) {
            // cerr << "prim array alloc" << endl;
            // Stack: ... | size

            // this actually depends on JNIF implementation specifics
            // potentially unstable across JNIF versions
            u1 pseudoClassID = (inst->newarray()->atype) - 3;
            // cerr << "class id: " << (int)pseudoClassID << endl;
            instList.addBiPush(pseudoClassID, inst);
            // Stack: ... | size | atype

            instList.addZero(Opcode::swap, inst);
            // Stack: ... | atype | size
            
            instList.addZero(Opcode::dup, inst);
            // Stack: ... | atype | size | size
            
            Inst *p = inst->next;
            // Stack: ... | atype | size | arrayRef
            
            instList.addZero(Opcode::dup_x2, p);
            // Stack: ... | arrayRef | atype | size | arrayRef

            ConstPool::Index allocSiteIndex = cf.addInteger(++lastNewSiteID);
            instList.addLdc(Opcode::ldc_w, allocSiteIndex, p);
            // Stack: ... | arrayRef | atype | size | arrayRef | allocSiteID
            
            // Yes, arrays are objects!
            instList.addInvoke(Opcode::invokestatic, instrMethod, p);
            // Stack: ... | arrayRef
            // Stack preserved
        }
    }   
}

inline static void instrumentMultiArrayAlloc(ClassFile &cf, Method &method, ConstPool::Index &instrMethod)
{
    
    InstList &instList = method.instList();
    
    for (Inst *inst : instList) {
        if (inst->opcode == Opcode::multianewarray) {
            // cerr << "multi array alloc" << endl;
            // cerr << "current class: " << cf.getThisClassName() << endl;
            
            // Stack: ... | size1 | size2 | ... | sizeN
            
            ConstPool::Index allocatedClassIndex = inst->multiarray()->classIndex;

            // cerr << "alloc'd class: " << string(cf.getClassName(allocatedClassIndex)) << endl;
            
            int classID = classTable.mapOrFind(string(cf.getClassName(allocatedClassIndex)));            
            ConstPool::Index classIDIndex = cf.addInteger(classID);

            // I hope no one ever wants an array of more than 255 dimensions, really
            u1 dims = inst->multiarray()->dims;

            // only care about outermost size, because multi-array is just
            // a bunch of nested arrays
            
            // Stack: ... | sizeN-1 | sizeN
            
            Inst *p = inst->next;
            // Stack: ... | arrayRef
            
            instList.addZero(Opcode::dup, p);
            // Stack: ... | arrayRef | arrayRef
            
            instList.addBiPush(dims, p);
            // Stack: ... | arrayRef | arrayRef | dims
            
            instList.addLdc(Opcode::ldc_w, classIDIndex, p);
            // Stack: ... | arrayRef | arrayRef | dims | arrayClassID
            
            instList.addZero(Opcode::dup2_x1, p);
            // Stack: ... | arrayRef | dims | arrayClassID | arrayRef | dims | arrayClassID
            
            instList.addZero(Opcode::pop2, p);
            // Stack: ... | arrayRef | dims | arrayClassID | arrayRef

            ConstPool::Index allocSiteIndex = cf.addInteger(++lastNewSiteID);
            instList.addLdc(Opcode::ldc_w, allocSiteIndex, p);
            // Stack: ... | arrayRef | dims | arrayClassID | arrayRef | allocSiteID
            
            instList.addInvoke(Opcode::invokestatic, instrMethod, p);
            // Stack: ... | arrayRef
        }
    }
}

inline static void witnessGetField(ClassFile &cf, Method &method, ConstPool::Index &instrMethod)
{
    InstList &instList = method.instList();

    for (Inst *inst : instList) {
        if (inst->opcode == Opcode::getfield) {
            ConstPool::Index fieldRefIndex = inst->field()->fieldRefIndex;
            string className, fieldName, fieldDesc;
            cf.getFieldRef(fieldRefIndex, &className, &fieldName, &fieldDesc);
            
            // Stack: ... | objectRef
            instList.addZero(Opcode::dup, inst);
            // Stack: ... | objectRef | objectRef

            int aliveClassID = classTable.mapOrFind(className);
            ConstPool::Index aliveClassIDIndex = cf.addInteger(aliveClassID);
            
            instList.addLdc(Opcode::ldc_w, aliveClassIDIndex, inst);
            // Stack: ... | objectRef | objectRef | aliveClassID

            instList.addInvoke(Opcode::invokestatic, instrMethod, inst);
        }
    }
}
