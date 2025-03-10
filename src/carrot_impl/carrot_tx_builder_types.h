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

using CarrotPaymentProposalVariant = tools::variant<
        CarrotPaymentProposalV1,
        CarrotPaymentProposalVerifiableSelfSendV1
    >;

using select_inputs_func_t = std::function<void(
        const boost::multiprecision::int128_t&,        // nominal output sum, w/o fee
        const std::map<std::size_t, rct::xmr_amount>&, // absolute fee per input count
        const std::size_t,                             // number of normal payment proposals
        const std::size_t,                             // number of self-send payment proposals
        std::vector<CarrotSelectedInput>&              // selected inputs result
    )>;

using carve_fees_and_balance_func_t = std::function<void(
        const boost::multiprecision::int128_t&,                 // input sum amount
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

/*struct FcmpInput
{
    // Rerandomized input tuple
    crypto::public_key O_tilde;
    crypto::public_key I_tilde;
    crypto::public_key C_tilde;

    // Commitment to randomness
    crypto::public_key R;
};

struct LegacyOnetimeAddressOpeningHintV1
{
    // WARNING: Using this opening hint is unsafe and enables for HW devices to
    //          accidentally burn XMR if an attacker controls the hot wallet and
    //          can publish a new enote with the same K_o as an existing enote,
    //          but with a different amount. However, it is unavoidable for
    //          legacy enotes, since the computation of K_o is not directly nor
    //          indirectly bound to the amount.

    // Informs remote prover (implied to know opening of K_s) how to open O such that:
    // O = K_s + k^g_pr G

    // O
    crypto::public_key onetime_address;

    // k^g_pr
    rct::key prerand_extension_g;
};

struct CarrotOnetimeAddressOpeningHintV1
{
    // source enote
    CarrotEnoteV1 source_enote;

    // pid_enc
    std::optional<encrypted_payment_id_t> encrypted_payment_id;

    // j, derive type
    subaddress_index_extended subaddr_index;
};

struct CarrotCoinbaseOnetimeAddressOpeningHintV1
{
    // source enote
    CarrotCoinbaseEnoteV1 source_enote;

    // no encrypted pids for coinbase transactions

    // subaddress index is assumed to be (0, 0) in coinbase transactions
    AddressDeriveType derive_type;
};

using OnetimeAddressOpeningHintVariant = tools::variant<
        LegacyOnetimeAddressOpeningHintV1,
        CarrotOnetimeAddressOpeningHintV1,
        CarrotCoinbaseOnetimeAddressOpeningHintV1
    >;
const crypto::public_key &onetime_address_ref(const OnetimeAddressOpeningHintVariant&);

struct CarrotRerandomizedOutputV1
{
    // O~, I~, C~, R
    FcmpInput input;

    // Opening information for O (and sometimes C if possible)
    OnetimeAddressOpeningHintVariant onetime_address_opening_hint;

    // Rerandomization values such that:
    // O~ = O + r_o T
    // I~ = I + r_i U
    // R = r_i V + r_r_i T
    // C~ = C + r_c G
    rct::key r_o;
    rct::key r_i;
    rct::key r_r_i;
    rct::key r_c;
};

struct CarrotUnsignedTransactionV1
{
    CarrotTransactionProposalV1 tx_proposal;

    std::vector<CarrotRerandomizedOutputV1> rerandomized_outputs;
};
*/

} //namespace carrot
