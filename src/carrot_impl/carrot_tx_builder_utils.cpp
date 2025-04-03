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
#include "carrot_tx_builder_utils.h"

//local headers
#include "carrot_core/output_set_finalization.h"
#include "carrot_tx_format_utils.h"
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctSigs.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void append_additional_payment_proposal_if_necessary(
    std::vector<CarrotPaymentProposalV1>& normal_payment_proposals_inout,
    std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals_inout,
    const crypto::public_key &change_address_spend_pubkey,
    const subaddress_index_extended &change_subaddr_index)
{
    struct append_additional_payment_proposal_if_necessary_visitor
    {
        void operator()(boost::blank) const {}
        void operator()(const CarrotPaymentProposalV1 &p) const { normal_proposals_inout.push_back(p); }
        void operator()(const CarrotPaymentProposalSelfSendV1 &p) const
        {
            selfsend_proposals_inout.push_back(CarrotPaymentProposalVerifiableSelfSendV1{
                .proposal = p,
                .subaddr_index = change_subaddr_index
            });
        }

        std::vector<CarrotPaymentProposalV1>& normal_proposals_inout;
        std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_proposals_inout;
        const subaddress_index_extended &change_subaddr_index;
    };

    bool have_payment_type_selfsend = false;
    for (const CarrotPaymentProposalVerifiableSelfSendV1 &selfsend_payment_proposal : selfsend_payment_proposals_inout)
    {
        if (selfsend_payment_proposal.proposal.enote_type == CarrotEnoteType::PAYMENT)
        {
            have_payment_type_selfsend = true;
            break;
        }
    }

    const auto additional_output_proposal = get_additional_output_proposal(normal_payment_proposals_inout.size(),
        selfsend_payment_proposals_inout.size(),
        /*needed_change_amount=*/0,
        have_payment_type_selfsend,
        change_address_spend_pubkey);

    additional_output_proposal.visit(append_additional_payment_proposal_if_necessary_visitor{
        normal_payment_proposals_inout,
        selfsend_payment_proposals_inout,
        change_subaddr_index
    });
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_transaction_proposal_v1(const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals_in,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals_in,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    select_inputs_func_t &&select_inputs,
    carve_fees_and_balance_func_t &&carve_fees_and_balance,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    CarrotTransactionProposalV1 &tx_proposal_out)
{
    tx_proposal_out.extra = extra;

    std::vector<CarrotPaymentProposalV1> &normal_payment_proposals
        = tx_proposal_out.normal_payment_proposals
        = normal_payment_proposals_in;
    std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals
        = tx_proposal_out.selfsend_payment_proposals
        = selfsend_payment_proposals_in;

    // add an additional payment proposal to satisfy scanning/consensus rules, if applicable
    append_additional_payment_proposal_if_necessary(normal_payment_proposals,
        selfsend_payment_proposals,
        account_spend_pubkey,
        {0, 0, AddressDeriveType::Auto}); // @TODO: let callers pass AddressDeriveType as an argument

    const size_t num_outs = normal_payment_proposals.size() + selfsend_payment_proposals.size();
    CHECK_AND_ASSERT_THROW_MES(num_outs >= CARROT_MIN_TX_OUTPUTS,
        "make_carrot_transaction_proposal_v1: too few outputs");

    // generate random X25519 ephemeral pubkeys for selfsend proposals if:
    //   a. not explicitly provided in a >2-out tx, OR
    //   b. not explicitly provided in a 2-out 2-self-send tx and the other is also missing
    const bool should_gen_selfsend_ephemeral_pubkeys = num_outs != 2 ||
        (normal_payment_proposals.empty()
            && !selfsend_payment_proposals.at(0).proposal.enote_ephemeral_pubkey
            && !selfsend_payment_proposals.at(1).proposal.enote_ephemeral_pubkey);
    if (should_gen_selfsend_ephemeral_pubkeys)
    {
        for (size_t i = 0; i < selfsend_payment_proposals.size(); ++i)
        {
            // should not provide two different D_e in a 2-out tx, so skip the second D_e in a 2-out
            const bool should_skip_generating = num_outs == 2 && i == 1;
            if (should_skip_generating)
                continue;
            CarrotPaymentProposalVerifiableSelfSendV1 &selfsend_payment_proposal = selfsend_payment_proposals[i];
            if (!selfsend_payment_proposal.proposal.enote_ephemeral_pubkey)
                selfsend_payment_proposal.proposal.enote_ephemeral_pubkey = gen_x25519_pubkey();
        }
    }

    // generate random dummy encrypted payment ID for if none of the normal payment proposals are integrated
    tx_proposal_out.dummy_encrypted_payment_id = gen_payment_id();

    // calculate size of tx.extra
    const size_t tx_extra_size = get_carrot_default_tx_extra_size(num_outs);

    // calculate the concrete fee for this transaction for each possible valid input count
    std::map<size_t, rct::xmr_amount> fee_per_input_count;
    for (size_t num_ins = 1; num_ins <= CARROT_MAX_TX_INPUTS; ++num_ins)
    {
        const size_t tx_weight = get_fcmppp_tx_weight(num_ins, num_outs, tx_extra_size);
        const rct::xmr_amount fee = tx_weight * fee_per_weight; // @TODO: check for overflow here
        fee_per_input_count.emplace(num_ins, fee);
    }

    // calculate sum of payment proposal amounts before fee carving
    boost::multiprecision::int128_t nominal_output_amount_sum = 0;
    for (const CarrotPaymentProposalV1 &normal_proposal : normal_payment_proposals)
        nominal_output_amount_sum += normal_proposal.amount;
    for (const CarrotPaymentProposalVerifiableSelfSendV1 &selfsend_proposal : selfsend_payment_proposals)
        nominal_output_amount_sum += selfsend_proposal.proposal.amount;

    // callback to select inputs given nominal output sum and fee per input count
    std::vector<CarrotSelectedInput> selected_inputs;
    select_inputs(nominal_output_amount_sum,
        fee_per_input_count,
        normal_payment_proposals.size(),
        selfsend_payment_proposals.size(),
        selected_inputs);

    // get fee given the number of selected inputs
    // note: this will fail if input selection returned a bad number of inputs
    tx_proposal_out.fee = fee_per_input_count.at(selected_inputs.size());

    // calculate input amount sum
    boost::multiprecision::int128_t input_amount_sum = 0;
    for (const CarrotSelectedInput &selected_input : selected_inputs)
        input_amount_sum += selected_input.amount;

    // callback to balance the outputs with the fee and input sum
    carve_fees_and_balance(input_amount_sum, tx_proposal_out.fee, normal_payment_proposals, selfsend_payment_proposals);

    // sanity check balance
    input_amount_sum -= tx_proposal_out.fee;
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
        input_amount_sum -= normal_payment_proposal.amount;
    for (const CarrotPaymentProposalVerifiableSelfSendV1 &selfsend_payment_proposal : selfsend_payment_proposals)
        input_amount_sum -= selfsend_payment_proposal.proposal.amount;
    CHECK_AND_ASSERT_THROW_MES(input_amount_sum == 0,
        "make_carrot_transaction_proposal_v1: post-carved transaction does not balance");

    // collect and sort key images
    tx_proposal_out.key_images_sorted.reserve(selected_inputs.size());
    for (const CarrotSelectedInput &selected_input : selected_inputs)
        tx_proposal_out.key_images_sorted.push_back(selected_input.key_image);
    std::sort(tx_proposal_out.key_images_sorted.begin(),
        tx_proposal_out.key_images_sorted.end(),
        std::greater{}); // consensus rules dictate inputs sorted in *reverse* lexicographical order since v7
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_transaction_proposal_v1_transfer_subtractable(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals_in,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    select_inputs_func_t &&select_inputs,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    const std::set<std::size_t> &subtractable_normal_payment_proposals,
    const std::set<std::size_t> &subtractable_selfsend_payment_proposals,
    CarrotTransactionProposalV1 &tx_proposal_out)
{
    std::vector<CarrotPaymentProposalVerifiableSelfSendV1> selfsend_payment_proposals = selfsend_payment_proposals_in;

    // always add implicit selfsend enote, so resultant enotes' amounts mirror given payments set close as possible 
    // note: we always do this, even if the amount ends up being 0 and we already have a selfsend. this is because if we
    //       realize later that the change output we added here has a 0 amount, and we try removing it, then the fee
    //       would go down and then the change amount *wouldn't* be 0, so it must stay. Although technically,
    //       the scenario could arise where a change in input selection changes the input sum amount and fee exactly
    //       such that we could remove the implicit change output and it happens to balance. IMO, handling this edge
    //       case isn't worth the additional code complexity, and may cause unexpected uniformity issues. The calling
    //       code might expect that transfers to N destinations always produces a transaction with N+1 outputs
    const bool add_payment_type_selfsend = normal_payment_proposals.empty() &&
        selfsend_payment_proposals.size() == 1 &&
        selfsend_payment_proposals.at(0).proposal.enote_type == CarrotEnoteType::CHANGE;

        selfsend_payment_proposals.push_back(CarrotPaymentProposalVerifiableSelfSendV1{
        .proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = account_spend_pubkey,
            .amount = 0,
            .enote_type = add_payment_type_selfsend ? CarrotEnoteType::PAYMENT : CarrotEnoteType::CHANGE
        },
        .subaddr_index = {0, 0, AddressDeriveType::Auto} // @TODO: let callers pass AddressDeriveType as an argument
    });

    // define carves fees and balance callback
    carve_fees_and_balance_func_t carve_fees_and_balance =
    [
        &subtractable_normal_payment_proposals,
        &subtractable_selfsend_payment_proposals
    ]
    (
        const boost::multiprecision::int128_t &input_sum_amount,
        const rct::xmr_amount fee,
        std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
        std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals
    )
    {
        const bool has_subbable_normal = !subtractable_normal_payment_proposals.empty();
        const bool has_subbable_selfsend = !subtractable_selfsend_payment_proposals.empty();
        const size_t num_normal = normal_payment_proposals.size();
        const size_t num_selfsend = selfsend_payment_proposals.size();

        // check subbable indices invariants
        CHECK_AND_ASSERT_THROW_MES(
            !has_subbable_normal || *subtractable_normal_payment_proposals.crbegin() < num_normal,
            "make unsigned transaction transfer subtractable: subtractable normal proposal index out of bounds");
        CHECK_AND_ASSERT_THROW_MES(
            !has_subbable_selfsend || *subtractable_selfsend_payment_proposals.crbegin() < num_selfsend,
            "make unsigned transaction transfer subtractable: subtractable selfsend proposal index out of bounds");
        CHECK_AND_ASSERT_THROW_MES(has_subbable_normal || has_subbable_selfsend,
            "make unsigned transaction transfer subtractable: no subtractable indices");

        // check selfsend proposal invariants
        CHECK_AND_ASSERT_THROW_MES(!selfsend_payment_proposals.empty(),
            "make unsigned transaction transfer subtractable: missing a selfsend proposal");
        CHECK_AND_ASSERT_THROW_MES(selfsend_payment_proposals.back().proposal.amount == 0,
            "make unsigned transaction transfer subtractable: bug: added implicit change output has non-zero amount");

        // start by setting the last selfsend amount equal to (inputs - outputs), before fee
        boost::multiprecision::int128_t implicit_change_amount = input_sum_amount;
        for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
            implicit_change_amount -= normal_payment_proposal.amount;
        for (const CarrotPaymentProposalVerifiableSelfSendV1 &selfsend_payment_proposal : selfsend_payment_proposals)
            implicit_change_amount -= selfsend_payment_proposal.proposal.amount;
        
        selfsend_payment_proposals.back().proposal.amount =
            boost::numeric_cast<rct::xmr_amount>(implicit_change_amount);

        // deduct an even fee amount from all subtractable outputs
        const size_t num_subtractble_normal = subtractable_normal_payment_proposals.size();
        const size_t num_subtractable_selfsend = subtractable_selfsend_payment_proposals.size();
        const size_t num_subtractable = num_subtractble_normal + num_subtractable_selfsend;
        const rct::xmr_amount minimum_subtraction = fee / num_subtractable; // no div by 0 since we checked subtractable
        for (size_t normal_sub_idx : subtractable_normal_payment_proposals)
        {
            CarrotPaymentProposalV1 &normal_payment_proposal = normal_payment_proposals[normal_sub_idx];
            CHECK_AND_ASSERT_THROW_MES(normal_payment_proposal.amount >= minimum_subtraction,
                "make unsigned transaction transfer subtractable: not enough funds in subtractable payment");
            normal_payment_proposal.amount -= minimum_subtraction;
        }
        for (size_t selfsend_sub_idx : subtractable_selfsend_payment_proposals)
        {
            CarrotPaymentProposalSelfSendV1 &selfsend_payment_proposal =
                selfsend_payment_proposals[selfsend_sub_idx].proposal;
            CHECK_AND_ASSERT_THROW_MES(selfsend_payment_proposal.amount >= minimum_subtraction,
                "make unsigned transaction transfer subtractable: not enough funds in subtractable payment");
            selfsend_payment_proposal.amount -= minimum_subtraction;
        }

        // deduct 1 at a time from selfsend proposals
        rct::xmr_amount fee_remainder = fee % num_subtractable;
        for (size_t selfsend_sub_idx : subtractable_selfsend_payment_proposals)
        {
            if (fee_remainder == 0)
                break;

            CarrotPaymentProposalSelfSendV1 &selfsend_payment_proposal =
                selfsend_payment_proposals[selfsend_sub_idx].proposal;
            CHECK_AND_ASSERT_THROW_MES(selfsend_payment_proposal.amount >= 1,
                "make unsigned transaction transfer subtractable: not enough funds in subtractable payment");
            selfsend_payment_proposal.amount -= 1;
            fee_remainder -= 1;
        }

        // now deduct 1 at a time from normal proposals, shuffled
        if (fee_remainder != 0)
        {
            // create vector of shuffled subtractble normal payment indices
            // note: we do this to hide the order that the normal payment proposals were described in this call, in case
            //       the recipients collude
            std::vector<size_t> shuffled_normal_subtractable(subtractable_normal_payment_proposals.cbegin(),
                subtractable_normal_payment_proposals.cend());
            std::shuffle(shuffled_normal_subtractable.begin(),
                shuffled_normal_subtractable.end(),
                crypto::random_device{});
            
            for (size_t normal_sub_idx : shuffled_normal_subtractable)
            {
                if (fee_remainder == 0)
                    break;

                CarrotPaymentProposalV1 &normal_payment_proposal = normal_payment_proposals[normal_sub_idx];
                CHECK_AND_ASSERT_THROW_MES(normal_payment_proposal.amount >= 1,
                    "make unsigned transaction transfer subtractable: not enough funds in subtractable payment");
                normal_payment_proposal.amount -= 1;
                fee_remainder -= 1;
            }
        }

        CHECK_AND_ASSERT_THROW_MES(fee_remainder == 0,
            "make unsigned transaction transfer subtractable: bug: fee remainder at end of carve function");
    }; //end carve_fees_and_balance

    // make unsigned transaction with fee carving callback
    make_carrot_transaction_proposal_v1(normal_payment_proposals,
        selfsend_payment_proposals,
        fee_per_weight,
        extra,
        std::forward<select_inputs_func_t>(select_inputs),
        std::move(carve_fees_and_balance),
        s_view_balance_dev,
        k_view_dev,
        account_spend_pubkey,
        tx_proposal_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_transaction_proposal_v1_transfer(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    select_inputs_func_t &&select_inputs,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    CarrotTransactionProposalV1 &tx_proposal_out)
{
    make_carrot_transaction_proposal_v1_transfer_subtractable(
        normal_payment_proposals,
        selfsend_payment_proposals,
        fee_per_weight,
        extra,
        std::forward<select_inputs_func_t>(select_inputs),
        s_view_balance_dev,
        k_view_dev,
        account_spend_pubkey,
        /*subtractable_normal_payment_proposals=*/{},
        /*subtractable_selfsend_payment_proposals=*/{selfsend_payment_proposals.size()},
        tx_proposal_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_transaction_proposal_v1_sweep(
    const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const rct::xmr_amount fee_per_weight,
    const std::vector<uint8_t> &extra,
    std::vector<CarrotSelectedInput> &&selected_inputs,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::public_key &account_spend_pubkey,
    CarrotTransactionProposalV1 &tx_proposal_out)
{
    // sanity check that all payment proposal amounts are 0
    for (const CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
    {
        CHECK_AND_ASSERT_THROW_MES(normal_payment_proposal.amount == 0,
            "make carrot transaction proposal v1 sweep: payment proposal amount not 0");
    }
    for (const CarrotPaymentProposalVerifiableSelfSendV1 &selfsend_payment_proposal : selfsend_payment_proposals)
    {
        CHECK_AND_ASSERT_THROW_MES(selfsend_payment_proposal.proposal.amount == 0,
            "make carrot transaction proposal v1 sweep: payment proposal amount not 0");
    }

    const bool is_selfsend_sweep = !selfsend_payment_proposals.empty();

    // define input selection callback, which is just a shuttle for `selected_inputs`
    select_inputs_func_t select_inputs = [&selected_inputs]
    (
        const boost::multiprecision::int128_t&,
        const std::map<std::size_t, rct::xmr_amount>&,
        const std::size_t,
        const std::size_t,
        std::vector<CarrotSelectedInput> &selected_inputs_out
    )
    {
        selected_inputs_out = std::move(selected_inputs);
    }; //end select_inputs

    // define carves fees and balance callback
    carve_fees_and_balance_func_t carve_fees_and_balance = [is_selfsend_sweep]
    (
        const boost::multiprecision::int128_t &input_sum_amount,
        const rct::xmr_amount fee,
        std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
        std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals
    )
    {
        // get pointers to proposal amounts and shuffle, excluding implicit selfsend
        const size_t n_outputs = normal_payment_proposals.size() + selfsend_payment_proposals.size();
        std::vector<rct::xmr_amount*> amount_ptrs;
        amount_ptrs.reserve(n_outputs);
        for (CarrotPaymentProposalV1 &normal_payment_proposal : normal_payment_proposals)
            amount_ptrs.push_back(&normal_payment_proposal.amount);
        if (is_selfsend_sweep)
            for (CarrotPaymentProposalVerifiableSelfSendV1 &selfsend_payment_proposal : selfsend_payment_proposals)
                amount_ptrs.push_back(&selfsend_payment_proposal.proposal.amount);
        std::shuffle(amount_ptrs.begin(), amount_ptrs.end(), crypto::random_device{});

        // disburse amount equally amongst modifiable amounts
        const boost::multiprecision::int128_t output_sum_amount = input_sum_amount - fee;
        const rct::xmr_amount minimum_sweep_amount =
            boost::numeric_cast<rct::xmr_amount>(output_sum_amount / amount_ptrs.size());
        const size_t num_remaining =
            boost::numeric_cast<rct::xmr_amount>(output_sum_amount % amount_ptrs.size());
        CHECK_AND_ASSERT_THROW_MES(num_remaining < amount_ptrs.size(),
            "make carrot transaction proposal v1 sweep: bug: num_remaining >= n_outputs");
        for (size_t i = 0; i < amount_ptrs.size(); ++i)
            *amount_ptrs.at(i) = minimum_sweep_amount + (i < num_remaining ? 1 : 0);
    }; //end carve_fees_and_balance

    // make unsigned transaction with sweep carving callback and selected inputs
    make_carrot_transaction_proposal_v1(normal_payment_proposals,
        selfsend_payment_proposals,
        fee_per_weight,
        extra,
        std::move(select_inputs),
        std::move(carve_fees_and_balance),
        s_view_balance_dev,
        k_view_dev,
        account_spend_pubkey,
        tx_proposal_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_signable_tx_hash_from_carrot_transaction_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    crypto::hash &signable_tx_hash_out)
{
    //! @TODO: there's a more efficient way to do this than constructing&serializing a whole cryptonote::transaction
    // HW devices will need to implement this function to sign tx proposals, and most of these devices don't have a lot of memory

    cryptonote::transaction pruned_tx;
    make_pruned_transaction_from_carrot_proposal_v1(tx_proposal,
        s_view_balance_dev,
        k_view_dev,
        pruned_tx);

    //! @TODO: better input number calculation in get_pre_mlsag_hash. possible?
    pruned_tx.rct_signatures.p.pseudoOuts.resize(pruned_tx.vin.size());

    hw::device &hwdev = hw::get_device("default");
    const rct::key signable_tx_hash_k = rct::get_pre_mlsag_hash(pruned_tx.rct_signatures, hwdev);

    signable_tx_hash_out = rct::rct2hash(signable_tx_hash_k);
}
//-------------------------------------------------------------------------------------------------------------------
void make_pruned_transaction_from_carrot_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    cryptonote::transaction &pruned_tx_out)
{
    // collect self-sends proposal cores
    std::vector<CarrotPaymentProposalSelfSendV1> selfsend_payment_proposal_cores;
    selfsend_payment_proposal_cores.reserve(tx_proposal.selfsend_payment_proposals.size());
    for (const auto &selfsend_payment_proposal : tx_proposal.selfsend_payment_proposals)
        selfsend_payment_proposal_cores.push_back(selfsend_payment_proposal.proposal);

    // derive enote proposals
    std::vector<RCTOutputEnoteProposal> output_enote_proposals;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_enote_proposals(tx_proposal.normal_payment_proposals,
        selfsend_payment_proposal_cores,
        tx_proposal.dummy_encrypted_payment_id,
        s_view_balance_dev,
        k_view_dev,
        tx_proposal.key_images_sorted.at(0),
        output_enote_proposals,
        encrypted_payment_id);

    // collect enotes
    std::vector<CarrotEnoteV1> enotes;
    enotes.reserve(output_enote_proposals.size());
    for (const RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals)
        enotes.push_back(output_enote_proposal.enote);

    // serialize tx
    pruned_tx_out = store_carrot_to_transaction_v1(enotes,
        tx_proposal.key_images_sorted,
        tx_proposal.fee,
        encrypted_payment_id);

    // add extra payload and sort
    if (!tx_proposal.extra.empty())
    {
        std::vector<std::uint8_t> sorted_extra;
        pruned_tx_out.extra.insert(pruned_tx_out.extra.end(), tx_proposal.extra.cbegin(), tx_proposal.extra.cend());
        CHECK_AND_ASSERT_THROW_MES(cryptonote::sort_tx_extra(pruned_tx_out.extra, sorted_extra),
            "make_pruned_transaction_from_carrot_proposal_v1: failed to sort ");
        pruned_tx_out.extra = std::move(sorted_extra);
    }
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
