// Copyright (c) 2017, The Monero Project
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

#include <cstddef>
#include "crypto/crypto.h"

extern "C" {
    int monero_wallet_amd64_64_24k_get_tx_key(char*, char const*, char const*);
    int monero_wallet_amd64_64_24k_get_tx_output_public(char*, char const*, char const*);
}

namespace tools {
    namespace wallet_only {
        namespace amd64_64_24k {
            inline bool generate_key_derivation(
                crypto::public_key const& pub, crypto::secret_key const& sec, crypto::key_derivation& out
            ) {
                return monero_wallet_amd64_64_24k_get_tx_key(out.data, pub.data, sec.data) == 0;
            }

            inline bool derive_public_key(
                crypto::key_derivation const& d, std::size_t index, crypto::public_key const& pub, crypto::public_key& out
            ) {
                ec_scalar scalar;
                crypto::derivation_to_scalar(d, index, scalar);
                return monero_wallet_amd64_64_24k_get_tx_output_public(out.data, pub.data, scalar.data) == 0;
            }
        }
    }
}
