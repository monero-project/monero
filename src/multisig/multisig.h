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

#pragma once

#include <vector>
#include <unordered_map>
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctTypes.h"

namespace cryptonote
{
  struct account_keys;

  crypto::secret_key get_multisig_blinded_secret_key(const crypto::secret_key &key);
  void generate_multisig_N_N(const account_keys &keys, const std::vector<crypto::public_key> &spend_keys, std::vector<crypto::secret_key> &multisig_keys, rct::key &spend_skey, rct::key &spend_pkey);
  void generate_multisig_N1_N(const account_keys &keys, const std::vector<crypto::public_key> &spend_keys, std::vector<crypto::secret_key> &multisig_keys, rct::key &spend_skey, rct::key &spend_pkey);
  crypto::secret_key generate_multisig_view_secret_key(const crypto::secret_key &skey, const std::vector<crypto::secret_key> &skeys);
  crypto::public_key generate_multisig_N1_N_spend_public_key(const std::vector<crypto::public_key> &pkeys);
  bool generate_multisig_key_image(const account_keys &keys, size_t multisig_key_index, const crypto::public_key& out_key, crypto::key_image& ki);
  void generate_multisig_LR(const crypto::public_key pkey, const crypto::secret_key &k, crypto::public_key &L, crypto::public_key &R);
  bool generate_multisig_composite_key_image(const account_keys &keys, const std::unordered_map<crypto::public_key, cryptonote::subaddress_index>& subaddresses, const crypto::public_key& out_key, const crypto::public_key &tx_public_key, const std::vector<crypto::public_key>& additional_tx_public_keys, size_t real_output_index, const std::vector<crypto::key_image> &pkis, crypto::key_image &ki);
}
