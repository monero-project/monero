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

#include "device.hpp"
#include "device_default.hpp"
#ifdef HAVE_PCSC
#include "device_ledger.hpp"
#endif
#include "misc_log_ex.h"


namespace hw {
    
    /* ======================================================================= */
    /*  SETUP                                                                  */
    /* ======================================================================= */   
    device& get_device(const std::string device_descriptor) {
        
        struct s_devices {
            std::map<std::string, std::unique_ptr<device>> registry;
            s_devices() : registry() {
                hw::core::register_all(registry);
                #ifdef HAVE_PCSC
                hw::ledger::register_all(registry); 
                #endif
           };
        };
        
        static const s_devices devices;

        auto device = devices.registry.find(device_descriptor);
        if (device == devices.registry.end()) {
            MERROR("device not found in registry: '" << device_descriptor << "'\n" <<
                      "known devices:");
            
            for( const auto& sm_pair : devices.registry ) {
                MERROR(" - " << sm_pair.first);
            }
            throw std::runtime_error("device not found: "+ device_descriptor);
        }
        return *device->second;
    }

}
