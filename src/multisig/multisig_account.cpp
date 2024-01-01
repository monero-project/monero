// Copyright (c) 2021-2024, The Monero Project
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

#include "multisig_account.h"

#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "include_base_utils.h"
#include "multisig.h"
#include "multisig_kex_msg.h"
#include "ringct/rctOps.h"
#include "ringct/rctTypes.h"

#include <cstdint>
#include <utility>
#include <vector>


#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "multisig"

namespace multisig
{
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  multisig_account::multisig_account(const crypto::secret_key &base_privkey,
    const crypto::secret_key &base_common_privkey) :
      m_base_privkey{base_privkey},
      m_base_common_privkey{base_common_privkey},
      m_multisig_pubkey{rct::rct2pk(rct::identity())},
      m_common_pubkey{rct::rct2pk(rct::identity())},
      m_kex_rounds_complete{0},
      m_next_round_kex_message{multisig_kex_msg{1, base_privkey, std::vector<crypto::public_key>{}, base_common_privkey}.get_msg()}
  {
    CHECK_AND_ASSERT_THROW_MES(crypto::secret_key_to_public_key(m_base_privkey, m_base_pubkey),
      "Failed to derive public key");
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  multisig_account::multisig_account(const std::uint32_t threshold,
    std::vector<crypto::public_key> signers,
    const crypto::secret_key &base_privkey,
    const crypto::secret_key &base_common_privkey,
    std::vector<crypto::secret_key> multisig_privkeys,
    const crypto::secret_key &common_privkey,
    const crypto::public_key &multisig_pubkey,
    const crypto::public_key &common_pubkey,
    const std::uint32_t kex_rounds_complete,
    multisig_keyset_map_memsafe_t kex_origins_map,
    std::string next_round_kex_message) :
      m_base_privkey{base_privkey},
      m_base_common_privkey{base_common_privkey},
      m_multisig_privkeys{std::move(multisig_privkeys)},
      m_common_privkey{common_privkey},
      m_multisig_pubkey{multisig_pubkey},
      m_common_pubkey{common_pubkey},
      m_kex_rounds_complete{kex_rounds_complete},
      m_kex_keys_to_origins_map{std::move(kex_origins_map)},
      m_next_round_kex_message{std::move(next_round_kex_message)}
  {
    CHECK_AND_ASSERT_THROW_MES(kex_rounds_complete > 0, "multisig account: can't reconstruct account if its kex wasn't initialized");
    CHECK_AND_ASSERT_THROW_MES(crypto::secret_key_to_public_key(m_base_privkey, m_base_pubkey),
      "Failed to derive public key");
    set_multisig_config(threshold, std::move(signers));

    // kex rounds should not exceed post-kex verification round
    const std::uint32_t kex_rounds_required{multisig_kex_rounds_required(m_signers.size(), m_threshold)};
    CHECK_AND_ASSERT_THROW_MES(m_kex_rounds_complete <= kex_rounds_required + 1,
      "multisig account: tried to reconstruct account, but kex rounds complete counter is invalid.");

    // once an account is done with kex, the 'next kex msg' is always the post-kex verification message
    //   i.e. the multisig account pubkey signed by the signer's privkey AND the common pubkey
    if (main_kex_rounds_done())
    {
      m_next_round_kex_message = multisig_kex_msg{kex_rounds_required + 1,
        m_base_privkey,
        std::vector<crypto::public_key>{m_multisig_pubkey, m_common_pubkey}}.get_msg();
    }
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  bool multisig_account::account_is_active() const
  {
    return m_kex_rounds_complete > 0;
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  bool multisig_account::main_kex_rounds_done() const
  {
    if (account_is_active())
      return m_kex_rounds_complete >= multisig_kex_rounds_required(m_signers.size(), m_threshold);
    else
      return false;
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  bool multisig_account::multisig_is_ready() const
  {
    if (main_kex_rounds_done())
      return m_kex_rounds_complete >= multisig_setup_rounds_required(m_signers.size(), m_threshold);
    else
      return false;
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: INTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_account::set_multisig_config(const std::size_t threshold, std::vector<crypto::public_key> signers)
  {
    // validate
    CHECK_AND_ASSERT_THROW_MES(threshold > 0 && threshold <= signers.size(), "multisig account: tried to set invalid threshold.");
    CHECK_AND_ASSERT_THROW_MES(signers.size() >= 2 && signers.size() <= config::MULTISIG_MAX_SIGNERS,
      "multisig account: tried to set invalid number of signers.");

    for (auto signer_it = signers.begin(); signer_it != signers.end(); ++signer_it)
    {
      // signer pubkeys must be in main subgroup, and not identity
      CHECK_AND_ASSERT_THROW_MES(rct::isInMainSubgroup(rct::pk2rct(*signer_it)) && !(*signer_it == rct::rct2pk(rct::identity())),
        "multisig account: tried to set signers, but a signer pubkey is invalid.");
    }

    // own pubkey should be in signers list
    CHECK_AND_ASSERT_THROW_MES(std::find(signers.begin(), signers.end(), m_base_pubkey) != signers.end(),
      "multisig account: tried to set signers, but did not find the account's base pubkey in signer list.");

    // sort signers
    std::sort(signers.begin(), signers.end());

    // signers should all be unique
    CHECK_AND_ASSERT_THROW_MES(std::adjacent_find(signers.begin(), signers.end()) == signers.end(),
      "multisig account: tried to set signers, but there are duplicate signers unexpectedly.");

    // set
    m_threshold = threshold;
    m_signers = std::move(signers);
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_account::initialize_kex(const std::uint32_t threshold,
    std::vector<crypto::public_key> signers,
    const std::vector<multisig_kex_msg> &expanded_msgs_rnd1)
  {
    CHECK_AND_ASSERT_THROW_MES(!account_is_active(), "multisig account: tried to initialize kex, but already initialized");

    // only mutate account if update succeeds
    multisig_account temp_account{*this};
    temp_account.set_multisig_config(threshold, std::move(signers));
    temp_account.kex_update_impl(expanded_msgs_rnd1, false);
    *this = std::move(temp_account);
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_account::kex_update(const std::vector<multisig_kex_msg> &expanded_msgs,
    const bool force_update_use_with_caution /*= false*/)
  {
    CHECK_AND_ASSERT_THROW_MES(account_is_active(), "multisig account: tried to update kex, but kex isn't initialized yet.");
    CHECK_AND_ASSERT_THROW_MES(!multisig_is_ready(), "multisig account: tried to update kex, but kex is already complete.");

    multisig_account temp_account{*this};
    temp_account.kex_update_impl(expanded_msgs, force_update_use_with_caution);
    *this = std::move(temp_account);
  }
  //----------------------------------------------------------------------------------------------------------------------
  // EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  std::uint32_t multisig_kex_rounds_required(const std::uint32_t num_signers, const std::uint32_t threshold)
  {
    CHECK_AND_ASSERT_THROW_MES(num_signers >= threshold, "num_signers must be >= threshold");
    CHECK_AND_ASSERT_THROW_MES(threshold >= 1, "threshold must be >= 1");
    return num_signers - threshold + 1;
  }
  //----------------------------------------------------------------------------------------------------------------------
  // EXTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  std::uint32_t multisig_setup_rounds_required(const std::uint32_t num_signers, const std::uint32_t threshold)
  {
    return multisig_kex_rounds_required(num_signers, threshold) + 1;
  }
  //----------------------------------------------------------------------------------------------------------------------
} //namespace multisig
