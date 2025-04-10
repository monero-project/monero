// Copyright (c) 2023-2024, The Monero Project
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

#include "carrot_impl/address_device_ram_borrowed.h"
#include "carrot_impl/carrot_tx_builder_inputs.h"
#include "carrot_mock_helpers.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "fake_pruned_blockchain.h"
#include "fcmp_pp/prove.h"
#include "tx_construction_helpers.h"
#include "wallet/tx_builder.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "unit_tests.wallet_scanning"

//----------------------------------------------------------------------------------------------------------------------
TEST(wallet_scanning, view_scan_as_sender_mainaddr)
{
    cryptonote::account_base aether;
    aether.generate();

    cryptonote::account_base bob;
    bob.generate();
    const cryptonote::account_public_address bob_main_addr = bob.get_keys().m_account_address;
    const crypto::public_key bob_main_spend_pubkey = bob_main_addr.m_spend_public_key;

    const rct::xmr_amount amount = rct::randXmrAmount(10 * COIN);

    const rct::xmr_amount fee = 565678;

    for (uint8_t hf_version = 1; hf_version < HF_VERSION_FCMP_PLUS_PLUS; ++hf_version)
    {
        MDEBUG("view_scan_as_sender_mainaddr: hf_version=" << static_cast<int>(hf_version));

        std::vector<cryptonote::tx_destination_entry> destinations{
            cryptonote::tx_destination_entry(amount, bob_main_addr, false)};

        crypto::secret_key main_tx_privkey;
        std::vector<crypto::secret_key> additional_tx_privkeys;
        const cryptonote::transaction tx = mock::construct_pre_carrot_tx_with_fake_inputs(aether.get_keys(),
            {{aether.get_keys().m_account_address.m_spend_public_key, {0, 0}}},
            {},
            destinations,
            {},
            fee,
            hf_version,
            main_tx_privkey,
            additional_tx_privkeys);

        ASSERT_EQ(0, additional_tx_privkeys.size());
        ASSERT_GT(tx.vout.size(), 0);

        // do K_d = 8 * r * K^j_v
        crypto::key_derivation main_derivation;
        ASSERT_TRUE(crypto::generate_key_derivation(bob_main_addr.m_view_public_key,
            main_tx_privkey,
            main_derivation));

        aether.generate();

        // call view_incoming_scan_transaction with no meaningful key nor subaddresses maps,
        // just with the proper ECDH
        std::vector<std::optional<tools::wallet::enote_view_incoming_scan_info_t>> enote_scan_infos(tx.vout.size());
        tools::wallet::view_incoming_scan_transaction(tx,
            {&main_derivation, 1},
            {},
            aether.get_keys(),
            {{bob_main_spend_pubkey, {}}}, // use a fake subaddress map with just the provided address in it
            epee::to_mut_span(enote_scan_infos));

        bool matched = false;
        for (const auto &enote_scan_info : enote_scan_infos)
        {
            if (enote_scan_info)
            {
                ASSERT_FALSE(matched);
                ASSERT_EQ(amount, enote_scan_info->amount);
                ASSERT_EQ(bob_main_spend_pubkey, enote_scan_info->address_spend_pubkey);
                matched = true;
            }
        }
        ASSERT_TRUE(matched);
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(wallet_scanning, positive_smallout_main_addr_all_types_outputs)
{
    // Test that wallet can scan enotes and prove SA/L for following types:
    //   a. pre-ringct coinbase
    //   b. pre-ringct
    //   c. ringct coinbase
    //   d. ringct long-amount
    //   e. ringct short-amount
    //   f. view-tagged ringct coinbase
    //   g. view-tagged pre-ringct (only possible in unmixable sweep txs)
    //   h. view-tagged ringct
    //   i. carrot v1 coinbase
    //   j. carrot v1 normal
    //   k. carrot v1 special
    //   l. carrot v1 internal (@TODO)
    //
    // All enotes are addressed to the main address in 2-out noin-coinbase txs or 1-out coinbase txs.
    // We also don't test reorgs here.

    // init blockchain
    mock::fake_pruned_blockchain bc(0);

    // generate wallet
    tools::wallet2 w(cryptonote::MAINNET, /*kdf_rounds=*/1, /*unattended=*/true);
    w.generate("", "");
    const cryptonote::account_keys &acc_keys = w.get_account().get_keys();
    const cryptonote::account_public_address main_addr = w.get_account().get_keys().m_account_address;
    ASSERT_EQ(0, w.balance(0, true));
    bc.init_wallet_for_starting_block(w); // needed b/c internal logic

    uint64_t refresh_height = 0;
    const auto wallet_process_new_blocks = [&w, &bc, &refresh_height]() -> boost::multiprecision::int128_t
    {
        const boost::multiprecision::int128_t old_balance = w.balance(0, true);

        // note: doesn't handle reorgs
        bc.refresh_wallet(w, 0);

        // update refresh_height
        refresh_height = bc.height();

        // return amount of money received
        return boost::multiprecision::int128_t(w.balance(0, true)) - old_balance;
    };

    size_t old_num_m_transfers = 0;
    const auto verify_sals_of_recent_transfers = [&w, &old_num_m_transfers]() -> bool
    {
        // assert m_transfers is monotonic (may change if internal design of wallet2 changes)
        size_t prev_blk_idx = 0;
        size_t prev_rct_output_idx = 0;
        for (const tools::wallet2::transfer_details &td : w.m_transfers)
        {
            CHECK_AND_ASSERT_THROW_MES(td.m_block_height >= prev_blk_idx, "m_transfers not monotonic");
            prev_blk_idx = td.m_block_height;
            if (td.m_rct)
            {
                CHECK_AND_ASSERT_THROW_MES(td.m_global_output_index >= prev_rct_output_idx, "m_transfers not monotonic");
                prev_rct_output_idx = td.m_global_output_index;
            }
        }

        const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

        // note: will skip reorged transfers unless we set num_m_transfers
        const size_t new_num_m_transfers = w.m_transfers.size();
        for (size_t i = old_num_m_transfers; i < new_num_m_transfers; ++i)
        {
            old_num_m_transfers = new_num_m_transfers; // skip failed SA/Ls for future calls

            LOG_PRINT_L2("Creating & verifying SA/L proof on m_transfers.at(" << i << ")");
            const tools::wallet2::transfer_details &td = w.m_transfers.at(i);

            const cryptonote::account_keys &acc = w.get_account().get_keys();

            const carrot::OutputOpeningHintVariant opening_hint =
                tools::wallet::make_sal_opening_hint_from_transfer_details(
                    td,
                    acc.m_view_secret_key,
                    acc.get_device());

            const FcmpRerandomizedOutputCompressed rerandomized_output = fcmp_pp::rerandomize_output(
                onetime_address_ref(opening_hint),
                rct::rct2pt(amount_commitment_ref(opening_hint)));

            const carrot::CarrotOpenableRerandomizedOutputV1 openable_rerandomized_output{
                .rerandomized_output = rerandomized_output,
                .opening_hint = opening_hint
            };

            //! @TODO: carrot hierarchy
            const carrot::cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
                acc.m_account_address.m_spend_public_key,
                acc.m_view_secret_key);

            fcmp_pp::FcmpPpSalProof sal_proof;
            crypto::key_image spent_key_image;
            carrot::make_sal_proof_any_to_legacy_v1(signable_tx_hash,
                openable_rerandomized_output,
                acc.m_spend_secret_key,
                addr_dev,
                sal_proof,
                spent_key_image);

            if (spent_key_image != td.m_key_image)
                return false; // fail verify

            if (!fcmp_pp::verify_sal(signable_tx_hash,
                    rerandomized_output.input,
                    spent_key_image,
                    sal_proof))
                return false; // fail verify
        }

        return true;
    };

    // a. push block containing a pre-ringct coinbase output to wallet
    bc.add_block(1, {}, main_addr);

    // a. scan pre-ringct coinbase tx
    auto balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(mock::fake_pruned_blockchain::miner_reward, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // b. construct and push a pre-ringct tx
    const rct::xmr_amount amount_b = rct::randXmrAmount(COIN);
    {
        const rct::xmr_amount fee = rct::randXmrAmount(COIN);
        std::vector<cryptonote::tx_destination_entry> dests = {
            cryptonote::tx_destination_entry(amount_b, acc_keys.m_account_address, false)};
        cryptonote::transaction curr_tx = mock::construct_pre_carrot_tx_with_fake_inputs(
            dests,
            fee,
            /*hf_version=*/1);
        ASSERT_FALSE(cryptonote::is_coinbase(curr_tx));
        ASSERT_EQ(1, curr_tx.version);
        ASSERT_EQ(rct::RCTTypeNull, curr_tx.rct_signatures.type);
        ASSERT_EQ(typeid(cryptonote::txout_to_key), curr_tx.vout.at(0).target.type());
        ASSERT_EQ(amount_b, curr_tx.vout.at(0).amount);
        bc.add_block(1, {std::move(curr_tx)}, mock::null_addr);
    }

    // b. scan pre-ringct tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(amount_b, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // c. construct and push a ringct coinbase tx
    bc.add_block(HF_VERSION_DYNAMIC_FEE, {}, main_addr);
    {
        auto top_block = bc.get_parsed_block(bc.height() - 1);
        const cryptonote::transaction &top_miner_tx = top_block.block.miner_tx;
        ASSERT_EQ(2, top_miner_tx.version);
        ASSERT_NE(0, top_miner_tx.vout.size());
        ASSERT_EQ(rct::RCTTypeNull, top_miner_tx.rct_signatures.type);
        ASSERT_EQ(0, top_miner_tx.signatures.size());
        ASSERT_EQ(mock::fake_pruned_blockchain::miner_reward, top_miner_tx.vout.at(0).amount);
    }

    // c. scan ringct coinbase tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(mock::fake_pruned_blockchain::miner_reward, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // d. construct and push a ringct long-amount tx
    const rct::xmr_amount amount_d = rct::randXmrAmount(COIN);
    {
        const rct::xmr_amount fee = rct::randXmrAmount(COIN);
        std::vector<cryptonote::tx_destination_entry> dests = {
            cryptonote::tx_destination_entry(amount_d, acc_keys.m_account_address, false)};
        cryptonote::transaction curr_tx = mock::construct_pre_carrot_tx_with_fake_inputs(
            dests,
            fee,
            HF_VERSION_DYNAMIC_FEE);
        ASSERT_FALSE(cryptonote::is_coinbase(curr_tx));
        ASSERT_EQ(2, curr_tx.version);
        ASSERT_EQ(rct::RCTTypeFull, curr_tx.rct_signatures.type);
        ASSERT_EQ(typeid(cryptonote::txout_to_key), curr_tx.vout.at(0).target.type());
        ASSERT_EQ(0, curr_tx.vout.at(0).amount);
        bc.add_block(HF_VERSION_DYNAMIC_FEE, {std::move(curr_tx)}, mock::null_addr);
    }

    // d. scan ringct long-amount tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(amount_d, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // e. construct and push a ringct short-amount tx
    const rct::xmr_amount amount_e = rct::randXmrAmount(COIN);
    {
        const rct::xmr_amount fee = rct::randXmrAmount(COIN);
        std::vector<cryptonote::tx_destination_entry> dests = {
            cryptonote::tx_destination_entry(amount_e, acc_keys.m_account_address, false)};
        cryptonote::transaction curr_tx = mock::construct_pre_carrot_tx_with_fake_inputs(
            dests,
            fee,
            HF_VERSION_SMALLER_BP);
        ASSERT_FALSE(cryptonote::is_coinbase(curr_tx));
        ASSERT_EQ(2, curr_tx.version);
        ASSERT_EQ(rct::RCTTypeBulletproof2, curr_tx.rct_signatures.type);
        ASSERT_EQ(typeid(cryptonote::txout_to_key), curr_tx.vout.at(0).target.type());
        ASSERT_EQ(0, curr_tx.vout.at(0).amount);
        bc.add_block(HF_VERSION_SMALLER_BP, {std::move(curr_tx)}, mock::null_addr);
    }

    // e. scan ringct short-amount tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(amount_e, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // f. construct and push a view-tagged ringct coinbase tx
    bc.add_block(HF_VERSION_VIEW_TAGS, {}, main_addr);
    {
        auto top_block = bc.get_parsed_block(bc.height() - 1);
        const cryptonote::transaction &top_miner_tx = top_block.block.miner_tx;
        ASSERT_EQ(2, top_miner_tx.version);
        ASSERT_EQ(1, top_miner_tx.vout.size());
        ASSERT_EQ(rct::RCTTypeNull, top_miner_tx.rct_signatures.type);
        ASSERT_EQ(0, top_miner_tx.signatures.size());
        ASSERT_EQ(typeid(cryptonote::txout_to_tagged_key), top_miner_tx.vout.at(0).target.type());
        ASSERT_EQ(mock::fake_pruned_blockchain::miner_reward, top_miner_tx.vout.at(0).amount);
    }

    // f. scan view-tagged ringct coinbase tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(mock::fake_pruned_blockchain::miner_reward, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // g. construct and push a view-tagged pre-ringct (only possible in unmixable sweep txs) tx
    const rct::xmr_amount amount_g = rct::randXmrAmount(COIN);
    {
        const rct::xmr_amount fee = rct::randXmrAmount(COIN);
        std::vector<cryptonote::tx_destination_entry> dests = {
            cryptonote::tx_destination_entry(amount_g, acc_keys.m_account_address, false)};
        cryptonote::transaction curr_tx = mock::construct_pre_carrot_tx_with_fake_inputs(
            dests,
            fee,
            HF_VERSION_VIEW_TAGS,
            /*sweep_unmixable_override=*/true);
        ASSERT_FALSE(cryptonote::is_coinbase(curr_tx));
        ASSERT_EQ(1, curr_tx.version);
        ASSERT_EQ(rct::RCTTypeNull, curr_tx.rct_signatures.type);
        ASSERT_EQ(typeid(cryptonote::txout_to_tagged_key), curr_tx.vout.at(0).target.type());
        ASSERT_EQ(amount_g, curr_tx.vout.at(0).amount);
        bc.add_block(HF_VERSION_VIEW_TAGS, {std::move(curr_tx)}, mock::null_addr);
    }

    // g. scan view-tagged pre-ringct (only possible in unmixable sweep txs) tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(amount_g, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // h. construct and push a view-tagged ringct tx
    const rct::xmr_amount amount_h = rct::randXmrAmount(COIN);
    {
        const rct::xmr_amount fee = rct::randXmrAmount(COIN);
        std::vector<cryptonote::tx_destination_entry> dests = {
            cryptonote::tx_destination_entry(amount_h, acc_keys.m_account_address, false)};
        cryptonote::transaction curr_tx = mock::construct_pre_carrot_tx_with_fake_inputs(
            dests,
            fee,
            HF_VERSION_VIEW_TAGS);
        ASSERT_FALSE(cryptonote::is_coinbase(curr_tx));
        ASSERT_EQ(2, curr_tx.version);
        ASSERT_EQ(rct::RCTTypeBulletproofPlus, curr_tx.rct_signatures.type);
        ASSERT_EQ(typeid(cryptonote::txout_to_tagged_key), curr_tx.vout.at(0).target.type());
        ASSERT_EQ(0, curr_tx.vout.at(0).amount);
        bc.add_block(HF_VERSION_VIEW_TAGS, {std::move(curr_tx)}, mock::null_addr);
    }

    // h. scan ringct view-tagged ringct tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(amount_h, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // i. construct and push a carrot v1 coinbase tx
    bc.add_block(HF_VERSION_CARROT, {}, main_addr);
    {
        auto top_block = bc.get_parsed_block(bc.height() - 1);
        const cryptonote::transaction &top_miner_tx = top_block.block.miner_tx;
        ASSERT_EQ(2, top_miner_tx.version);
        ASSERT_EQ(1, top_miner_tx.vout.size());
        ASSERT_EQ(rct::RCTTypeNull, top_miner_tx.rct_signatures.type);
        ASSERT_EQ(0, top_miner_tx.signatures.size());
        ASSERT_EQ(typeid(cryptonote::txout_to_carrot_v1), top_miner_tx.vout.at(0).target.type());
        ASSERT_EQ(mock::fake_pruned_blockchain::miner_reward, top_miner_tx.vout.at(0).amount);
    }

    // i. scan carrot v1 coinbase tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(mock::fake_pruned_blockchain::miner_reward, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // j. construct and push a carrot v1 normal tx
    const rct::xmr_amount amount_j = rct::randXmrAmount(COIN);
    {
        std::vector<cryptonote::tx_destination_entry> dests = {
            cryptonote::tx_destination_entry(amount_j, acc_keys.m_account_address, false)};
        cryptonote::transaction curr_tx = mock::construct_carrot_pruned_transaction_fake_inputs(
            {carrot::mock::convert_normal_payment_proposal_v1(dests.front())},
            /*selfsend_payment_proposals=*/{},
            acc_keys);
        ASSERT_FALSE(cryptonote::is_coinbase(curr_tx));
        ASSERT_EQ(2, curr_tx.version);
        ASSERT_EQ(rct::RCTTypeFcmpPlusPlus, curr_tx.rct_signatures.type);
        ASSERT_EQ(typeid(cryptonote::txout_to_carrot_v1), curr_tx.vout.at(0).target.type());
        ASSERT_EQ(0, curr_tx.vout.at(0).amount);
        bc.add_block(HF_VERSION_CARROT, {std::move(curr_tx)}, mock::null_addr);
    }

    // j. scan carrot v1 normal tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(amount_j, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // k. construct and push a carrot v1 special tx
    const rct::xmr_amount amount_k = rct::randXmrAmount(COIN);
    {
        std::vector<cryptonote::tx_destination_entry> dests = {
            cryptonote::tx_destination_entry(amount_k, acc_keys.m_account_address, false)};
        cryptonote::transaction curr_tx = mock::construct_carrot_pruned_transaction_fake_inputs(
            /*normal_payment_proposals=*/{},
            {{carrot::mock::convert_selfsend_payment_proposal_v1(dests.front()), {/*main*/}}},
            acc_keys);
        ASSERT_FALSE(cryptonote::is_coinbase(curr_tx));
        ASSERT_EQ(2, curr_tx.version);
        ASSERT_EQ(rct::RCTTypeFcmpPlusPlus, curr_tx.rct_signatures.type);
        ASSERT_EQ(2, curr_tx.vout.size());
        ASSERT_EQ(typeid(cryptonote::txout_to_carrot_v1), curr_tx.vout.at(0).target.type());
        ASSERT_EQ(0, curr_tx.vout.at(0).amount);
        bc.add_block(HF_VERSION_CARROT, {std::move(curr_tx)}, mock::null_addr);
    }

    // k. scan carrot v1 special tx
    balance_diff = wallet_process_new_blocks();
    EXPECT_EQ(amount_k, balance_diff);
    EXPECT_TRUE(verify_sals_of_recent_transfers());

    // assert we did all SA/L proving
    ASSERT_EQ(w.m_transfers.size(), old_num_m_transfers);
}
//----------------------------------------------------------------------------------------------------------------------
