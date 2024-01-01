// Copyright (c) 2017-2024, The Monero Project
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

#include "crypto/crypto.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_config.h"
#include "include_base_utils.h"
#include "multisig.h"
#include "ringct/rctOps.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "multisig"

namespace multisig
{
  //----------------------------------------------------------------------------------------------------------------------
  crypto::secret_key get_multisig_blinded_secret_key(const crypto::secret_key &key)
  {
    CHECK_AND_ASSERT_THROW_MES(key != crypto::null_skey, "Unexpected null secret key (danger!).");

    rct::key multisig_salt;
    static_assert(sizeof(rct::key) == sizeof(config::HASH_KEY_MULTISIG), "Hash domain separator is an unexpected size");
    memcpy(multisig_salt.bytes, config::HASH_KEY_MULTISIG, sizeof(rct::key));

    // private key = H(key, domain-sep)
    rct::keyV data;
    data.reserve(2);
    data.push_back(rct::sk2rct(key));
    data.push_back(multisig_salt);
    crypto::secret_key result = rct::rct2sk(rct::hash_to_scalar(data));
    memwipe(&data[0], sizeof(rct::key));
    return result;
  }
  //----------------------------------------------------------------------------------------------------------------------
  bool generate_multisig_key_image(const cryptonote::account_keys &keys,
    std::size_t multisig_key_index,
    const crypto::public_key& out_key,
    crypto::key_image& ki)
  {
    if (multisig_key_index >= keys.m_multisig_keys.size())
      return false;
    crypto::generate_key_image(out_key, keys.m_multisig_keys[multisig_key_index], ki);
    return true;
  }
  //----------------------------------------------------------------------------------------------------------------------
  void generate_multisig_LR(const crypto::public_key pkey,
    const crypto::secret_key &k,
    crypto::public_key &L,
    crypto::public_key &R)
  {
    rct::scalarmultBase((rct::key&)L, rct::sk2rct(k));
    crypto::generate_key_image(pkey, k, (crypto::key_image&)R);
  }
  //----------------------------------------------------------------------------------------------------------------------
  bool generate_multisig_composite_key_image(const cryptonote::account_keys &keys,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddresses,
    const crypto::public_key &out_key,
    const crypto::public_key &tx_public_key,
    const std::vector<crypto::public_key> &additional_tx_public_keys,
    std::size_t real_output_index,
    const std::vector<crypto::key_image> &pkis,
    crypto::key_image &ki)
  {
    // create a multisig partial key image
    // KI_partial = ([view key component] + [subaddress component] + [multisig privkeys]) * Hp(output one-time address)
    // - the 'multisig priv keys' here are those held by the local account
    // - later, we add in the components held by other participants
    cryptonote::keypair in_ephemeral;
    if (!cryptonote::generate_key_image_helper(keys, subaddresses, out_key, tx_public_key, additional_tx_public_keys, real_output_index, in_ephemeral, ki, keys.get_device()))
      return false;
    std::unordered_set<crypto::key_image> used;

    // create a key image component for each of the local account's multisig private keys
    for (std::size_t m = 0; m < keys.m_multisig_keys.size(); ++m)
    {
      crypto::key_image pki;
      // pki = keys.m_multisig_keys[m] * Hp(out_key)
      // pki = key image component
      // out_key = one-time address of an output owned by the multisig group
      bool r = generate_multisig_key_image(keys, m, out_key, pki);
      if (!r)
        return false;

      // this KI component is 'used' because it was included in the partial key image 'ki' above
      used.insert(pki);
    }

    // add the KI components from other participants to the partial KI
    // if they not included yet
    for (const auto &pki: pkis)
    {
      if (used.find(pki) == used.end())
      {
        // ignore components that have already been 'used'
        used.insert(pki);

        // KI_partial = KI_partial + KI_component[...]
        rct::addKeys((rct::key&)ki, rct::ki2rct(ki), rct::ki2rct(pki));
      }
    }

    // at the end, 'ki' will hold the true key image for our output if inputs were sufficient
    // - if 'pkis' (the other participants' KI components) is missing some components
    //   then 'ki' will not be complete

    return true;
  }
  //----------------------------------------------------------------------------------------------------------------------
} //namespace multisig
