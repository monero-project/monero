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
  * brief: check_multisig_config - validate multisig configuration details
  * param: round - the round of the message that should be produced
  * param: threshold - threshold for multisig (M in M-of-N)
  * param: num_signers - number of participants in multisig (N)
  */
  //----------------------------------------------------------------------------------------------------------------------
  static void check_multisig_config(const std::uint32_t round,
    const std::uint32_t threshold,
    const std::uint32_t num_signers)
  {
    CHECK_AND_ASSERT_THROW_MES(num_signers > 1, "Must be at least one other multisig signer.");
    CHECK_AND_ASSERT_THROW_MES(num_signers <= config::MULTISIG_MAX_SIGNERS,
      "Too many multisig signers specified (limit = 16 to prevent dangerous combinatorial explosion during key exchange).");
    CHECK_AND_ASSERT_THROW_MES(num_signers >= threshold,
      "Multisig threshold may not be larger than number of signers.");
    CHECK_AND_ASSERT_THROW_MES(threshold > 0, "Multisig threshold must be > 0.");
    CHECK_AND_ASSERT_THROW_MES(round > 0, "Multisig kex round must be > 0.");
    CHECK_AND_ASSERT_THROW_MES(round <= multisig_setup_rounds_required(num_signers, threshold),
      "Trying to process multisig kex for an invalid round.");
  }
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
  * param: privkeys_inout - private keys of address components known by local; each key will be multiplied by an aggregation
  *                         coefficient (return by reference)
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
      CHECK_AND_ASSERT_THROW_MES(crypto::secret_key_to_public_key(privkeys_inout[multisig_keys_index], pubkey),
        "Failed to derive public key");

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
  * brief: multisig_kex_make_round_keys - Makes a kex round's keys.
  *    - Involves DH exchanges with pubkeys provided by other participants.
  *    - Conserves mapping [pubkey -> DH derivation] : [origin keys of participants that share this secret with you].
  * param: base_privkey - account's base private key, for performing DH exchanges and signing messages
  * param: pubkey_origins_map - map between pubkeys to produce DH derivations with and identity keys of
  *    participants who will share each derivation with you
  * outparam: derivation_origins_map_out - map between DH derivations (shared secrets) and identity keys
  */
  //----------------------------------------------------------------------------------------------------------------------
  static void multisig_kex_make_round_keys(const crypto::secret_key &base_privkey,
    multisig_keyset_map_memsafe_t pubkey_origins_map,
    multisig_keyset_map_memsafe_t &derivation_origins_map_out)
  {
    // make shared secrets with input pubkeys
    derivation_origins_map_out.clear();

    for (auto &pubkey_and_origins : pubkey_origins_map)
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

      // retain mapping between pubkey's origins and the DH derivation
      // note: if working on last kex round, then caller must know how to handle these derivations properly
      derivation_origins_map_out[rct::rct2pk(derivation_rct)] = std::move(pubkey_and_origins.second);
    }
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  *
  * brief: check_messages_round - Check that a set of messages have an expected round number.
  * param: expanded_msgs - set of multisig kex messages to process
  * param: expected_round - round number the kex messages should have
  */
  //----------------------------------------------------------------------------------------------------------------------
  static void check_messages_round(const std::vector<multisig_kex_msg> &expanded_msgs,
    const std::uint32_t expected_round)
  {
    CHECK_AND_ASSERT_THROW_MES(expanded_msgs.size() > 0, "At least one input message expected.");
    const std::uint32_t round{expanded_msgs[0].get_round()};
    CHECK_AND_ASSERT_THROW_MES(round == expected_round, "Messages don't have the expected kex round number.");

    for (const auto &expanded_msg : expanded_msgs)
      CHECK_AND_ASSERT_THROW_MES(expanded_msg.get_round() == round, "All messages must have the same kex round number.");
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  *
  * brief: multisig_kex_msgs_sanitize_pubkeys - Sanitize multisig kex messages.
  *    - Removes duplicates from msg pubkeys, ignores keys found in input 'exclusion set',
  *      constructs map of pubkey:origins.
  *    - Requires that all input msgs have the same round number.
  *
  *    origins = all the signing pubkeys that recommended a given pubkey found in input msgs
  *
  *    - If the messages' round numbers are all '1', then only the message signing pubkey is considered
  *      'recommended'. Furthermore, the 'exclusion set' is ignored.
  * param: expanded_msgs - set of multisig kex messages to process
  * param: exclude_pubkeys - pubkeys to exclude from output set
  * outparam: sanitized_pubkeys_out - processed pubkeys obtained from msgs, mapped to their origins
  * return: round number shared by all input msgs
  */
  //----------------------------------------------------------------------------------------------------------------------
  static std::uint32_t multisig_kex_msgs_sanitize_pubkeys(const std::vector<multisig_kex_msg> &expanded_msgs,
    const std::vector<crypto::public_key> &exclude_pubkeys,
    multisig_keyset_map_memsafe_t &sanitized_pubkeys_out)
  {
    // all messages should have the same round (redundant sanity check)
    CHECK_AND_ASSERT_THROW_MES(expanded_msgs.size() > 0, "At least one input message expected.");
    const std::uint32_t round{expanded_msgs[0].get_round()};
    check_messages_round(expanded_msgs, round);

    sanitized_pubkeys_out.clear();

    // get all pubkeys from input messages, add them to pubkey:origins map
    // - origins = all the signing pubkeys that recommended a given msg pubkey
    for (const auto &expanded_msg : expanded_msgs)
    {
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
  * brief: remove_key_from_mapped_sets - Remove a specified key from the mapped sets in a multisig keyset map.
  * param: key_to_remove - specified key to remove
  * inoutparam: keyset_inout - keyset to update
  */
  //----------------------------------------------------------------------------------------------------------------------
  static void remove_key_from_mapped_sets(const crypto::public_key &key_to_remove,
    multisig_keyset_map_memsafe_t &keyset_inout)
  {
    // remove specified key from each mapped set
    for (auto keyset_it = keyset_inout.begin(); keyset_it != keyset_inout.end();)
    {
      // remove specified key from this set
      keyset_it->second.erase(key_to_remove);

      // remove empty keyset positions or increment iterator
      if (keyset_it->second.size() == 0)
        keyset_it = keyset_inout.erase(keyset_it);
      else
        ++keyset_it;
    }
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  *
  * brief: evaluate_multisig_kex_round_msgs - Evaluate pubkeys from a kex round in order to prepare for the next round.
  *    - Sanitizes input msgs.
  *    - Require uniqueness in: 'exclude_pubkeys'.
  *    - Requires each input pubkey be recommended by 'num_recommendations = expected_round' msg signers.
  *      - For a final multisig key to be truly 'M-of-N', each of the the private key's components must be
  *        shared by (N - M + 1) signers.
  *    - Requires that msgs are signed by only keys in 'signers'.
  *    - Requires that each key in 'signers' recommends [num_signers - 2 CHOOSE (expected_round - 1)] pubkeys.
  *      - These should be derivations each signer recommends for round 'expected_round', excluding derivations shared
  *        with the local account.
  *    - Requires that 'exclude_pubkeys' has [num_signers - 1 CHOOSE (expected_round - 1)] pubkeys.
  *      - These should be derivations the local account has corresponding to round 'expected_round'.
  * param: base_pubkey - multisig account's base public key
  * param: expected_round - expected kex round of input messages
  * param: signers - expected participants in multisig kex
  * param: expanded_msgs - set of multisig kex messages to process
  * param: exclude_pubkeys - derivations held by the local account corresponding to round 'expected_round'
  * param: incomplete_signer_set - only require the minimum number of signers to complete this round
  *                                minimum = num_signers - (round num - 1)   (including local signer)
  * return: fully sanitized and validated pubkey:origins map for building the account's next kex round message
  */
  //----------------------------------------------------------------------------------------------------------------------
  static multisig_keyset_map_memsafe_t evaluate_multisig_kex_round_msgs(
    const crypto::public_key &base_pubkey,
    const std::uint32_t expected_round,
    const std::vector<crypto::public_key> &signers,
    const std::vector<multisig_kex_msg> &expanded_msgs,
    const std::vector<crypto::public_key> &exclude_pubkeys,
    const bool incomplete_signer_set)
  {
    // exclude_pubkeys should all be unique
    for (auto it = exclude_pubkeys.begin(); it != exclude_pubkeys.end(); ++it)
    {
      CHECK_AND_ASSERT_THROW_MES(std::find(exclude_pubkeys.begin(), it, *it) == it,
        "Found duplicate pubkeys for exclusion unexpectedly.");
    }

    // sanitize input messages
    multisig_keyset_map_memsafe_t pubkey_origins_map;  //map: [pubkey : [origins]]
    const std::uint32_t round = multisig_kex_msgs_sanitize_pubkeys(expanded_msgs, exclude_pubkeys, pubkey_origins_map);
    CHECK_AND_ASSERT_THROW_MES(round == expected_round,
      "Kex messages were for round [" << round << "], but expected round is [" << expected_round << "]");

    // remove the local signer from each origins set in the sanitized pubkey map
    // note: intermediate kex rounds only need keys from other signers to make progress (keys from self are useless)
    remove_key_from_mapped_sets(base_pubkey, pubkey_origins_map);

    // evaluate pubkeys collected
    std::unordered_map<crypto::public_key, std::unordered_set<crypto::public_key>> origin_pubkeys_map;  //map: [origin: [pubkeys]]

    // 1. each pubkey should be recommended by a precise number of signers
    const std::size_t num_recommendations_per_pubkey_required{
        incomplete_signer_set
        ? 1
        : round
      };

    for (const auto &pubkey_and_origins : pubkey_origins_map)
    {
      // expected amount = round_num
      // With each successive round, pubkeys are shared by incrementally larger groups,
      //  starting at 1 in round 1 (i.e. the local multisig key to start kex with).
      CHECK_AND_ASSERT_THROW_MES(pubkey_and_origins.second.size() >= num_recommendations_per_pubkey_required,
        "A pubkey recommended by multisig kex messages had an unexpected number of recommendations.");

      // map (sanitized) pubkeys back to origins
      for (const auto &origin : pubkey_and_origins.second)
        origin_pubkeys_map[origin].insert(pubkey_and_origins.first);
    }

    // 2. the number of unique signers recommending pubkeys should equal the number of signers passed in (minus the local signer)
    // - if an incomplete set is allowed, then we need at least one signer to represent each subgroup in this round that
    //   doesn't include the local signer
    const std::size_t num_signers_required{
        incomplete_signer_set
        ? signers.size() - 1 - (round - 1)
        : signers.size() - 1
      };

    CHECK_AND_ASSERT_THROW_MES(origin_pubkeys_map.size() >= num_signers_required,
      "Number of unique other signers recommending pubkeys does not equal number of required other signers "
      "(kex round: " << round << ", num signers found: " << origin_pubkeys_map.size() << ", num signers required: " <<
      num_signers_required << ").");

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
      // - In each round, every group of size 'round num' will have a key. From a single signer's perspective,
      //   they will share a key with every group of size 'round num - 1' of other signers.
      // - Since 'origins pubkey map' excludes keys shared with the local account, only keys shared with participants
      //   'other than local and self' will be in the map (e.g. N - 2 signers).
      // - Other signers will recommend (N - 2) choose (msg_round_num - 1) pubkeys (after removing keys shared with local).
      // Note: Keys shared with local are filtered out to facilitate kex round boosting, where one or more signers may
      //       have boosted the local signer (implying they didn't have access to the local signer's previous round msg).
    const std::uint32_t expected_recommendations_others = n_choose_k_f(signers.size() - 2, round - 1);

    // local: (N - 1) choose (msg_round_num - 1)
    const std::uint32_t expected_recommendations_self = n_choose_k_f(signers.size() - 1, round - 1);

    // note: expected_recommendations_others would be 0 in the last round of 1-of-N, but we don't call this function for
    //       that case
    CHECK_AND_ASSERT_THROW_MES(expected_recommendations_self > 0 && expected_recommendations_others > 0,
      "Bad num signers or round num (possibly numerical limits exceeded).");

    // check that local account recommends expected number of keys
    CHECK_AND_ASSERT_THROW_MES(exclude_pubkeys.size() == expected_recommendations_self,
      "Local account did not recommend expected number of multisig keys.");

    // check that other signers recommend expected number of keys
    for (const auto &origin_and_pubkeys : origin_pubkeys_map)
    {
      CHECK_AND_ASSERT_THROW_MES(origin_and_pubkeys.second.size() == expected_recommendations_others,
        "A multisig signer recommended an unexpected number of pubkeys.");

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
  * brief: evaluate_multisig_post_kex_round_msgs - Evaluate messages for the post-kex verification round.
  *    - Sanitizes input msgs.
  *    - Requires that only one pubkey is recommended.
  *    - Requires that all signers (other than self) recommend that one pubkey.
  * param: base_pubkey - multisig account's base public key
  * param: expected_round - expected kex round of input messages
  * param: signers - expected participants in multisig kex
  * param: expanded_msgs - set of multisig kex messages to process
  * param: incomplete_signer_set - only require the minimum amount of messages to complete this round (1 message)
  * return: sanitized and validated pubkey:origins map
  */
  //----------------------------------------------------------------------------------------------------------------------
  static multisig_keyset_map_memsafe_t evaluate_multisig_post_kex_round_msgs(
    const crypto::public_key &base_pubkey,
    const std::uint32_t expected_round,
    const std::vector<crypto::public_key> &signers,
    const std::vector<multisig_kex_msg> &expanded_msgs,
    const bool incomplete_signer_set)
  {
    // sanitize input messages
    const std::vector<crypto::public_key> dummy;
    multisig_keyset_map_memsafe_t pubkey_origins_map;  //map: [pubkey : [origins]]
    const std::uint32_t round = multisig_kex_msgs_sanitize_pubkeys(expanded_msgs, dummy, pubkey_origins_map);
    CHECK_AND_ASSERT_THROW_MES(round == expected_round,
      "Kex messages were for round [" << round << "], but expected round is [" << expected_round << "]");

    // note: do NOT remove the local signer from the pubkey origins map, since the post-kex round can be force-updated with
    //       just the local signer's post-kex message (if the local signer were removed, then the post-kex message's pubkeys
    //       would be completely lost)

    // evaluate pubkeys collected

    // 1) there should only be two pubkeys
    CHECK_AND_ASSERT_THROW_MES(pubkey_origins_map.size() == 2,
      "Multisig post-kex round messages from other signers did not all contain two pubkeys.");

    // 2) both keys should be recommended by the same set of signers
    CHECK_AND_ASSERT_THROW_MES(pubkey_origins_map.begin()->second == (++(pubkey_origins_map.begin()))->second,
      "Multisig post-kex round messages from other signers did not all recommend the same pubkey pair.");

    // 3) all signers should be present in the recommendation list (unless an incomplete list is permitted)
    auto origins = pubkey_origins_map.begin()->second;
    origins.insert(base_pubkey);  //add self if missing

    const std::size_t num_signers_required{
        incomplete_signer_set
        ? 1
        : signers.size()
      };

    CHECK_AND_ASSERT_THROW_MES(origins.size() >= num_signers_required,
      "Multisig post-kex round message origins don't line up with multisig signer set "
      "(num signers found: " << origins.size() << ", num signers required: " << num_signers_required << ").");

    for (const crypto::public_key &origin : origins)
    {
      // note: if num_signers_required == signers.size(), then this test will ensure all signers are present in 'origins',
      //       which contains only unique pubkeys
      CHECK_AND_ASSERT_THROW_MES(std::find(signers.begin(), signers.end(), origin) != signers.end(),
        "An unknown origin recommended a multisig post-kex verification messsage.");
    }

    return pubkey_origins_map;
  }
  //----------------------------------------------------------------------------------------------------------------------
  /**
  * INTERNAL
  *
  * brief: multisig_kex_process_round_msgs - Process kex messages for the active kex round.
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
  * param: incomplete_signer_set - allow messages from an incomplete signer set
  * outparam: keys_to_origins_map_out - map between round keys and identity keys
  *    - If in the final round, these are key shares recommended by other signers for the final aggregate key.
  *    - Otherwise, these are the local account's DH derivations for the next round.
  *      - See multisig_kex_make_next_msg() for an explanation.
  * return: multisig kex message for next round, or empty message if 'current_round' is the final round
  */
  //----------------------------------------------------------------------------------------------------------------------
  static void multisig_kex_process_round_msgs(const crypto::secret_key &base_privkey,
    const crypto::public_key &base_pubkey,
    const std::uint32_t current_round,
    const std::uint32_t threshold,
    const std::vector<crypto::public_key> &signers,
    const std::vector<multisig_kex_msg> &expanded_msgs,
    const std::vector<crypto::public_key> &exclude_pubkeys,
    const bool incomplete_signer_set,
    multisig_keyset_map_memsafe_t &keys_to_origins_map_out)
  {
    check_multisig_config(current_round, threshold, signers.size());
    const std::uint32_t kex_rounds_required{multisig_kex_rounds_required(signers.size(), threshold)};

    // process messages into a [pubkey : {origins}] map
    multisig_keyset_map_memsafe_t evaluated_pubkeys;

    if (threshold == 1 && current_round == kex_rounds_required)
    {
      // in the last main kex round of 1-of-N, all signers share a key so the local signer doesn't care about evaluating
      // recommendations from other signers
    }
    else if (current_round <= kex_rounds_required)
    {
      // for normal kex rounds, fully evaluate kex round messages
      evaluated_pubkeys = evaluate_multisig_kex_round_msgs(base_pubkey,
        current_round,
        signers,
        expanded_msgs,
        exclude_pubkeys,
        incomplete_signer_set);
    }
    else //(current_round == kex_rounds_required + 1)
    {
      // for the post-kex verification round, validate the last kex round's messages
      evaluated_pubkeys = evaluate_multisig_post_kex_round_msgs(base_pubkey,
        current_round,
        signers,
        expanded_msgs,
        incomplete_signer_set);
    }

    // prepare keys-to-origins map for updating the multisig account
    if (current_round < kex_rounds_required)
    {
      // normal kex round: make new keys
      multisig_kex_make_round_keys(base_privkey, std::move(evaluated_pubkeys), keys_to_origins_map_out);
    }
    else if (current_round >= kex_rounds_required)
    {
      // last kex round: collect the key shares recommended by other signers for the final aggregate key
      // post-kex verification round: save the keys found in input messages
      keys_to_origins_map_out = std::move(evaluated_pubkeys);
    }
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: INTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_account::initialize_kex_update(const std::vector<multisig_kex_msg> &expanded_msgs,
    const std::uint32_t kex_rounds_required,
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
      if (kex_rounds_required == 1)
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
  void multisig_account::finalize_kex_update(const std::uint32_t kex_rounds_required,
    multisig_keyset_map_memsafe_t result_keys_to_origins_map)
  {
    std::vector<crypto::public_key> next_msg_keys;

    // prepare for next round (or complete the multisig account fully)
    if (m_kex_rounds_complete == kex_rounds_required)
    {
      // post-kex verification round: check that the multisig pubkey and common pubkey were recommended by other signers
      CHECK_AND_ASSERT_THROW_MES(result_keys_to_origins_map.count(m_multisig_pubkey) > 0,
        "Multisig post-kex round: expected multisig pubkey wasn't found in input messages.");
      CHECK_AND_ASSERT_THROW_MES(result_keys_to_origins_map.count(m_common_pubkey) > 0,
        "Multisig post-kex round: expected common pubkey wasn't found in input messages.");

      // save keys that should be recommended to other signers
      // - for convenience, re-recommend the post-kex verification message once an account is complete
      next_msg_keys.reserve(2);
      next_msg_keys.push_back(m_multisig_pubkey);
      next_msg_keys.push_back(m_common_pubkey);
    }
    else if (m_kex_rounds_complete + 1 == kex_rounds_required)
    {
      // finished with main kex rounds (have set of msgs to complete address)

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

      // save keys that should be recommended to other signers
      // - for post-kex verification, recommend the multisig pubkeys to notify other signers that the local signer is done
      next_msg_keys.reserve(2);
      next_msg_keys.push_back(m_multisig_pubkey);
      next_msg_keys.push_back(m_common_pubkey);
    }
    else if (m_kex_rounds_complete + 2 == kex_rounds_required)
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
      next_msg_keys.reserve(result_keys_to_origins_map.size());

      for (const auto &derivation_and_origins : result_keys_to_origins_map)
      {
        // multisig_privkey = H(derivation)
        // derived pubkey = multisig_key * G
        crypto::public_key_memsafe derived_pubkey;
        m_multisig_privkeys.push_back(
          calculate_multisig_keypair_from_derivation(derivation_and_origins.first, derived_pubkey));

        // save the account's kex key mappings for this round [derived pubkey : other signers who will have the same key]
        m_kex_keys_to_origins_map[derived_pubkey] = std::move(derivation_and_origins.second);

        // save keys that should be recommended to other signers
        // - The keys multisig_key*G are sent to other participants in the message, so they can be used to produce the final
        //   multisig key via generate_multisig_spend_public_key().
        next_msg_keys.push_back(derived_pubkey);
      }
    }
    else  //(m_kex_rounds_complete + 3 <= kex_rounds_required)
    {
      // next round is an 'intermediate' key exchange round, so there is nothing special to do here

      // save keys that should be recommended to other signers
      // - Send this round's DH derivations to other participants, who will make more DH derivations for the following round.
      next_msg_keys.reserve(result_keys_to_origins_map.size());

      for (const auto &derivation_and_origins : result_keys_to_origins_map)
        next_msg_keys.push_back(derivation_and_origins.first);

      // save the account's kex keys for this round [DH derivation : other signers who should have the same derivation]
      m_kex_keys_to_origins_map = std::move(result_keys_to_origins_map);
    }

    // a full set of msgs has been collected and processed, so the 'round is complete'
    ++m_kex_rounds_complete;

    // make next round's message (or reproduce the post-kex verification round if kex is complete)
    m_next_round_kex_message = multisig_kex_msg{
      (m_kex_rounds_complete > kex_rounds_required ? kex_rounds_required : m_kex_rounds_complete) + 1,
      m_base_privkey,
      std::move(next_msg_keys)}.get_msg();
  }
  //----------------------------------------------------------------------------------------------------------------------
  // multisig_account: INTERNAL
  //----------------------------------------------------------------------------------------------------------------------
  void multisig_account::kex_update_impl(const std::vector<multisig_kex_msg> &expanded_msgs,
    const bool incomplete_signer_set)
  {
    // check messages are for the expected kex round
    check_messages_round(expanded_msgs, m_kex_rounds_complete + 1);

    // check kex round count
    const std::uint32_t kex_rounds_required{multisig_kex_rounds_required(m_signers.size(), m_threshold)};

    CHECK_AND_ASSERT_THROW_MES(kex_rounds_required > 0, "Multisig kex rounds required unexpectedly 0.");
    CHECK_AND_ASSERT_THROW_MES(m_kex_rounds_complete < kex_rounds_required + 1,
      "Multisig kex has already completed all required rounds (including post-kex verification).");

    // initialize account update
    std::vector<crypto::public_key> exclude_pubkeys;
    initialize_kex_update(expanded_msgs, kex_rounds_required, exclude_pubkeys);

    // process messages into a [pubkey : {origins}] map
    multisig_keyset_map_memsafe_t result_keys_to_origins_map;
    multisig_kex_process_round_msgs(
      m_base_privkey,
      m_base_pubkey,
      m_kex_rounds_complete + 1,
      m_threshold,
      m_signers,
      expanded_msgs,
      exclude_pubkeys,
      incomplete_signer_set,
      result_keys_to_origins_map);

    // finish account update
    finalize_kex_update(kex_rounds_required, std::move(result_keys_to_origins_map));
  }
  //----------------------------------------------------------------------------------------------------------------------
} //namespace multisig
