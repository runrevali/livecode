/* Copyright (C) 2003-2016 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"

#ifdef TARGET_PLATFORM_MACOS_X
#include <JavaVM/jni.h>
#elif TARGET_SUBPLATFORM_ANDROID
#include <jni.h>
extern JNIEnv *MCJavaGetThreadEnv();
extern JNIEnv *MCJavaAttachCurrentThread();
extern void MCJavaDetachCurrentThread();
#endif

#if defined(TARGET_PLATFORM_MACOS_X) || defined(TARGET_SUBPLATFORM_ANDROID)
#define TARGET_SUPPORTS_JAVA
#endif

#ifdef TARGET_SUPPORTS_JAVA
static JNIEnv *s_env;
static JavaVM *s_jvm;
#endif

bool __MCJavaInitialize()
{
#ifdef TARGET_PLATFORM_MACOS_X
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_6;
    JNI_GetDefaultJavaVMInitArgs(&vm_args);

    JavaVMOption* options = new JavaVMOption[1];
    options[0].optionString = "-Djava.class.path=/usr/lib/java";
    
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;
    
    JNI_CreateJavaVM(&s_jvm, (void **)&s_env, &vm_args);
#endif
    return true;
}

void __MCJavaFinalize()
{
#ifdef TARGET_PLATFORM_MACOS_X
    s_jvm->DestroyJavaVM();
#endif
}

enum MCJavaCallType {
    MCJavaCallTypeInstance,
    MCJavaCallTypeStatic,
    MCJavaCallTypeNonVirtual
};

enum MCJavaType {
    kMCJavaTypeBoolean,
    kMCJavaTypeByte,
    kMCJavaTypeChar,
    kMCJavaTypeShort,
    kMCJavaTypeInt,
    kMCJavaTypeLong,
    kMCJavaTypeFloat,
    kMCJavaTypeDouble,
    kMCJavaTypeArray,
    kMCJavaTypeObject,
};

typedef struct
{
    const char *name;
    MCJavaType type;
}
java_type_map;

static java_type_map type_map[] =
{
    {"Z", kMCJavaTypeBoolean},
    {"B", kMCJavaTypeByte},
    {"C", kMCJavaTypeChar},
    {"S", kMCJavaTypeShort},
    {"I", kMCJavaTypeInt},
    {"J", kMCJavaTypeLong},
    {"F", kMCJavaTypeFloat},
    {"D", kMCJavaTypeDouble},
};

#ifdef TARGET_SUPPORTS_JAVA
void MCJavaDoAttachCurrentThread()
{
#ifdef TARGET_PLATFORM_MACOS_X
    s_jvm -> AttachCurrentThread((void **)&s_env, nil);
#else
    s_env = MCJavaAttachCurrentThread();
#endif
}

static bool __MCJavaStringFromJString(jstring p_string, MCStringRef& r_string)
{
    MCJavaDoAttachCurrentThread();
    
    const char *nativeString = s_env -> GetStringUTFChars(p_string, 0);
    
    bool t_success;
    t_success = MCStringCreateWithCString(nativeString, r_string);
    
    s_env->ReleaseStringUTFChars(p_string, nativeString);
    return t_success;
}

static bool __MCJavaStringToJString(MCStringRef p_string, jstring& r_string)
{
    MCJavaDoAttachCurrentThread();
    
    MCAutoStringRefAsCString t_string_cstring;
    t_string_cstring . Lock(p_string);
    
    jstring t_result;
    t_result = s_env->NewStringUTF(*t_string_cstring);
    
    if (t_result != nil)
    {
        r_string = t_result;
        return true;
    }
    
    return false;
}

static jstring MCJavaGetJObjectClassName(jobject p_obj)
{
    jclass t_class = s_env->GetObjectClass(p_obj);
    
    jclass javaClassClass = s_env->FindClass("java/lang/Class");
    
    jmethodID javaClassNameMethod = s_env->GetMethodID(javaClassClass, "getName", "()Ljava/lang/String;");
    
    jstring className = (jstring)s_env->CallObjectMethod(t_class, javaClassNameMethod);
    
    return className;
}

static bool __JavaJNIInstanceMethodResult(jobject p_instance, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    MCJavaType t_return_type = (MCJavaType)p_return_type;
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result;
            t_result = s_env -> CallBooleanMethodA(p_instance, p_method_id, p_params);
            *(jboolean *)r_result = t_result;
            break;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result;
            t_result = s_env -> CallByteMethodA(p_instance, p_method_id, p_params);
            *(jbyte *)r_result = t_result;
            break;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result;
            t_result = s_env -> CallCharMethodA(p_instance, p_method_id, p_params);
            *(jchar *)r_result = t_result;
            break;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result;
            t_result = s_env -> CallShortMethodA(p_instance, p_method_id, p_params);
            *(jshort *)r_result = t_result;
            break;
        }
        case kMCJavaTypeInt:
        {
            jint t_result;
            t_result = s_env -> CallIntMethodA(p_instance, p_method_id, p_params);
            *(jint *)r_result = t_result;
            break;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result;
            t_result = s_env -> CallLongMethodA(p_instance, p_method_id, p_params);
            *(jlong *)r_result = t_result;
            break;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result;
            t_result = s_env -> CallFloatMethodA(p_instance, p_method_id, p_params);
            *(jfloat *)r_result = t_result;
            break;
        }
        case kMCJavaTypeObject:
        default:
        {
            jobject t_result;
            t_result = s_env -> CallObjectMethodA(p_instance, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreate(t_result, t_result_value))
                return false;
            *(MCJavaObjectRef *)r_result = t_result_value;
            break;
        }
    }
    return true;
}

static bool __JavaJNIStaticMethodResult(jclass p_class, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    MCJavaType t_return_type = (MCJavaType)p_return_type;
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result;
            t_result = s_env -> CallStaticBooleanMethodA(p_class, p_method_id, p_params);
            *(jboolean *)r_result = t_result;
            break;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result;
            t_result = s_env -> CallStaticByteMethodA(p_class, p_method_id, p_params);
            *(jbyte *)r_result = t_result;
            break;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result;
            t_result = s_env -> CallStaticCharMethodA(p_class, p_method_id, p_params);
            *(jchar *)r_result = t_result;
            break;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result;
            t_result = s_env -> CallStaticShortMethodA(p_class, p_method_id, p_params);
            *(jshort *)r_result = t_result;
            break;
        }
        case kMCJavaTypeInt:
        {
            jint t_result;
            t_result = s_env -> CallStaticIntMethodA(p_class, p_method_id, p_params);
            *(jint *)r_result = t_result;
            break;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result;
            t_result = s_env -> CallStaticLongMethodA(p_class, p_method_id, p_params);
            *(jlong *)r_result = t_result;
            break;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result;
            t_result = s_env -> CallStaticFloatMethodA(p_class, p_method_id, p_params);
            *(jfloat *)r_result = t_result;
            break;
        }
        case kMCJavaTypeObject:
        default:
        {
            jobject t_result;
            t_result = s_env -> CallStaticObjectMethodA(p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreate(t_result, t_result_value))
                return false;
            *(MCJavaObjectRef *)r_result = t_result_value;
            break;
        }
    }
    return true;
}

static bool __JavaJNINonVirtualMethodResult(jobject p_instance, jclass p_class, jmethodID p_method_id, jvalue *p_params, int p_return_type, void *r_result)
{
    MCJavaDoAttachCurrentThread();
    MCJavaType t_return_type = (MCJavaType)p_return_type;
    
    switch (t_return_type) {
        case kMCJavaTypeBoolean:
        {
            jboolean t_result;
            t_result = s_env -> CallNonvirtualBooleanMethodA(p_instance, p_class, p_method_id, p_params);
            *(jboolean *)r_result = t_result;
            break;
        }
        case kMCJavaTypeByte:
        {
            jbyte t_result;
            t_result = s_env -> CallNonvirtualByteMethodA(p_instance, p_class, p_method_id, p_params);
            *(jbyte *)r_result = t_result;
            break;
        }
        case kMCJavaTypeChar:
        {
            jchar t_result;
            t_result = s_env -> CallNonvirtualCharMethodA(p_instance, p_class, p_method_id, p_params);
            *(jchar *)r_result = t_result;
            break;
        }
        case kMCJavaTypeShort:
        {
            jshort t_result;
            t_result = s_env -> CallNonvirtualShortMethodA(p_instance, p_class, p_method_id, p_params);
            *(jshort *)r_result = t_result;
            break;
        }
        case kMCJavaTypeInt:
        {
            jint t_result;
            t_result = s_env -> CallNonvirtualIntMethodA(p_instance, p_class, p_method_id, p_params);
            *(jint *)r_result = t_result;
            break;
        }
        case kMCJavaTypeLong:
        {
            jlong t_result;
            t_result = s_env -> CallNonvirtualLongMethodA(p_instance, p_class, p_method_id, p_params);
            *(jlong *)r_result = t_result;
            break;
        }
        case kMCJavaTypeFloat:
        {
            jfloat t_result;
            t_result = s_env -> CallNonvirtualFloatMethodA(p_instance, p_class, p_method_id, p_params);
            *(jfloat *)r_result = t_result;
            break;
        }
        case kMCJavaTypeObject:
        default:
        {
            jobject t_result;
            t_result = s_env -> CallNonvirtualObjectMethodA(p_instance, p_class, p_method_id, p_params);
            
            MCJavaObjectRef t_result_value;
            if (!MCJavaObjectCreate(t_result, t_result_value))
                return false;
            *(MCJavaObjectRef *)r_result = t_result_value;
            break;
        }
    }
    return true;
}

static bool __JavaJNIGetParams(void **args, uindex_t p_count, int *p_types, jvalue *&r_params)
{
    MCAutoArray<jvalue> t_args;
    if (!t_args . New(p_count))
        return false;
    
    for (uindex_t i = 0; i < p_count; i++)
    {
        switch (p_types[i])
        {
            case kMCJavaTypeObject:
                t_args[i] . l = (jobject)MCJavaObjectGetObject(*(MCJavaObjectRef *)args[i]);
                break;
            case kMCJavaTypeArray:
                // Not yet implemented
                MCAssert(false);
                break;
            case kMCJavaTypeBoolean:
                t_args[i].z = *(jboolean *)args[i];
                break;
            case kMCJavaTypeByte:
                t_args[i].b = *(jbyte *)args[i];
                break;
            case kMCJavaTypeChar:
                t_args[i].c = *(jchar *)args[i];
                break;
            case kMCJavaTypeShort:
                t_args[i].s = *(jshort *)args[i];
                break;
            case kMCJavaTypeInt:
                t_args[i].i = *(jint *)args[i];
                break;
            case kMCJavaTypeLong:
                t_args[i].j = *(jlong *)args[i];
                break;
            case kMCJavaTypeFloat:
                t_args[i].f = *(jfloat *)args[i];
                break;
            case kMCJavaTypeDouble:
                t_args[i].d = *(jdouble *)args[i];
                break;
        }
    }
    
    t_args . Take(r_params, p_count);
    return true;
}

static bool MCJavaClassNameToPathString(MCNameRef p_class_name, MCStringRef& r_string)
{
    MCAutoStringRef t_escaped;
    if (!MCStringMutableCopy(MCNameGetString(p_class_name), &t_escaped))
        return false;
    
    if (!MCStringFindAndReplaceChar(*t_escaped, '.', '/', kMCStringOptionCompareExact))
        return false;
    
    return MCStringCopy(*t_escaped, r_string);
}
#endif

typedef struct __MCJavaObject *MCJavaObjectRef;

MC_DLLEXPORT_DEF MCTypeInfoRef kMCJavaObjectTypeInfo;

MC_DLLEXPORT_DEF MCTypeInfoRef MCJavaGetObjectTypeInfo() { return kMCJavaObjectTypeInfo; }

struct __MCJavaObjectImpl
{
    void *object;
};

__MCJavaObjectImpl *MCJavaObjectGet(MCJavaObjectRef p_obj)
{
    return (__MCJavaObjectImpl*)MCValueGetExtraBytesPtr(p_obj);
}

static inline __MCJavaObjectImpl MCJavaObjectImplMake(void* p_obj)
{
    __MCJavaObjectImpl t_obj;
    t_obj.object = p_obj;
    return t_obj;
}

static void __MCJavaObjectDestroy(MCValueRef p_value)
{
    // no-op
}

static bool __MCJavaObjectCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
    if (p_release)
        r_copy = p_value;
    else
        r_copy = MCValueRetain(p_value);
    return true;
}

static bool __MCJavaObjectEqual(MCValueRef p_left, MCValueRef p_right)
{
    if (p_left == p_right)
        return true;
    
    return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCJavaObjectImpl)) == 0;
}

static hash_t __MCJavaObjectHash(MCValueRef p_value)
{
    return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCJavaObjectImpl));
}

static bool __MCJavaObjectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
#ifdef TARGET_SUPPORTS_JAVA
    MCJavaObjectRef t_obj = static_cast<MCJavaObjectRef>(p_value);
    
    jobject t_target = (jobject)MCJavaObjectGetObject(t_obj);
    
    MCAutoStringRef t_class_name;
    __MCJavaStringFromJString(MCJavaGetJObjectClassName(t_target), &t_class_name);
    return MCStringFormat (r_desc, "<java: %@>", *t_class_name);
#else
    return MCStringFormat (r_desc, "<java: %s>", "not supported");
#endif
}

static MCValueCustomCallbacks kMCJavaObjectCustomValueCallbacks =
{
    true,
    __MCJavaObjectDestroy,
    __MCJavaObjectCopy,
    __MCJavaObjectEqual,
    __MCJavaObjectHash,
    __MCJavaObjectDescribe,
    
    nil,
    nil,
};

MC_DLLEXPORT_DEF bool MCJavaObjectCreate(void *p_object, MCJavaObjectRef &r_object)
{
    bool t_success;
    t_success = true;
    
    MCJavaObjectRef t_obj;
    t_obj = nil;
    
    t_success = MCValueCreateCustom(kMCJavaObjectTypeInfo, sizeof(__MCJavaObjectImpl), t_obj);
    
    if (t_success)
    {
        *MCJavaObjectGet(t_obj) = MCJavaObjectImplMake(p_object);
        r_object = t_obj;
    }

    return t_success;
}

bool MCJavaCreateJavaObjectTypeInfo()
{
    return MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.java.JavaObject"), kMCNullTypeInfo, &kMCJavaObjectCustomValueCallbacks, kMCJavaObjectTypeInfo);
}

MC_DLLEXPORT_DEF void *MCJavaObjectGetObject(const MCJavaObjectRef p_obj)
{
    __MCJavaObjectImpl *t_impl;
    t_impl = MCJavaObjectGet(p_obj);
    return t_impl -> object;
}

////////////////////////////////////////////////////////////////////////////////

static MCJavaType MCJavaMapTypeCodeSubstring(MCStringRef p_type_code, MCRange p_range)
{
    if (MCStringBeginsWithCString(p_type_code, (const char_t *)"[", kMCStringOptionCompareExact))
        return kMCJavaTypeArray;
    
    for (uindex_t i = 0; i < sizeof(type_map) / sizeof(type_map[0]); i++)
    {
        if (MCStringSubstringIsEqualToCString(p_type_code, p_range, type_map[i] . name, kMCStringOptionCompareExact))
        {
            return type_map[i] . type;
        }
    }
    
    return kMCJavaTypeObject;
}

MC_DLLEXPORT_DEF
int MCJavaMapTypeCode(MCStringRef p_type_code)
{
    return (int)MCJavaMapTypeCodeSubstring(p_type_code, MCRangeMake(0, MCStringGetLength(p_type_code)));
}

static bool __NextArgument(MCStringRef p_arguments, MCRange& x_range)
{
    if (x_range . offset + x_range . length >= MCStringGetLength(p_arguments))
        return false;
    
    x_range . offset = x_range . offset + x_range . length;
    
    MCRange t_new_range;
    t_new_range . offset = x_range . offset;
    t_new_range . length = 1;
    
    uindex_t t_length = 1;
    
    MCJavaType t_next_type;
    while ((t_next_type = MCJavaMapTypeCodeSubstring(p_arguments, t_new_range)) == kMCJavaTypeArray)
    {
        t_new_range . offset++;
        t_length++;
    }
    
    if (t_next_type == kMCJavaTypeObject)
    {
        if (!MCStringFirstIndexOfChar(p_arguments, ';', x_range . offset, kMCStringOptionCompareExact, t_length))
            return false;
    }

    x_range . length = t_length;
    return true;
}

MC_DLLEXPORT_DEF
bool MCJavaGetArgumentTypes(MCStringRef p_arguments, int *&r_argument_types, uindex_t& r_count)
{
    // Remove brackets from arg string
    MCAutoStringRef t_args;
    if (!MCStringCopySubstring(p_arguments, MCRangeMake(1, MCStringGetLength(p_arguments) - 2), &t_args))
        return false;
    
    MCAutoArray<int> t_types;
    t_types . New(0);

    MCRange t_range = MCRangeMake(0, 0);
    while (__NextArgument(p_arguments, t_range))
    {
        if (!t_types . Push((int)MCJavaMapTypeCodeSubstring(p_arguments, t_range)))
            return false;
    }
    
    t_types . Take(r_argument_types, r_count);
    return true;
}

MC_DLLEXPORT_DEF
bool MCJavaCallJNIMethod(MCNameRef p_class_name, void *p_method_id, int p_call_type, int p_return_type, void *r_return, int *p_arg_types, void **p_args, uindex_t p_arg_count)
{
#ifdef TARGET_SUPPORTS_JAVA
    jmethodID t_method_id = (jmethodID)p_method_id;

    if (t_method_id == nil)
        return false;
    
    MCAutoStringRef t_class;
    MCAutoStringRefAsCString t_class_cstring;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class))
        return false;
    
    if (!t_class_cstring . Lock(*t_class))
        return false;
    
    /*
     convert_params_to_jvalue_array
     */
    bool t_is_instance = p_call_type != MCJavaCallTypeStatic;
    jvalue *t_params = nil;
    if (t_is_instance)
        __JavaJNIGetParams(&p_args[1], p_arg_count - 1, p_arg_types, t_params);
    else
        __JavaJNIGetParams(p_args, p_arg_count, p_arg_types, t_params);
    
    switch (p_call_type)
    {
        case MCJavaCallTypeInstance:
        {
            // Java object on which to call instance method should always be first argument.
            jobject t_instance = (jobject)MCJavaObjectGetObject(*(MCJavaObjectRef *)p_args[0]);
            return __JavaJNIInstanceMethodResult(t_instance, t_method_id, t_params, p_return_type, r_return);
        }
        case MCJavaCallTypeStatic:
        {
            jclass t_target_class = s_env -> FindClass(*t_class_cstring);
            return __JavaJNIStaticMethodResult(t_target_class, t_method_id, t_params, p_return_type, r_return);
        }
        case MCJavaCallTypeNonVirtual:
        {
            jobject t_instance = (jobject)MCJavaObjectGetObject(*(MCJavaObjectRef *)p_args[0]);
            jclass t_target_class = s_env -> FindClass(*t_class_cstring);
            return __JavaJNINonVirtualMethodResult(t_instance, t_target_class, t_method_id, t_params, p_return_type, r_return);
        }
    }
