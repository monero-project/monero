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

#pragma once

#include <jni.h>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace jni2p
{

typedef std::unordered_map<std::string, std::string> RouterProperties;
typedef std::map<std::string, std::vector<double> > RouterStats;

// Java classes
#define JC_OBJECT "java/lang/Object"
#define JC_STRING "java/lang/String"
#define JC_PROPERTIES "java/util/Properties"
#define JC_ROUTER "net/i2p/router/Router"
#define JC_ROUTERCONTEXT "net/i2p/router/RouterContext"
#define JC_I2PTUNNEL "net/i2p/i2ptunnel/I2PTunnel"
#define JC_STATMANAGER "net/i2p/stat/StatManager"
#define JC_STATISTICSMANAGER "net/i2p/router/StatisticsManager"
#define JC_OUTPUTSTREAM "java/io/OutputStream"
#define JC_BYTEARRAYOUTPUTSTREAM "java/io/ByteArrayOutputStream"
#define JC_DESTINATION "net/i2p/data/Destination"

// TODO provide contextual defaults ?
static const RouterProperties default_properties {
    {"i2p.dir.base.template", "i2p.base"}
   ,{"router.sharePercentage", "80"}
   ,{"i2np.bandwidth.inboundKBytesPerSecond", "xy"}
   ,{"i2np.bandwidth.inboundBurstKBytesPerSecond", "xy"}
   ,{"i2np.bandwidth.inboundBurstKBytes", "xy"}
   ,{"i2np.bandwidth.outboundKBytesPerSecond", "xy"}
   ,{"i2np.bandwidth.outboundBurstKBytesPerSecond", "xy"}
   ,{"i2np.bandwidth.outboundBurstKBytes", "xy"}
};
static const RouterProperties client_default_properties {
    {"router.floodfillParticipant", "0"}
   ,{"i2np.laptopMode", "1"}
};
static const RouterProperties server_default_properties {
    {"router.floodfillParticipant", "1"}
};

int create_jvm(JavaVM **jvm, const std::string &);

// Class & method getters
jclass get_class(JNIEnv *env, const std::string &name);
jclass get_java_router_class(JNIEnv *env);
jmethodID get_method(JNIEnv *env, const std::string&, const std::string &, const std::string &);
jmethodID get_router_method(JNIEnv *env, const std::string &, const std::string &);

// type helpers
RouterProperties args_to_properties(const std::vector<std::string> &);
jobject router_properties_to_java(JNIEnv *env, const RouterProperties&);
std::string properties_to_string(JNIEnv*, jobject);

} // jni2p
