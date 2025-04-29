// Copyright (c) 2024, The Monero Project
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

//local headers
#include "carrot_core/carrot_enote_types.h"
#include "carrot_core/destination.h"
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/tx_extra.h"

//third party headers

//standard headers
#include <cstdint>
#include <optional>
#include <type_traits>

//forward declarations

namespace carrot
{
static constexpr std::uint8_t carrot_v1_rct_type = rct::RCTTypeFcmpPlusPlus;

template <typename T, typename U>
static inline T raw_byte_convert(const U &u)
{
    static_assert(sizeof(T) == sizeof(U));
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::has_unique_object_representations_v<T>);
    static_assert(std::has_unique_object_representations_v<U>);
    static_assert(alignof(T) == 1);
    static_assert(alignof(U) == 1);

    T t;
    memcpy(&t, &u, sizeof(T));
    return t;
}

/**
 * is_carrot_transaction_v1 - determine whether a transaction uses the Carrot addressing protocol
 */
bool is_carrot_transaction_v1(const cryptonote::transaction_prefix &tx_prefix);
/**
 * brief: parse_carrot_input_context - try parsing carrot input context from cryptonote transaction components
 */
input_context_t parse_carrot_input_context(const cryptonote::txin_gen &txin);
input_context_t parse_carrot_input_context(const cryptonote::txin_to_key &txin);
bool parse_carrot_input_context(const cryptonote::txin_v &txin, input_context_t &input_context_out);
bool parse_carrot_input_context(const cryptonote::transaction_prefix &tx_prefix, input_context_t &input_context_out);
/**
 * try_load_carrot_extra_v1 - load Carrot info which is stored in tx_extra
 * param: tx_extra_fields -
 * outparam: enote_ephemeral_pubkeys_out - D_e
 * outparam: encrypted_payment_id_out - pid_enc
 */
bool try_load_carrot_extra_v1(
    const std::vector<std::uint8_t> &tx_extra,
    std::vector<mx25519_pubkey> &enote_ephemeral_pubkeys_out,
    std::optional<encrypted_payment_id_t> &encrypted_payment_id_out);
bool try_load_carrot_extra_v1(
    const std::vector<cryptonote::tx_extra_field> &tx_extra_fields,
    std::vector<mx25519_pubkey> &enote_ephemeral_pubkeys_out,
    std::optional<encrypted_payment_id_t> &encrypted_payment_id_out);
/**
 * brief: store_carrot_to_transaction_v1 - store non-coinbase Carrot info to a cryptonote::transaction
 * param: enotes -
 * param: key_images -
 * param: fee -
 * param: encrypted_payment_id - pid_enc
 * return: a fully populated, pruned, non-coinbase transaction containing given Carrot information
 */
cryptonote::transaction store_carrot_to_transaction_v1(const std::vector<CarrotEnoteV1> &enotes,
    const std::vector<crypto::key_image> &key_images,
    const rct::xmr_amount fee,
    const encrypted_payment_id_t encrypted_payment_id);
/**
 * brief: try_load_carrot_enote_from_transaction_v1 - load one non-coinbase Carrot enote from a cryptonote::transaction
 * param: tx -
 * param: enote_ephemeral_pubkeys - D_e
 * param: local_output_index -
 * outparam: enote_out -
 * return: true iff enote was successfully parsed
 */
bool try_load_carrot_enote_from_transaction_v1(const cryptonote::transaction &tx,
    const epee::span<const mx25519_pubkey> enote_ephemeral_pubkeys,
    const std::size_t local_output_index,
    CarrotEnoteV1 &enote_out);
/**
 * brief: load_carrot_from_transaction_v1 - load non-coinbase Carrot info from a cryptonote::transaction
 * param: tx -
 * outparam: enotes_out -
 * outparam: key_images_out -
 * outparam: fee_out -
 * outparam: encrypted_payment_id_out -
 * return: Carrot enotes, key images, fee, and encrypted pid contained within a non-coinbase transaction
 */
bool try_load_carrot_from_transaction_v1(const cryptonote::transaction &tx,
    std::vector<CarrotEnoteV1> &enotes_out,
    std::vector<crypto::key_image> &key_images_out,
    rct::xmr_amount &fee_out,
    std::optional<encrypted_payment_id_t> &encrypted_payment_id_out);
/**
 * brief: store_carrot_to_coinbase_transaction_v1 - store coinbase Carrot info to a cryptonote::transaction
 * param: enotes -
 * param: extra_nonce -
 * return: a full coinbase transaction containing given Carrot information
 */
cryptonote::transaction store_carrot_to_coinbase_transaction_v1(
    const std::vector<CarrotCoinbaseEnoteV1> &enotes,
    const cryptonote::blobdata &extra_nonce);
/**
 * brief: make_single_enote_carrot_coinbase_transaction_v1 - store one coinbase Carrot enote to a cryptonote::transaction
 * param: destination -
 * param: block_reward -
 * param: block_index -
 * param: extra_nonce -
 * return: a full coinbase transaction containing given Carrot information
 */
cryptonote::transaction make_single_enote_carrot_coinbase_transaction_v1(const CarrotDestinationV1 &destination,
    const rct::xmr_amount block_reward,
    const std::uint64_t block_index,
    const cryptonote::blobdata &extra_nonce);
/**
 * brief: try_load_carrot_coinbase_enote_from_transaction_v1 - load one coinbase Carrot enote from a cryptonote::transaction
 * param: tx -
 * param: enote_ephemeral_pubkeys -
 * param: local_output_index -
 * outparam: enote_out -
 * return:  true iff enote was successfully parsed
 */
bool try_load_carrot_coinbase_enote_from_transaction_v1(const cryptonote::transaction &tx,
    const epee::span<const mx25519_pubkey> enote_ephemeral_pubkeys,
    const std::size_t local_output_index,
    CarrotCoinbaseEnoteV1 &enote_out);
/**
 * brief: try_load_carrot_from_coinbase_transaction_v1 - load coinbase Carrot info from a cryptonote::transaction
 * param: tx -
 * outparam: enotes_out -
 * return: Carrot coinbase enotes and block index contained within a coinbase transaction
 */
bool try_load_carrot_from_coinbase_transaction_v1(const cryptonote::transaction &tx,
    std::vector<CarrotCoinbaseEnoteV1> &enotes_out);
/**
 * brief: store_fcmp_proofs_to_rct_prunable_v1 -
 * param: bulletproof_plus -
 * param: rerandomized_outputs -
 * param: sal_proofs -
 * param: membership_proof -
 * param: fcmp_reference_block -
 * param: n_tree_layers -
 * return: prunable RCT signature data that can be attached to corresponding pruned tx
 */
rct::rctSigPrunable store_fcmp_proofs_to_rct_prunable_v1(
    rct::BulletproofPlus &&bulletproof_plus,
    const std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs,
    const std::vector<fcmp_pp::FcmpPpSalProof> &sal_proofs,
    const fcmp_pp::FcmpMembershipProof &membership_proof,
    const std::uint64_t fcmp_reference_block,
    const std::uint8_t n_tree_layers);

} //namespace carrot
