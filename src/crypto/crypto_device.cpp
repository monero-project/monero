// Copyright (c) 2014-2018, The Monero Project
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


#include "crypto.h"
#include "device/device.hpp"
#include "device/log.hpp"

namespace crypto {

    secret_key generate_keys(public_key &pub, secret_key &sec, const secret_key& recovery_key, bool recover, hw::device &hwdev) {
        return hwdev.generate_keys(pub, sec, recovery_key, recover);
    }

    secret_key generate_keys(public_key &pub, secret_key &sec, hw::device &hwdev) {
        return hwdev.generate_keys(pub, sec,  secret_key(), false);
    }


    bool secret_key_to_public_key(const secret_key &sec, public_key &pub, hw::device &hwdev) {
       return hwdev.secret_key_to_public_key(sec, pub);
    }

    bool generate_key_derivation(const public_key &key1, const secret_key &key2, key_derivation &derivation, hw::device &hwdev) {
        return hwdev.generate_key_derivation(key1, key2, derivation);
    }

    void derivation_to_scalar(const key_derivation &derivation, size_t output_index, ec_scalar &res, hw::device &hwdev) {
        hwdev.derivation_to_scalar(derivation, output_index, res);
    }

    bool derive_public_key(const key_derivation &derivation, size_t output_index,
                           const public_key &base, public_key &derived_key, hw::device &hwdev) {
        return hwdev.derive_public_key(derivation, output_index, base, derived_key);
    }

    void derive_secret_key(const key_derivation &derivation, size_t output_index,
                           const secret_key &base, secret_key &derived_key, hw::device &hwdev) {
        hwdev.derive_secret_key(derivation, output_index, base, derived_key);
    }

    bool derive_subaddress_public_key(const public_key &out_key, const key_derivation &derivation, std::size_t output_index, public_key &derived_key, hw::device &hwdev) {
        return hwdev.derive_subaddress_public_key(out_key, derivation, output_index, derived_key);
    }

    void generate_key_image(const public_key &pub, const secret_key &sec, key_image &image, hw::device &hwdev) {
        hwdev.generate_key_image(pub,sec,image);
    }
}