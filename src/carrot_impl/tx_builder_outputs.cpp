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
#include "tx_builder_outputs.h"

//local headers
#include "carrot_core/enote_utils.h"
#include "carrot_core/exceptions.h"
#include "carrot_core/output_set_finalization.h"
#include "common/container_helpers.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "format_utils.h"
#include "ringct/rctSigs.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl.tbo"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
void get_sorted_input_key_images_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const key_image_device &key_image_dev,
    std::vector<crypto::key_image> &sorted_key_images_out,
    std::vector<std::size_t> *order_out)
{
    const std::size_t n_inputs = tx_proposal.input_proposals.size();
    sorted_key_images_out.clear();
    sorted_key_images_out.reserve(n_inputs);
    if (order_out)
    {
        order_out->clear();
        order_out->reserve(n_inputs);
    }

    // derive key images
    std::vector<std::pair<crypto::key_image, std::size_t>> sortable_data;
    sortable_data.reserve(tx_proposal.input_proposals.size());
    for (std::size_t i = 0; i < n_inputs; ++i)
        sortable_data.emplace_back(key_image_dev.derive_key_image(tx_proposal.input_proposals.at(i)), i);

    // sort key images
    std::sort(sortable_data.begin(), sortable_data.end(), std::greater{});

    // collect result
    for (const auto &p : sortable_data)
    {
        sorted_key_images_out.push_back(p.first);
        if (order_out)
            order_out->push_back(p.second);
    }
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_enote_proposals_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::key_image &tx_first_key_image,
    std::vector<RCTOutputEnoteProposal> &output_enote_proposals_out,
    encrypted_payment_id_t &encrypted_payment_id_out,
    std::vector<std::pair<bool, std::size_t>> *payment_proposal_order_out)
{
    // collect self-sends proposal cores
    std::vector<CarrotPaymentProposalSelfSendV1> selfsend_payment_proposal_cores;
    selfsend_payment_proposal_cores.reserve(tx_proposal.selfsend_payment_proposals.size());
    for (const auto &selfsend_payment_proposal : tx_proposal.selfsend_payment_proposals)
        selfsend_payment_proposal_cores.push_back(selfsend_payment_proposal.proposal);

    // derive enote proposals
    get_output_enote_proposals(tx_proposal.normal_payment_proposals,
        selfsend_payment_proposal_cores,
        tx_proposal.dummy_encrypted_payment_id,
        s_view_balance_dev,
        k_view_dev,
        tx_first_key_image,
        output_enote_proposals_out,
        encrypted_payment_id_out,
        payment_proposal_order_out);
}
//-------------------------------------------------------------------------------------------------------------------
void get_output_enote_proposals_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const key_image_device &key_image_dev,
    std::vector<RCTOutputEnoteProposal> &output_enote_proposals_out,
    encrypted_payment_id_t &encrypted_payment_id_out,
    std::vector<std::pair<bool, std::size_t>> *payment_proposal_order_out)
{
    // derive input key images
    std::vector<crypto::key_image> sorted_input_key_images;
    get_sorted_input_key_images_from_proposal_v1(tx_proposal, key_image_dev, sorted_input_key_images);
    CARROT_CHECK_AND_THROW(!sorted_input_key_images.empty(), too_few_inputs, "No inputs in proposal");

    get_output_enote_proposals_from_proposal_v1(tx_proposal,
        s_view_balance_dev,
        k_view_dev,
        sorted_input_key_images.at(0),
        output_enote_proposals_out,
        encrypted_payment_id_out,
        payment_proposal_order_out);
}
//-------------------------------------------------------------------------------------------------------------------
void get_sender_receiver_secrets_from_proposal_v1(const std::vector<CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const crypto::key_image &tx_first_key_image,
    std::vector<crypto::secret_key> &s_sender_receiver_unctx_out,
    std::vector<std::pair<bool, std::size_t>> &payment_proposal_order_out)
{
    s_sender_receiver_unctx_out.clear();

    const std::size_t n_outputs = normal_payment_proposals.size() + selfsend_payment_proposals.size();
    s_sender_receiver_unctx_out.reserve(n_outputs);

    // collect self-sends proposal cores
    std::vector<CarrotPaymentProposalSelfSendV1> selfsend_payment_proposal_cores;
    selfsend_payment_proposal_cores.reserve(selfsend_payment_proposals.size());
    for (const auto &selfsend_payment_proposal : selfsend_payment_proposals)
        selfsend_payment_proposal_cores.push_back(selfsend_payment_proposal.proposal);

    // derive enote proposals
    std::vector<RCTOutputEnoteProposal> output_enote_proposals_out;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_enote_proposals(normal_payment_proposals,
        selfsend_payment_proposal_cores,
        encrypted_payment_id_t{},
        s_view_balance_dev,
        k_view_dev,
        tx_first_key_image,
        output_enote_proposals_out,
        encrypted_payment_id,
        &payment_proposal_order_out);

    // special case: 2-out, 2-selfsend tx
    if (n_outputs == 2 && selfsend_payment_proposals.size() == 2)
    {
        s_sender_receiver_unctx_out.push_back(rct::rct2sk(rct::identity())); //! @TODO
        return;
    }

    // derive s_sr in finalized output order
    for (const std::pair<bool, std::size_t> &payment_proposal_idx : payment_proposal_order_out)
    {
        const bool is_selfsend = payment_proposal_idx.first;
        if (is_selfsend)
        {
            if (n_outputs == 2)
                continue;
            s_sender_receiver_unctx_out.push_back(rct::rct2sk(rct::identity())); //! @TODO
        }
        else
        {
            const auto &normal_payment_proposal = normal_payment_proposals.at(payment_proposal_idx.second);
            // d_e = H_n(anchor_norm, input_context, K^j_s, pid))
            const crypto::secret_key enote_ephemeral_privkey = get_enote_ephemeral_privkey(normal_payment_proposal,
                make_carrot_input_context(tx_first_key_image));
            // s_sr = d_e ConvertPointE(K^j_v)
            mx25519_pubkey s_sender_receiver_unctx;
            make_carrot_uncontextualized_shared_key_sender(enote_ephemeral_privkey,
                normal_payment_proposal.destination.address_view_pubkey, s_sender_receiver_unctx);
            crypto::secret_key &s_sender_receiver_unctx_sk = s_sender_receiver_unctx_out.emplace_back();
            memcpy(&s_sender_receiver_unctx_sk, &s_sender_receiver_unctx, sizeof(s_sender_receiver_unctx_sk));
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
void make_signable_tx_hash_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const std::vector<crypto::key_image> &sorted_input_key_images,
    crypto::hash &signable_tx_hash_out)
{
    //! @TODO: there's a more efficient way to do this than constructing&serializing a whole cryptonote::transaction
    // HW devices will need to implement this function to sign tx proposals, and most of these devices don't have a lot of memory

    cryptonote::transaction pruned_tx;
    make_pruned_transaction_from_proposal_v1(tx_proposal,
        s_view_balance_dev,
        k_view_dev,
        sorted_input_key_images,
        pruned_tx);

    signable_tx_hash_out = calculate_signable_fcmp_pp_transaction_hash(pruned_tx);
}
//-------------------------------------------------------------------------------------------------------------------
void make_signable_tx_hash_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const key_image_device &key_image_dev,
    crypto::hash &signable_tx_hash_out)
{
    // derive input key images
    std::vector<crypto::key_image> sorted_input_key_images;
    get_sorted_input_key_images_from_proposal_v1(tx_proposal, key_image_dev, sorted_input_key_images);
    CARROT_CHECK_AND_THROW(!sorted_input_key_images.empty(), too_few_inputs, "No inputs in proposal");

    make_signable_tx_hash_from_proposal_v1(tx_proposal,
        s_view_balance_dev,
        k_view_dev,
        sorted_input_key_images,
        signable_tx_hash_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_pruned_transaction_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const std::vector<crypto::key_image> &sorted_input_key_images,
    cryptonote::transaction &pruned_tx_out)
{
    const std::size_t n_inputs = tx_proposal.input_proposals.size();
    CARROT_CHECK_AND_THROW(n_inputs, too_few_inputs, "No inputs in proposal");
    CARROT_CHECK_AND_THROW(sorted_input_key_images.size() == n_inputs,
        too_few_inputs, "Mismatching size of transaction input proposal and passed input key images");

    // derive enote proposals
    std::vector<RCTOutputEnoteProposal> output_enote_proposals;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_enote_proposals_from_proposal_v1(tx_proposal,
        s_view_balance_dev,
        k_view_dev,
        sorted_input_key_images.at(0),
        output_enote_proposals,
        encrypted_payment_id);

    // collect enotes
    std::vector<CarrotEnoteV1> enotes;
    enotes.reserve(output_enote_proposals.size());
    for (const RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals)
        enotes.push_back(output_enote_proposal.enote);

    // serialize tx
    pruned_tx_out = store_carrot_to_transaction_v1(enotes,
        sorted_input_key_images,
        tx_proposal.fee,
        encrypted_payment_id);

    // add extra payload and sort
    if (!tx_proposal.extra.empty())
    {
        std::vector<std::uint8_t> sorted_extra;
        pruned_tx_out.extra.insert(pruned_tx_out.extra.end(), tx_proposal.extra.cbegin(), tx_proposal.extra.cend());
        CHECK_AND_ASSERT_THROW_MES(cryptonote::sort_tx_extra(pruned_tx_out.extra, sorted_extra),
            "make_pruned_transaction_from_proposal_v1: failed to sort tx extra");
        pruned_tx_out.extra = std::move(sorted_extra);
    }
}
//-------------------------------------------------------------------------------------------------------------------
void make_pruned_transaction_from_proposal_v1(const CarrotTransactionProposalV1 &tx_proposal,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_dev,
    const key_image_device &key_image_dev,
    cryptonote::transaction &pruned_tx_out)
{
    // derive input key images
    std::vector<crypto::key_image> sorted_input_key_images;
    get_sorted_input_key_images_from_proposal_v1(tx_proposal, key_image_dev, sorted_input_key_images);

    make_pruned_transaction_from_proposal_v1(tx_proposal,
        s_view_balance_dev,
        k_view_dev,
        sorted_input_key_images,
        pruned_tx_out);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
