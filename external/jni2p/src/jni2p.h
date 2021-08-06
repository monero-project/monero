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

#include "utils.h"
#include <iomanip>
#include <ostream>

namespace jni2p
{

class Router
{
public:
    // Main constructor
    // In Android, no need to create a new JVM
    Router(JavaVM *jvm, const RouterProperties &);
    // Constructor for command line args
    Router(const std::vector<std::string> &args, const RouterProperties &defaults);
    virtual ~Router();

    // Lifecycle management
    void start(void);
    void stop(void);
    bool started(void) const;
    void waitForRunning(void);

    // Create I2P client tunnel to destination
    std::string connectTo(const std::string &);
    // Create I2P server tunnel with specific identity and forwards traffic to address
    std::string listen(const std::string &, const std::string &);

    std::string createDestination(const std::string &);
    std::string getDestination(const std::string &);

    std::string version(void);
    std::string status(void);
    RouterStats getAllStats(void);

    std::string getProperty(const std::string &, const std::string & = std::string());

    template<typename T> void printStats(T& writer)
    {
        auto stats = getAllStats();
        writer << std::left;
        for(auto stat: stats)
        {
            writer << std::setw(44) << stat.first;
            for(auto value: stat.second)
            {
                writer << " | " << std::setw(12) << value;
            }
            writer << "\n";
        }
    }

private:
    std::string getJavaProperty(const std::string &, const std::string & = std::string());
    jobject getRouterContext(JNIEnv *env);
    jobject executeI2PTunnelCommand(const std::string &);

    bool callVoidReturnBool(const std::string &name);
    bool isAlive(void);
    bool isRunning(void);

    void cleanup(void);
    JNIEnv* get_env(void);

    JavaVM *m_jvm;
    bool m_created_jvm;
    RouterProperties m_local_properties;
    jobject m_router;
};

} // jni2p
