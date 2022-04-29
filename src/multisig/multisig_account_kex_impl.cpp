// Copyright (c) 2021-2022, The Monero Project
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

#include <boost/math/special_functions/binomial.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "multisig"

namespace multisig
{
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  * 
  * brief: calculate_multisig_keypair_from_derivation - wrapper on calculate_multisig_keypair() for an input public key
  *    Converts an input public key into a crypto private key (type cast, does not change serialization),
  *    then passes it to get_multisig_blinded_secret_key().
  * 
  *    Result:
  *      - privkey = H(derivation)
  *      - pubkey = privkey * G
  * param: derivation - a curve point
  * outparam: derived_pubkey_out - public key of the resulting privkey
  * return: multisig private key
  */
  //----------------------------------------------------------------------------------------------------------------------
  static crypto::secret_key calculate_multisig_keypair_from_derivation(const crypto::public_key_memsafe &derivation,
    crypto::public_key &derived_pubkey_out)
  {
    crypto::secret_key blinded_skey = get_multisig_blinded_secret_key(rct::rct2sk(rct::pk2rct(derivation)));
    CHECK_AND_ASSERT_THROW_MES(crypto::secret_key_to_public_key(blinded_skey, derived_pubkey_out), "Failed to derive public key");

    return blinded_skey;
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  *
  * brief: make_multisig_common_privkey - Create the 'common' multisig privkey, owned by all multisig participants.
  *    - common privkey = H(sorted base common privkeys)
  * param: participant_base_common_privkeys - Base common privkeys contributed by multisig participants.
  * outparam: common_privkey_out - result
  */
  //----------------------------------------------------------------------------------------------------------------------
  static void make_multisig_common_privkey(std::vector<crypto::secret_key> participant_base_common_privkeys,
    crypto::secret_key &common_privkey_out)
  {
    // sort the privkeys for consistency
    //TODO: need a constant-time operator< for sorting secret keys
    std::sort(participant_base_common_privkeys.begin(), participant_base_common_privkeys.end(),
        [](const crypto::secret_key &key1, const crypto::secret_key &key2) -> bool
        {
          return memcmp(&key1, &key2, sizeof(crypto::secret_key)) < 0;
        }
      );

    // privkey = H(sorted ancillary base privkeys)
    crypto::hash_to_scalar(participant_base_common_privkeys.data(),
      participant_base_common_privkeys.size()*sizeof(crypto::secret_key),
      common_privkey_out);

    CHECK_AND_ASSERT_THROW_MES(common_privkey_out != crypto::null_skey, "Unexpected null secret key (danger!).");
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  * 
  * brief: compute_multisig_aggregation_coefficient - creates aggregation coefficient for a specific public key in a set
  *    of public keys
  *    
  *    WARNING: The coefficient will only be deterministic if...
  *      1) input keys are pre-sorted
  *         - tested here
  *      2) input keys are in canonical form (compressed points in the prime-order subgroup of Ed25519)
  *         - untested here for performance
  * param: sorted_keys - set of component public keys that will be merged into a multisig public spend key
  * param: aggregation_key - one of the component public keys
  * return: aggregation coefficient
  */
  //----------------------------------------------------------------------------------------------------------------------
  static rct::key compute_multisig_aggregation_coefficient(const std::vector<crypto::public_key> &sorted_keys,
    const crypto::public_key &aggregation_key)
  {
    CHECK_AND_ASSERT_THROW_MES(std::is_sorted(sorted_keys.begin(), sorted_keys.end()),
      "Keys for aggregation coefficient aren't sorted.");

    // aggregation key must be in sorted_keys
    CHECK_AND_ASSERT_THROW_MES(std::find(sorted_keys.begin(), sorted_keys.end(), aggregation_key) != sorted_keys.end(),
      "Aggregation key expected to be in input keyset.");

    // aggregation coefficient salt
    rct::key salt = rct::zero();
    static_assert(sizeof(rct::key) >= sizeof(config::HASH_KEY_MULTISIG_KEY_AGGREGATION), "Hash domain separator is too big.");
    memcpy(salt.bytes, config::HASH_KEY_MULTISIG_KEY_AGGREGATION, sizeof(config::HASH_KEY_MULTISIG_KEY_AGGREGATION));

    // coeff = H(aggregation_key, sorted_keys, domain-sep)
    rct::keyV data;
    data.reserve(sorted_keys.size() + 2);
    data.push_back(rct::pk2rct(aggregation_key));
    for (const auto &key : sorted_keys)
      data.push_back(rct::pk2rct(key));
    data.push_back(salt);

    // note: coefficient is considered public knowledge, no need to memwipe data
    return rct::hash_to_scalar(data);
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  * 
  * brief: generate_multisig_aggregate_key - generates a multisig public spend key via key aggregation
  *    Key aggregation via aggregation coefficients prevents key cancellation attacks.
  *    See: https://www.getmonero.org/resources/research-lab/pubs/MRL-0009.pdf
  * param: final_keys - address components (public keys) obtained from other participants (not shared with local)
  * param: privkeys_inout - private keys of address components known by local; each key will be multiplied by an aggregation coefficient (return by reference)
  * return: final multisig public spend key for the account
  */
  //----------------------------------------------------------------------------------------------------------------------
  static crypto::public_key generate_multisig_aggregate_key(std::vector<crypto::public_key> final_keys,
    std::vector<crypto::secret_key> &privkeys_inout)
  {
    // collect all public keys that will go into the spend key (these don't need to be memsafe)
    final_keys.reserve(final_keys.size() + privkeys_inout.size());

    // 1. convert local multisig private keys to pub keys
    // 2. insert to final keyset if not there yet
    // 3. save the corresponding index of input priv key set for later reference
    std::unordered_map<crypto::public_key, std::size_t> own_keys_mapping;

    for (std::size_t multisig_keys_index{0}; multisig_keys_index < privkeys_inout.size(); ++multisig_keys_index)
    {
      crypto::public_key pubkey;
      CHECK_AND_ASSERT_THROW_MES(crypto::secret_key_to_public_key(privkeys_inout[multisig_keys_index], pubkey), "Failed to derive public key");

      own_keys_mapping[pubkey] = multisig_keys_index;

      final_keys.push_back(pubkey);
    }

    // sort input final keys for computing aggregation coefficients (lowest to highest)
    // note: input should be sanitized (no duplicates)
    std::sort(final_keys.begin(), final_keys.end());
    CHECK_AND_ASSERT_THROW_MES(std::adjacent_find(final_keys.begin(), final_keys.end()) == final_keys.end(),
        "Unexpected duplicate found in input list.");

    // key aggregation
    rct::key aggregate_key = rct::identity();

    for (const crypto::public_key &key : final_keys)
    {
      // get aggregation coefficient
      rct::key coeff = compute_multisig_aggregation_coefficient(final_keys, key);

      // convert private key if possible
      // note: retain original priv key index in input list, in case order matters upstream
      auto found_key = own_keys_mapping.find(key);
      if (found_key != own_keys_mapping.end())
      {
        // k_agg = coeff*k_base
        sc_mul((unsigned char*)&(privkeys_inout[found_key->second]),
          coeff.bytes,
          (const unsigned char*)&(privkeys_inout[found_key->second]));

        CHECK_AND_ASSERT_THROW_MES(privkeys_inout[found_key->second] != crypto::null_skey,
          "Multisig privkey with aggregation coefficient unexpectedly null.");
      }

      // convert public key (pre-merge operation)
      // K_agg = coeff*K_base
      rct::key converted_pubkey = rct::scalarmultKey(rct::pk2rct(key), coeff);

      // build aggregate key (merge operation)
      rct::addKeys(aggregate_key, aggregate_key, converted_pubkey);
    }

    return rct::rct2pk(aggregate_key);
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  *
  * brief: multisig_kex_make_next_msg - Construct a kex msg for any round > 1 of multisig key construction.
  *    - Involves DH exchanges with pubkeys provided by other participants.
  *    - Conserves mapping [pubkey -> DH derivation] : [origin keys of participants that share this secret with you].
  * param: base_privkey - account's base private key, for performing DH exchanges and signing messages
  * param: round - the round of the message that should be produced
  * param: threshold - threshold for multisig (M in M-of-N)
  * param: num_signers - number of participants in multisig (N)
  * param: pubkey_origins_map - map between pubkeys to produce DH derivations with and identity keys of
  *    participants who will share each derivation with you
  * outparam: derivation_origins_map_out - map between DH derivations (shared secrets) and identity keys
  *        - If msg is not for the last round, then these derivations are also stored in the output message
  *          so they can be sent to other participants, who will make more DH derivations for the next kex round.
  *        - If msg is for the last round, then these derivations won't be sent to other participants.
  *          Instead, they are converted to share secrets (i.e. s = H(derivation)) and multiplied by G.
  *          The keys s*G are sent to other participants in the message, so they can be used to produce the final
  *          multisig key via generate_multisig_spend_public_key().
  *            - The values s are the local account's shares of the final multisig key's private key. The caller can
  *              compute those values with calculate_multisig_keypair_from_derivation() (or compute them directly).
  * return: multisig kex message for the specified round
  */
  //----------------------------------------------------------------------------------------------------------------------
  static multisig_kex_msg multisig_kex_make_next_msg(const crypto::secret_key &base_privkey,
    const std::uint32_t round,
    const std::uint32_t threshold,
    const std::uint32_t num_signers,
    const std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>> &pubkey_origins_map,
    std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>> &derivation_origins_map_out)
  {
    CHECK_AND_ASSERT_THROW_MES(num_signers > 1, "Must be at least one other multisig signer.");
    CHECK_AND_ASSERT_THROW_MES(num_signers <= config::MULTISIG_MAX_SIGNERS,
      "Too many multisig signers specified (limit = 16 to prevent dangerous combinatorial explosion during key exchange).");
    CHECK_AND_ASSERT_THROW_MES(num_signers >= threshold,
      "Multisig threshold may not be larger than number of signers.");
    CHECK_AND_ASSERT_THROW_MES(threshold > 0, "Multisig threshold must be > 0.");
    CHECK_AND_ASSERT_THROW_MES(round > 1, "Round for next msg must be > 1.");
    CHECK_AND_ASSERT_THROW_MES(round <= multisig_kex_rounds_required(num_signers, threshold),
      "Trying to make key exchange message for an invalid round.");

    // make shared secrets with input pubkeys
    std::vector<crypto::public_key> msg_pubkeys;
    msg_pubkeys.reserve(pubkey_origins_map.size());
    derivation_origins_map_out.clear();

    for (const auto &pubkey_and_origins : pubkey_origins_map)
    {
      // D = 8 * k_base * K_pubkey
      // note: must be mul8 (cofactor), otherwise it is possible to leak to a malicious participant if the local
      //       base_privkey is a multiple of 8 or not
      // note2: avoid making temporaries that won't be memwiped
      rct::key derivation_rct;
      auto a_wiper = epee::misc_utils::create_scope_leave_handler([&]{
        memwipe(&derivation_rct, sizeof(rct::key));
      });

      rct::scalarmultKey(derivation_rct, rct::pk2rct(pubkey_and_origins.first), rct::sk2rct(base_privkey));
      rct::scalarmultKey(derivation_rct, derivation_rct, rct::EIGHT);

      crypto::public_key_memsafe derivation{rct::rct2pk(derivation_rct)};

      // retain mapping between pubkey's origins and the DH derivation
      // note: if msg for last round, then caller must know how to handle these derivations properly
      derivation_origins_map_out[derivation] = pubkey_and_origins.second;

      // if the last round, convert derivations to public keys for the output message
      if (round == multisig_kex_rounds_required(num_signers, threshold))
      {
        // derived_pubkey = H(derivation)*G
        crypto::public_key derived_pubkey;
        calculate_multisig_keypair_from_derivation(derivation, derived_pubkey);
        msg_pubkeys.push_back(derived_pubkey);
      }
      // otherwise, put derivations in message directly, so other signers can in turn create derivations (shared secrets)
      //  with them for the next round
      else
        msg_pubkeys.push_back(derivation);
    }

    return multisig_kex_msg{round, base_privkey, std::move(msg_pubkeys)};
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  *
  * brief: multisig_kex_msgs_sanitize_pubkeys - Sanitize multisig kex messages.
  *    - Removes duplicates from msg pubkeys, ignores pubkeys equal to the local account's signing key,
  *      ignores messages signed by the local account, ignores keys found in input 'exclusion set',
  *      constructs map of pubkey:origins.
  *    - Requires that all input msgs have the same round number.
  *
  *    origins = all the signing pubkeys that recommended a given pubkey found in input msgs
  *
  *    - If the messages' round numbers are all '1', then only the message signing pubkey is considered
  *      'recommended'. Furthermore, the 'exclusion set' is ignored.
  * param: own_pubkey - local account's signing key (key used to sign multisig messages)
  * param: expanded_msgs - set of multisig kex messages to process
  * param: exclude_pubkeys - pubkeys to exclude from output set
  * outparam: sanitized_pubkeys_out - processed pubkeys obtained from msgs, mapped to their origins
  * return: round number shared by all input msgs
  */
  //----------------------------------------------------------------------------------------------------------------------
  static std::uint32_t multisig_kex_msgs_sanitize_pubkeys(const crypto::public_key &own_pubkey,
    const std::vector<multisig_kex_msg> &expanded_msgs,
    const std::vector<crypto::public_key> &exclude_pubkeys,
    std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>> &sanitized_pubkeys_out)
  {
    CHECK_AND_ASSERT_THROW_MES(expanded_msgs.size() > 0, "At least one input message expected.");

    std::uint32_t round = expanded_msgs[0].get_round();
    sanitized_pubkeys_out.clear();

    // get all pubkeys from input messages, add them to pubkey:origins map
    // - origins = all the signing pubkeys that recommended a given msg pubkey
    for (const auto &expanded_msg : expanded_msgs)
    {
      CHECK_AND_ASSERT_THROW_MES(expanded_msg.get_round() == round, "All messages must have the same kex round number.");

      // ignore messages from self
      if (expanded_msg.get_signing_pubkey() == own_pubkey)
        continue;

      // in round 1, only the signing pubkey is treated as a msg pubkey
      if (round == 1)
      {
        // note: ignores duplicates
        sanitized_pubkeys_out[expanded_msg.get_signing_pubkey()].insert(expanded_msg.get_signing_pubkey());
      }
      // in other rounds, only the msg pubkeys are treated as msg pubkeys
      else
      {
        // copy all pubkeys from message into list
        for (const auto &pubkey : expanded_msg.get_msg_pubkeys())
        {
          // ignore own pubkey
          if (pubkey == own_pubkey)
            continue;

          // ignore pubkeys in 'ignore' set
          if (std::find(exclude_pubkeys.begin(), exclude_pubkeys.end(), pubkey) != exclude_pubkeys.end())
            continue;

          // note: ignores duplicates
          sanitized_pubkeys_out[pubkey].insert(expanded_msg.get_signing_pubkey());
        }
      }
    }

    return round;
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  *
  * brief: evaluate_multisig_kex_round_msgs - Evaluate pubkeys from a kex round in order to prepare for the next round.
  *    - Sanitizes input msgs.
  *    - Require uniqueness in: 'signers', 'exclude_pubkeys'.
  *    - Requires each input pubkey be recommended by 'num_recommendations = expected_round' msg signers.
  *      - For a final multisig key to be truly 'M-of-N', each of the the private key's components must be
  *        shared by (N - M + 1) signers.
  *    - Requires that msgs are signed by only keys in 'signers'.
  *    - Requires that each key in 'signers' recommends [num_signers - 2 CHOOSE (expected_round - 1)] pubkeys.
  *      - These should be derivations each signer recommends for round 'expected_round', excluding derivations shared
  *        with the local account.
  *    - Requires that 'exclude_pubkeys' has [num_signers - 1 CHOOSE (expected_round - 1)] pubkeys.
  *      - These should be derivations the local account has corresponding to round 'expected_round'.
  * param: base_privkey - multisig account's base private key
  * param: expected_round - expected kex round of input messages
  * param: threshold - threshold for multisig (M in M-of-N)
  * param: signers - expected participants in multisig kex
  * param: expanded_msgs - set of multisig kex messages to process
  * param: exclude_pubkeys - derivations held by the local account corresponding to round 'expected_round'
  * return: fully sanitized and validated pubkey:origins map for building the account's next kex round message
  */
  //----------------------------------------------------------------------------------------------------------------------
  static std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>> evaluate_multisig_kex_round_msgs(
    const crypto::public_key &base_pubkey,
    const std::uint32_t expected_round,
    const std::uint32_t threshold,
    const std::vector<crypto::public_key> &signers,
    const std::vector<multisig_kex_msg> &expanded_msgs,
    const std::vector<crypto::public_key> &exclude_pubkeys)
  {
    CHECK_AND_ASSERT_THROW_MES(signers.size() > 1, "Must be at least one other multisig signer.");
    CHECK_AND_ASSERT_THROW_MES(signers.size() <= config::MULTISIG_MAX_SIGNERS,
      "Too many multisig signers specified (limit = 16 to prevent dangerous combinatorial explosion during key exchange).");
    CHECK_AND_ASSERT_THROW_MES(signers.size() >= threshold, "Multisig threshold may not be larger than number of signers.");
    CHECK_AND_ASSERT_THROW_MES(threshold > 0, "Multisig threshold must be > 0.");
    CHECK_AND_ASSERT_THROW_MES(expected_round > 0, "Expected round must be > 0.");
    CHECK_AND_ASSERT_THROW_MES(expected_round <= multisig_kex_rounds_required(signers.size(), threshold),
      "Expecting key exchange messages for an invalid round.");

    std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>> pubkey_origins_map;

    // leave early in the last round of 1-of-N, where all signers share a key so the local signer doesn't care about
    // recommendations from other signers
    if (threshold == 1 && expected_round == multisig_kex_rounds_required(signers.size(), threshold))
      return pubkey_origins_map;

    // exclude_pubkeys should all be unique
    for (auto it = exclude_pubkeys.begin(); it != exclude_pubkeys.end(); ++it)
    {
      CHECK_AND_ASSERT_THROW_MES(std::find(exclude_pubkeys.begin(), it, *it) == it,
        "Found duplicate pubkeys for exclusion unexpectedly.");
    }

    // sanitize input messages
    std::uint32_t round = multisig_kex_msgs_sanitize_pubkeys(base_pubkey, expanded_msgs, exclude_pubkeys, pubkey_origins_map);
    CHECK_AND_ASSERT_THROW_MES(round == expected_round,
      "Kex messages were for round [" << round << "], but expected round is [" << expected_round << "]");

    // evaluate pubkeys collected
    std::unordered_map<crypto::public_key, std::unordered_set<crypto::public_key>> origin_pubkeys_map;

    // 1. each pubkey should be recommended by a precise number of signers
    for (const auto &pubkey_and_origins : pubkey_origins_map)
    {
      // expected amount = round_num
      // With each successive round, pubkeys are shared by incrementally larger groups,
      //  starting at 1 in round 1 (i.e. the local multisig key to start kex with).
      CHECK_AND_ASSERT_THROW_MES(pubkey_and_origins.second.size() == round,
        "A pubkey recommended by multisig kex messages had an unexpected number of recommendations.");

      // map (sanitized) pubkeys back to origins
      for (const auto &origin : pubkey_and_origins.second)
        origin_pubkeys_map[origin].insert(pubkey_and_origins.first);
    }

    // 2. the number of unique signers recommending pubkeys should equal the number of signers passed in (minus the local signer)
    CHECK_AND_ASSERT_THROW_MES(origin_pubkeys_map.size() == signers.size() - 1,
      "Number of unique other signers does not equal number of other signers that recommended pubkeys.");

    // 3. each origin should recommend a precise number of pubkeys

    // TODO: move to a 'math' library, with unit tests
    auto n_choose_k_f =
      [](const std::uint32_t n, const std::uint32_t k) -> std::uint32_t
      {
        static_assert(std::numeric_limits<std::int32_t>::digits <= std::numeric_limits<double>::digits,
          "n_choose_k requires no rounding issues when converting between int32 <-> double.");

        if (n < k)
          return 0;

        double fp_result = boost::math::binomial_coefficient<double>(n, k);

        if (fp_result < 0)
          return 0;

        if (fp_result > std::numeric_limits<std::int32_t>::max())  // note: std::round() returns std::int32_t
          return 0;

        return static_cast<std::uint32_t>(std::round(fp_result));
      };

    // other signers: (N - 2) choose (msg_round_num - 1)
      // - Each signer recommends keys they share with other signers.
      // - In each round, a signer shares a key with 'round num - 1' other signers.
      // - Since 'origins pubkey map' excludes keys shared with the local account,
      //   only keys shared with participants 'other than local and self' will be in the map (e.g. N - 2 signers).
      // - So other signers will recommend (N - 2) choose (msg_round_num - 1) pubkeys (after removing keys shared with local).
      // - Each origin should have a shared key with each group of size 'round - 1'.
      // Note: Keys shared with local are ignored to facilitate kex round boosting, where one or more signers may
      //       have boosted the local signer (implying they didn't have access to the local signer's previous round msg).
    std::uint32_t expected_recommendations_others = n_choose_k_f(signers.size() - 2, round - 1);

    // local: (N - 1) choose (msg_round_num - 1)
    std::uint32_t expected_recommendations_self = n_choose_k_f(signers.size() - 1, round - 1);

    // note: expected_recommendations_others would be 0 in the last round of 1-of-N, but we return early for that case
    CHECK_AND_ASSERT_THROW_MES(expected_recommendations_self > 0 && expected_recommendations_others > 0,
      "Bad num signers or round num (possibly numerical limits exceeded).");

    // check that local account recommends expected number of keys
    CHECK_AND_ASSERT_THROW_MES(exclude_pubkeys.size() == expected_recommendations_self,
      "Local account did not recommend expected number of multisig keys.");

    // check that other signers recommend expected number of keys
    for (const auto &origin_and_pubkeys : origin_pubkeys_map)
    {
      CHECK_AND_ASSERT_THROW_MES(origin_and_pubkeys.second.size() == expected_recommendations_others,
        "A pubkey recommended by multisig kex messages had an unexpected number of recommendations.");

      // 2 (continued). only expected signers should be recommending keys
      CHECK_AND_ASSERT_THROW_MES(std::find(signers.begin(), signers.end(), origin_and_pubkeys.first) != signers.end(),
        "Multisig kex message with unexpected signer encountered.");
    }

    // note: above tests implicitly detect if the total number of recommended keys is correct or not
    return pubkey_origins_map;
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  *
  * brief: multisig_kex_process_round - Process kex messages for the active kex round.
  *    - A wrapper around evaluate_multisig_kex_round_msgs() -> multisig_kex_make_next_msg().
  *      - In other words, evaluate the input messages and try to make a message for the next round.
  *    - Note: Must be called on the final round's msgs to evaluate the final key components
  *            recommended by other participants.
  * param: base_privkey - multisig account's base private key
  * param: current_round - round of kex the input messages should be designed for
  * param: threshold - threshold for multisig (M in M-of-N)
  * param: signers - expected participants in multisig kex
  * param: expanded_msgs - set of multisig kex messages to process
  * param: exclude_pubkeys - keys held by the local account corresponding to round 'current_round'
  *    - If 'current_round' is the final round, these are the local account's shares of the final aggregate key.
  * outparam: keys_to_origins_map_out - map between round keys and identity keys
  *    - If in the final round, these are key shares recommended by other signers for the final aggregate key.
  *    - Otherwise, these are the local account's DH derivations for the next round.
  *      - See multisig_kex_make_next_msg() for an explanation.
  * return: multisig kex message for next round, or empty message if 'current_round' is the final round
  */
  //----------------------------------------------------------------------------------------------------------------------
  static multisig_kex_msg multisig_kex_process_round(const crypto::secret_key &base_privkey,
    const crypto::public_key &base_pubkey,
    const std::uint32_t current_round,
    const std::uint32_t threshold,
    const std::vector<crypto::public_key> &signers,
    const std::vector<multisig_kex_msg> &expanded_msgs,
    const std::vector<crypto::public_key> &exclude_pubkeys,
    std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>> &keys_to_origins_map_out)
  {
    // evaluate messages
    std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>> evaluated_pubkeys =
      evaluate_multisig_kex_round_msgs(base_pubkey, current_round, threshold, signers, expanded_msgs, exclude_pubkeys);

    // produce message for next round (if there is one)
    if (current_round < multisig_kex_rounds_required(signers.size(), threshold))
    {
      return multisig_kex_make_next_msg(base_privkey,
        current_round + 1,
        threshold,
        signers.size(),
        evaluated_pubkeys,
        keys_to_origins_map_out);
    }
    else
    {
      // no more rounds, so collect the key shares recommended by other signers for the final aggregate key
      keys_to_origins_map_out.clear();
      keys_to_origins_map_out = std::move(evaluated_pubkeys);

      return multisig_kex_msg{};
    }
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: INTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_account::initialize_kex_update(const std::vector<multisig_kex_msg> &expanded_msgs,
    const std::uint32_t rounds_required,
    std::vector<crypto::public_key> &exclude_pubkeys_out)
  {
    if (m_kex_rounds_complete == 0)
    {
      // the first round of kex msgs will contain each participant's base pubkeys and ancillary privkeys

      // collect participants' base common privkey shares
      // note: duplicate privkeys are acceptable, and duplicates due to duplicate signers
      //       will be blocked by duplicate-signer errors after this function is called
      std::vector<crypto::secret_key> participant_base_common_privkeys;
      participant_base_common_privkeys.reserve(expanded_msgs.size() + 1);

      // add local ancillary base privkey
      participant_base_common_privkeys.emplace_back(m_base_common_privkey);

      // add other signers' base common privkeys
      for (const auto &expanded_msg : expanded_msgs)
      {
        if (expanded_msg.get_signing_pubkey() != m_base_pubkey)
        {
          participant_base_common_privkeys.emplace_back(expanded_msg.get_msg_privkey());
        }
      }

      // make common privkey
      make_multisig_common_privkey(std::move(participant_base_common_privkeys), m_common_privkey);

      // set common pubkey
      CHECK_AND_ASSERT_THROW_MES(crypto::secret_key_to_public_key(m_common_privkey, m_common_pubkey),
        "Failed to derive public key");

      // if N-of-N, then the base privkey will be used directly to make the account's share of the final key
      if (rounds_required == 1)
      {
        m_multisig_privkeys.clear();
        m_multisig_privkeys.emplace_back(m_base_privkey);
      }

      // exclude all keys the local account recommends
      // - in the first round, only the local pubkey is recommended by the local signer
      exclude_pubkeys_out.emplace_back(m_base_pubkey);
    }
    else
    {
      // in other rounds, kex msgs will contain participants' shared keys

      // ignore shared keys the account helped create for this round
      for (const auto &shared_key_with_origins : m_kex_keys_to_origins_map)
      {
        exclude_pubkeys_out.emplace_back(shared_key_with_origins.first);
      }
    }
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: INTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_account::finalize_kex_update(const std::uint32_t rounds_required,
    std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>> result_keys_to_origins_map)
  {
    // prepare for next round (or complete the multisig account fully)
    if (rounds_required == m_kex_rounds_complete + 1)
    {
      // finished (have set of msgs to complete address)

      // when 'completing the final round', result keys are other signers' shares of the final key
      std::vector<crypto::public_key> result_keys;
      result_keys.reserve(result_keys_to_origins_map.size());

      for (const auto &result_key_and_origins : result_keys_to_origins_map)
      {
        result_keys.emplace_back(result_key_and_origins.first);
      }

      // compute final aggregate key, update local multisig privkeys with aggregation coefficients applied
      m_multisig_pubkey = generate_multisig_aggregate_key(std::move(result_keys), m_multisig_privkeys);

      // no longer need the account's pubkeys saved for this round (they were only used to build exclude_pubkeys)
      // TODO: record [pre-aggregation pubkeys : origins] map for aggregation-style signing
      m_kex_keys_to_origins_map.clear();
    }
    else if (rounds_required == m_kex_rounds_complete + 2)
    {
      // one more round (must send/receive one more set of kex msgs)
      // - at this point, have local signer's pre-aggregation private key shares of the final address

      // result keys are the local signer's DH derivations for the next round

      // derivations are shared secrets between each group of N - M + 1 signers of which the local account is a member
      // - convert them to private keys: multisig_key = H(derivation)
      // - note: shared key = multisig_key[i]*G is recorded in the kex msg for sending to other participants
      //   instead of the original 'derivation' value (which MUST be kept secret!)
      m_multisig_privkeys.clear();
      m_multisig_privkeys.reserve(result_keys_to_origins_map.size());

      m_kex_keys_to_origins_map.clear();

      for (const auto &derivation_and_origins : result_keys_to_origins_map)
      {
        // multisig_privkey = H(derivation)
        // derived pubkey = multisig_key * G
        crypto::public_key_memsafe derived_pubkey;
        m_multisig_privkeys.push_back(
          calculate_multisig_keypair_from_derivation(derivation_and_origins.first, derived_pubkey));

        // save the account's kex key mappings for this round [derived pubkey : other signers who will have the same key]
        m_kex_keys_to_origins_map[derived_pubkey] = std::move(derivation_and_origins.second);
      }
    }
    else
    {
      // next round is an 'intermediate' key exchange round, so there is nothing special to do here

      // save the account's kex keys for this round [DH derivation : other signers who will have the same derivation]
      m_kex_keys_to_origins_map = std::move(result_keys_to_origins_map);
    }

    // a full set of msgs has been collected and processed, so the 'round is complete'
    ++m_kex_rounds_complete;
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: INTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_account::kex_update_impl(const std::vector<multisig_kex_msg> &expanded_msgs)
  {
    CHECK_AND_ASSERT_THROW_MES(expanded_msgs.size() > 0, "No key exchange messages passed in.");

    const std::uint32_t rounds_required = multisig_kex_rounds_required(m_signers.size(), m_threshold);
    CHECK_AND_ASSERT_THROW_MES(rounds_required > 0, "Multisig kex rounds required unexpectedly 0.");

    // initialize account update
    std::vector<crypto::public_key> exclude_pubkeys;
    initialize_kex_update(expanded_msgs, rounds_required, exclude_pubkeys);

    // evaluate messages and get this account's kex msg for the next round
    std::unordered_map<crypto::public_key_memsafe, std::unordered_set<crypto::public_key>> result_keys_to_origins_map;

    m_next_round_kex_message = multisig_kex_process_round(
      m_base_privkey,
      m_base_pubkey,
      m_kex_rounds_complete + 1,
      m_threshold,
      m_signers,
      expanded_msgs,
      exclude_pubkeys,
      result_keys_to_origins_map).get_msg();

    // finish account update
    finalize_kex_update(rounds_required, std::move(result_keys_to_origins_map));
  }
  //----------------------------------------------------------------------------------------------------------------------
} //namespace multisig
