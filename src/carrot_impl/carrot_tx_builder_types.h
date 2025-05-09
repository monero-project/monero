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
#include "carrot_core/payment_proposal.h"
#include "crypto/crypto.h"
#include "ringct/rctTypes.h"
#include "subaddress_index.h"

//third party headers
#include <boost/multiprecision/cpp_int.hpp>

//standard headers
#include <functional>
#include <map>

//forward declarations

namespace carrot
{
struct CarrotSelectedInput
{
    rct::xmr_amount amount;
    crypto::key_image key_image;
};
static inline bool operator==(const CarrotSelectedInput &a, const CarrotSelectedInput &b)
{
    return a.amount == b.amount && a.key_image == b.key_image;
}
static inline bool operator!=(const CarrotSelectedInput &a, const CarrotSelectedInput &b)
{
    return !(a == b);
}

struct CarrotPaymentProposalVerifiableSelfSendV1
{
    CarrotPaymentProposalSelfSendV1 proposal;
    subaddress_index_extended subaddr_index;
};

using select_inputs_func_t = std::function<void(
        const boost::multiprecision::uint128_t&,       // nominal output sum, w/o fee
        const std::map<std::size_t, rct::xmr_amount>&, // absolute fee per input count
        const std::size_t,                             // number of normal payment proposals
        const std::size_t,                             // number of self-send payment proposals
        std::vector<CarrotSelectedInput>&              // selected inputs result
    )>;

using carve_fees_and_balance_func_t = std::function<void(
        const boost::multiprecision::uint128_t&,                // input sum amount
        const rct::xmr_amount,                                  // fee
        std::vector<CarrotPaymentProposalV1>&,                  // normal payment proposals [inout]
        std::vector<CarrotPaymentProposalVerifiableSelfSendV1>& // selfsend payment proposals [inout]
    )>;

/**
 * brief: CarrotTransactionProposalV1
 */
struct CarrotTransactionProposalV1
{
    std::vector<crypto::key_image> key_images_sorted;

    std::vector<CarrotPaymentProposalV1> normal_payment_proposals;
    std::vector<CarrotPaymentProposalVerifiableSelfSendV1> selfsend_payment_proposals;
    encrypted_payment_id_t dummy_encrypted_payment_id;
    rct::xmr_amount fee;

    std::vector<std::uint8_t> extra;
};

} //namespace carrot
