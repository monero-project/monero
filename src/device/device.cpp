// Copyright (c) 2017-2018, The Monero Project
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
//

#include <memory>
#include <mutex>
#include "device.hpp"
#include "device_default.hpp"
#ifdef WITH_DEVICE_LEDGER
#include "device_ledger.hpp"
#endif
#include "misc_log_ex.h"


namespace hw {
    
    /* ======================================================================= */
    /*  SETUP                                                                  */
    /* ======================================================================= */

    static std::shared_ptr<device_registry> registry;
    static std::once_flag registry_init_flag;

    device_registry::device_registry(){
        hw::core::register_all(registry);
        #ifdef WITH_DEVICE_LEDGER
        hw::ledger::register_all(registry);
        #endif
    }

    bool device_registry::register_device(const std::string & device_name, device * hw_device){
        auto search = registry.find(device_name);
        if (search != registry.end()){
            return false;
        }

        registry.insert(std::make_pair(device_name, std::unique_ptr<device>(hw_device)));
        return true;
    }

    device& device_registry::get_device(const std::string & device_descriptor){
        // Device descriptor can contain further specs after first :
        auto delim = device_descriptor.find(':');
        auto device_descriptor_lookup = device_descriptor;
        if (delim != std::string::npos) {
            device_descriptor_lookup = device_descriptor.substr(0, delim);
        }

        auto device = registry.find(device_descriptor_lookup);
        if (device == registry.end()) {
            MERROR("Device not found in registry: '" << device_descriptor << "'. Known devices: ");
            for( const auto& sm_pair : registry ) {
                MERROR(" - " << sm_pair.first);
            }
            throw std::runtime_error("device not found: " + device_descriptor);
        }
        return *device->second;
    }

    void device_registry::deinit(){
        registry.clear();
    }

    device_registry::~device_registry() {
        deinit();
    }

    static void init_device_registry(){
        CHECK_AND_ASSERT_THROW_MES(!registry, "Device registry already initialized");
        const int tmp = atexit(deinit_device_registry);
        CHECK_AND_ASSERT_THROW_MES(!tmp, "Device registry finalizer registration failed");
        registry = std::make_shared<device_registry>();
    }

    std::shared_ptr<device_registry> get_device_registry(){
        if (!registry) {
          std::call_once(registry_init_flag, init_device_registry);
        }
        return registry;
    }

    void deinit_device_registry(){
        if (!registry){
          return;
        }
        registry->deinit();
    }

    device& get_device(const std::string & device_descriptor) {
        return get_device_registry()->get_device(device_descriptor);
    }

}
