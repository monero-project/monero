// Copyright (c) 2026, The Monero Project
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

#include <string>
#include <vector>

#include "include_base_utils.h"
#include "string_tools.h"
#include "wallet/wallet2.h"

static const std::string MULTISIG_UNSIGNED_TX_PREFIX = "Monero multisig unsigned tx set\001";
static const std::string MULTISIG_EXPORT_FILE_MAGIC = "Monero multisig export\001";

static crypto::public_key multisig_other_signer;

static void multisig_init_make_wallet(tools::wallet2 &wallet, const char *spendkey_hex)
{
  crypto::secret_key spendkey;
  CHECK_AND_ASSERT_THROW_MES(epee::string_tools::hex_to_pod(spendkey_hex, spendkey), "Bad spend key");

  wallet.init("", boost::none, "", 0, true, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
  wallet.set_subaddress_lookahead(1, 1);
  wallet.generate("", "", spendkey, true, false);
}

inline static void multisig_init(tools::wallet2 &wallet)
{
  static const char* const spendkey_hex_1 = "2dd6e34a234c3e8b5d29a371789e4601e96dee4ea6f7ef79224d1a2d91164c01";
  static const char* const spendkey_hex_2 = "fac47aecc948ce9d3531aa042abb18235b1df632087c55a361b632ffdd6ede0c";

  tools::wallet2 other;
  multisig_init_make_wallet(wallet, spendkey_hex_1);
  multisig_init_make_wallet(other, spendkey_hex_2);

  wallet.decrypt_keys("");
  other.decrypt_keys("");
  const std::vector<std::string> initial_kex_msgs{wallet.get_multisig_first_kex_msg(), other.get_multisig_first_kex_msg()};
  wallet.encrypt_keys("");
  other.encrypt_keys("");

  std::vector<std::string> kex_msgs{wallet.make_multisig("", initial_kex_msgs, 2), other.make_multisig("", initial_kex_msgs, 2)};
  while (!wallet.get_multisig_status().is_ready || !other.get_multisig_status().is_ready)
  {
    kex_msgs = {wallet.exchange_multisig_keys("", kex_msgs), other.exchange_multisig_keys("", kex_msgs)};
  }

  wallet.decrypt_keys("");
  other.decrypt_keys("");
  multisig_other_signer = other.get_multisig_signer_public_key();
}
