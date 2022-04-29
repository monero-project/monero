// Copyright (c) 2021-2022, The Monero Project
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

#include "crypto/crypto.h"
#include "serialization/containers.h"
#include "serialization/crypto.h"
#include "serialization/serialization.h"

#include <cstdint>
#include <vector>


namespace multisig
{
  /// round 1 kex message
  struct multisig_kex_msg_serializable_round1
  {
    // privkey stored in msg
    crypto::secret_key msg_privkey;
    // pubkey used to sign this msg
    crypto::public_key signing_pubkey;
    // message signature
    crypto::signature signature;

    BEGIN_SERIALIZE()
      FIELD(msg_privkey)
      FIELD(signing_pubkey)
      FIELD(signature)
    END_SERIALIZE()
  };

  /// general kex message (if round > 1)
  struct multisig_kex_msg_serializable_general
  {
    // key exchange round this msg was produced for
    std::uint32_t kex_round;
    // pubkeys stored in msg
    std::vector<crypto::public_key> msg_pubkeys;
    // pubkey used to sign this msg
    crypto::public_key signing_pubkey;
    // message signature
    crypto::signature signature;

    BEGIN_SERIALIZE()
      VARINT_FIELD(kex_round)
      FIELD(msg_pubkeys)
      FIELD(signing_pubkey)
      FIELD(signature)
    END_SERIALIZE()
  };
} //namespace multisig
