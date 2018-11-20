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

#pragma once

#include <cstddef>
#include <string>

#include "ringct/rctOps.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/account.h"

#include "device.hpp"

namespace hw {

    void buffer_to_str(char *to_buff,  size_t to_len, const char *buff, size_t len) ;
    void log_hexbuffer(const std::string &msg,  const char* buff, size_t len);
    void log_message(const std::string &msg, const std::string &info );

    #ifdef WITH_DEVICE_LEDGER    
    namespace ledger {

        #ifdef DEBUG_HWDEVICE
        #define TRACK printf("file %s:%d\n",__FILE__, __LINE__)
        //#define TRACK MCDEBUG("ledger"," At file " << __FILE__ << ":" << __LINE__)
        //#define TRACK while(0);

        void decrypt(char* buf, size_t len) ;
        crypto::key_derivation decrypt(const crypto::key_derivation &derivation) ;
        cryptonote::account_keys decrypt(const cryptonote::account_keys& keys) ;
        crypto::secret_key decrypt(const crypto::secret_key &sec) ;
        rct::key  decrypt(const rct::key &sec);
        crypto::ec_scalar decrypt(const crypto::ec_scalar &res);
        rct::keyV decrypt(const rct::keyV &keys);

        void check32(const std::string &msg, const std::string &info, const char *h, const char *d, bool crypted=false);
        void check8(const std::string &msg, const std::string &info, const char *h, const char *d,  bool crypted=false);

        void set_check_verbose(bool verbose);
        #endif
    }
    #endif
}
