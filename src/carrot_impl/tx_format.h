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

//forward declarations

namespace carrot
{
cryptonote::transaction store_carrot_to_transaction_v1(const std::vector<CarrotEnoteV1> &enotes,
    const std::vector<crypto::key_image> &key_images,
    const encrypted_payment_id_t encrypted_payment_id);

std::tuple<std::vector<CarrotEnoteV1>, std::vector<crypto::key_image>, encrypted_payment_id_t>
load_carrot_from_transaction_v1(const cryptonote::transaction &tx);

cryptonote::transaction store_carrot_to_coinbase_transaction_v1(
    const std::vector<CarrotCoinbaseEnoteV1> &enotes,
    const std::uint64_t block_index);

std::tuple<std::vector<CarrotCoinbaseEnoteV1>, std::uint64_t>
load_carrot_from_coinbase_transaction_v1(const cryptonote::transaction &tx);

} //namespace carrot
