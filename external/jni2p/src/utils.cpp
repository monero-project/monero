// Copyright (c) 2014-2021, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "utils.h"
#include <iomanip>
#include <iostream>

namespace jni2p
{

int create_jvm(JavaVM **jvm, const std::string &jbigi_path)
{
    JNIEnv* env;
    JavaVMInitArgs args;
    JavaVMOption options;
    args.version = JNI_VERSION_1_6;
    args.nOptions = 1;
    std::string arg = std::string("-Djava.class.path=") + jbigi_path;
    options.optionString = const_cast<char*>(arg.c_str());
    args.options = &options;
    args.ignoreUnrecognized = false;
    return JNI_CreateJavaVM(jvm, (void**)&env, &args);
}

jclass get_class(JNIEnv *env, const std::string &name)
{
    //std::cout << "Looking for class " << name << " ...";
    auto jrouter_class = env->FindClass(name.c_str());
    if( not jrouter_class)
       throw std::runtime_error("Failed to find class " + name);
    //std::cout << "OK\n";
    return jrouter_class;
}

jclass get_java_router_class(JNIEnv *env)
{
    return get_class(env, JC_ROUTER);
}

jmethodID get_method(JNIEnv *env, const std::string &class_name, const std::string &name, const std::string &signature)
{
    auto jrouter_class = get_class(env, class_name);
    //std::cout << "Looking for method '" << name << "' ...";
    auto method = env->GetMethodID(jrouter_class, name.c_str(), signature.c_str());
    if(not method)
        throw std::runtime_error("Failed to get method " + name);
    //std::cout << "OK\n";
    return method;
}

jmethodID get_router_method(JNIEnv *env, const std::string &name, const std::string &signature)
{
    return get_method(env, JC_ROUTER, name, signature);
}

RouterProperties args_to_properties(const std::vector<std::string> &args)
{
    RouterProperties props;
    for(auto& it: args)
    {
        auto pos = it.find("=");
        if(pos == std::string::npos)
            throw std::runtime_error("Invalid argument : no equal sign");
        props.insert(std::make_pair(it.substr(0, pos), it.substr(pos+1)));
    }
    return props;
}

jobject router_properties_to_java(JNIEnv *env, const RouterProperties &properties)
{
    auto constructor = get_method(env, "java/util/Properties", "<init>", "()V");
    auto put = get_method(env, "java/util/Properties", "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    auto props = env->NewObject(get_class(env, "java/util/Properties"), constructor);
    for (auto& it: properties)
    {
        auto first = env->NewStringUTF(it.first.c_str());
        auto second = env->NewStringUTF(it.second.c_str());
        env->CallVoidMethod(props, put, first, second);
        //TODO env->ReleaseStringUTFChars(first, it.first.c_str());
        //TODO env->ReleaseStringUTFChars(second, it.second.c_str());
    }
    return props;
}

std::string properties_to_string(JNIEnv *env, jobject properties)
{
    auto toString = get_method(env, "java/util/Properties", "toString", "()Ljava/lang/String;");
    auto rv = (jstring)env->CallObjectMethod(properties, toString);
    auto string = env->GetStringUTFChars(rv, 0);
    std::string res(string);
    //env->ReleaseStringUTFChars(rv, string);
    return res;
}

} // jni2p
