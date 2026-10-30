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

// Fuzzes wallet input that comes from third parties: monero: payment URIs (parse_uri/make_uri)
// and transaction proofs (get_tx_proof/check_tx_proof on a given transaction, no daemon).

#include <cstdio>
#include <string>
#include <vector>

#include "include_base_utils.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "net/abstract_http_client.h"
#include "ringct/rctOps.h"
#include "wallet/wallet2.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

#define FUZZ_CHECK(cond) \
  do { if (!(cond)) { fprintf(stderr, "check failed: %s (line %d)\n", #cond, __LINE__); abort(); } } while (0)

static constexpr size_t kMaxOuts = 6;
static constexpr size_t kMaxUriParams = 6;

static tools::wallet2 *wallet = NULL;
static tools::wallet2 *foreign = NULL;
static std::vector<std::string> uri_addresses;

// Small seed space on purpose: tx keys repeat, producing shared derivations.
static crypto::secret_key consume_scalar(FuzzedDataProvider &fdp)
{
  const std::string seed = fdp.ConsumeBytesAsString(2);
  crypto::secret_key k;
  crypto::hash_to_scalar(seed.data(), seed.size(), k);
  return k;
}

static crypto::public_key tx_pub_key_for(const crypto::secret_key &r, const cryptonote::account_public_address &to, bool is_subaddress)
{
  if (is_subaddress)
    return rct::rct2pk(rct::scalarmultKey(rct::pk2rct(to.m_spend_public_key), rct::sk2rct(r)));
  return rct::rct2pk(rct::scalarmultBase(rct::sk2rct(r)));
}

//----------------------------------------------------------------------------------------------------------------------
// monero: URIs
//----------------------------------------------------------------------------------------------------------------------
static std::string consume_uri_value(FuzzedDataProvider &fdp)
{
  const std::string raw = fdp.ConsumeRandomLengthString(48);
  return fdp.ConsumeBool() ? epee::net_utils::conver_to_url_format(raw) : raw;
}

static std::string build_uri(FuzzedDataProvider &fdp)
{
  if (fdp.ConsumeIntegralInRange<int>(0, 7) == 0)
    return "monero:" + fdp.ConsumeRandomLengthString(512);

  static const char *const schemes[] = {"monero:", "monero:", "monero:", "Monero:", "bitcoin:", ""};
  std::string uri = fdp.PickValueInArray(schemes);
  uri += uri_addresses[fdp.ConsumeIntegralInRange<size_t>(0, uri_addresses.size() - 1)];

  const size_t n_params = fdp.ConsumeIntegralInRange<size_t>(0, kMaxUriParams);
  for (size_t i = 0; i < n_params; ++i)
  {
    uri += i == 0 ? "?" : "&";
    switch (fdp.ConsumeIntegralInRange<int>(0, 5))
    {
      case 0:
        uri += "tx_amount=";
        uri += fdp.ConsumeBool() ? cryptonote::print_money(fdp.ConsumeIntegral<uint64_t>()) : fdp.ConsumeRandomLengthString(24);
        break;
      case 1:
      {
        uri += "tx_payment_id=";
        crypto::hash payment_id = crypto::null_hash;
        const std::string bytes = fdp.ConsumeBytesAsString(sizeof(payment_id));
        memcpy(&payment_id, bytes.data(), bytes.size());
        uri += fdp.ConsumeBool() ? epee::string_tools::pod_to_hex(payment_id) : fdp.ConsumeRandomLengthString(70);
        break;
      }
      case 2:
        uri += "recipient_name=" + consume_uri_value(fdp);
        break;
      case 3:
        uri += "tx_description=" + consume_uri_value(fdp);
        break;
      case 4:
        uri += fdp.ConsumeRandomLengthString(16) + "=" + consume_uri_value(fdp);
        break;
      default:
        uri += fdp.ConsumeRandomLengthString(24);
        break;
    }
  }
  return uri;
}

static void fuzz_uri(FuzzedDataProvider &fdp)
{
  const std::string uri = build_uri(fdp);
  std::string address, payment_id, description, recipient, error;
  uint64_t amount = 0;
  std::vector<std::string> unknown;
  if (!wallet->parse_uri(uri, address, payment_id, amount, description, recipient, unknown, error))
    return;

  // make_uri refuses standalone payment ids, so only those URIs can't be rebuilt.
  if (!payment_id.empty())
    return;
  const std::string rebuilt = wallet->make_uri(address, "", amount, description, recipient, error);
  FUZZ_CHECK(!rebuilt.empty());

  std::string address2, payment_id2, description2, recipient2;
  uint64_t amount2 = 0;
  std::vector<std::string> unknown2;
  FUZZ_CHECK(wallet->parse_uri(rebuilt, address2, payment_id2, amount2, description2, recipient2, unknown2, error));
  FUZZ_CHECK(address2 == address);
  FUZZ_CHECK(payment_id2.empty());
  FUZZ_CHECK(amount2 == amount);
  FUZZ_CHECK(description2 == description);
  FUZZ_CHECK(recipient2 == recipient);
  FUZZ_CHECK(unknown2.empty());
}

//----------------------------------------------------------------------------------------------------------------------
// Transaction proofs
//----------------------------------------------------------------------------------------------------------------------
static bool try_check_tx_proof(const cryptonote::transaction &tx, const cryptonote::account_public_address &address,
    bool is_subaddress, const std::string &message, const std::string &sig, uint64_t &received)
{
  try
  {
    return wallet->check_tx_proof(tx, address, is_subaddress, message, sig, received);
  }
  catch (const std::exception &)
  {
    return false;
  }
}

static std::string mutate_proof(FuzzedDataProvider &fdp, std::string sig)
{
  switch (fdp.ConsumeIntegralInRange<int>(0, 4))
  {
    case 0:
    {
      const size_t pos = sig.find("V2");
      if (pos != std::string::npos)
        sig[pos + 1] = '1';
      return sig;
    }
    case 1:
    {
      static const char base58[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
      const size_t n = fdp.ConsumeIntegralInRange<size_t>(1, 4);
      for (size_t i = 0; i < n && !sig.empty(); ++i)
        sig[fdp.ConsumeIntegralInRange<size_t>(0, sig.size() - 1)] = base58[fdp.ConsumeIntegralInRange<size_t>(0, sizeof(base58) - 2)];
      return sig;
    }
    case 2:
      sig.resize(fdp.ConsumeIntegralInRange<size_t>(0, sig.size()));
      return sig;
    case 3:
      return sig + fdp.ConsumeRandomLengthString(96);
    default:
      return fdp.ConsumeRandomLengthString(512);
  }
}

static void fuzz_tx_proof(FuzzedDataProvider &fdp)
{
  // Outbound: our wallet proves a payment it sent to a foreign address. Inbound: it proves receipt.
  const bool outbound = fdp.ConsumeBool();
  const tools::wallet2 &owner = outbound ? *foreign : *wallet;
  const tools::wallet2 &third = outbound ? *wallet : *foreign;
  const bool proof_subaddress = fdp.ConsumeBool();
  const cryptonote::account_public_address proof_address = owner.get_subaddress({0, proof_subaddress ? 1u : 0u});

  cryptonote::transaction tx;
  const int kind = fdp.ConsumeIntegralInRange<int>(0, 3);
  tx.version = kind == 0 ? 1 : 2;
  const bool explicit_amounts = kind <= 1;
  tx.rct_signatures.type = kind == 2 ? rct::RCTTypeCLSAG : kind == 3 ? rct::RCTTypeBulletproofPlus : rct::RCTTypeNull;
  if (!explicit_amounts)
    tx.rct_signatures.txnFee = fdp.ConsumeIntegral<uint64_t>();

  const crypto::secret_key r = consume_scalar(fdp);
  const bool use_additional = fdp.ConsumeBool();
  std::vector<crypto::secret_key> additional_keys;
  std::vector<crypto::public_key> additional_pub_keys;
  uint64_t expected_received = 0;

  const size_t n_outs = fdp.ConsumeIntegralInRange<size_t>(1, kMaxOuts);
  for (size_t i = 0; i < n_outs; ++i)
  {
    const bool to_proof_address = i == 0 || fdp.ConsumeBool();
    const bool to_subaddress = to_proof_address ? proof_subaddress : fdp.ConsumeBool();
    const cryptonote::account_public_address to = to_proof_address ? proof_address : third.get_subaddress({0, to_subaddress ? 1u : 0u});

    crypto::secret_key k = r;
    if (use_additional)
    {
      k = consume_scalar(fdp);
      additional_keys.push_back(k);
      additional_pub_keys.push_back(tx_pub_key_for(k, to, to_subaddress));
    }

    crypto::key_derivation derivation;
    crypto::public_key out_key;
    FUZZ_CHECK(crypto::generate_key_derivation(to.m_view_public_key, k, derivation));
    FUZZ_CHECK(crypto::derive_public_key(derivation, i, to.m_spend_public_key, out_key));

    cryptonote::tx_out out;
    if (fdp.ConsumeBool())
    {
      crypto::view_tag view_tag;
      crypto::derive_view_tag(derivation, i, view_tag);
      out.target = cryptonote::txout_to_tagged_key(out_key, view_tag);
    }
    else
    {
      out.target = cryptonote::txout_to_key(out_key);
    }

    const uint64_t amount = fdp.ConsumeIntegral<uint64_t>();
    if (explicit_amounts)
    {
      out.amount = amount;
    }
    else
    {
      out.amount = 0;
      rct::key shared_secret;
      crypto::derivation_to_scalar(derivation, i, reinterpret_cast<crypto::ec_scalar&>(shared_secret));
      rct::ecdhTuple ecdh;
      ecdh.amount = rct::d2h(amount);
      ecdh.mask = rct::genCommitmentMask(shared_secret);
      rct::ctkey out_pk;
      out_pk.dest = rct::pk2rct(out_key);
      out_pk.mask = rct::commit(amount, ecdh.mask);
      rct::ecdhEncode(ecdh, shared_secret, true);
      tx.rct_signatures.ecdhInfo.push_back(ecdh);
      tx.rct_signatures.outPk.push_back(out_pk);
    }
    tx.vout.push_back(out);
    if (to_proof_address)
      expected_received += amount;
  }

  cryptonote::add_tx_pub_key_to_extra(tx.extra, tx_pub_key_for(r, proof_address, proof_subaddress));
  if (use_additional)
    cryptonote::add_additional_tx_pub_keys_to_extra(tx.extra, additional_pub_keys);

  // A wallet sees these as pruned txs from the daemon; fix the hash the same way.
  if (tx.version > 1)
  {
    crypto::hash prunable_hash = crypto::null_hash;
    const std::string bytes = fdp.ConsumeBytesAsString(sizeof(prunable_hash));
    memcpy(&prunable_hash, bytes.data(), bytes.size());
    crypto::hash txid;
    FUZZ_CHECK(cryptonote::get_pruned_transaction_hash(tx, prunable_hash, txid));
  }

  const std::string message = fdp.ConsumeRandomLengthString(64);
  std::string sig;
  try
  {
    sig = wallet->get_tx_proof(tx, r, additional_keys, proof_address, proof_subaddress, message);
  }
  catch (const std::exception &)
  {
    // e.g. "No funds received in this tx" when every amount to the proof address is zero.
    return;
  }

  uint64_t received = 0;
  FUZZ_CHECK(try_check_tx_proof(tx, proof_address, proof_subaddress, message, sig, received));
  FUZZ_CHECK(received == expected_received);

  uint64_t ignored = 0;
  FUZZ_CHECK(!try_check_tx_proof(tx, proof_address, proof_subaddress, message + "x", sig, ignored));
  FUZZ_CHECK(!try_check_tx_proof(tx, proof_address, !proof_subaddress, message, sig, ignored));

  try_check_tx_proof(tx, proof_address, proof_subaddress, message, mutate_proof(fdp, sig), ignored);
}

BEGIN_INIT_SIMPLE_FUZZER()
  static tools::wallet2 local_wallet(cryptonote::TESTNET, /*kdf_rounds=*/1, /*unattended=*/true);
  static tools::wallet2 local_foreign(cryptonote::TESTNET, /*kdf_rounds=*/1, /*unattended=*/true);
  wallet = &local_wallet;
  foreign = &local_foreign;

  static const char * const spendkey_hex = "f285d4ac9e66271256fc7cde0d3d6b36f66efff6ccd766706c408e86f4997a0d";
  static const char * const foreign_spendkey_hex = "0b4f47697ec99c3de6579304e5f25c68b07afbe55b71d99620bf6cbf4e45a80f";
  for (const auto &w : {std::make_pair(wallet, spendkey_hex), std::make_pair(foreign, foreign_spendkey_hex)})
  {
    crypto::secret_key spendkey;
    epee::string_tools::hex_to_pod(w.second, spendkey);
    w.first->init("", boost::none, "", 0, true, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
    w.first->set_subaddress_lookahead(2, 3);
    w.first->generate("", "", spendkey, true, false);
  }

  const cryptonote::account_public_address main_address = wallet->get_account().get_keys().m_account_address;
  crypto::hash8 payment_id8;
  memset(&payment_id8, 0x42, sizeof(payment_id8));
  uri_addresses = {
    wallet->get_address_as_str(),
    wallet->get_subaddress_as_str({0, 1}),
    wallet->get_integrated_address_as_str(payment_id8),
    cryptonote::get_account_address_as_str(cryptonote::MAINNET, false, main_address),
    "notanaddress",
  };
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);
  if (fdp.ConsumeBool())
    fuzz_uri(fdp);
  else
    fuzz_tx_proof(fdp);
END_SIMPLE_FUZZER()
