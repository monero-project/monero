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

#pragma once

#include "crypto/crypto.h"
#include "multisig_kex_msg.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace multisig
{
  /**
  * multisig account:
  * 
  * - handles account keys for an M-of-N multisig participant (M <= N; M >= 1; N >= 2)
  * - encapsulates multisig account construction process (via key exchange [kex])
  * - TODO: encapsulates key preparation for aggregation-style signing
  *
  * :: multisig pubkey: the private key is split, M group participants are required to reassemble (e.g. to sign something)
  *    - in cryptonote, this is the multisig spend key
  * :: multisig common pubkey: the private key is known to all participants (e.g. for authenticating as a group member)
  *    - in cryptonote, this is the multisig view key
  * 
  * 
  * multisig key exchange:
  * 
  * An 'M-of-N' (M <= N; M >= 1; N >= 2) multisignature key is a public key where at least 'M' out of 'N'
  * possible co-signers must collaborate in order to create a signature.
  * 
  * Constructing a multisig key involves a series of Diffie-Hellman exchanges between participants.
  * At the end of key exchange (kex), each participant will hold a number of private keys. Each private
  * key is shared by a group of (N - M + 1) participants. This way if (N - M) co-signers are missing, every
  * private key will be held by at least one of the remaining M people.
  * 
  * Note on MULTISIG_MAX_SIGNERS: During key exchange, participants will have up to '(N - 1) choose (N - M)'
  *   key shares. If N is large, then the max number of key shares (when M = (N-1)/2) can be huge. A limit of N <= 16 was
  *   arbitrarily chosen as a power of 2 that can accomodate the vast majority of practical use-cases. To increase the
  *   limit, FROST-style key aggregation should be used instead (it is more efficient than DH-based key generation
  *   when N - M > 1).
  * 
  * - Further reading
  *   - MRL-0009: https://www.getmonero.org/resources/research-lab/pubs/MRL-0009.pdf
  *   - MuSig2: https://eprint.iacr.org/2020/1261
  *   - ZtM2: https://web.getmonero.org/library/Zero-to-Monero-2-0-0.pdf Ch. 9, especially Section 9.6.3
  *   - FROST: https://eprint.iacr.org/2018/417
  */
  using multisig_keyset_map_memsafe_t = 
    std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>>;

  class multisig_account final
  {
  public:
  //constructors
    // default constructor
    multisig_account() = default;

    /**
    * construct from base privkeys
    * 
    * - prepares a kex msg for the first round of multisig key construction.
    *    - the local account's kex msgs are signed with the base_privkey
    *    - the first kex msg transmits the local base_common_privkey to other participants, for creating the group's common_privkey
    */
    multisig_account(const crypto::secret_key &base_privkey,
      const crypto::secret_key &base_common_privkey);

    // reconstruct from full account details (not recommended)
    multisig_account(const std::uint32_t threshold,
      std::vector<crypto::public_key> signers,
      const crypto::secret_key &base_privkey,
      const crypto::secret_key &base_common_privkey,
      std::vector<crypto::secret_key> multisig_privkeys,
      const crypto::secret_key &common_privkey,
      const crypto::public_key &multisig_pubkey,
      const crypto::public_key &common_pubkey,
      const std::uint32_t kex_rounds_complete,
      multisig_keyset_map_memsafe_t kex_origins_map,
      std::string next_round_kex_message);

    // copy constructor: default

  //destructor: default
    ~multisig_account() = default;

  //overloaded operators: none

  //getters
    // get threshold
    std::uint32_t get_threshold() const { return m_threshold; }
    // get signers
    const std::vector<crypto::public_key>& get_signers() const { return m_signers; }
    // get base privkey
    const crypto::secret_key& get_base_privkey() const { return m_base_privkey; }
    // get base pubkey
    const crypto::public_key& get_base_pubkey() const { return m_base_pubkey; }
    // get base common privkey
    const crypto::secret_key& get_base_common_privkey() const { return m_base_common_privkey; }
    // get multisig privkeys
    const std::vector<crypto::secret_key>& get_multisig_privkeys() const { return m_multisig_privkeys; }
    // get common privkey
    const crypto::secret_key& get_common_privkey() const { return m_common_privkey; }
    // get multisig pubkey
    const crypto::public_key& get_multisig_pubkey() const { return m_multisig_pubkey; }
    // get common pubkey
    const crypto::public_key& get_common_pubkey() const { return m_common_pubkey; }
    // get kex rounds complete
    std::uint32_t get_kex_rounds_complete() const { return m_kex_rounds_complete; }
    // get kex keys to origins map
    const multisig_keyset_map_memsafe_t& get_kex_keys_to_origins_map() const { return m_kex_keys_to_origins_map; }
    // get the kex msg for the next round
    const std::string& get_next_kex_round_msg() const { return m_next_round_kex_message; }

  //account status functions
    // account has been intialized, and the account holder can use the 'common' key
    bool account_is_active() const;
    // account has gone through main kex rounds, only remaining step is to verify all other participants are ready
    bool main_kex_rounds_done() const;
    // account is ready to make multisig signatures
    bool multisig_is_ready() const;

  //account helpers
  private:
    // set the threshold (M) and signers (N)
    void set_multisig_config(const std::size_t threshold, std::vector<crypto::public_key> signers);

  //account mutators: key exchange to set up account
  public:
    /**
    * brief: initialize_kex - initialize key exchange
    *    - Updates the account with a 'transactional' model. This account will only be mutated if the update succeeds.
    */
    void initialize_kex(const std::uint32_t threshold,
      std::vector<crypto::public_key> signers,
      const std::vector<multisig_kex_msg> &expanded_msgs_rnd1);
    /**
    * brief: kex_update - Complete the 'in progress' kex round and set the kex message for the next round.
    *    - Updates the account with a 'transactional' model. This account will only be mutated if the update succeeds.
    *    - The main interface for multisig key exchange, this handles all the work of processing input messages,
    *      creating new messages for new rounds, and finalizing the multisig shared public key when kex is complete.
    * param: expanded_msgs - kex messages corresponding to the account's 'in progress' round
    * param: force_update_use_with_caution - try to force the account to update with messages from an incomplete signer set.
    *    - If this is the post-kex verification round, only require one input message.
    *      - Force updating here should only be done if we can safely assume an honest signer subgroup of size 'threshold'
    *        will complete the account.
    *    - If this is an intermediate round, only require messages from 'num signers - 1 - (round - 1)' other signers.
    *      - If force updating with maliciously-crafted messages, the resulting account will be invalid (either unable
    *        to complete signatures, or a 'hostage' to the malicious signer [i.e. can't sign without his participation]).
    */
    void kex_update(const std::vector<multisig_kex_msg> &expanded_msgs,
      const bool force_update_use_with_caution = false);

  private:
    // implementation of kex_update() (non-transactional)
    void kex_update_impl(const std::vector<multisig_kex_msg> &expanded_msgs, const bool incomplete_signer_set);
    /**
    * brief: initialize_kex_update - Helper for kex_update_impl()
    *    - Collect the local signer's shared keys to ignore in incoming messages, build the aggregate ancillary key
    *      if appropriate.
    * param: expanded_msgs - set of multisig kex messages to process
    * param: kex_rounds_required - number of rounds required for kex (not including post-kex verification round)
    * outparam: exclude_pubkeys_out - keys held by the local account corresponding to round 'current_round'
    *    - If 'current_round' is the final round, these are the local account's shares of the final aggregate key.
    */
    void initialize_kex_update(const std::vector<multisig_kex_msg> &expanded_msgs,
      const std::uint32_t kex_rounds_required,
      std::vector<crypto::public_key> &exclude_pubkeys_out);
    /**
    * brief: finalize_kex_update - Helper for kex_update_impl()
    * param: kex_rounds_required - number of rounds required for kex (not including post-kex verification round)
    * param: result_keys_to_origins_map - map between keys for the next round and the other participants they correspond to
    * inoutparam: temp_account_inout - account to perform last update steps on
    */
    void finalize_kex_update(const std::uint32_t kex_rounds_required,
      multisig_keyset_map_memsafe_t result_keys_to_origins_map);

  //member variables
  private:
    /// misc. account details
    // [M] minimum number of co-signers to sign a message with the aggregate pubkey
    std::uint32_t m_threshold{0};
    // [N] base keys of all participants in the multisig (used to initiate key exchange, and as participant ids for msg signing)
    std::vector<crypto::public_key> m_signers;

    /// local participant's personal keys
    // base keypair of the participant
    // - used for signing messages, as the initial base key for key exchange, and to make DH derivations for key exchange
    crypto::secret_key m_base_privkey;
    crypto::public_key m_base_pubkey;
    // common base privkey, used to produce the aggregate common privkey
    crypto::secret_key m_base_common_privkey;

    /// core multisig account keys
    // the account's private key shares of the multisig address
    // TODO: also record which other signers have these privkeys, to enable aggregation signing (instead of round-robin)
    std::vector<crypto::secret_key> m_multisig_privkeys;
    // a privkey owned by all multisig participants (e.g. a cryptonote view key)
    crypto::secret_key m_common_privkey;
    // the multisig public key (e.g. a cryptonote spend key)
    crypto::public_key m_multisig_pubkey;
    // the common public key (e.g. a view spend key)
    crypto::public_key m_common_pubkey;

    /// kex variables
    // number of key exchange rounds that have been completed (all messages for the round collected and processed)
    std::uint32_t m_kex_rounds_complete{0};
    // this account's pubkeys for the in-progress key exchange round
    // - either DH derivations (intermediate rounds), H(derivation)*G (final round), empty (when kex is done)
    multisig_keyset_map_memsafe_t m_kex_keys_to_origins_map;
    // the account's message for the in-progress key exchange round
    std::string m_next_round_kex_message;
  };

  /**
  * brief: multisig_kex_rounds_required - The number of key exchange rounds required to produce an M-of-N shared key.
  *    - Key exchange (kex) is a synchronous series of 'rounds'. In an 'active round', participants send messages
  *      to each other.
  *    - A participant considers a round 'complete' when they have collected sufficient messages
  *      from other participants, processed those messages, and updated their multisig account state.
  *    - Typically (as implemented in this module), completing a round coincides with making a message for the next round.
  * param: num_signers - number of participants in multisig (N)
  * param: threshold - threshold of multisig (M)
  * return: number of kex rounds required
  */
  std::uint32_t multisig_kex_rounds_required(const std::uint32_t num_signers, const std::uint32_t threshold);

  /**
  * brief: multisig_setup_rounds_required - The number of setup rounds required to produce an M-of-N shared key.
  *    - A participant must complete all kex rounds and 1 initialization round.
  * param: num_signers - number of participants in multisig (N)
  * param: threshold - threshold of multisig (M)
  * return: number of setup rounds required
  */
  std::uint32_t multisig_setup_rounds_required(const std::uint32_t num_signers, const std::uint32_t threshold);
} //namespace multisig
