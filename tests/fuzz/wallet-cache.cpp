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

// Fuzzes wallet2::load_wallet_cache() (the wallet .cache file parser).
// The fuzz input is the *plaintext* cache archive; the harness encrypts it with
// the wallet's real cache key so the decrypt -> binary_archive path is exercised.

#include "include_base_utils.h"
#include "file_io_utils.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "wallet/wallet2.h"
#include "wallet/wallet2_basic/wallet2_serialization.h"
#include "serialization/binary_utils.h"
#include "fuzzer.h"

// wallet2 befriends this class (used by unit tests too) to reach private members.
class wallet_accessor_test
{
public:
  static crypto::chacha_key cache_key(tools::wallet2 &w) { return w.get_cache_key(); }
  static void clear(tools::wallet2 &w) { w.clear(); }
  static void load_wallet_cache(tools::wallet2 &w, const std::string &cache_buf) { w.load_wallet_cache(false, cache_buf); }
};

static tools::wallet2 *wallet = NULL;
static tools::wallet2 *scratch = NULL;
static crypto::chacha_key cache_key;

static bool parses_as_binary_archive(const uint8_t *buf, size_t len, bool varint_bug)
{
  binary_archive<false> ar{epee::span<const std::uint8_t>(buf, len)};
  if (varint_bug)
    ar.enable_varint_bug_backward_compatibility();
  return ::serialization::serialize(ar, *scratch) && ::serialization::check_stream_state(ar);
}

BEGIN_INIT_SIMPLE_FUZZER()
  static tools::wallet2 local_wallet(cryptonote::TESTNET, /*kdf_rounds=*/1, /*unattended=*/true);
  static tools::wallet2 local_scratch(cryptonote::TESTNET, /*kdf_rounds=*/1, /*unattended=*/true);
  wallet = &local_wallet;
  scratch = &local_scratch;

  static const char * const spendkey_hex = "f285d4ac9e66271256fc7cde0d3d6b36f66efff6ccd766706c408e86f4997a0d";
  crypto::secret_key spendkey;
  epee::string_tools::hex_to_pod(spendkey_hex, spendkey);

  wallet->init("", boost::none, "", 0, true, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
  wallet->set_subaddress_lookahead(1, 1);
  wallet->generate("", "", spendkey, true, false);

  // Populate a few containers so the seed reaches their element parsers.
  const cryptonote::account_public_address &addr = wallet->get_account().get_keys().m_account_address;
  wallet->add_subaddress_account("fuzz account");
  wallet->set_subaddress_label({1, 0}, "fuzz label");
  wallet->add_address_book_row(addr, nullptr, "fuzz address book entry", false);
  crypto::hash txid = crypto::null_hash;
  txid.data[0] = 1;
  wallet->set_tx_note(txid, "fuzz note");
  wallet->set_attribute("fuzz.attribute", "value");
  wallet->set_account_tag({0, 1}, "fuzz tag");

  cache_key = wallet_accessor_test::cache_key(*wallet);

  auto cache_file_data = wallet->get_cache_file_data();
  if (!cache_file_data)
  {
    std::cerr << "failed to build baseline cache" << std::endl;
    return 1;
  }
  std::string plaintext(cache_file_data->cache_data.size(), '\0');
  crypto::chacha20(cache_file_data->cache_data.data(), cache_file_data->cache_data.size(), cache_key, cache_file_data->iv, &plaintext[0]);
  if (!parses_as_binary_archive(reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size(), false))
  {
    std::cerr << "baseline cache does not round-trip, cache key mismatch?" << std::endl;
    return 1;
  }

  // Used by build.sh to generate the seed corpus with the current cache format.
  const char *seed_out = getenv("WALLET_CACHE_FUZZ_WRITE_SEED");
  if (seed_out && !epee::file_io_utils::save_string_to_file(seed_out, plaintext))
  {
    std::cerr << "failed to write seed to " << seed_out << std::endl;
    return 1;
  }
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  // Skip inputs that would make load_wallet_cache() fall back to the legacy boost
  // archives, which resize() to unchecked lengths and just produce OOM noise.
  if (!parses_as_binary_archive(buf, len, false) && !parses_as_binary_archive(buf, len, true))
    return 0;

  tools::wallet2::cache_file_data cache_file_data{};
  cache_file_data.iv = crypto::chacha_iv{};
  cache_file_data.cache_data.resize(len);
  crypto::chacha20(buf, len, cache_key, cache_file_data.iv, &cache_file_data.cache_data[0]);

  std::string cache_buf;
  if (!::serialization::dump_binary(cache_file_data, cache_buf))
    return 0;

  wallet_accessor_test::clear(*wallet);
  wallet_accessor_test::load_wallet_cache(*wallet, cache_buf);
END_SIMPLE_FUZZER()
