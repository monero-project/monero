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

// Fuzzes the wallet's pending-tx sanity checks (pending_tx_validation.cpp): these guard
// unsigned/pending tx sets handed to a wallet by an untrusted coordinator (exported
// unsigned-tx files, MMS, multisig), so the wallet2::pending_tx /
// tx_construction_data structures are built from attacker-controlled bytes and pushed
// through sanity_check_pending_tx(), check_consistent_ins_outs() and
// sanity_check_tx_construction_data().
//
// The wallet account keys and the pending transaction itself are built with the
// real construction path (construct_tx_and_get_tx_key), so the validators run
// against a cryptographically consistent transaction: valid keys (reduced
// scalars), matching ephemeral output keys, masks and key images.  Mutations
// of the fuzz input drive the option profile, the amounts, the destination
// count and the wire format knobs.

#include <cstring>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "ringct/rctOps.h"
#include "wallet/pending_tx_validation.h"
#include "wallet/wallet2_basic/wallet2_types.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

namespace
{
  // Deterministic, valid secret key: hash a fixed label and reduce modulo
  // the group order.  Repeated-byte scalars (e.g. all 0xab) exceed the
  // group order, so secret_key_to_public_key() would fail and every input
  // would return before reaching the validators.
  crypto::secret_key valid_secret_key(const char *label, std::size_t len)
  {
    crypto::hash h;
    crypto::cn_fast_hash(label, len, h);
    crypto::secret_key k;
    memcpy(k.data, h.data, sizeof(k.data));
    sc_reduce32(reinterpret_cast<unsigned char *>(k.data));
    return k;
  }

  void copy32_into(void *dst, const std::string &src)
  {
    memset(dst, 0, 32);
    memcpy(dst, src.data(), src.size() < 32 ? src.size() : 32);
  }

