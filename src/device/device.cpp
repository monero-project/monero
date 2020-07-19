// Copyright (c) 2017-2020, The Monero Project
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

    static device_registry *get_device_registry(bool clear = false){
      static device_registry *registry = new device_registry();
      if (clear)
      {
        delete registry;
        registry = NULL;
      }
      return registry;
    }

    static void clear_device_registry(){
      get_device_registry(true);
    }

    device_registry::device_registry(){
        hw::core::register_all(registry);
        #ifdef WITH_DEVICE_LEDGER
        hw::ledger::register_all(registry);
        #endif
        atexit(clear_device_registry);
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

    device& get_device(const std::string & device_descriptor) {
        device_registry *registry = get_device_registry();
        return registry->get_device(device_descriptor);
    }

    bool register_device(const std::string & device_name, device * hw_device){
        device_registry *registry = get_device_registry();
        return registry->register_device(device_name, hw_device);
    }

}
