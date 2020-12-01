// Copyright (c) 2017-2021, The Monero Project
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

#include "device_registry_impl.hpp"
#include "device.hpp"
#include "device_default.hpp"
#ifdef WITH_DEVICE_LEDGER
#include "device_ledger.hpp"
#endif
#include "misc_log_ex.h"


namespace hw {
    
    class device_registry_factory : public device_registry_factory_abstract
    {
    public:
        device_registry_factory(){}
        virtual ~device_registry_factory(){}
        device_registry * Create() const override
        {
            return new device_registry_impl();
        }
    };
    /* ======================================================================= */
    /*  SETUP                                                                  */
    /* ======================================================================= */
    static void clear_device_registry(){
      const device_registry_factory reg_factory;
      get_device_registry(reg_factory, true);
    }
    
    device_registry_impl::device_registry_impl(){
        hw::core::register_all(registry);
        #ifdef WITH_DEVICE_LEDGER
        hw::ledger::register_all(registry);
        #endif
        atexit(clear_device_registry);
    }
    
    void device_registry_impl::init()
    {
        // Protects against using the device_registry directly
    }
    
    device& get_device(const std::string & device_descriptor) {
        const device_registry_factory reg_factory;
        device_registry *registry = get_device_registry(reg_factory);
        return registry->get_device(device_descriptor);
    }

    bool register_device(const std::string & device_name, device * hw_device){
        const device_registry_factory reg_factory;
        device_registry *registry = get_device_registry(reg_factory);
        return registry->register_device(device_name, hw_device);
    }

}