  // Deterministic valid curve point (for destinations and ring members):
  // invalid points would make the transaction construction fail.
  crypto::public_key valid_public_key(uint64_t seed)
  {
    crypto::secret_key sk;
    crypto::hash_to_scalar(&seed, sizeof(seed), sk);
    crypto::public_key pk;
    if (!crypto::secret_key_to_public_key(sk, pk))
      pk = crypto::null_pkey;
    return pk;
  }
} // anonymous namespace

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);

  // Configuration from the back.
  const std::size_t n_vout = 1 + fdp.ConsumeIntegralInRange<std::size_t>(0, 3);
  const std::size_t n_vin = 1 + fdp.ConsumeIntegralInRange<std::size_t>(0, 1);
  const std::size_t n_add_keys = fdp.ConsumeIntegralInRange<std::size_t>(0, 2);
  const std::size_t n_dests = fdp.ConsumeIntegralInRange<std::size_t>(0, 3);
  const std::size_t n_transfers = fdp.ConsumeIntegralInRange<std::size_t>(0, 3);
  const std::size_t extra_len = fdp.ConsumeIntegralInRange<std::size_t>(0, 48);
  const bool redacted = fdp.ConsumeBool();
  const bool allow_read_only = fdp.ConsumeBool();
  const bool use_view_tags = !fdp.ConsumeBool();
  const cryptonote::network_type nettype = (cryptonote::network_type)
      fdp.ConsumeIntegralInRange<unsigned>(0, 3);

  // Valid deterministic wallet account keys (see valid_secret_key above):
  // key derivation succeeds, so the validators are always reached.
  cryptonote::account_keys account_keys;
  account_keys.m_spend_secret_key = valid_secret_key("spend", 5);
  account_keys.m_view_secret_key = valid_secret_key("view", 4);
  crypto::public_key spend_pub, view_pub;
  if (!crypto::secret_key_to_public_key(account_keys.m_spend_secret_key, spend_pub))
    return 0;
  if (!crypto::secret_key_to_public_key(account_keys.m_view_secret_key, view_pub))
    return 0;
  account_keys.m_account_address.m_spend_public_key = spend_pub;
  account_keys.m_account_address.m_view_public_key = view_pub;

  std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
  subaddresses.emplace(spend_pub, cryptonote::subaddress_index{});

  // Transfers (wallet-owned ring members the checks may consult).
  std::vector<wallet2_basic::transfer_details> transfers(n_transfers);
  for (std::size_t i = 0; i < n_transfers; ++i)
  {
    transfers[i].m_block_height = 1;
    transfers[i].m_txid = crypto::null_hash;
    transfers[i].m_internal_output_index = 0;
    transfers[i].m_global_output_index = fdp.ConsumeIntegralInRange<uint64_t>(0, 64);
    transfers[i].m_spent = false;
    transfers[i].m_frozen = false;
    transfers[i].m_spent_height = 0;
    copy32_into(transfers[i].m_key_image.data, fdp.ConsumeBytesAsString(32));
    transfers[i].m_amount = fdp.ConsumeIntegralInRange<uint64_t>(0, 4096);
    transfers[i].m_rct = true;
    transfers[i].m_key_image_known = true;
  }

  // Ring output public keys and destination addresses must be valid curve
  // points for the transaction construction to succeed; derive them from
  // valid scalars (deterministic per index).
  auto valid_public_key = [](uint64_t seed) {
    crypto::secret_key sk;
    crypto::hash_to_scalar(&seed, sizeof(seed), sk);
    crypto::public_key pk;
    if (!crypto::secret_key_to_public_key(sk, pk))
      pk = crypto::null_pkey;
    return pk;
  };

  // Transaction sources: a ring of two valid output keys with index 0 as the
  // real output.
  std::vector<cryptonote::tx_source_entry> sources(1);
  {
    cryptonote::tx_source_entry &src = sources[0];
    src.outputs.resize(2);
    for (std::size_t j = 0; j < 2; ++j)
    {
      src.outputs[j].first = j;
      src.outputs[j].second.dest = rct::pk2rct(valid_public_key(0x60 + j));
      src.outputs[j].second.mask = rct::zero();
    }
    src.real_output = 0;
    src.amount = fdp.ConsumeIntegralInRange<uint64_t>(0, 4096);
    src.rct = true;
  }

  // Destinations: valid addresses owned by distinct derived keys, with the
  // wallet's own view key so the ephemeral key re-derivation in the
  // validator matches the construction.
  std::vector<cryptonote::tx_destination_entry> destinations(n_dests);
  for (std::size_t i = 0; i < n_dests; ++i)
  {
    cryptonote::tx_destination_entry d;
    d.amount = fdp.ConsumeIntegralInRange<uint64_t>(0, 4096);
    d.addr.m_spend_public_key = valid_public_key(0x40 + i);
    d.addr.m_view_public_key = view_pub;
    d.is_subaddress = fdp.ConsumeBool();
    d.is_integrated = false;
    destinations[i] = d;
  }
  const std::vector<uint8_t> tx_extra = fdp.ConsumeBytes<std::uint8_t>(extra_len);

  // Build a genuinely valid, signed rct transaction with the real
  // construction path: the validators re-derive ephemeral keys, ecdh amounts
  // and commitments from the transaction key, so only a properly constructed
  // transaction can pass their checks.
  cryptonote::transaction tx;
  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> additional_tx_keys;
  const boost::optional<cryptonote::account_public_address> change_addr(
      account_keys.m_account_address);
  if (!cryptonote::construct_tx_and_get_tx_key(account_keys, subaddresses,
          sources, destinations, change_addr, tx_extra, tx, tx_key,
          additional_tx_keys, true, {rct::RangeProofBorromean, 0}, use_view_tags))
    return 0;

  tools::wallet2::pending_tx ptx{};
  ptx.tx = tx;
  ptx.tx_key = tx_key;
  ptx.additional_tx_keys = additional_tx_keys;
  // Key images in the wallet's format: space-separated, mirroring what
  // sanity_check_pending_tx() re-derives from the transaction's inputs.
  std::string key_images;
  for (const auto &in : tx.vin)
    key_images += boost::to_string(boost::get<cryptonote::txin_to_key>(in).k_image) + " ";
  ptx.key_images = key_images;
  ptx.fee = fdp.ConsumeIntegralInRange<uint64_t>(0, 1024);
  ptx.dust = 0;
  ptx.dust_added_to_fee = false;
  ptx.change_dts.amount = fdp.ConsumeIntegralInRange<uint64_t>(0, 4096);
  ptx.change_dts.addr = account_keys.m_account_address;
  ptx.selected_transfers.push_back(0);
  ptx.dests = destinations;

  // construction_data mirroring the tx above.
  tools::wallet2::tx_construction_data &cd = ptx.construction_data;
  cd.use_rct = true;
  cd.rct_config = {rct::RangeProofBorromean, 0};
  cd.use_view_tags = use_view_tags;
  cd.unlock_time = 0;
  cd.extra = ptx.tx.extra;
  cd.subaddr_account = 0;
  cd.subaddr_indices.insert(0);
  cd.change_dts = ptx.change_dts;
  cd.selected_transfers.push_back(0);
  cd.dests = ptx.dests;
  cd.sources = sources;
  cd.splitted_dsts = destinations;

  // The checks under test: each validator runs on its own, so a throw from
  // one cannot prevent the others from being exercised on the same input.
  const auto run_validator = [](const char *name, auto &&check) {
    try
    {
      check();
    }
    catch (const std::exception &) {}
  };
  run_validator("pending_tx", [&] {
    tools::wallet::sanity_check_pending_tx(ptx, nettype, account_keys,
        subaddresses, transfers, redacted, std::nullopt, allow_read_only);
  });
  run_validator("ins_outs", [&] {
    tools::wallet::check_consistent_ins_outs(
        std::vector<tools::wallet2::pending_tx>{ptx});
  });
  run_validator("tx_construction_data", [&] {
    tools::wallet::sanity_check_tx_construction_data(ptx.construction_data);
  });
  run_validator("unsigned_tx_set", [&] {
    tools::wallet::sanity_check_unsigned_tx_set(
        std::vector<tools::wallet2::tx_construction_data>{cd},
        account_keys, subaddresses);
  });
  run_validator("pending_tx_set", [&] {
    tools::wallet::sanity_check_pending_tx_set(
        std::vector<tools::wallet2::pending_tx>{ptx},
        nettype, account_keys, subaddresses, transfers, redacted, std::nullopt, allow_read_only);
  });
END_SIMPLE_FUZZER()
