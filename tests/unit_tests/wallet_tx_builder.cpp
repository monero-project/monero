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

#include "carrot_core/config.h"
#include "carrot_impl/carrot_tx_builder_utils.h"
#include "carrot_mock_helpers.h"
#include "common/container_helpers.h"
#include "cryptonote_core/blockchain.h"
#include "cryptonote_core/tx_verification_utils.h"
#include "fake_pruned_blockchain.h"
#include "fcmp_pp/prove.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"
#include "tx_construction_helpers.h"
#include "wallet/tx_builder.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "unit_tests.wallet_scanning"

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static tools::wallet2::transfer_details gen_transfer_details()
{
    return tools::wallet2::transfer_details{
        .m_block_height = crypto::rand_idx<uint64_t>(CRYPTONOTE_MAX_BLOCK_NUMBER),
        .m_tx = {},
        .m_txid = crypto::rand<crypto::hash>(),
        .m_internal_output_index = crypto::rand_idx<uint64_t>(carrot::CARROT_MAX_TX_OUTPUTS),
        .m_global_output_index = crypto::rand_idx<uint64_t>(CRYPTONOTE_MAX_BLOCK_NUMBER * 1000ull),
        .m_spent = false,
        .m_frozen = false,
        .m_spent_height = 0,
        .m_key_image = crypto::key_image{rct::rct2pk(rct::pkGen())},
        .m_mask = rct::skGen(),
        .m_amount = crypto::rand_range<rct::xmr_amount>(0, COIN), // [0, 1] XMR i.e. [0, 1e12] pXMR
        .m_rct = true,
        .m_key_image_known = true,
        .m_key_image_request = false,
        .m_pk_index = 1,
        .m_subaddr_index = {},
        .m_key_image_partial = false,
        .m_multisig_k = {},
        .m_multisig_info = {},
        .m_uses = {},
    };
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool compare_transfer_to_selected_input(const tools::wallet2::transfer_details &td,
    const carrot::CarrotSelectedInput &input)
{
    return td.m_amount == input.amount && td.m_key_image == input.key_image;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(wallet_tx_builder, input_selection_basic)
{
    std::map<std::size_t, rct::xmr_amount> fee_by_input_count;
    for (size_t i = carrot::CARROT_MIN_TX_INPUTS; i <= carrot::CARROT_MAX_TX_INPUTS; ++i)
        fee_by_input_count[i] = 30680000 * i - i*i;

    const boost::multiprecision::int128_t nominal_output_sum = 4444444444444; // 4.444... XMR 

    // add 10 random transfers
    tools::wallet2::transfer_container transfers;
    for (size_t i = 0; i < 10; ++i)
    {
        tools::wallet2::transfer_details &td = tools::add_element(transfers);
        td = gen_transfer_details();
        td.m_block_height = transfers.size(); // small ascending block heights
    }

    // modify one so that it funds the transfer all by itself
    const size_t rand_idx = crypto::rand_idx(transfers.size());
    transfers[rand_idx].m_amount = boost::numeric_cast<rct::xmr_amount>(nominal_output_sum +
            fee_by_input_count.crbegin()->second +
            crypto::rand_range<rct::xmr_amount>(0, COIN));

    // set such that all transfers are unlocked
    const std::uint64_t top_block_index = transfers.size() + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

    // make input selector
    std::set<size_t> selected_transfer_indices;
    const carrot::select_inputs_func_t input_selector = tools::wallet::make_wallet2_single_transfer_input_selector(
        transfers,
        /*from_account=*/0,
        /*from_subaddresses=*/{},
        /*ignore_above=*/std::numeric_limits<rct::xmr_amount>::max(),
        /*ignore_below=*/0,
        top_block_index,
        /*allow_carrot_external_inputs_in_normal_transfers=*/true,
        /*allow_pre_carrot_inputs_in_normal_transfers=*/true,
        selected_transfer_indices
    );

    // select inputs
    std::vector<carrot::CarrotSelectedInput> selected_inputs;
    input_selector(nominal_output_sum,
        fee_by_input_count,
        1,                             // number of normal payment proposals
        1,                             // number of self-send payment proposals
        selected_inputs);

    ASSERT_TRUE(1 == selected_inputs.size() || 2 == selected_inputs.size()); // assert one or two inputs selected
    ASSERT_EQ(selected_inputs.size(), selected_transfer_indices.size());
    ASSERT_LT(*selected_transfer_indices.crbegin(), transfers.size());

    // Assert content of selected inputs matches the content in `transfers`
    std::set<size_t> matched_transfer_indices;
    for (const carrot::CarrotSelectedInput &selected_input : selected_inputs)
    {
        for (const size_t selected_transfer_index : selected_transfer_indices)
        {
            if (compare_transfer_to_selected_input(transfers.at(selected_transfer_index), selected_input))
            {
                const auto insert_res = matched_transfer_indices.insert(selected_transfer_index);
                if (insert_res.second)
                    break;
            }
        }
    }
    ASSERT_EQ(selected_transfer_indices.size(), matched_transfer_indices.size());
}
//----------------------------------------------------------------------------------------------------------------------
TEST(wallet_tx_builder, make_carrot_transaction_proposals_wallet2_transfer_1)
{
    cryptonote::account_base alice;
    alice.generate();
    cryptonote::account_base bob;
    bob.generate();

    const tools::wallet2::transfer_container transfers{
        gen_transfer_details()};

    const rct::xmr_amount out_amount = rct::randXmrAmount(transfers.front().amount() / 2);

    const std::vector<cryptonote::tx_destination_entry> dsts{
        cryptonote::tx_destination_entry(out_amount, bob.get_keys().m_account_address, false)
    };

    const uint64_t top_block_index = std::max(transfers.front().m_block_height, transfers.back().m_block_height)
        + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

    const std::vector<carrot::CarrotTransactionProposalV1> tx_proposals = tools::wallet::make_carrot_transaction_proposals_wallet2_transfer(
        transfers,
        /*subaddress_map=*/{},
        dsts,
        /*fee_per_weight=*/1,
        /*extra=*/{},
        /*subaddr_account=*/0,
        /*subaddr_indices=*/{},
        /*ignore_above=*/MONEY_SUPPLY,
        /*ignore_below=*/0,
        {},
        top_block_index,
        alice.get_keys());

    ASSERT_EQ(1, tx_proposals.size());
    const carrot::CarrotTransactionProposalV1 tx_proposal = tx_proposals.at(0);

    std::vector<crypto::key_image> expected_key_images{transfers.front().m_key_image};

    // Assert basic length facts about tx proposal
    ASSERT_EQ(1, tx_proposal.key_images_sorted.size()); // we always try 2 when available
    EXPECT_EQ(expected_key_images, tx_proposal.key_images_sorted);
    ASSERT_EQ(1, tx_proposal.normal_payment_proposals.size());
    ASSERT_EQ(1, tx_proposal.selfsend_payment_proposals.size());
    EXPECT_EQ(0, tx_proposal.extra.size());

    // Assert amounts
    EXPECT_EQ(out_amount, tx_proposal.normal_payment_proposals.front().amount);
    EXPECT_EQ(out_amount + tx_proposal.selfsend_payment_proposals.front().proposal.amount + tx_proposal.fee,
        transfers.front().amount());
}
//----------------------------------------------------------------------------------------------------------------------
TEST(wallet_tx_builder, make_carrot_transaction_proposals_wallet2_sweep_1)
{
    cryptonote::account_base alice;
    alice.generate();
    cryptonote::account_base bob;
    bob.generate();

    const tools::wallet2::transfer_container transfers{gen_transfer_details()};

    const std::vector<carrot::CarrotTransactionProposalV1> tx_proposals = tools::wallet::make_carrot_transaction_proposals_wallet2_sweep(
        transfers,
        /*subaddress_map=*/{},
        {transfers.front().m_key_image},
        bob.get_keys().m_account_address,
        /*is_subaddress=*/false,
        /*n_dests=*/1,
        /*fee_per_weight=*/1,
        /*extra=*/{},
        transfers.front().m_block_height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE,
        alice.get_keys());
    ASSERT_EQ(1, tx_proposals.size());
    const carrot::CarrotTransactionProposalV1 &tx_proposal = tx_proposals.at(0);

    // Assert basic length facts about tx proposal
    ASSERT_EQ(1, tx_proposal.key_images_sorted.size());
    EXPECT_EQ(transfers.front().m_key_image, tx_proposal.key_images_sorted.front());
    ASSERT_EQ(1, tx_proposal.normal_payment_proposals.size());
    ASSERT_EQ(1, tx_proposal.selfsend_payment_proposals.size());
    EXPECT_EQ(0, tx_proposal.extra.size());

    // Assert amounts
    EXPECT_EQ(0, tx_proposal.selfsend_payment_proposals.front().proposal.amount);
    EXPECT_EQ(transfers.front().amount(), tx_proposal.fee + tx_proposal.normal_payment_proposals.front().amount);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(wallet_tx_builder, make_carrot_transaction_proposals_wallet2_sweep_2)
{
    cryptonote::account_base alice;
    alice.generate();
    cryptonote::account_base bob;
    bob.generate();

    const tools::wallet2::transfer_container transfers{gen_transfer_details()};

    const std::vector<carrot::CarrotTransactionProposalV1> tx_proposals = tools::wallet::make_carrot_transaction_proposals_wallet2_sweep(
        transfers,
        /*subaddress_map=*/{},
        {transfers.front().m_key_image},
        bob.get_keys().m_account_address,
        /*is_subaddress=*/false,
        /*n_dests=*/FCMP_PLUS_PLUS_MAX_OUTPUTS - 1,
        /*fee_per_weight=*/1,
        /*extra=*/{},
        transfers.front().m_block_height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE,
        alice.get_keys());
    ASSERT_EQ(1, tx_proposals.size());
    const carrot::CarrotTransactionProposalV1 &tx_proposal = tx_proposals.at(0);

    // Assert basic length facts about tx proposal
    ASSERT_EQ(1, tx_proposal.key_images_sorted.size());
    EXPECT_EQ(transfers.front().m_key_image, tx_proposal.key_images_sorted.front());
    ASSERT_EQ(FCMP_PLUS_PLUS_MAX_OUTPUTS - 1, tx_proposal.normal_payment_proposals.size());
    ASSERT_EQ(1, tx_proposal.selfsend_payment_proposals.size());
    EXPECT_EQ(0, tx_proposal.extra.size());

    // Assert amounts
    EXPECT_EQ(0, tx_proposal.selfsend_payment_proposals.front().proposal.amount);
    rct::xmr_amount total_output_amount = tx_proposal.fee;
    const rct::xmr_amount first_output_amount = tx_proposal.normal_payment_proposals.at(0).amount;
    for (const auto &normal_payment_proposal : tx_proposal.normal_payment_proposals)
    {
        const rct::xmr_amount amount = normal_payment_proposal.amount;
        const rct::xmr_amount max_amount = std::max(amount, first_output_amount);
        const rct::xmr_amount min_amount = std::min(amount, first_output_amount);
        EXPECT_LE(max_amount - min_amount, 1);
        total_output_amount += amount;
    }
    EXPECT_EQ(transfers.front().amount(), total_output_amount);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(wallet_tx_builder, make_carrot_transaction_proposals_wallet2_sweep_3)
{
    cryptonote::account_base alice;
    alice.generate();

    const tools::wallet2::transfer_container transfers{gen_transfer_details()};

    const std::vector<carrot::CarrotTransactionProposalV1> tx_proposals = tools::wallet::make_carrot_transaction_proposals_wallet2_sweep(
        transfers,
        /*subaddress_map=*/{{alice.get_keys().m_account_address.m_spend_public_key, {}}},
        {transfers.front().m_key_image},
        alice.get_keys().m_account_address,
        /*is_subaddress=*/false,
        /*n_dests=*/FCMP_PLUS_PLUS_MAX_OUTPUTS,
        /*fee_per_weight=*/1,
        /*extra=*/{},
        transfers.front().m_block_height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE,
        alice.get_keys());
    ASSERT_EQ(1, tx_proposals.size());
    const carrot::CarrotTransactionProposalV1 &tx_proposal = tx_proposals.at(0);

    // Assert basic length facts about tx proposal
    ASSERT_EQ(1, tx_proposal.key_images_sorted.size());
    EXPECT_EQ(transfers.front().m_key_image, tx_proposal.key_images_sorted.front());
    ASSERT_EQ(0, tx_proposal.normal_payment_proposals.size());
    ASSERT_EQ(FCMP_PLUS_PLUS_MAX_OUTPUTS, tx_proposal.selfsend_payment_proposals.size());
    EXPECT_EQ(0, tx_proposal.extra.size());

    // Assert amounts
    rct::xmr_amount total_output_amount = tx_proposal.fee;
    const rct::xmr_amount first_output_amount = tx_proposal.selfsend_payment_proposals.at(0).proposal.amount;
    for (const auto &selfsend_payment_proposal : tx_proposal.selfsend_payment_proposals)
    {
        const rct::xmr_amount amount = selfsend_payment_proposal.proposal.amount;
        const rct::xmr_amount max_amount = std::max(amount, first_output_amount);
        const rct::xmr_amount min_amount = std::min(amount, first_output_amount);
        EXPECT_LE(max_amount - min_amount, 1);
        total_output_amount += amount;
    }
    EXPECT_EQ(transfers.front().amount(), total_output_amount);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(wallet_tx_builder, make_carrot_transaction_proposals_wallet2_sweep_4)
{
    // output-limited sweep

    cryptonote::account_base alice;
    alice.generate();
    cryptonote::account_base bob;
    bob.generate();

    // generate transfers list
    const size_t n_transfers = 35;
    tools::wallet2::transfer_container transfers;
    transfers.reserve(n_transfers);
    for (size_t i = 0; i < n_transfers; ++i)
        transfers.push_back(gen_transfer_details());

    // generate random indices into transfer list
    const size_t n_selected_transfers = 31;
    std::set<size_t> selected_transfer_indices;
    while (selected_transfer_indices.size() < n_selected_transfers)
        selected_transfer_indices.insert(crypto::rand_idx(n_transfers));

    // generate map of amounts by key image, key image vector, and height of chain
    std::vector<crypto::key_image> selected_key_images;
    std::unordered_map<crypto::key_image, rct::xmr_amount> amounts_by_ki;
    uint64_t top_block_index = 0;
    for (const size_t selected_transfer_index : selected_transfer_indices)
    {
        const tools::wallet2::transfer_details &td = transfers.at(selected_transfer_index);
        selected_key_images.push_back(td.m_key_image);
        amounts_by_ki.emplace(td.m_key_image, td.amount());
        top_block_index = std::max(top_block_index, td.m_block_height);
    }
    top_block_index += CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

    ASSERT_EQ(n_selected_transfers, selected_key_images.size());
    ASSERT_EQ(n_selected_transfers, amounts_by_ki.size());

    const size_t n_dests = 4;

    // make tx proposals
    const std::vector<carrot::CarrotTransactionProposalV1> tx_proposals = tools::wallet::make_carrot_transaction_proposals_wallet2_sweep(
        transfers,
        /*subaddress_map=*/{},
        selected_key_images,
        bob.get_keys().m_account_address,
        /*is_subaddress=*/false,
        /*n_dests=*/n_dests,
        /*fee_per_weight=*/1,
        /*extra=*/{},
        top_block_index,
        alice.get_keys());
    ASSERT_EQ(4, tx_proposals.size());

    std::set<crypto::key_image> actual_seen_kis;
    size_t n_actual_inputs = 0;
    size_t n_actual_dests = 0;
    for (const carrot::CarrotTransactionProposalV1 &tx_proposal : tx_proposals)
    {
        ASSERT_LE(tx_proposal.key_images_sorted.size(), FCMP_PLUS_PLUS_MAX_INPUTS);
        ASSERT_EQ(1, tx_proposal.normal_payment_proposals.size());
        ASSERT_EQ(1, tx_proposal.selfsend_payment_proposals.size());
        ASSERT_EQ(1, tx_proposal.normal_payment_proposals.size());
        ASSERT_EQ(1, tx_proposal.selfsend_payment_proposals.size());
        ASSERT_EQ(0, tx_proposal.selfsend_payment_proposals.at(0).proposal.amount);
        EXPECT_EQ(0, tx_proposal.extra.size());

        rct::xmr_amount tx_inputs_amount = 0;
        for (const crypto::key_image &ki : tx_proposal.key_images_sorted)
        {
            ASSERT_TRUE(amounts_by_ki.count(ki));
            ASSERT_FALSE(actual_seen_kis.count(ki));
            actual_seen_kis.insert(ki);
            tx_inputs_amount += amounts_by_ki.at(ki);
        }
        const rct::xmr_amount tx_outputs_amount = tx_proposal.fee + tx_proposal.normal_payment_proposals.at(0).amount;
        ASSERT_EQ(tx_inputs_amount, tx_outputs_amount);

        n_actual_inputs += tx_proposal.key_images_sorted.size();
        n_actual_dests += tx_proposal.normal_payment_proposals.size();
    }

    EXPECT_EQ(n_selected_transfers, n_actual_inputs);
    EXPECT_EQ(n_dests, n_actual_dests);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(wallet_tx_builder, wallet2_scan_propose_sign_prove_member_and_scan_1)
{
    // 1. create fake blockchain
    // 2. create Alice, Bob wallet2 instance
    // 3. send a mix of fake-input legacy and carrot txs to Alice
    // 4. step blockchain forward 10 blocks
    // 5. scan blockchain with Alice wallet
    // 6. create carrot transaction proposal
    // 7. construct proofs for transaction
    // 8. serialize tx
    // 9. deserialize tx
    // 10. check ver_non_input_consensus()
    // 11. check verRctNonSemanticsSimple()
    // 12. add Alice's transaction to blockchain
    // 13. scan blockchain with Bob's wallet and assert money received
    // 14. scan blockchain with Alice's wallet and assert money left

    // 1.
    LOG_PRINT_L2("Initiating my imaginary, friendly chain of blocks");
    mock::fake_pruned_blockchain bc(0);

    // 2.
    LOG_PRINT_L2("Generating wallets for Alice and Bob, the usual suspects");
    tools::wallet2 alice(cryptonote::MAINNET, /*kdf_rounds=*/1, /*unattended=*/true);
    tools::wallet2 bob(cryptonote::MAINNET, /*kdf_rounds=*/1, /*unattended=*/true);
    alice.set_offline(true);
    bob.set_offline(true);
    alice.generate("", "");
    bob.generate("", "");
    const cryptonote::account_keys &alice_keys = alice.get_account().get_keys();
    const cryptonote::account_public_address alice_main_addr = alice.get_account().get_keys().m_account_address;
    const cryptonote::account_public_address bob_main_addr = bob.get_account().get_keys().m_account_address;
    bc.init_wallet_for_starting_block(alice);
    bc.init_wallet_for_starting_block(bob);

    // 3.
    LOG_PRINT_L2("Sending transactions from the aether to Alice (0)");
    const rct::xmr_amount amount0 = rct::randXmrAmount(COIN);
    std::vector<cryptonote::tx_destination_entry> dests0{cryptonote::tx_destination_entry(amount0, alice_main_addr, false)};
    cryptonote::transaction tx = mock::construct_pre_carrot_tx_with_fake_inputs(dests0, /*fee=*/1234, /*hf_version=*/2);
    bc.add_block(2, {std::move(tx)}, mock::null_addr);
    LOG_PRINT_L2("Sending transactions from the aether to Alice (1)");
    const rct::xmr_amount amount1 = rct::randXmrAmount(COIN);
    std::vector<cryptonote::tx_destination_entry> dests1{cryptonote::tx_destination_entry(amount1, alice.get_subaddress({0, 13}), true)};
    cryptonote::account_base aether;
    aether.generate();
    tx = mock::construct_carrot_pruned_transaction_fake_inputs({carrot::mock::convert_normal_payment_proposal_v1(dests1.front())}, {}, aether.get_keys());
    bc.add_block(HF_VERSION_CARROT, {std::move(tx)}, mock::null_addr);

    // 4. 
    //!@TODO: figure out why membership proving fails if there's fewer leaves than the curve1 width
    const size_t target_num_outputs = fcmp_pp::curve_trees::SELENE_CHUNK_WIDTH * fcmp_pp::curve_trees::HELIOS_CHUNK_WIDTH + 7;
    while (bc.num_outputs() < target_num_outputs)
        bc.add_block(HF_VERSION_CARROT, {}, mock::null_addr, target_num_outputs - bc.num_outputs());

    LOG_PRINT_L2("Twiddling thumbs");
    for (size_t i = 0; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
        bc.add_block(HF_VERSION_CARROT, {}, mock::null_addr);

    // 5.
    LOG_PRINT_L2("Alice's vision is filled with shadowy keys, hashes, points, rings, trees, curves, chains, all flowing in and out of one another");
    uint64_t blocks_added = bc.refresh_wallet(alice, 0);
    ASSERT_EQ(bc.height()-1, blocks_added);
    ASSERT_EQ(2, alice.m_transfers.size());
    ASSERT_EQ(amount0 + amount1, alice.balance_all(true)); // really, we care about unlocked_balance_all() for sending, but that call uses RPC

    // 6. 
    LOG_PRINT_L2("Alice feels pity on Bob and proposes to send his broke ass some dough");
    const rct::xmr_amount out_amount = rct::randXmrAmount(amount0 + amount1);
    const std::vector<carrot::CarrotTransactionProposalV1> tx_proposals = 
        tools::wallet::make_carrot_transaction_proposals_wallet2_transfer( // stupidly long function name ;(
            alice.m_transfers,
            alice.m_subaddresses,
            {cryptonote::tx_destination_entry(out_amount, bob_main_addr, false)},
            /*fee_per_weight=*/1,
            /*extra=*/{},
            /*subaddr_account=*/0,
            /*subaddr_indices=*/{},
            /*ignore_above=*/std::numeric_limits<rct::xmr_amount>::max(),
            /*ignore_below=*/0,
            {},
            /*top_block_index=*/bc.height()-1,
            alice_keys);
    
    ASSERT_EQ(1, tx_proposals.size());
    const carrot::CarrotTransactionProposalV1 tx_proposal = tx_proposals.at(0);

    // 7.
    LOG_PRINT_L2("Alice has something to prove");
    tx = tools::wallet::finalize_all_proofs_from_transfer_details(tx_proposal,
        alice.m_transfers,
        alice.m_tree_cache,
        *alice.m_curve_trees,
        alice_keys);

    // 8.
    LOG_PRINT_L2("Hello, Mr. Blobby");
    const cryptonote::blobdata alicebob_tx_blob = cryptonote::tx_to_blob(tx);

    // 9.
    LOG_PRINT_L2("Goodbye, Mr. Blobby");
    cryptonote::transaction alicebob_tx;
    ASSERT_TRUE(cryptonote::parse_and_validate_tx_from_blob(alicebob_tx_blob, alicebob_tx));

    // 10.
    LOG_PRINT_L2("Bob couldn't believe someone to be so generous in his time of need, so he verifies");
    ASSERT_GE(bc.hf_version(), HF_VERSION_FCMP_PLUS_PLUS);
    cryptonote::tx_verification_context tvc{};
    ASSERT_TRUE(cryptonote::ver_non_input_consensus(alicebob_tx, tvc, bc.hf_version()));
    EXPECT_FALSE(tvc.m_verifivation_failed);

    // 11.
    LOG_PRINT_L2("'Perhaps this is valid money that belongs to another chain', Bob postulates");
    const uint8_t *tree_root = bc.get_fcmp_tree_root_at(bc.height() - 1);
    ASSERT_TRUE(cryptonote::Blockchain::expand_transaction_2(alicebob_tx,
        cryptonote::get_transaction_prefix_hash(alicebob_tx),
        /*pubkeys=*/{},
        tree_root));
    EXPECT_TRUE(rct::verRctNonSemanticsSimple(alicebob_tx.rct_signatures));

    // 12.
    LOG_PRINT_L2("'Chain, chain, chain (Chain, chain, chain)' - Aretha Franklin");
    const rct::xmr_amount alicebob_tx_fee = alicebob_tx.rct_signatures.txnFee;
    bc.add_block(HF_VERSION_CARROT, {std::move(alicebob_tx)}, mock::null_addr);

    // 13. 
    LOG_PRINT_L2("A great day for Bob");
    ASSERT_EQ(0, bob.balance_all(true));
    blocks_added = bc.refresh_wallet(bob, 0);
    ASSERT_EQ(bc.height()-1, blocks_added);
    ASSERT_EQ(1, bob.m_transfers.size());
    EXPECT_EQ(out_amount, bob.balance_all(true));

    // 14.
    LOG_PRINT_L2("Alice obtains the fulfillment that only stems from selfless generosity");
    const rct::xmr_amount alice_old_balance = alice.balance_all(true);
    ASSERT_GE(alice_old_balance, out_amount + alicebob_tx_fee);
    blocks_added = bc.refresh_wallet(alice, 0);
    ASSERT_EQ(1, blocks_added);
    const rct::xmr_amount alice_new_balance = alice.balance_all(true);
    ASSERT_LT(alice_new_balance, alice_old_balance);
    EXPECT_EQ(alice_new_balance + out_amount + alicebob_tx_fee, alice_old_balance);
}
//----------------------------------------------------------------------------------------------------------------------
