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
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of
//    conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific prior
//    written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
// TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Fuzzes the wallet's unsigned/signed tx-set FILE parsing (wallet2.cpp
// parse_unsigned_tx_from_str / parse_tx_from_str): the magic-prefix gate, the
// format version dispatch, the view-key-encrypted container path, and the
// binary_archive deserialization of the full unsigned_tx_set / signed_tx_set
// trees. Uses the key-aware-writer pattern (fixed deterministic wallet keys) so
// the decrypted plaintext is attacker-controlled. Distinct from the wallet cache
// format and from the (excluded) wallet2 refresh process.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "serialization/binary_archive.h"
#include "wallet/wallet2.h"
#include "wallet/wallet2_basic/wallet2_serialization.h"

#define UNSIGNED_TX_PREFIX "Monero unsigned tx set\005"
#define SIGNED_TX_PREFIX "Monero signed tx set\005"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

namespace
{
  tools::wallet2 *wallet = NULL;
}

BEGIN_INIT_SIMPLE_FUZZER()
  static tools::wallet2 local_wallet(cryptonote::TESTNET, /*kdf_rounds=*/1, /*unattended=*/true);
  static const char * const spendkey_hex = "f285d4ac9e66271256fc7cde0d3d6b36f66efff6ccd766706c408e86f4997a0d";
  crypto::secret_key spendkey;
  epee::string_tools::hex_to_pod(spendkey_hex, spendkey);
  local_wallet.init("", boost::none, "", 0, true, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
  local_wallet.set_subaddress_lookahead(1, 1);
  local_wallet.generate("", "", spendkey, true, false);
  wallet = &local_wallet;
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);
  const unsigned sel = fdp.ConsumeIntegralInRange<unsigned>(0, 2);
  std::string input;
  const std::string blob = fdp.ConsumeBytesAsString(
      std::min<std::size_t>(fdp.remaining_bytes(), 4096));

  if (sel == 2)
  {
    // Key-aware writer: serialize a fuzz-populated unsigned_tx_set, encrypt it with
    // the wallet's view key, wrap it in the current (encrypted) file format, and
    // hand it to the parser — the decrypt -> binary_archive chain with structured
    // plaintext, exactly what a key-compromised/MMS-adjacent handoff produces.
    tools::wallet2::unsigned_tx_set txs{};
    tools::wallet2::tx_construction_data &cd = txs.txes.emplace_back();
    cd.use_rct = true;
    cd.rct_config = {rct::RangeProofBorromean, 0};
    cd.use_view_tags = true;
    cd.subaddr_account = 0;
    cd.subaddr_indices.insert(0);
    cd.selected_transfers.push_back(0);
    cd.sources.resize(1);
    cd.sources[0].outputs.resize(2);
    for (std::size_t j = 0; j < 2; ++j)
    {
      cd.sources[0].outputs[j].first = j;
      memset(&cd.sources[0].outputs[j].second, (int)(0x60 + j),
          sizeof(cd.sources[0].outputs[j].second));
    }
    cd.sources[0].real_output = 0;
    cd.sources[0].amount = fdp.ConsumeIntegralInRange<uint64_t>(0, 4096);
    cd.sources[0].rct = true;
    cryptonote::tx_destination_entry d;
    d.amount = fdp.ConsumeIntegralInRange<uint64_t>(0, 4096);
    d.addr = wallet->get_account().get_keys().m_account_address;
    cd.splitted_dsts.push_back(d);
    std::stringstream oss;
    binary_archive<true> ar(oss);
    if (!::serialization::serialize(ar, txs))
      return 0;
    input = std::string(UNSIGNED_TX_PREFIX) + wallet->encrypt_with_view_secret_key(oss.str());
    tools::wallet2::unsigned_tx_set parsed{};
    wallet->parse_unsigned_tx_from_str(input, parsed);
    return 0;
  }

  if (sel == 0)
    input = std::string(UNSIGNED_TX_PREFIX) + blob;      // unencrypted path
  else
    input = std::string(SIGNED_TX_PREFIX) + blob;        // signed tx set path

  try
  {
    if (sel == 0)
    {
      tools::wallet2::unsigned_tx_set out{};
      wallet->parse_unsigned_tx_from_str(input, out);
    }
    else
    {
      std::vector<tools::wallet2::pending_tx> ptx;
      wallet->parse_tx_from_str(input, ptx, NULL);
    }
  }
  catch (...) {}
END_SIMPLE_FUZZER()