#endif
    return false;
}

MC_DLLEXPORT_DEF bool MCJavaConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string)
{
#ifdef TARGET_SUPPORTS_JAVA
    jstring t_string;
    t_string = (jstring)MCJavaObjectGetObject(p_object);
    return __MCJavaStringFromJString(t_string, r_string);
#else
    return false;
#endif
}

MC_DLLEXPORT_DEF bool MCJavaConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object)
{
#ifdef TARGET_SUPPORTS_JAVA
    jstring t_string;
    if (!__MCJavaStringToJString(p_string, t_string))
        return false;
    
    return MCJavaObjectCreate(t_string, r_object);
#else
    return false;
#endif
}

MC_DLLEXPORT_DEF bool MCJavaCallConstructor(MCNameRef p_class_name, MCListRef p_args, MCJavaObjectRef& r_object)
{
    void *t_jobject_ptr = nil;
#ifdef TARGET_SUPPORTS_JAVA
    MCJavaDoAttachCurrentThread();
    
    MCAutoStringRef t_class_path;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class_path))
        return false;
    
    MCAutoStringRefAsCString t_class_cstring;
    t_class_cstring . Lock(*t_class_path);
    
    jclass t_class = s_env->FindClass(*t_class_cstring);
    
    jmethodID t_constructor = nil;
    if (t_class != nil)
        t_constructor = s_env->GetMethodID(t_class, "<init>", "()V");

    jobject t_object = nil;
    if (t_constructor != nil)
        t_object = s_env->NewObject(t_class, t_constructor);

    if (t_object != nil)
        t_object = s_env -> NewGlobalRef(t_object);
    
    t_jobject_ptr = t_object;
#endif
    
    return MCJavaObjectCreate(t_jobject_ptr, r_object);
}

MC_DLLEXPORT_DEF void *MCJavaGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_signature)
{
    void *t_method_id_ptr = nil;
    
#ifdef TARGET_SUPPORTS_JAVA
    MCAutoStringRef t_class_path;
    if (!MCJavaClassNameToPathString(p_class_name, &t_class_path))
        return nil;
    
    MCAutoStringRefAsCString t_class_cstring, t_method_cstring, t_signature_cstring;
    t_class_cstring . Lock(*t_class_path);
    t_method_cstring . Lock(p_method_name);
    t_signature_cstring . Lock(p_signature);
    
    jclass t_java_class = nil;
    t_java_class = s_env->FindClass(*t_class_cstring);
    
    jmethodID t_method_id = nil;
    if (t_java_class != nil)
        t_method_id = s_env->GetMethodID(t_java_class, *t_method_cstring, *t_signature_cstring);
    
    t_method_id_ptr = t_method_id;
#endif
    
    return t_method_id_ptr;
}