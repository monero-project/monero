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
#include "exceptions.h"
#include "misc_log_ex.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers
#include <set>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot.osf"

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
        CARROT_THROW(too_few_outputs, "set contains 0 outputs");
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
       CARROT_THROW(too_many_outputs, "set needs finalization but already contains too many outputs");
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

    CARROT_THROW(std::invalid_argument, "unrecognized additional output type");
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_enote_proposals(const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalSelfSendV1> &selfsend_payment_proposals,
    const std::optional<encrypted_payment_id_t> &dummy_encrypted_payment_id,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::key_image &tx_first_key_image,
    std::vector<RCTOutputEnoteProposal> &output_enote_proposals_out,
    encrypted_payment_id_t &encrypted_payment_id_out,
    std::vector<std::pair<bool, std::size_t>> *payment_proposal_order_out)
{
    output_enote_proposals_out.clear();
    encrypted_payment_id_out = {{0}};
    if (payment_proposal_order_out)
        payment_proposal_order_out->clear();

    // assert payment proposals numbers
    const size_t num_selfsend_proposals = selfsend_payment_proposals.size();
    const size_t num_proposals = normal_payment_proposals.size() + num_selfsend_proposals;
    CARROT_CHECK_AND_THROW(num_proposals >= CARROT_MIN_TX_OUTPUTS, too_few_outputs, "too few payment proposals");
    CARROT_CHECK_AND_THROW(num_proposals <= CARROT_MAX_TX_OUTPUTS, too_many_outputs, "too many payment proposals");
    CARROT_CHECK_AND_THROW(num_selfsend_proposals, too_few_outputs, "no selfsend payment proposal");

    // assert there is a max of 1 integrated address payment proposals
    size_t num_integrated = 0;
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        if (normal_payment_proposal.destination.payment_id != null_payment_id)
            ++num_integrated;
    CARROT_CHECK_AND_THROW(num_integrated <= 1,
        bad_address_type, "only one integrated address is allowed per tx output set");

    // assert anchor_norm != 0 for payments
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        CARROT_CHECK_AND_THROW(normal_payment_proposal.randomness != janus_anchor_t{},
            missing_randomness, "normal payment proposal has unset anchor_norm AKA randomness");

    // assert uniqueness of randomness for each payment
    memcmp_set<janus_anchor_t> randomnesses;
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        randomnesses.insert(normal_payment_proposal.randomness);
    const bool has_unique_randomness = randomnesses.size() == normal_payment_proposals.size();
    CARROT_CHECK_AND_THROW(has_unique_randomness,
        missing_randomness, "normal payment proposals contain duplicate anchor_norm AKA randomness");

    // for each output:  (enote proposal        ,         is ss?,  payment idx )
    std::vector<std::pair<RCTOutputEnoteProposal, std::pair<bool, size_t>>> sortable_data;
    sortable_data.reserve(num_proposals);

    // D^other_e
    std::optional<mx25519_pubkey> other_enote_ephemeral_pubkey;

    // construct normal enotes
    for (size_t i = 0; i < normal_payment_proposals.size(); ++i)
    {
        auto &output_entry = sortable_data.emplace_back();
        output_entry.second = {false, i};

        encrypted_payment_id_t encrypted_payment_id;
        get_output_proposal_normal_v1(normal_payment_proposals[i],
            tx_first_key_image,
            output_entry.first,
            encrypted_payment_id);

        // if 1 normal, and 2 self-send, set D^other_e equal to this D_e
        if (num_proposals == 2)
            other_enote_ephemeral_pubkey = output_entry.first.enote.enote_ephemeral_pubkey;

        // set pid_enc from integrated address proposal pic_enc
        const bool is_integrated = normal_payment_proposals[i].destination.payment_id != null_payment_id;
        if (is_integrated)
            encrypted_payment_id_out = encrypted_payment_id;
    }

    // in the case that there is no required pid_enc, set it to the provided dummy
    if (0 == num_integrated)
    {
        CARROT_CHECK_AND_THROW(dummy_encrypted_payment_id,
            missing_components, "missing encrypted payment ID: no integrated address nor provided dummy");
        encrypted_payment_id_out = *dummy_encrypted_payment_id;
    }

    // if 0 normal, and 2 self-send, set D^other_e equal to whichever *has* a D_e
    if (num_proposals == 2 && num_selfsend_proposals == 2)
    {
        const size_t present_ephem_pk_index = selfsend_payment_proposals.at(0).enote_ephemeral_pubkey ? 0 : 1;
        other_enote_ephemeral_pubkey = selfsend_payment_proposals.at(present_ephem_pk_index).enote_ephemeral_pubkey;
    }

    // construct selfsend enotes, preferring internal enotes over special enotes when possible
    for (size_t i = 0; i < num_selfsend_proposals; ++i)
    {
        const CarrotPaymentProposalSelfSendV1 &selfsend_payment_proposal = selfsend_payment_proposals.at(i);

        auto &output_entry = sortable_data.emplace_back();
        output_entry.second = {true, i};

        if (s_view_balance_dev != nullptr)
        {
            get_output_proposal_internal_v1(selfsend_payment_proposal,
                *s_view_balance_dev,
                tx_first_key_image,
                other_enote_ephemeral_pubkey,
                output_entry.first);
        }
        else if (k_view_dev != nullptr)
        {
            get_output_proposal_special_v1(selfsend_payment_proposal,
                *k_view_dev,
                tx_first_key_image,
                other_enote_ephemeral_pubkey,
                output_entry.first);
        }
        else // neither k_v nor s_vb device passed
        {
            CARROT_THROW(std::invalid_argument, "neither a view-balance nor view-incoming device was provided");
        }
    }

    // sort enotes by K_o
    const auto sort_output_enote_proposal = [](const auto &a, const auto &b) -> bool
        { return a.first.enote.onetime_address < b.first.enote.onetime_address; };
    std::sort(sortable_data.begin(), sortable_data.end(), sort_output_enote_proposal);

    // collect output_enote_proposals_out and payment_proposal_order_out
    output_enote_proposals_out.reserve(num_proposals);
    if (payment_proposal_order_out)
        payment_proposal_order_out->reserve(num_proposals);
    for (const auto &output_entry : sortable_data)
    {
        output_enote_proposals_out.push_back(output_entry.first);
        if (payment_proposal_order_out)
            payment_proposal_order_out->push_back(output_entry.second);
    }

    // assert uniqueness of D_e if >2-out, shared otherwise. also check D_e is not trivial
    memcmp_set<mx25519_pubkey> ephemeral_pubkeys;
    for (const RCTOutputEnoteProposal &p : output_enote_proposals_out)
    {
        const bool trivial_enote_ephemeral_pubkey = memcmp(p.enote.enote_ephemeral_pubkey.data,
            mx25519_pubkey{}.data,
            sizeof(mx25519_pubkey)) == 0;
        CARROT_CHECK_AND_THROW(!trivial_enote_ephemeral_pubkey, missing_randomness,
            "this set contains enote ephemeral pubkeys with x=0");
        ephemeral_pubkeys.insert(p.enote.enote_ephemeral_pubkey);
    }
    const bool has_unique_ephemeral_pubkeys = ephemeral_pubkeys.size() == output_enote_proposals_out.size();
    CARROT_CHECK_AND_THROW(!(num_proposals == 2 && has_unique_ephemeral_pubkeys),
        component_out_of_order, "this 2-out set needs to share their ephemeral pubkey");
    CARROT_CHECK_AND_THROW(!(num_proposals != 2 && !has_unique_ephemeral_pubkeys),
        missing_randomness, "this >2-out set contains duplicate enote ephemeral pubkeys");

    // assert uniqueness of K_o
    CARROT_CHECK_AND_THROW(tools::is_sorted_and_unique(sortable_data, sort_output_enote_proposal),
        component_out_of_order, "this set contains duplicate onetime addresses");

    // assert all K_o lie in prime order subgroup
    for (const RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals_out)
    {
        CARROT_CHECK_AND_THROW(rct::isInMainSubgroup(rct::pk2rct(output_enote_proposal.enote.onetime_address)),
            invalid_point, "this set contains an invalid onetime address");
    }

    // assert unique and non-trivial k_a
    memcmp_set<crypto::secret_key> amount_blinding_factors;
    for (const RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals_out)
    {
        CARROT_CHECK_AND_THROW(output_enote_proposal.amount_blinding_factor != crypto::null_skey,
            missing_randomness, "this set contains a trivial amount blinding factor");

        amount_blinding_factors.insert(output_enote_proposal.amount_blinding_factor);
    }
    CARROT_CHECK_AND_THROW(amount_blinding_factors.size() == num_proposals, missing_randomness,
        "this set contains duplicate amount blinding factors");
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
        CARROT_CHECK_AND_THROW(destination.payment_id == null_payment_id && !destination.is_subaddress,
            bad_address_type, "get coinbase output enotes: no integrated addresses or subaddresses allowed");
    }

    // assert anchor_norm != 0 for payments
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        CARROT_CHECK_AND_THROW(normal_payment_proposal.randomness != janus_anchor_t{},
            missing_randomness, "normal payment proposal has unset anchor_norm AKA randomness");

    // assert uniqueness of randomness for each payment
    memcmp_set<janus_anchor_t> randomnesses;
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        randomnesses.insert(normal_payment_proposal.randomness);
    const bool has_unique_randomness = randomnesses.size() == normal_payment_proposals.size();
    CARROT_CHECK_AND_THROW(has_unique_randomness,
        missing_randomness, "normal payment proposals contain duplicate anchor_norm AKA randomness");

    // construct normal enotes
    output_coinbase_enotes_out.reserve(num_proposals);
    for (size_t i = 0; i < normal_payment_proposals.size(); ++i)
    {
        get_coinbase_output_proposal_v1(normal_payment_proposals[i],
            block_index,
            output_coinbase_enotes_out.emplace_back());
    }

    // assert uniqueness and non-trivial-ness of D_e
    memcmp_set<mx25519_pubkey> ephemeral_pubkeys;
    for (const CarrotCoinbaseEnoteV1 &enote : output_coinbase_enotes_out)
    {
        const bool trivial_enote_ephemeral_pubkey = memcmp(enote.enote_ephemeral_pubkey.data,
            mx25519_pubkey{}.data,
            sizeof(mx25519_pubkey)) == 0;
        CARROT_CHECK_AND_THROW(!trivial_enote_ephemeral_pubkey,
            missing_randomness, "this set contains enote ephemeral pubkeys with x=0");
        ephemeral_pubkeys.insert(enote.enote_ephemeral_pubkey);
    }
    const bool has_unique_ephemeral_pubkeys = ephemeral_pubkeys.size() == output_coinbase_enotes_out.size();
    CARROT_CHECK_AND_THROW(has_unique_ephemeral_pubkeys,
        missing_randomness, "a coinbase enote set needs unique ephemeral pubkeys, but this set doesn't");

    // sort enotes by K_o
    const auto sort_output_enote_proposal = [](const CarrotCoinbaseEnoteV1 &a, const CarrotCoinbaseEnoteV1 &b)
        -> bool { return a.onetime_address < b.onetime_address; };
    std::sort(output_coinbase_enotes_out.begin(), output_coinbase_enotes_out.end(), sort_output_enote_proposal);

    // assert uniqueness of K_o
    CARROT_CHECK_AND_THROW(tools::is_sorted_and_unique(output_coinbase_enotes_out, sort_output_enote_proposal),
        component_out_of_order, "this set contains duplicate onetime addresses");

    // assert all K_o lie in prime order subgroup
    for (const CarrotCoinbaseEnoteV1 &output_enote : output_coinbase_enotes_out)
    {
        CARROT_CHECK_AND_THROW(rct::isInMainSubgroup(rct::pk2rct(output_enote.onetime_address)),
            invalid_point, "this set contains an invalid onetime address");
    }
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
