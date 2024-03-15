// Copyright (c) 2020-2024, The Monero Project

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
#include "crypto/wallet/ops.h"

namespace crypto {
  namespace wallet {
// if C functions defined from external/supercop - cmake generates crypto/wallet/ops.h
#if defined(monero_crypto_generate_key_derivation)
      inline
      bool generate_key_derivation(const public_key &tx_pub, const secret_key &view_sec, key_derivation &out)
      {
        return monero_crypto_generate_key_derivation(out.data, tx_pub.data, view_sec.data) == 0;
      }

      inline
      bool derive_subaddress_public_key(const public_key &output_pub, const key_derivation &d, std::size_t index, public_key &out)
      {
        ec_scalar scalar;
        derivation_to_scalar(d, index, scalar);
        return monero_crypto_generate_subaddress_public_key(out.data, output_pub.data, scalar.data) == 0;
      }
#else
    using ::crypto::generate_key_derivation;
    using ::crypto::derive_subaddress_public_key;
#endif
  }
}
