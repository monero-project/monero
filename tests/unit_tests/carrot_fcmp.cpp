// Copyright (c) 2025, The Monero Project
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

#include "gtest/gtest.h"

#include "carrot_core/output_set_finalization.h"
#include "carrot_core/payment_proposal.h"
#include "carrot_impl/address_device_ram_borrowed.h"
#include "carrot_impl/format_utils.h"
#include "carrot_impl/input_selection.h"
#include "carrot_impl/key_image_device_composed.h"
#include "carrot_impl/tx_builder_inputs.h"
#include "carrot_impl/tx_builder_outputs.h"
#include "carrot_impl/tx_proposal_utils.h"
#include "carrot_mock_helpers.h"
#include "common/container_helpers.h"
#include "crypto/generators.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/subaddress_index.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/blockchain.h"
#include "curve_trees.h"
#include "fcmp_pp/prove.h"
#include "ringct/bulletproofs_plus.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"

using namespace carrot;

namespace
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static constexpr rct::xmr_amount MAX_AMOUNT_FCMP_PP = MONEY_SUPPLY /
(FCMP_PLUS_PLUS_MAX_INPUTS + FCMP_PLUS_PLUS_MAX_OUTPUTS + 1);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
using CarrotEnoteVariant = tools::variant<CarrotCoinbaseEnoteV1, CarrotEnoteV1>;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct CarrotOutputContextsAndKeys
{
    std::vector<CarrotEnoteVariant> enotes;
    std::vector<encrypted_payment_id_t> encrypted_payment_ids;
    std::vector<fcmp_pp::curve_trees::OutputContext> output_pairs;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static const CarrotOutputContextsAndKeys generate_random_carrot_outputs(
    const mock::mock_carrot_and_legacy_keys &keys,
    const std::size_t old_n_leaf_tuples,
    const std::size_t new_n_leaf_tuples)
{
    CarrotOutputContextsAndKeys outs;
    outs.enotes.reserve(new_n_leaf_tuples);
    outs.encrypted_payment_ids.reserve(new_n_leaf_tuples);
    outs.output_pairs.reserve(new_n_leaf_tuples);

    for (std::size_t i = 0; i < new_n_leaf_tuples; ++i)
    {
        const std::uint64_t output_id = old_n_leaf_tuples + i;
        fcmp_pp::curve_trees::OutputContext output_pair{
            .output_id = output_id
        };

        CarrotPaymentProposalV1 normal_payment_proposal{
            .destination = keys.cryptonote_address(),
            .amount = rct::randXmrAmount(MAX_AMOUNT_FCMP_PP),
            .randomness = gen_janus_anchor()
        };
        CarrotPaymentProposalVerifiableSelfSendV1 selfsend_payment_proposal{
            .proposal = CarrotPaymentProposalSelfSendV1{
                .destination_address_spend_pubkey = keys.cryptonote_address().address_spend_pubkey,
                .amount = rct::randXmrAmount(MAX_AMOUNT_FCMP_PP),
                .enote_type = i % 2 ? CarrotEnoteType::CHANGE : CarrotEnoteType::PAYMENT,
                .enote_ephemeral_pubkey = gen_x25519_pubkey()
            },
            .subaddr_index = {0, 0}
        };

        bool push_coinbase = false;
        CarrotCoinbaseEnoteV1 coinbase_enote;
        RCTOutputEnoteProposal rct_output_enote_proposal;
        encrypted_payment_id_t encrypted_payment_id;

        const unsigned int enote_derive_type = i % 7;
        switch (enote_derive_type)
        {
        case 0: // coinbase enote
            get_coinbase_output_proposal_v1(normal_payment_proposal,
                mock::gen_block_index(),
                coinbase_enote);
            push_coinbase = true;
            break;
        case 1: // normal enote main address
            get_output_proposal_normal_v1(normal_payment_proposal,
                mock::gen_key_image(),
                rct_output_enote_proposal,
                encrypted_payment_id);
            break;
        case 2: // normal enote subaddress
            normal_payment_proposal.destination = keys.subaddress({mock::gen_subaddress_index()});
            get_output_proposal_normal_v1(normal_payment_proposal,
                mock::gen_key_image(),
                rct_output_enote_proposal,
                encrypted_payment_id);
            break;
        case 3: // special enote main address
            get_output_proposal_special_v1(selfsend_payment_proposal.proposal,
                keys.k_view_incoming_dev,
                mock::gen_key_image(),
                std::nullopt,
                rct_output_enote_proposal);
            break;
        case 4: // special enote subaddress
            selfsend_payment_proposal.subaddr_index.index = mock::gen_subaddress_index();
            selfsend_payment_proposal.proposal.destination_address_spend_pubkey
                = keys.subaddress(selfsend_payment_proposal.subaddr_index).address_spend_pubkey;
            get_output_proposal_special_v1(selfsend_payment_proposal.proposal,
                keys.k_view_incoming_dev,
                mock::gen_key_image(),
                std::nullopt,
                rct_output_enote_proposal);
            break;
        case 5: // internal main address
            get_output_proposal_internal_v1(selfsend_payment_proposal.proposal,
                keys.s_view_balance_dev,
                mock::gen_key_image(),
                std::nullopt,
                rct_output_enote_proposal);
            break;
        case 6: // internal subaddress
            selfsend_payment_proposal.subaddr_index.index = mock::gen_subaddress_index();
            selfsend_payment_proposal.proposal.destination_address_spend_pubkey
                = keys.subaddress(selfsend_payment_proposal.subaddr_index).address_spend_pubkey;
            get_output_proposal_internal_v1(selfsend_payment_proposal.proposal,
                keys.s_view_balance_dev,
                mock::gen_key_image(),
                std::nullopt,
                rct_output_enote_proposal);
            break;
        }

        if (push_coinbase)
        {
            output_pair.output_pair.output_pubkey = coinbase_enote.onetime_address;
            output_pair.output_pair.commitment = rct::zeroCommitVartime(coinbase_enote.amount);
            outs.enotes.push_back(coinbase_enote);
            outs.encrypted_payment_ids.emplace_back();
        }
        else
        {
            output_pair.output_pair.output_pubkey = rct_output_enote_proposal.enote.onetime_address;
            output_pair.output_pair.commitment = rct_output_enote_proposal.enote.amount_commitment;
            outs.enotes.push_back(rct_output_enote_proposal.enote);
        }

        outs.encrypted_payment_ids.push_back(encrypted_payment_id);
        outs.output_pairs.push_back(output_pair);
    }

    return outs;
}
} //anonymous namespace
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_fcmp, receive_scan_spend_and_verify_serialized_carrot_tx)
{
    // In this test we:
    // 1. Populate a curve tree with Carrot-derived enotes to Alice
    // 2. Scan those enotes and construct a transfer-style tx to Bob
    // 3. Serialize that tx, then deserialize it
    // 4. Verify non-input consensus rules on the deserialized tx
    // 5. Verify FCMP membership in the curve tree on the deserialized tx
    // 6. Scan the deserialized tx to Bob

    mock::mock_carrot_and_legacy_keys alice;
    mock::mock_carrot_and_legacy_keys bob;
    alice.generate();
    bob.generate();

    const size_t n_inputs = crypto::rand_range<size_t>(CARROT_MIN_TX_INPUTS, FCMP_PLUS_PLUS_MAX_INPUTS);
    const size_t n_outputs = crypto::rand_range<size_t>(CARROT_MIN_TX_OUTPUTS, FCMP_PLUS_PLUS_MAX_OUTPUTS);

    const std::size_t selene_chunk_width = fcmp_pp::curve_trees::SELENE_CHUNK_WIDTH;
    const std::size_t helios_chunk_width = fcmp_pp::curve_trees::HELIOS_CHUNK_WIDTH;
    const std::size_t tree_depth = 3;
    const std::size_t n_tree_layers = tree_depth + 1;
    const size_t expected_num_selene_branch_blinds = (tree_depth + 1) / 2;
    const size_t expected_num_helios_branch_blinds = tree_depth / 2;

    LOG_PRINT_L1("Test carrot_impl.receive_scan_spend_and_verify_serialized_carrot_tx with selene chunk width "
        << selene_chunk_width << ", helios chunk width " << helios_chunk_width << ", tree depth " << tree_depth
        << ", number of inputs " << n_inputs << ", number of outputs " << n_outputs);

    // Tree params
    uint64_t min_leaves_needed_for_tree_depth = 0;
    const auto curve_trees = test::init_curve_trees_test(selene_chunk_width,
        helios_chunk_width,
        tree_depth,
        min_leaves_needed_for_tree_depth);

    // Generate enotes...
    LOG_PRINT_L1("Generating carrot-derived enotes to Alice");
    const auto new_outputs = generate_random_carrot_outputs(alice,
        0,
        min_leaves_needed_for_tree_depth
      );
    ASSERT_GT(min_leaves_needed_for_tree_depth, n_inputs);

    // generate output ids to use as inputs...
    std::set<size_t> picked_output_ids_set;
    while (picked_output_ids_set.size() < n_inputs)
        picked_output_ids_set.insert(crypto::rand_idx(min_leaves_needed_for_tree_depth));
    std::vector<size_t> picked_output_ids(picked_output_ids_set.cbegin(), picked_output_ids_set.cend());
    std::shuffle(picked_output_ids.begin(), picked_output_ids.end(), crypto::random_device{});

    // scan inputs and make key images and opening hints...
    //                                a                  z       C_a           K_o                opening hint          output id
    using input_info_t = std::tuple<rct::xmr_amount, rct::key, rct::key, crypto::public_key, OutputOpeningHintVariant, std::uint64_t>;
    LOG_PRINT_L1("Alice scanning inputs");
    std::unordered_map<crypto::key_image, input_info_t> input_info_by_ki;
    rct::xmr_amount input_amount_sum = 0;
    for (const size_t picked_output_id : picked_output_ids)
    {
        // find index into new_outputs based on picked_output_id
        size_t new_outputs_idx;
        for (new_outputs_idx = 0; new_outputs_idx < new_outputs.output_pairs.size(); ++new_outputs_idx)
        {
            if (new_outputs.output_pairs.at(new_outputs_idx).output_id == picked_output_id)
                break;
        }
        ASSERT_LT(new_outputs_idx, new_outputs.enotes.size());

        // compile information about this enote
        const CarrotEnoteVariant &enote_v = new_outputs.enotes.at(new_outputs_idx);
        OutputOpeningHintVariant opening_hint;
        std::vector<mock::mock_scan_result_t> scan_results;
        if (enote_v.is_type<CarrotEnoteV1>())
        {
            const CarrotEnoteV1 &enote = enote_v.unwrap<CarrotEnoteV1>();
            const encrypted_payment_id_t encrypted_payment_id = new_outputs.encrypted_payment_ids.at(new_outputs_idx);
            mock::mock_scan_enote_set({enote},
                encrypted_payment_id,
                alice,
                scan_results);
            ASSERT_EQ(1, scan_results.size());
            const mock::mock_scan_result_t &scan_result = scan_results.front();
            const auto subaddr_it = alice.subaddress_map.find(scan_result.address_spend_pubkey);
            ASSERT_NE(alice.subaddress_map.cend(), subaddr_it);
            opening_hint = CarrotOutputOpeningHintV1{
                .source_enote = enote,
                .encrypted_payment_id = encrypted_payment_id,
                .subaddr_index = subaddr_it->second
            };
        }
        else // is coinbase
        {
            const CarrotCoinbaseEnoteV1 &enote = enote_v.unwrap<CarrotCoinbaseEnoteV1>();
            mock::mock_scan_coinbase_enote_set({enote},
                alice,
                scan_results);
            ASSERT_EQ(1, scan_results.size());
            const mock::mock_scan_result_t &scan_result = scan_results.front();
            ASSERT_EQ(alice.cryptonote_address().address_spend_pubkey, scan_result.address_spend_pubkey);
            opening_hint = CarrotCoinbaseOutputOpeningHintV1{
                .source_enote = enote,
                .derive_type = AddressDeriveType::Carrot
            };
        }
        ASSERT_EQ(1, scan_results.size());
        const mock::mock_scan_result_t &scan_result = scan_results.front();
        const fcmp_pp::curve_trees::OutputPair &output_pair = new_outputs.output_pairs.at(new_outputs_idx).output_pair;
        const crypto::key_image ki = alice.derive_key_image(scan_result.address_spend_pubkey,
            scan_result.sender_extension_g,
            scan_result.sender_extension_t,
            output_pair.output_pubkey);

        ASSERT_EQ(0, input_info_by_ki.count(ki));

        input_info_by_ki[ki] = {scan_result.amount,
            rct::sk2rct(scan_result.amount_blinding_factor),
            output_pair.commitment,
            output_pair.output_pubkey,
            opening_hint,
            new_outputs.output_pairs.at(new_outputs_idx).output_id};
        input_amount_sum += scan_result.amount;
    }

    // generate n_outputs-1 payment proposals to bob ...
    LOG_PRINT_L1("Generating payment proposals to Bob");
    rct::xmr_amount output_amount_remaining = rct::randXmrAmount(input_amount_sum);
    std::vector<CarrotPaymentProposalV1> bob_payment_proposals;
    for (size_t i = 0; i < n_outputs - 1; ++i)
    {
        const bool use_subaddress = i % 2 == 1;
        const CarrotDestinationV1 addr = use_subaddress ?
            bob.subaddress({mock::gen_subaddress_index()}) :
            bob.cryptonote_address();
        const rct::xmr_amount amount = rct::randXmrAmount(output_amount_remaining);
        bob_payment_proposals.push_back(CarrotPaymentProposalV1{
            .destination = addr,
            .amount = amount,
            .randomness = gen_janus_anchor()
        });
        output_amount_remaining -= amount;
    }

    // make a transfer-type tx proposal
    // @TODO: this can fail sporadically if fee exceeds remaining funds
    LOG_PRINT_L1("Creating transaction proposal");
    const rct::xmr_amount fee_per_weight = 1;
    CarrotTransactionProposalV1 tx_proposal;
    make_carrot_transaction_proposal_v1_transfer(bob_payment_proposals,
        /*selfsend_payment_proposals=*/{},
        fee_per_weight,
        /*extra=*/{},
        [&input_info_by_ki]
        (
            const boost::multiprecision::int128_t&,
            const std::map<std::size_t, rct::xmr_amount>&,
            const std::size_t,
            const std::size_t,
            std::vector<CarrotSelectedInput>& key_images_out)
        {
            key_images_out.clear();
            key_images_out.reserve(input_info_by_ki.size());
            for (const auto &info : input_info_by_ki)
            {
                key_images_out.push_back(CarrotSelectedInput{
                    .amount = std::get<0>(info.second),
                    .input = std::get<4>(info.second)
                });
            }
        },
        alice.carrot_account_spend_pubkey,
        {{0, 0}, AddressDeriveType::Carrot},
        {},
        {},
        tx_proposal);

    ASSERT_EQ(n_outputs, tx_proposal.normal_payment_proposals.size() + tx_proposal.selfsend_payment_proposals.size());

    // collect core selfsend proposals
    std::vector<CarrotPaymentProposalSelfSendV1> selfsend_payment_proposal_cores;
    for (const CarrotPaymentProposalVerifiableSelfSendV1 &selfsend_payment_proposal : tx_proposal.selfsend_payment_proposals)
        selfsend_payment_proposal_cores.push_back(selfsend_payment_proposal.proposal);

    // derive input key images
    std::vector<crypto::key_image> sorted_input_key_images;
    carrot::get_sorted_input_key_images_from_proposal_v1(tx_proposal,
        alice.carrot_key_image_dev,
        sorted_input_key_images);

    // derive output enote set
    LOG_PRINT_L1("Deriving enotes");
    std::vector<RCTOutputEnoteProposal> output_enote_proposals;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_enote_proposals(tx_proposal.normal_payment_proposals,
        selfsend_payment_proposal_cores,
        tx_proposal.dummy_encrypted_payment_id,
        &alice.s_view_balance_dev,
        &alice.k_view_incoming_dev,
        sorted_input_key_images.at(0),
        output_enote_proposals,
        encrypted_payment_id);

    // Collect balance info and enotes
    std::vector<crypto::public_key> input_onetime_addresses;
    std::vector<rct::key> input_amount_commitments;
    std::vector<rct::key> input_amount_blinding_factors;
    std::vector<rct::xmr_amount> output_amounts;
    std::vector<rct::key> output_amount_blinding_factors;
    std::vector<CarrotEnoteV1> output_enotes;
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const input_info_t &input_info = input_info_by_ki.at(sorted_input_key_images.at(i));
        input_onetime_addresses.push_back(std::get<3>(input_info));
        input_amount_commitments.push_back(std::get<2>(input_info));
        input_amount_blinding_factors.push_back(std::get<1>(input_info));
    }
    for (const RCTOutputEnoteProposal &output_enote_proposal : output_enote_proposals)
    {
        output_amounts.push_back(output_enote_proposal.amount);
        output_amount_blinding_factors.push_back(rct::sk2rct(output_enote_proposal.amount_blinding_factor));
        output_enotes.push_back(output_enote_proposal.enote);
    }

    // make pruned tx
    LOG_PRINT_L1("Storing carrot to transaction");
    cryptonote::transaction tx = store_carrot_to_transaction_v1(output_enotes,
        sorted_input_key_images,
        tx_proposal.fee,
        encrypted_payment_id);

    ASSERT_EQ(2, tx.version);
    ASSERT_EQ(0, tx.unlock_time);
    ASSERT_EQ(n_inputs, tx.vin.size());
    ASSERT_EQ(n_outputs, tx.vout.size());
    ASSERT_EQ(n_outputs, tx.rct_signatures.outPk.size());

    // Generate bulletproof+
    LOG_PRINT_L1("Generating Bulletproof+");
    rct::BulletproofPlus bpp = rct::bulletproof_plus_PROVE(output_amounts, output_amount_blinding_factors);
    ASSERT_EQ(n_outputs, bpp.V.size());

    // expand tx and calculate signable tx hash
    LOG_PRINT_L1("Calculating signable tx hash");
    hw::device &hwdev = hw::get_device("default");
    ASSERT_TRUE(cryptonote::expand_transaction_1(tx, /*base_only=*/true));
    const crypto::hash tx_prefix_hash = cryptonote::get_transaction_prefix_hash(tx);
    tx.rct_signatures.message = rct::hash2rct(tx_prefix_hash);
    tx.rct_signatures.p.pseudoOuts.resize(n_inputs); // @TODO: make this not necessary to call get_mlsag_hash
    const crypto::hash signable_tx_hash = rct::rct2hash(rct::get_pre_mlsag_hash(tx.rct_signatures, hwdev));

    // rerandomize inputs
    LOG_PRINT_L1("Making rerandomized inputs");
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable(input_onetime_addresses,
        input_amount_commitments,
        input_amount_blinding_factors,
        output_amount_blinding_factors,
        rerandomized_outputs);

    // Make SA/L proofs
    LOG_PRINT_L1("Generating FCMP++ SA/L proofs");
    std::vector<crypto::key_image> actual_key_images;
    std::vector<fcmp_pp::FcmpPpSalProof> sal_proofs;
    for (size_t i = 0; i < n_inputs; ++i)
    {
        make_sal_proof_any_to_carrot_v1(signable_tx_hash,
            rerandomized_outputs.at(i),
            std::get<4>(input_info_by_ki.at(sorted_input_key_images.at(i))),
            alice.k_prove_spend,
            alice.k_generate_image,
            alice.s_view_balance_dev,
            alice.k_view_incoming_dev,
            alice.s_generate_address_dev,
            tools::add_element(sal_proofs),
            tools::add_element(actual_key_images));
    }

    // Init tree in memory
    LOG_PRINT_L1("Initializing tree with " << min_leaves_needed_for_tree_depth << " leaves");
    CurveTreesGlobalTree global_tree(*curve_trees);
    ASSERT_TRUE(global_tree.grow_tree(0, min_leaves_needed_for_tree_depth, new_outputs.output_pairs));
    LOG_PRINT_L1("Finished initializing tree with " << min_leaves_needed_for_tree_depth << " leaves");

    // Make FCMP paths
    LOG_PRINT_L1("Calculating FCMP paths");
    std::vector<fcmp_pp::ProofInput> fcmp_proof_inputs(n_inputs);
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const size_t leaf_idx = std::get<5>(input_info_by_ki.at(sorted_input_key_images.at(i)));
        const auto path = global_tree.get_path_at_leaf_idx(leaf_idx);
        const std::size_t path_leaf_idx = leaf_idx % curve_trees->m_c1_width;

        const fcmp_pp::curve_trees::OutputPair output_pair = {rct::rct2pk(path.leaves[path_leaf_idx].O),
            path.leaves[path_leaf_idx].C};
        const auto output_tuple = fcmp_pp::curve_trees::output_to_tuple(output_pair);

        const auto path_for_proof = curve_trees->path_for_proof(path, output_tuple);

        const auto helios_scalar_chunks = fcmp_pp::tower_cycle::scalar_chunks_to_chunk_vector<fcmp_pp::HeliosT>(
            path_for_proof.c2_scalar_chunks);
        const auto selene_scalar_chunks = fcmp_pp::tower_cycle::scalar_chunks_to_chunk_vector<fcmp_pp::SeleneT>(
            path_for_proof.c1_scalar_chunks);

        const auto path_rust = fcmp_pp::path_new({path_for_proof.leaves.data(), path_for_proof.leaves.size()},
            path_for_proof.output_idx,
            {helios_scalar_chunks.data(), helios_scalar_chunks.size()},
            {selene_scalar_chunks.data(), selene_scalar_chunks.size()});

        fcmp_proof_inputs[i].path = path_rust;
    }

    // make FCMP blinds
    LOG_PRINT_L1("Calculating branch and output blinds");
    for (size_t i = 0; i < n_inputs; ++i)
    {
        fcmp_pp::ProofInput &proof_input = fcmp_proof_inputs[i];
        const FcmpRerandomizedOutputCompressed &rerandomized_output = rerandomized_outputs.at(i);

        // calculate individual blinds
        uint8_t *blinded_o_blind = fcmp_pp::blind_o_blind(fcmp_pp::o_blind(rerandomized_output));
        uint8_t *blinded_i_blind = fcmp_pp::blind_i_blind(fcmp_pp::i_blind(rerandomized_output));
        uint8_t *blinded_i_blind_blind = fcmp_pp::blind_i_blind_blind(fcmp_pp::i_blind_blind(rerandomized_output));
        uint8_t *blinded_c_blind = fcmp_pp::blind_c_blind(fcmp_pp::c_blind(rerandomized_output));

        // make output blinds
        proof_input.output_blinds = fcmp_pp::output_blinds_new(
            blinded_o_blind, blinded_i_blind, blinded_i_blind_blind, blinded_c_blind);

        // generate selene blinds
        proof_input.selene_branch_blinds.reserve(expected_num_selene_branch_blinds);
        for (size_t j = 0; j < expected_num_selene_branch_blinds; ++j)
            proof_input.selene_branch_blinds.push_back(fcmp_pp::SeleneBranchBlindGen());

        // generate helios blinds
        proof_input.helios_branch_blinds.reserve(expected_num_helios_branch_blinds);
        for (size_t j = 0; j < expected_num_helios_branch_blinds; ++j)
            proof_input.helios_branch_blinds.push_back(fcmp_pp::HeliosBranchBlindGen());

        // dealloc individual blinds
        free(blinded_o_blind);
        free(blinded_i_blind);
        free(blinded_i_blind_blind);
        free(blinded_c_blind);
    }

    // Make FCMP membership proof
    LOG_PRINT_L1("Generating FCMP++ membership proofs");
    std::vector<uint8_t*> fcmp_proof_inputs_rust;
    for (size_t i = 0; i < n_inputs; ++i)
    {
        fcmp_pp::ProofInput &proof_input = fcmp_proof_inputs.at(i);
        fcmp_proof_inputs_rust.push_back(fcmp_pp::fcmp_prove_input_new(
            rerandomized_outputs.at(i),
            proof_input.path,
            proof_input.output_blinds,
            proof_input.selene_branch_blinds,
            proof_input.helios_branch_blinds));
        free(proof_input.path);
        free(proof_input.output_blinds);
    }
    const fcmp_pp::FcmpMembershipProof membership_proof = fcmp_pp::prove_membership(fcmp_proof_inputs_rust,
        n_tree_layers);

    // Dealloc FCMP proof inputs
    for (uint8_t *proof_input : fcmp_proof_inputs_rust)
      free(proof_input);

    // Attach rctSigPrunable to tx
    LOG_PRINT_L1("Storing rctSig prunable");
    const std::uint64_t fcmp_block_reference_index = mock::gen_block_index();
    tx.rct_signatures.p = store_fcmp_proofs_to_rct_prunable_v1(std::move(bpp),
        rerandomized_outputs,
        sal_proofs,
        membership_proof,
        fcmp_block_reference_index,
        n_tree_layers);
    tx.pruned = false;

    // Serialize tx to bytes
    LOG_PRINT_L1("Serializing & deserializing transaction");
    const cryptonote::blobdata tx_blob = cryptonote::tx_to_blob(tx);

    // Deserialize tx
    cryptonote::transaction deserialized_tx;
    ASSERT_TRUE(cryptonote::parse_and_validate_tx_from_blob(tx_blob, deserialized_tx));

    // Expand tx
    auto tree_root = global_tree.get_tree_root();
    const crypto::hash tx_prefix_hash_2 = cryptonote::get_transaction_prefix_hash(deserialized_tx);
    ASSERT_TRUE(cryptonote::Blockchain::expand_transaction_2(deserialized_tx, tx_prefix_hash_2, {}, tree_root));

    // Verify non-input consensus rules on tx
    LOG_PRINT_L1("Verifying non-input consensus rules");
    cryptonote::tx_verification_context tvc{};
    ASSERT_TRUE(cryptonote::ver_non_input_consensus(deserialized_tx, tvc, HF_VERSION_FCMP_PLUS_PLUS));
    ASSERT_FALSE(tvc.m_verifivation_failed);
    ASSERT_FALSE(tvc.m_verifivation_impossible);
    ASSERT_FALSE(tvc.m_added_to_pool);
    ASSERT_FALSE(tvc.m_low_mixin);
    ASSERT_FALSE(tvc.m_double_spend);
    ASSERT_FALSE(tvc.m_invalid_input);
    ASSERT_FALSE(tvc.m_invalid_output);
    ASSERT_FALSE(tvc.m_too_big);
    ASSERT_FALSE(tvc.m_overspend);
    ASSERT_FALSE(tvc.m_fee_too_low);
    ASSERT_FALSE(tvc.m_too_few_outputs);
    ASSERT_FALSE(tvc.m_tx_extra_too_big);
    ASSERT_FALSE(tvc.m_nonzero_unlock_time);

    // Recalculate signable tx hash from deserialized tx and check
    const crypto::hash signable_tx_hash_2 = rct::rct2hash(rct::get_pre_mlsag_hash(deserialized_tx.rct_signatures, hwdev));
    ASSERT_EQ(signable_tx_hash, signable_tx_hash_2);

    // Pre-verify SAL proofs
    LOG_PRINT_L1("Verify SA/L proofs");
    ASSERT_EQ(deserialized_tx.vin.size(), n_inputs);
    ASSERT_EQ(deserialized_tx.vin.size(), deserialized_tx.rct_signatures.p.fcmp_ver_helper_data.key_images.size());
    ASSERT_EQ(deserialized_tx.vin.size(), deserialized_tx.rct_signatures.p.pseudoOuts.size());
    ASSERT_GT(deserialized_tx.rct_signatures.p.fcmp_pp.size(), (3*32 + FCMP_PP_SAL_PROOF_SIZE_V1) * n_inputs);
    for (size_t i = 0; i < n_inputs; ++i)
    {
        const uint8_t * const pbytes = deserialized_tx.rct_signatures.p.fcmp_pp.data() +
            (3*32 + FCMP_PP_SAL_PROOF_SIZE_V1) * i;
        FcmpInputCompressed input;
        fcmp_pp::FcmpPpSalProof sal_proof(FCMP_PP_SAL_PROOF_SIZE_V1);
        memcpy(&input, pbytes, 3*32);
        memcpy(&sal_proof[0], pbytes + 3*32, FCMP_PP_SAL_PROOF_SIZE_V1);
        memcpy(input.C_tilde, deserialized_tx.rct_signatures.p.pseudoOuts.at(i).bytes, 32);
        const crypto::key_image &ki = deserialized_tx.rct_signatures.p.fcmp_ver_helper_data.key_images.at(i);
        ASSERT_TRUE(fcmp_pp::verify_sal(signable_tx_hash_2, input, ki, sal_proof));
    }

    // Verify all RingCT non-semantics
    LOG_PRINT_L1("Verify RingCT non-semantics consensus rules");
    ASSERT_TRUE(rct::verRctNonSemanticsSimple(deserialized_tx.rct_signatures));
    free(tree_root);

    // Load carrot from tx
    LOG_PRINT_L1("Parsing carrot info from deserialized transaction");
    std::vector<CarrotEnoteV1> parsed_enotes;
    std::vector<crypto::key_image> parsed_key_images;
    rct::xmr_amount parsed_fee;
    std::optional<encrypted_payment_id_t> parsed_encrypted_payment_id;
    ASSERT_TRUE(try_load_carrot_from_transaction_v1(deserialized_tx,
        parsed_enotes,
        parsed_key_images,
        parsed_fee,
        parsed_encrypted_payment_id));

    // Bob scan
    LOG_PRINT_L1("Bob scanning");
    std::vector<mock::mock_scan_result_t> bob_scan_results;
    mock::mock_scan_enote_set(parsed_enotes,
        parsed_encrypted_payment_id.value_or(encrypted_payment_id_t{}),
        bob,
        bob_scan_results);
    ASSERT_EQ(bob_payment_proposals.size(), bob_scan_results.size());

    // Compare bob scan results to bob payment proposals
    std::unordered_set<size_t> matched_scan_results;
    for (size_t i = 0; i < bob_payment_proposals.size(); ++i)
    {
        bool matched = false;
        for (size_t j = 0; j < bob_scan_results.size(); ++j)
        {
            if (matched_scan_results.count(j))
                continue;
            else if (compare_scan_result(bob_scan_results.at(j),
                bob_payment_proposals.at(i)))
            {
                matched = true;
                matched_scan_results.insert(j);
                break;
            }
        }
        ASSERT_TRUE(matched);
    }
}
//----------------------------------------------------------------------------------------------------------------------
