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

#include "jni2p.h"

#include <chrono>
#include <iostream>
#include <string.h>
#include <thread>

namespace jni2p
{

Router::Router(const std::vector<std::string> &args, const RouterProperties &defaults)
   : Router(NULL, defaults)
{
    for (const auto &it : args_to_properties(args))
        m_local_properties[it.first] = it.second;
}

Router::Router(JavaVM *jvm, const RouterProperties &properties)
    : m_jvm(jvm)
    , m_created_jvm(false)
    , m_local_properties(properties)
{
}

Router::~Router()
{
    cleanup();
}

void Router::cleanup()
{
    if(not (m_created_jvm && m_jvm)) // Nothing to do
        return;

    //std::cout << "Destroying jvm ... \n";
    //TODO m_jvm->DestroyJavaVM();
    m_jvm = NULL;
    m_created_jvm = false;
    //std::cout << "Destroy OK\n";
}

JNIEnv* Router::get_env(void)
{
    // Get a thread specific pointer
    JNIEnv *env;
    if( m_jvm->AttachCurrentThread((void**)&env, NULL) < 0)
       throw std::runtime_error("Failed to attach to current thread");
    return env;
}

std::string Router::getProperty(const std::string &key, const std::string &default_val)
{
    auto it = m_local_properties.find(key);
    if(it != m_local_properties.end())
        return it->second;
    return getJavaProperty(key, default_val);
}

std::string Router::getJavaProperty(const std::string &key, const std::string &default_val)
{
    if(!started())
        return default_val;
    auto env = get_env();
    auto method = get_method(env, JC_ROUTERCONTEXT, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");
    auto property = env->CallObjectMethod(getRouterContext(env), method, env->NewStringUTF(key.c_str()));
    if(!property)
        return default_val;
    if(env->IsInstanceOf(property, get_class(env, JC_STRING)))
        return std::string(env->GetStringUTFChars(static_cast<jstring>(property), 0));

    throw std::runtime_error("Type not implemented");
}

void Router::start(void)
{
    // Instantiate new Java VM if necessary
    if(not m_jvm)
    {
        if( create_jvm(&m_jvm, getProperty("i2p.dir.base") + "/jbigi.jar") < 0)
            throw std::runtime_error("Failed to create a new JVM");
        m_created_jvm = true;
    }
    auto env = get_env();
    // Find Methods
    auto constructor = get_router_method(env, "<init>", "(Ljava/util/Properties;)V");
    auto setKillVMOnEnd = get_router_method(env, "setKillVMOnEnd", "(Z)V");
    auto runRouter = get_router_method(env, "runRouter", "()V");
    auto jrouter_class = get_java_router_class(env);
    // Instantiate and start Java router
    m_router = env->NewObject(jrouter_class, constructor, router_properties_to_java(env, m_local_properties));
    env->CallVoidMethod(m_router, setKillVMOnEnd, false);
    env->CallVoidMethod(m_router, runRouter);
}

bool Router::started(void) const
{
    return  m_jvm && m_router;
}

void Router::waitForRunning(void)
{
    if(!started())
        start();
    while(!isRunning())
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

std::string Router::connectTo(const std::string &destination)
{
    waitForRunning();
    std::string bind_port = getProperty("jni2p.default_client_port", "54018");
    executeI2PTunnelCommand("client " + bind_port + " " + destination);
    return std::string("127.0.0.1:") + bind_port;
}

std::string Router::listen(const std::string &path, const std::string &port)
{
    waitForRunning();
    auto destination = getDestination(path);
    if(destination.empty())
        destination = createDestination(path);
    executeI2PTunnelCommand("server localhost " + port + " " + path);
    return destination;
}

jobject Router::executeI2PTunnelCommand(const std::string &command)
{
    auto env = get_env();
    auto constructor = get_method(env, JC_I2PTUNNEL, "<init>", "([Ljava/lang/String;)V");
    jobjectArray args;
    args = (jobjectArray)env->NewObjectArray(6, env->FindClass("java/lang/String"), NULL);
    env->SetObjectArrayElement(args, 0, env->NewStringUTF("-die"));
    env->SetObjectArrayElement(args, 1, env->NewStringUTF("-nocli"));
    env->SetObjectArrayElement(args, 2, env->NewStringUTF("-e"));
    env->SetObjectArrayElement(args, 3, env->NewStringUTF("config localhost 7654"));
    env->SetObjectArrayElement(args, 4, env->NewStringUTF("-e"));
    env->SetObjectArrayElement(args, 5, env->NewStringUTF(command.c_str()));
    return env->NewObject(get_class(env, JC_I2PTUNNEL), constructor, args);
}

/**
 * FileOutputStream writeTo = new FileOutputStream(path)
 * I2PClient client = I2PClientFactory.createClient();
 * Destination d = client.createDestination(writeTo);
 * writeTo.flush();
 * writeTo.close();
 * return d.toBase32();
 */
// NB: Even if ephemeral, Java I2P API requires the keys to be in the filesystem
std::string Router::createDestination(const std::string &path)
{
    auto env = get_env();
    auto fos_ctor = get_method(env, "java/io/FileOutputStream", "<init>", "(Ljava/lang/String;)V");
    auto createClient = env->GetStaticMethodID(get_class(env, "net/i2p/client/I2PClientFactory"), "createClient", "()Lnet/i2p/client/I2PClient;");
    auto createDestination = get_method(env, "net/i2p/client/I2PClient", "createDestination", "(Ljava/io/OutputStream;)Lnet/i2p/data/Destination;");
    auto toBase32 = get_method(env, "net/i2p/data/Destination", "toBase32", "()Ljava/lang/String;");
    auto flush = get_method(env, "java/io/FileOutputStream", "flush", "()V");
    auto close = get_method(env, "java/io/FileOutputStream", "close", "()V");

    auto fos = env->NewObject(get_class(env, "java/io/FileOutputStream"), fos_ctor, env->NewStringUTF(path.c_str()));
    auto client = env->CallStaticObjectMethod(get_class(env, "net/i2p/client/I2PClientFactory"), createClient);

    auto destination = env->CallObjectMethod(client, createDestination, fos);
    env->CallObjectMethod(fos, flush);
    env->CallObjectMethod(fos, close);
    auto base32 = (jstring)env->CallObjectMethod(destination, toBase32);
    if (!base32)
        throw std::runtime_error("Null base 32 address");
    return std::string(env->GetStringUTFChars(base32, 0));
}

/**
 *   FileInputStream readFrom = new FileInputStream(path);
 *   Destination d = new Destination();
 *   d.readBytes(readFrom);
 *   return d.toBase32();
 */
std::string Router::getDestination(const std::string &path)
{
    auto env = get_env();
    auto fis_ctor = get_method(env, "java/io/FileInputStream", "<init>", "(Ljava/lang/String;)V");
    auto dest_ctor = get_method(env, JC_DESTINATION, "<init>", "()V");
    auto readBytes = get_method(env, JC_DESTINATION, "readBytes", "(Ljava/io/InputStream;)V");
    auto toBase32 = get_method(env, JC_DESTINATION, "toBase32", "()Ljava/lang/String;");

    auto fis = env->NewObject(get_class(env, "java/io/FileInputStream"), fis_ctor, env->NewStringUTF(path.c_str()));
    auto destination = env->NewObject(get_class(env, JC_DESTINATION), dest_ctor);
    env->CallObjectMethod(destination, readBytes, fis);
    auto base32 = (jstring)env->CallObjectMethod(destination, toBase32);
    if (!base32)
        return std::string();
    return std::string(env->GetStringUTFChars(base32, 0));
}

/**
 *  router.shutdownGracefully();
 */
void Router::stop()
{
    auto env = get_env();
    auto shutdownGracefully = get_router_method(env, "shutdownGracefully", "()V");
    env->CallVoidMethod(m_router, shutdownGracefully);
    cleanup();
}

bool Router::callVoidReturnBool(const std::string &name)
{
    auto env = get_env();
    auto method = get_router_method(env, name, "()Z");
    auto rv = env->CallBooleanMethod(m_router, method);
    return rv == JNI_TRUE;
}

bool Router::isAlive(void)
{
    return callVoidReturnBool("isAlive");
}

bool Router::isRunning(void)
{
    return callVoidReturnBool("isRunning");
}

std::string Router::version(void)
{
    return getProperty("router.version");
}

std::string Router::status(void)
{
    if(!started())
        return "down";
    //TODO getReachability
    return (isRunning() ? "running (" : "warming up (") + version() + ")";
}

jobject Router::getRouterContext(JNIEnv *env)
{
    auto getContext = get_router_method(env, "getContext", "()Lnet/i2p/router/RouterContext;");
    return env->CallObjectMethod(m_router, getContext);
}

RouterStats Router::getAllStats(void)
{
    auto env = get_env();
    auto context = getRouterContext(env);
    auto statManager = get_method(env, JC_ROUTERCONTEXT, "statManager", std::string("()L").append(JC_STATMANAGER).append(";"));
    auto manager = env->CallObjectMethod(context, statManager);
    auto getRateNames = get_method(env, JC_STATMANAGER, "getRateNames", "()Ljava/util/Set;");
    auto names = (jobjectArray)env->CallObjectMethod(manager, getRateNames);

    jclass setClass = env->FindClass("java/util/Set");
    jclass iteratorClass = env->FindClass("java/util/Iterator");
    jmethodID iteratorMethod = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
    auto iterator = env->CallObjectMethod(names, iteratorMethod);

    auto hasNext = get_method(env, "java/util/Iterator", "hasNext", "()Z");
    auto next = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    auto getRate = get_method(env, JC_STATMANAGER, "getRate", "(Ljava/lang/String;)Lnet/i2p/stat/RateStat;");
    auto getPeriodRate = get_method(env, "net/i2p/stat/RateStat", "getRate", "(J)Lnet/i2p/stat/Rate;");
    auto getLifetimeAverageValue = get_method(env, "net/i2p/stat/RateStat", "getLifetimeAverageValue", "()D");
    auto getAverageValue = get_method(env, "net/i2p/stat/Rate", "getAverageValue", "()D");

    RouterStats stats;
    // TODO choose periods
    auto periods = {"0", "60000", "300000", "600000"};
    std::cout.setf(std::ios::left, std::ios::adjustfield);
    while (env->CallBooleanMethod(iterator, hasNext))
    {
        auto entry = (jstring)env->CallObjectMethod(iterator, next);
        const char* c_string = env->GetStringUTFChars(entry, 0);
        // TODO env->ReleaseStringUTFChars(env, obj, c_string);
        auto rate = env->CallObjectMethod(manager, getRate, entry);
        std::vector<double> values;
        for (const auto &period: periods)
        {
            if(strncmp(period, "0", 1) == 0)
            {
                values.push_back(env->CallDoubleMethod(rate, getLifetimeAverageValue));
            }
            else
            {
                auto period_rate = env->CallObjectMethod(rate, getPeriodRate, env->NewStringUTF(period));
                values.push_back(env->CallDoubleMethod(period_rate, getAverageValue));
            }
        }
        stats[c_string] = values;
    }
    // TODO add frequency stats
    return stats;
}

} // jni2p
