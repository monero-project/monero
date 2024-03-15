// Copyright (c) 2014-2024, The Monero Project
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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"

#include "single_tx_test_base.h"

using namespace crypto;

// use_view_tags: whether to enable view tag checking
// is_owned: whether the output is owned by us
template<bool use_view_tags, bool is_owned>
class test_out_can_be_to_acc : public single_tx_test_base
{
  public:
    static const size_t loop_count = 1000;

    bool init()
    {
      if (!single_tx_test_base::init())
        return false;

      crypto::key_derivation key_derivation;
      crypto::view_tag vt;

      m_output_index = 0;
      m_view_secret_key = m_bob.get_keys().m_view_secret_key;
      m_spend_public_key = m_bob.get_keys().m_account_address.m_spend_public_key;

      cryptonote::get_output_public_key(m_tx.vout[m_output_index], m_output_public_key);

      if (use_view_tags)
      {
        crypto::generate_key_derivation(m_tx_pub_key, m_view_secret_key, key_derivation);
        crypto::derive_view_tag(key_derivation, m_output_index, vt);
        m_view_tag_opt = vt;
      }
      else
        m_view_tag_opt = boost::optional<crypto::view_tag>();

      return true;
    }

    bool test()
    {
      // include key derivation to demonstrate performance improvement when using view tags
      crypto::key_derivation key_derivation;
      crypto::generate_key_derivation(m_tx_pub_key, m_view_secret_key, key_derivation);

      // if using view tags, this ensures we computed the view tag properly
      if (!cryptonote::out_can_be_to_acc(m_view_tag_opt, key_derivation, m_output_index))
        return false;

      // if user owns output, this tests the output public key matches the derived
      if (is_owned)
      {
        crypto::public_key output_public_key;
        crypto::derive_public_key(key_derivation, m_output_index, m_spend_public_key, output_public_key);

        if (m_output_public_key != output_public_key)
          return false;
      }

      return true;
    }

    private:
      size_t m_output_index;
      crypto::secret_key m_view_secret_key;
      crypto::public_key m_spend_public_key;
      crypto::public_key m_output_public_key;
      boost::optional<crypto::view_tag> m_view_tag_opt;
};
