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
#include "cryptonote_basic/cryptonote_basic.h"

//third party headers

//standard headers
#include <cstdint>
#include <optional>

//forward declarations

namespace carrot
{

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
 * param: block_index -
 * return: a full coinbase transaction containing given Carrot information
 */
cryptonote::transaction store_carrot_to_coinbase_transaction_v1(
    const std::vector<CarrotCoinbaseEnoteV1> &enotes,
    const std::uint64_t block_index);
/**
 * brief: try_load_carrot_from_coinbase_transaction_v1 - load coinbase Carrot info from a cryptonote::transaction
 * param: tx -
 * outparam: enotes_out -
 * outparam: block_index_out -
 * return: Carrot coinbase enotes and block index contained within a coinbase transaction
 */
bool try_load_carrot_from_coinbase_transaction_v1(const cryptonote::transaction &tx,
    std::vector<CarrotCoinbaseEnoteV1> &enotes_out,
    std::uint64_t &block_index_out);

} //namespace carrot
