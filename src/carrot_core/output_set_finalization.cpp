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

//paired header
#include "output_set_finalization.h"

//local headers
#include "common/container_helpers.h"
#include "enote_utils.h"
#include "misc_log_ex.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers
#include <set>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename T>
struct compare_memcmp{ bool operator()(const T &a, const T &b) const { return memcmp(&a, &b, sizeof(T)) < 0; } };
template <typename T>
using memcmp_set = std::set<T, compare_memcmp<T>>;
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
std::optional<AdditionalOutputType> get_additional_output_type(const size_t num_outgoing,
    const size_t num_selfsend,
    const bool need_change_output,
    const bool have_payment_type_selfsend)
{
    const size_t num_outputs = num_outgoing + num_selfsend;
    const bool already_completed = num_outputs >= 2 && num_selfsend >= 1 && !need_change_output;
    if (num_outputs == 0)
    {
        ASSERT_MES_AND_THROW("get additional output type: set contains 0 outputs");
    }
    else if (already_completed)
    {
        return std::nullopt;
    }
    else if (num_outputs == 1)
    {
        if (num_selfsend == 0)
        {
            return AdditionalOutputType::CHANGE_SHARED;
        }
        else if (!need_change_output)
        {
            return AdditionalOutputType::DUMMY;
        }
        else // num_selfsend == 1 && need_change_output
        {
            if (have_payment_type_selfsend)
            {
                return AdditionalOutputType::CHANGE_SHARED;
            }
            else
            {
                return AdditionalOutputType::PAYMENT_SHARED;
            }
        }
    }
    else if (num_outputs < CARROT_MAX_TX_OUTPUTS)
    {
        return AdditionalOutputType::CHANGE_UNIQUE;
    }
    else // num_outputs >= CARROT_MAX_TX_OUTPUTS
    {
       ASSERT_MES_AND_THROW("get additional output type: "
        "set needs finalization but already contains too many outputs");
    }
}
//-------------------------------------------------------------------------------------------------------------------
tools::optional_variant<CarrotPaymentProposalV1, CarrotPaymentProposalSelfSendV1> get_additional_output_proposal(
    const size_t num_outgoing,
    const size_t num_selfsend,
    const rct::xmr_amount needed_change_amount,
    const bool have_payment_type_selfsend,
    const crypto::public_key &change_address_spend_pubkey)
{
    const std::optional<AdditionalOutputType> additional_output_type = get_additional_output_type(
            num_outgoing,
            num_selfsend,
            needed_change_amount,
            have_payment_type_selfsend
        );

    if (!additional_output_type)
        return {};

    switch (*additional_output_type)
    {
    case AdditionalOutputType::PAYMENT_SHARED:
        return CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = change_address_spend_pubkey,
            .amount = needed_change_amount,
            .enote_type = CarrotEnoteType::PAYMENT,
            .enote_ephemeral_pubkey = std::nullopt
        };
    case AdditionalOutputType::CHANGE_SHARED:
        return CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = change_address_spend_pubkey,
            .amount = needed_change_amount,
            .enote_type = CarrotEnoteType::CHANGE,
            .enote_ephemeral_pubkey = std::nullopt
        };
    case AdditionalOutputType::CHANGE_UNIQUE:
        return CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = change_address_spend_pubkey,
            .amount = needed_change_amount,
            .enote_type = CarrotEnoteType::CHANGE,
            .enote_ephemeral_pubkey = std::nullopt
        };
    case AdditionalOutputType::DUMMY:
        return CarrotPaymentProposalV1{
            .destination = gen_carrot_main_address_v1(),
            .amount = 0,
            .randomness = gen_janus_anchor()
        };
    }

    ASSERT_MES_AND_THROW("get additional output proposal: unrecognized additional output type");
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_enote_proposals(const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalSelfSendV1> &selfsend_payment_proposals,
    const std::optional<encrypted_payment_id_t> &dummy_encrypted_payment_id,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::key_image &tx_first_key_image,
    std::vector<RCTOutputEnoteProposal> &output_enote_proposals_out,
    encrypted_payment_id_t &encrypted_payment_id_out)
{
    output_enote_proposals_out.clear();

    // assert payment proposals numbers
    const size_t num_proposals = normal_payment_proposals.size() + selfsend_payment_proposals.size();
    CHECK_AND_ASSERT_THROW_MES(num_proposals >= CARROT_MIN_TX_OUTPUTS, 
        "get output enote proposals: too few payment proposals");
    CHECK_AND_ASSERT_THROW_MES(num_proposals <= CARROT_MAX_TX_OUTPUTS,
        "get output enote proposals: too many payment proposals");
    CHECK_AND_ASSERT_THROW_MES(selfsend_payment_proposals.size(),
        "get output enote proposals: no selfsend payment proposal");

    // assert there is a max of 1 integrated address payment proposals
    size_t num_integrated = 0;
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        if (normal_payment_proposal.destination.payment_id != null_payment_id)
            ++num_integrated;
    CHECK_AND_ASSERT_THROW_MES(num_integrated <= 1,
        "get output enote proposals: only one integrated address is allowed per tx output set");

    // assert anchor_norm != 0 for payments
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        CHECK_AND_ASSERT_THROW_MES(normal_payment_proposal.randomness != janus_anchor_t{},
            "get output enote proposals: normal payment proposal has unset anchor_norm AKA randomness");

    // assert uniqueness of randomness for each payment
    memcmp_set<janus_anchor_t> randomnesses;
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        randomnesses.insert(normal_payment_proposal.randomness);
    const bool has_unique_randomness = randomnesses.size() == normal_payment_proposals.size();
    CHECK_AND_ASSERT_THROW_MES(has_unique_randomness,
        "get output enote proposals: normal payment proposals contain duplicate anchor_norm AKA randomness");

    // input_context = "R" || KI_1
    input_context_t input_context;
    make_carrot_input_context(tx_first_key_image, input_context);

    // construct normal enotes
    output_enote_proposals_out.reserve(num_proposals);
    for (size_t i = 0; i < normal_payment_proposals.size(); ++i)
    {
        encrypted_payment_id_t encrypted_payment_id;
        get_output_proposal_normal_v1(normal_payment_proposals[i],
            tx_first_key_image,
            tools::add_element(output_enote_proposals_out),
            encrypted_payment_id);

        // set pid_enc from integrated address proposal pic_enc
        const bool is_integrated = normal_payment_proposals[i].destination.payment_id != null_payment_id;
        if (is_integrated)
            encrypted_payment_id_out = encrypted_payment_id;
    }

    // in the case that there is no required pid_enc, set it to the provided dummy
    if (0 == num_integrated)
    {
        CHECK_AND_ASSERT_THROW_MES(dummy_encrypted_payment_id,
            "get output enote proposals: missing encrypted payment ID: no integrated address nor provided dummy");
        encrypted_payment_id_out = *dummy_encrypted_payment_id;
    }

    // construct selfsend enotes, preferring internal enotes over special enotes when possible
    for (const CarrotPaymentProposalSelfSendV1 &selfsend_payment_proposal : selfsend_payment_proposals)
    {
        const std::optional<mx25519_pubkey> other_enote_ephemeral_pubkey =
            (num_proposals == 2 && output_enote_proposals_out.size())
                ? output_enote_proposals_out.at(0).enote.enote_ephemeral_pubkey
                : std::optional<mx25519_pubkey>{};

        if (s_view_balance_dev != nullptr)
        {
            get_output_proposal_internal_v1(selfsend_payment_proposal,
                *s_view_balance_dev,
                tx_first_key_image,
                other_enote_ephemeral_pubkey,
                tools::add_element(output_enote_proposals_out));
        }
        else if (k_view_dev != nullptr)
        {
            get_output_proposal_special_v1(selfsend_payment_proposal,
                *k_view_dev,
                tx_first_key_image,
                other_enote_ephemeral_pubkey,
                tools::add_element(output_enote_proposals_out));
        }
        else // neither k_v nor s_vb device passed
        {
            ASSERT_MES_AND_THROW(
                "get output enote proposals: neither a view-balance nor view-incoming device was provided");
        }
    }

    // assert uniqueness of D_e if >2-out, shared otherwise. also check D_e is not trivial
    memcmp_set<mx25519_pubkey> ephemeral_pubkeys;
    for (const RCTOutputEnoteProposal &p : output_enote_proposals_out)
    {
        const bool trivial_enote_ephemeral_pubkey = memcmp(p.enote.enote_ephemeral_pubkey.data,
            mx25519_pubkey{}.data,
            sizeof(mx25519_pubkey)) == 0;
        CHECK_AND_ASSERT_THROW_MES(!trivial_enote_ephemeral_pubkey,
            "get output enote proposals: this set contains enote ephemeral pubkeys with x=0");
        ephemeral_pubkeys.insert(p.enote.enote_ephemeral_pubkey);
    }
    const bool has_unique_ephemeral_pubkeys = ephemeral_pubkeys.size() == output_enote_proposals_out.size();
    CHECK_AND_ASSERT_THROW_MES(!(num_proposals == 2 && has_unique_ephemeral_pubkeys),
        "get output enote proposals: a 2-out set needs to share an ephemeral pubkey, but this 2-out set doesn't");
    CHECK_AND_ASSERT_THROW_MES(!(num_proposals != 2 && !has_unique_ephemeral_pubkeys),
        "get output enote proposals: this >2-out set contains duplicate enote ephemeral pubkeys");

    // sort enotes by K_o
    const auto sort_output_enote_proposal = [](const RCTOutputEnoteProposal &a, const RCTOutputEnoteProposal &b)
        -> bool { return a.enote.onetime_address < b.enote.onetime_address; };
    std::sort(output_enote_proposals_out.begin(), output_enote_proposals_out.end(), sort_output_enote_proposal);

    // assert uniqueness of K_o
    CHECK_AND_ASSERT_THROW_MES(tools::is_sorted_and_unique(output_enote_proposals_out, sort_output_enote_proposal),
        "get output enote proposals: this set contains duplicate onetime addresses");

    // assert all K_o lie in prime order subgroup
    for (const RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals_out)
    {
        CHECK_AND_ASSERT_THROW_MES(rct::isInMainSubgroup(rct::pk2rct(output_enote_proposal.enote.onetime_address)),
            "get output enote proposals: this set contains an invalid onetime address");
    }

    // assert unique and non-trivial k_a
    memcmp_set<crypto::secret_key> amount_blinding_factors;
    for (const RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals_out)
    {
        CHECK_AND_ASSERT_THROW_MES(output_enote_proposal.amount_blinding_factor != crypto::null_skey,
            "get output enote proposals: this set contains a trivial amount blinding factor");

        amount_blinding_factors.insert(output_enote_proposal.amount_blinding_factor);
    }
    CHECK_AND_ASSERT_THROW_MES(amount_blinding_factors.size() == num_proposals,
        "get output enote proposals: this set contains duplicate amount blinding factors");
}
//-------------------------------------------------------------------------------------------------------------------
void get_coinbase_output_enotes(const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const uint64_t block_index,
    std::vector<CarrotCoinbaseEnoteV1> &output_coinbase_enotes_out)
{
    output_coinbase_enotes_out.clear();

    // assert payment proposals numbers
    const size_t num_proposals = normal_payment_proposals.size();

    // assert there is a max of 1 integrated address payment proposals
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
    {
        const CarrotDestinationV1 &destination = normal_payment_proposal.destination;
        CHECK_AND_ASSERT_THROW_MES(destination.payment_id == null_payment_id && !destination.is_subaddress,
            "get coinbase output enotes: no integrated addresses or subaddresses allowed");
    }

    // assert anchor_norm != 0 for payments
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        CHECK_AND_ASSERT_THROW_MES(normal_payment_proposal.randomness != janus_anchor_t{},
            "get coinbase output enotes: normal payment proposal has unset anchor_norm AKA randomness");

    // assert uniqueness of randomness for each payment
    memcmp_set<janus_anchor_t> randomnesses;
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        randomnesses.insert(normal_payment_proposal.randomness);
    const bool has_unique_randomness = randomnesses.size() == normal_payment_proposals.size();
    CHECK_AND_ASSERT_THROW_MES(has_unique_randomness,
        "get coinbase output enotes: normal payment proposals contain duplicate anchor_norm AKA randomness");

    // input_context = "C" || IntToBytes256(block_index)
    input_context_t input_context;
    make_carrot_input_context_coinbase(block_index, input_context);

    // construct normal enotes
    output_coinbase_enotes_out.reserve(num_proposals);
    for (size_t i = 0; i < normal_payment_proposals.size(); ++i)
    {
        get_coinbase_output_proposal_v1(normal_payment_proposals[i],
            block_index,
            tools::add_element(output_coinbase_enotes_out));
    }

    // assert uniqueness and non-trivial-ness of D_e
    memcmp_set<mx25519_pubkey> ephemeral_pubkeys;
    for (const CarrotCoinbaseEnoteV1 &enote : output_coinbase_enotes_out)
    {
        const bool trivial_enote_ephemeral_pubkey = memcmp(enote.enote_ephemeral_pubkey.data,
            mx25519_pubkey{}.data,
            sizeof(mx25519_pubkey)) == 0;
        CHECK_AND_ASSERT_THROW_MES(!trivial_enote_ephemeral_pubkey,
            "get coinbase output enotes: this set contains enote ephemeral pubkeys with x=0");
        ephemeral_pubkeys.insert(enote.enote_ephemeral_pubkey);
    }
    const bool has_unique_ephemeral_pubkeys = ephemeral_pubkeys.size() == output_coinbase_enotes_out.size();
    CHECK_AND_ASSERT_THROW_MES(has_unique_ephemeral_pubkeys,
        "get coinbase output enotes: a coinbase enote set needs unique ephemeral pubkeys, but this set doesn't");

    // sort enotes by K_o
    const auto sort_output_enote_proposal = [](const CarrotCoinbaseEnoteV1 &a, const CarrotCoinbaseEnoteV1 &b)
        -> bool { return a.onetime_address < b.onetime_address; };
    std::sort(output_coinbase_enotes_out.begin(), output_coinbase_enotes_out.end(), sort_output_enote_proposal);

    // assert uniqueness of K_o
    CHECK_AND_ASSERT_THROW_MES(tools::is_sorted_and_unique(output_coinbase_enotes_out, sort_output_enote_proposal),
        "get coinbase output enotes: this set contains duplicate onetime addresses");

    // assert all K_o lie in prime order subgroup
    for (const CarrotCoinbaseEnoteV1 &output_enote : output_coinbase_enotes_out)
    {
        CHECK_AND_ASSERT_THROW_MES(rct::isInMainSubgroup(rct::pk2rct(output_enote.onetime_address)),
            "get coinbase output enotes: this set contains an invalid onetime address");
    }
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
