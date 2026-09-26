// Copyright (c) 2023-2026, The Monero Project
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

#include "unit_tests_utils.h"
#include "gtest/gtest.h"

#include <cctype>

#include "file_io_utils.h"
#include "wallet/wallet2.h"
#include "common/util.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

using namespace boost::filesystem;
using namespace epee::file_io_utils;

static constexpr const char WALLET_00fd416a_PRIMARY_ADDRESS[] =
    "45p2SngJAPSJbqSiUvYfS3BfhEdxZmv8pDt25oW1LzxrZv9Uq6ARagiFViMGUE3gJk5VPWingCXVf1p2tyAy6SUeSHPhbve";

// https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L156
static constexpr const char WALLET2_ASCII_OUTPUT_MAGIC[] = "MoneroAsciiDataV1";

class wallet_accessor_test
{
public:
    static void forget_cached_key_image(tools::wallet2 &wallet, const size_t index)
    {
        crypto::key_image stale_key_image = AUTO_VAL_INIT(stale_key_image);
        tools::wallet2::transfer_details &td = wallet.m_transfers.at(index);
        td.m_key_image = stale_key_image;
        td.m_key_image_known = false;
        td.m_key_image_request = true;
        td.m_key_image_partial = false;
    }

    static crypto::public_key get_public_key(const tools::wallet2 &wallet, const size_t index)
    {
        return wallet.m_transfers.at(index).get_public_key();
    }

    struct confirmation_blocks
    {
        std::vector<tools::wallet2::parsed_block> parsed_blocks;
        std::vector<cryptonote::block_complete_entry> blocks;
        uint64_t added = 0;
    };

    static void process_pool_transaction(tools::wallet2 &wallet, const cryptonote::transaction &tx)
    {
        wallet.process_pool_state({{tx, cryptonote::get_transaction_hash(tx), false}});
    }

    static void prepare_confirmation(tools::wallet2 &wallet, const cryptonote::transaction &tx,
        confirmation_blocks &fixture,
        const cryptonote::transaction *failing_next = nullptr, bool same_block = false)
    {
        auto &parsed_blocks = fixture.parsed_blocks;
        parsed_blocks.resize(failing_next && !same_block ? 3 : 2);
        auto &genesis = parsed_blocks[0];
        wallet.generate_genesis(genesis.block);
        genesis.hash = cryptonote::get_block_hash(genesis.block);
        auto &confirmed = parsed_blocks[1];
        confirmed.block.major_version = 12;
        confirmed.block.timestamp = time(nullptr);
        confirmed.block.prev_id = genesis.hash;
        confirmed.block.tx_hashes = {cryptonote::get_transaction_hash(tx)};
        cryptonote::account_base miner;
        miner.generate();
        ASSERT_TRUE(cryptonote::construct_miner_tx(1, 0, 0, 0, 0,
            miner.get_keys().m_account_address, confirmed.block.miner_tx, {}, 1, 12));
        confirmed.hash = cryptonote::get_block_hash(confirmed.block);
        confirmed.txes = {tx};
        confirmed.o_indices.indices.resize(2);
        confirmed.o_indices.indices[0].indices.resize(confirmed.block.miner_tx.vout.size());
        confirmed.o_indices.indices[1].indices.resize(tx.vout.size());
        auto &blocks = fixture.blocks;
        blocks.resize(parsed_blocks.size());
        blocks[1].txs.resize(1);
        if (failing_next && same_block)
        {
            confirmed.block.tx_hashes.push_back(cryptonote::get_transaction_hash(*failing_next));
            confirmed.block.invalidate_hashes();
            confirmed.hash = cryptonote::get_block_hash(confirmed.block);
            confirmed.txes.push_back(*failing_next);
            confirmed.o_indices.indices.resize(3);
            blocks[1].txs.resize(2);
            // Only the second transaction lacks indices; the first receipt is already credited.
        }
        else if (failing_next)
        {
            auto &invalid = parsed_blocks[2];
            invalid.block = confirmed.block;
            invalid.block.prev_id = confirmed.hash;
            invalid.block.tx_hashes = {cryptonote::get_transaction_hash(*failing_next)};
            invalid.block.invalidate_hashes();
            invalid.hash = cryptonote::get_block_hash(invalid.block);
            invalid.txes = {*failing_next};
            blocks[2].txs.resize(1);
            // Missing output indices fail block entry, after the preceding block commits.
        }
    }

    static void process_confirmation(tools::wallet2 &wallet, confirmation_blocks &fixture)
    {
        fixture.added = 0;
        wallet.process_parsed_blocks(0, fixture.blocks, fixture.parsed_blocks, fixture.added, nullptr);
        EXPECT_EQ(1, fixture.added);
    }

    static size_t pending_payments(const tools::wallet2 &wallet, const crypto::hash &txid)
    {
        return std::count_if(wallet.m_unconfirmed_payments.begin(), wallet.m_unconfirmed_payments.end(),
            [&](const auto &entry) { return entry.second.m_pd.m_tx_hash == txid; });
    }

    static void reorg_payment(tools::wallet2 &wallet, const crypto::hash &txid,
        const crypto::hash *other_txid = nullptr)
    {
        wallet.handle_reorg(1);
        EXPECT_TRUE(wallet.accept_pool_tx_for_processing(txid, {}));
        if (other_txid)
        {
            EXPECT_TRUE(wallet.accept_pool_tx_for_processing(*other_txid, {}));
        }
    }
};

TEST(wallet_storage, confirmation_removes_cached_pool_payment)
{
    enum class failure_point { none, next_block, next_transaction, first_transaction };
    for (const auto failure : {failure_point::none, failure_point::next_block,
        failure_point::next_transaction, failure_point::first_transaction})
    for (const bool other_pending : {false, true})
    for (const auto &payment_id : {crypto::null_hash, crypto::rand<crypto::hash>()})
    {
        SCOPED_TRACE(::testing::Message() << "failure=" << static_cast<int>(failure)
            << ", other_pending=" << other_pending << ", null_payment_id=" << (payment_id == crypto::null_hash));
        const bool failed_first = failure == failure_point::first_transaction;
        const bool same_block = failed_first || failure == failure_point::next_transaction;
        tools::wallet2 wallet(cryptonote::MAINNET, 1, true);
        wallet.set_subaddress_lookahead(1, 1);
        wallet.generate("", "");
        wallet.set_refresh_from_block_height(0);
        const auto make_payment = [&](uint64_t amount)
        {
            cryptonote::transaction tx{};
            tx.version = 1;
            cryptonote::txin_to_key input{};
            input.amount = 2 * amount;
            input.key_offsets = {0};
            input.k_image = crypto::rand<crypto::key_image>();
            tx.vin.push_back(input);
            tx.signatures.resize(1);
            tx.signatures[0].resize(1);

            const auto tx_key = cryptonote::keypair::generate(hw::get_device("default"));
            EXPECT_TRUE(cryptonote::add_tx_pub_key_to_extra(tx, tx_key.pub));
            std::string nonce;
            cryptonote::set_payment_id_to_tx_extra_nonce(nonce, payment_id);
            EXPECT_TRUE(cryptonote::add_extra_nonce_to_tx_extra(tx.extra, nonce));
            crypto::key_derivation derivation;
            EXPECT_TRUE(crypto::generate_key_derivation(wallet.get_account().get_keys().m_account_address.m_view_public_key,
                tx_key.sec, derivation));
            for (size_t i = 0; i < 2; ++i)
            {
                cryptonote::txout_to_key output{};
                EXPECT_TRUE(crypto::derive_public_key(derivation, i,
                    wallet.get_account().get_keys().m_account_address.m_spend_public_key, output.key));
                tx.vout.push_back({amount, output});
            }
            return tx;
        };

        const auto confirming = make_payment(7);
        const auto pending = make_payment(11);
        const auto confirming_id = cryptonote::get_transaction_hash(confirming);
        const auto pending_id = cryptonote::get_transaction_hash(pending);
        wallet_accessor_test::process_pool_transaction(wallet, confirming);
        if (other_pending)
            wallet_accessor_test::process_pool_transaction(wallet, pending);
        const uint64_t expected_balance = other_pending ? 36 : 14;
        ASSERT_EQ(1, wallet_accessor_test::pending_payments(wallet, confirming_id));
        ASSERT_EQ(other_pending ? 1 : 0, wallet_accessor_test::pending_payments(wallet, pending_id));
        ASSERT_EQ(expected_balance, wallet.balance(0, false));
        ASSERT_EQ(0, wallet.balance(0, true));

        wallet_accessor_test::confirmation_blocks fixture;
        wallet_accessor_test::prepare_confirmation(wallet, confirming, fixture,
            failure == failure_point::none ? nullptr : &pending, same_block);
        if (failed_first)
        {
            fixture.parsed_blocks[1].o_indices.indices[1].indices.clear();
            fixture.parsed_blocks[1].o_indices.indices[2].indices.resize(pending.vout.size());
        }

        // Confirm without another pool query, as during deferred-polling catch-up.
        if (failure != failure_point::none)
            EXPECT_THROW(wallet_accessor_test::process_confirmation(wallet, fixture),
                tools::error::wallet_internal_error);
        else
            wallet_accessor_test::process_confirmation(wallet, fixture);
        EXPECT_EQ(same_block ? 0 : 1, fixture.added);
        EXPECT_EQ(failed_first ? 1 : 0, wallet_accessor_test::pending_payments(wallet, confirming_id));
        EXPECT_EQ(other_pending ? 1 : 0, wallet_accessor_test::pending_payments(wallet, pending_id));
        EXPECT_EQ(expected_balance, wallet.balance(0, false));
        EXPECT_EQ(failed_first ? 0 : 14, wallet.balance(0, true));
        EXPECT_EQ(same_block ? 1 : 2, wallet.get_blockchain_current_height());
        tools::wallet2::transfer_container transfers;
        wallet.get_transfers(transfers);
        EXPECT_EQ(failed_first ? 0 : 2, transfers.size());
        std::list<tools::wallet2::payment_details> payments;
        wallet.get_payments(crypto::null_hash, payments);
        ASSERT_EQ(failed_first ? 0 : 1, payments.size());
        if (!failed_first)
        {
            EXPECT_EQ(confirming_id, payments.front().m_tx_hash);
            EXPECT_EQ(14, payments.front().m_amount);
        }

        if (same_block)
        {
            // Retry the identical block, repairing only the failing transaction's indices.
            fixture.parsed_blocks[1].o_indices.indices[failed_first ? 1 : 2].indices.resize(
                failed_first ? confirming.vout.size() : pending.vout.size());
            wallet_accessor_test::process_confirmation(wallet, fixture);
            EXPECT_EQ(2, wallet.get_blockchain_current_height());
            EXPECT_EQ(0, wallet_accessor_test::pending_payments(wallet, confirming_id));
            EXPECT_EQ(0, wallet_accessor_test::pending_payments(wallet, pending_id));
            EXPECT_EQ(36, wallet.balance(0, false));
            EXPECT_EQ(36, wallet.balance(0, true));
            wallet.get_transfers(transfers);
            EXPECT_EQ(4, transfers.size());
            payments.clear();
            wallet.get_payments(crypto::null_hash, payments);
            // Duplicate-output handling may append zero-amount history; each positive credit must be unique.
            EXPECT_EQ(2, std::count_if(payments.begin(), payments.end(),
                [](const auto &payment) { return payment.m_amount != 0; }));
            EXPECT_EQ(1, std::count_if(payments.begin(), payments.end(),
                [&](const auto &payment) { return payment.m_tx_hash == confirming_id && payment.m_amount == 14; }));
            EXPECT_EQ(1, std::count_if(payments.begin(), payments.end(),
                [&](const auto &payment) { return payment.m_tx_hash == pending_id && payment.m_amount == 22; }));
        }

        // All reorged transactions must be eligible for scanning when they return to the pool.
        wallet_accessor_test::reorg_payment(wallet, confirming_id, same_block ? &pending_id : nullptr);
        wallet.get_transfers(transfers);
        EXPECT_TRUE(transfers.empty());
        payments.clear();
        wallet.get_payments(crypto::null_hash, payments);
        EXPECT_TRUE(payments.empty());
        wallet_accessor_test::process_pool_transaction(wallet, confirming);
        if (same_block)
            wallet_accessor_test::process_pool_transaction(wallet, pending);
        EXPECT_EQ(1, wallet_accessor_test::pending_payments(wallet, confirming_id));
        EXPECT_EQ(same_block || other_pending ? 1 : 0, wallet_accessor_test::pending_payments(wallet, pending_id));
        EXPECT_EQ(same_block ? 36 : expected_balance, wallet.balance(0, false));
        EXPECT_EQ(0, wallet.balance(0, true));
    }
}

TEST(wallet_storage, store_to_file2file)
{
    const path source_wallet_file = unit_test::data_dir / "wallet_00fd416a";
    const path interm_wallet_file = unit_test::data_dir / "wallet_00fd416a_copy_file2file";
    const path target_wallet_file = unit_test::data_dir / "wallet_00fd416a_new_file2file";

    ASSERT_TRUE(is_file_exist(source_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(source_wallet_file.string() + ".keys"));

    tools::copy_file(source_wallet_file.string(), interm_wallet_file.string());
    tools::copy_file(source_wallet_file.string() + ".keys", interm_wallet_file.string() + ".keys");

    ASSERT_TRUE(is_file_exist(interm_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(interm_wallet_file.string() + ".keys"));

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    epee::wipeable_string password("beepbeep");

    const auto files_are_expected = [&]()
    {
        EXPECT_FALSE(is_file_exist(interm_wallet_file.string()));
        EXPECT_FALSE(is_file_exist(interm_wallet_file.string() + ".keys"));
        EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
        EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));
    };

    {
        tools::wallet2 w;
        w.load(interm_wallet_file.string(), password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
        w.store_to(target_wallet_file.string(), password);
        files_are_expected();
    }

    files_are_expected();

    {
        tools::wallet2 w;
        w.load(target_wallet_file.string(), password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
        w.store_to("", "");
        files_are_expected();
    }

    files_are_expected();
}

TEST(wallet_storage, store_to_mem2file)
{
    const path target_wallet_file = unit_test::data_dir / "wallet_mem2file";

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    epee::wipeable_string password("beepbeep2");

    {
        tools::wallet2 w;
        w.generate("", password);
        w.store_to(target_wallet_file.string(), password);

        EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
        EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));
    }

    EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));

    {
        tools::wallet2 w;
        w.load(target_wallet_file.string(), password);

        EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
        EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));
    }

    EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));
}

TEST(wallet_storage, export_key_images_uses_generated_key_image)
{
    const path wallet_file = unit_test::data_dir / "wallet_9svHk1";
    epee::wipeable_string password("test");

    tools::wallet2 w(cryptonote::TESTNET);
    w.load(wallet_file.string(), password);
    tools::wallet_keys_unlocker unlocker(w, &password);

    const auto original = w.export_key_images(true);
    ASSERT_EQ(0, original.first);
    ASSERT_FALSE(original.second.empty());
    const crypto::key_image expected_key_image = original.second.front().first;

    wallet_accessor_test::forget_cached_key_image(w, 0);

    const auto exported = w.export_key_images(false);
    ASSERT_EQ(0, exported.first);
    ASSERT_EQ(original.second.size(), exported.second.size());

    const crypto::key_image &exported_key_image = exported.second.front().first;
    EXPECT_TRUE(expected_key_image == exported_key_image);

    const crypto::public_key pkey = wallet_accessor_test::get_public_key(w, 0);
    std::vector<const crypto::public_key*> key_ptrs;
    key_ptrs.push_back(&pkey);
    EXPECT_TRUE(crypto::check_ring_signature((const crypto::hash&)exported_key_image,
        exported_key_image, key_ptrs.data(), key_ptrs.size(), &exported.second.front().second));
}

TEST(wallet_storage, change_password_same_file)
{
    const path source_wallet_file = unit_test::data_dir / "wallet_00fd416a";
    const path interm_wallet_file = unit_test::data_dir / "wallet_00fd416a_copy_change_password_same";

    ASSERT_TRUE(is_file_exist(source_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(source_wallet_file.string() + ".keys"));

    tools::copy_file(source_wallet_file.string(), interm_wallet_file.string());
    tools::copy_file(source_wallet_file.string() + ".keys", interm_wallet_file.string() + ".keys");

    ASSERT_TRUE(is_file_exist(interm_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(interm_wallet_file.string() + ".keys"));

    epee::wipeable_string old_password("beepbeep");
    epee::wipeable_string new_password("meepmeep");

    {
        tools::wallet2 w;
        w.load(interm_wallet_file.string(), old_password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
        w.change_password(w.get_wallet_file(), old_password, new_password);
    }

    {
        tools::wallet2 w;
        w.load(interm_wallet_file.string(), new_password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
    }

    {
        tools::wallet2 w;
        EXPECT_THROW(w.load(interm_wallet_file.string(), old_password), tools::error::invalid_password);
    }
}

TEST(wallet_storage, change_password_different_file)
{
    const path source_wallet_file = unit_test::data_dir / "wallet_00fd416a";
    const path interm_wallet_file = unit_test::data_dir / "wallet_00fd416a_copy_change_password_diff";
    const path target_wallet_file = unit_test::data_dir / "wallet_00fd416a_new_change_password_diff";

    ASSERT_TRUE(is_file_exist(source_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(source_wallet_file.string() + ".keys"));

    tools::copy_file(source_wallet_file.string(), interm_wallet_file.string());
    tools::copy_file(source_wallet_file.string() + ".keys", interm_wallet_file.string() + ".keys");

    ASSERT_TRUE(is_file_exist(interm_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(interm_wallet_file.string() + ".keys"));

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    epee::wipeable_string old_password("beepbeep");
    epee::wipeable_string new_password("meepmeep");

    {
        tools::wallet2 w;
        w.load(interm_wallet_file.string(), old_password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
        w.change_password(target_wallet_file.string(), old_password, new_password);
    }

    EXPECT_FALSE(is_file_exist(interm_wallet_file.string()));
    EXPECT_FALSE(is_file_exist(interm_wallet_file.string() + ".keys"));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));

    {
        tools::wallet2 w;
        w.load(target_wallet_file.string(), new_password);
        const std::string primary_address = w.get_address_as_str();
        EXPECT_EQ(WALLET_00fd416a_PRIMARY_ADDRESS, primary_address);
    }
}

TEST(wallet_storage, change_password_in_memory)
{
    const epee::wipeable_string password1("monero");
    const epee::wipeable_string password2("means money");
    const epee::wipeable_string password_wrong("is traceable");

    tools::wallet2 w;
    w.generate("", password1);
    const std::string primary_address_1 = w.get_address_as_str();
    w.change_password("", password1, password2);
    const std::string primary_address_2 = w.get_address_as_str();
    EXPECT_EQ(primary_address_1, primary_address_2);

    EXPECT_THROW(w.change_password("", password_wrong, password1), tools::error::invalid_password);
}

TEST(wallet_storage, change_password_mem2file)
{
    const path target_wallet_file = unit_test::data_dir / "wallet_change_password_mem2file";

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    const epee::wipeable_string password1("https://safecurves.cr.yp.to/rigid.html");
    const epee::wipeable_string password2(
        "https://csrc.nist.gov/csrc/media/projects/crypto-standards-development-process/documents/dualec_in_x982_and_sp800-90.pdf");
    
    std::string primary_address_1, primary_address_2;
    {
        tools::wallet2 w;
        w.generate("", password1);
        primary_address_1 = w.get_address_as_str();
        w.change_password(target_wallet_file.string(), password1, password2);
    }

    EXPECT_TRUE(is_file_exist(target_wallet_file.string()));
    EXPECT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));

    {
        tools::wallet2 w;
        w.load(target_wallet_file.string(), password2);
        primary_address_2 = w.get_address_as_str();
    }

    EXPECT_EQ(primary_address_1, primary_address_2);
}

TEST(wallet_storage, gen_ascii_format)
{
    const path target_wallet_file = unit_test::data_dir / "wallet_gen_ascii_format";

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    const epee::wipeable_string password("https://safecurves.cr.yp.to/rigid.html");
    
    std::string primary_address_1, primary_address_2;
    {
        tools::wallet2 w;
        w.set_export_format(tools::wallet2::Ascii);
        ASSERT_EQ(tools::wallet2::Ascii, w.export_format());
        w.generate(target_wallet_file.string(), password);
        primary_address_1 = w.get_address_as_str();
    }

    ASSERT_TRUE(is_file_exist(target_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));

    // Assert that we store keys in ascii format
    {
        std::string key_file_contents;
        ASSERT_TRUE(epee::file_io_utils::load_file_to_string(target_wallet_file.string() + ".keys", key_file_contents));
        EXPECT_NE(std::string::npos, key_file_contents.find(WALLET2_ASCII_OUTPUT_MAGIC));
        for (const char c : key_file_contents)
            ASSERT_TRUE(std::isprint(static_cast<unsigned char>(c)) || c == '\n' || c == '\r');
    }

    {
        tools::wallet2 w;
        w.set_export_format(tools::wallet2::Ascii);
        ASSERT_EQ(tools::wallet2::Ascii, w.export_format());
        w.load(target_wallet_file.string(), password);
        primary_address_2 = w.get_address_as_str();
    }

    EXPECT_EQ(primary_address_1, primary_address_2);
}

TEST(wallet_storage, change_export_format)
{
    const path target_wallet_file = unit_test::data_dir / "wallet_change_export_format";

    if (is_file_exist(target_wallet_file.string()))
        remove(target_wallet_file);
    if (is_file_exist(target_wallet_file.string() + ".keys"))
        remove(target_wallet_file.string() + ".keys");
    ASSERT_FALSE(is_file_exist(target_wallet_file.string()));
    ASSERT_FALSE(is_file_exist(target_wallet_file.string() + ".keys"));

    const epee::wipeable_string password("https://safecurves.cr.yp.to/rigid.html");
    
    std::string primary_address_1, primary_address_2;
    {
        tools::wallet2 w;
        ASSERT_EQ(tools::wallet2::Binary, w.export_format());
        w.generate(target_wallet_file.string(), password);
        primary_address_1 = w.get_address_as_str();
        w.store();

        // Assert that we initially store keys in binary format
        {
            std::string key_file_contents;
            ASSERT_TRUE(w.unlock_keys_file());
            const bool loaded = epee::file_io_utils::load_file_to_string(target_wallet_file.string() + ".keys", key_file_contents);
            ASSERT_TRUE(w.lock_keys_file());
            ASSERT_TRUE(w.is_keys_file_locked());
            ASSERT_TRUE(loaded);
            EXPECT_EQ(std::string::npos, key_file_contents.find(WALLET2_ASCII_OUTPUT_MAGIC));
            bool only_printable = true;
            for (const char c : key_file_contents)
            {
                if (!std::isprint(static_cast<unsigned char>(c)) && c != '\n' && c != '\r')
                {
                    only_printable = false;
                    break;
                }
            }
            EXPECT_FALSE(only_printable);
        }

        // switch formats and store
        w.set_export_format(tools::wallet2::Ascii);
        ASSERT_EQ(tools::wallet2::Ascii, w.export_format());
        w.store_to("", password, /*force_rewrite_keys=*/ true);
    }

    ASSERT_TRUE(is_file_exist(target_wallet_file.string()));
    ASSERT_TRUE(is_file_exist(target_wallet_file.string() + ".keys"));

    // Assert that we store keys in ascii format
    {
        std::string key_file_contents;
        ASSERT_TRUE(epee::file_io_utils::load_file_to_string(target_wallet_file.string() + ".keys", key_file_contents));
        EXPECT_NE(std::string::npos, key_file_contents.find(WALLET2_ASCII_OUTPUT_MAGIC));
        for (const char c : key_file_contents)
            ASSERT_TRUE(std::isprint(static_cast<unsigned char>(c)) || c == '\n' || c == '\r');
    }

    {
        tools::wallet2 w;
        w.set_export_format(tools::wallet2::Ascii);
        ASSERT_EQ(tools::wallet2::Ascii, w.export_format());
        w.load(target_wallet_file.string(), password);
        primary_address_2 = w.get_address_as_str();
    }

    EXPECT_EQ(primary_address_1, primary_address_2);
}

#define OLD_WALLET_KEYS_UNLOCKER 0
#if OLD_WALLET_KEYS_UNLOCKER
#define WALLET_KEYS_UNLOCKER_CTOR(wal, p_pwd) tools::wallet_keys_unlocker(wal, \
    p_pwd ? tools::password_container(*static_cast<const epee::wipeable_string*>(p_pwd)) \
    : boost::optional<tools::password_container>{})
#else
#define WALLET_KEYS_UNLOCKER_CTOR tools::wallet_keys_unlocker
#endif

static bool verify_wallet_privkeys(const tools::wallet2 &w)
{
    hw::device &hwdev = hw::get_device("default");
    const cryptonote::account_keys &keys = w.get_account().get_keys();
    return hwdev.verify_keys(keys.m_spend_secret_key, keys.m_account_address.m_spend_public_key)
        && hwdev.verify_keys(keys.m_view_secret_key, keys.m_account_address.m_view_public_key);
}

TEST(wallet_keys_unlocker, is_key_encryption_enabled)
{
    const epee::wipeable_string password1("Beleza pura, malandro!");
    const epee::wipeable_string password2("correct horse battery staple");
    {
        tools::wallet2 w;
        w.generate("", password1);
        ASSERT_TRUE(w.is_key_encryption_enabled());
    }
    {
        tools::wallet2 w(cryptonote::MAINNET, /*kdf_rounds=*/1, /*unattended=*/true);
        w.generate("", password1);
        ASSERT_FALSE(w.is_key_encryption_enabled()); // because unattended
    }
    {
        tools::wallet2 w_cold;
        w_cold.generate("", password1);
        ASSERT_TRUE(w_cold.is_key_encryption_enabled());
        tools::wallet2 w_hot;
        w_hot.generate("", password2,
            w_cold.get_address(),
            w_cold.get_account().get_keys().m_view_secret_key);
        ASSERT_FALSE(w_hot.is_key_encryption_enabled()); // because watch only
    }
    {
        const path bg_wallet_file = unit_test::data_dir / "is_key_encryption_enabled_bg1";
        if (is_file_exist(bg_wallet_file.string()))
            remove(bg_wallet_file);
        if (is_file_exist(bg_wallet_file.string() + ".keys"))
            remove(bg_wallet_file.string() + ".keys");

        tools::wallet2 w;
        w.generate(bg_wallet_file.string(), password1);
        ASSERT_TRUE(w.is_key_encryption_enabled());
        w.setup_background_sync(tools::wallet2::BackgroundSyncReusePassword, password1, boost::none);
        ASSERT_FALSE(w.is_background_syncing());
        w.start_background_sync();
        ASSERT_TRUE(w.is_background_syncing());
        ASSERT_FALSE(verify_wallet_privkeys(w));
        ASSERT_FALSE(w.is_key_encryption_enabled()); // because background syncing
    }
    {
        const path bg_wallet_file = unit_test::data_dir / "is_key_encryption_enabled_bg2";
        if (is_file_exist(bg_wallet_file.string()))
            remove(bg_wallet_file);
        if (is_file_exist(bg_wallet_file.string() + ".keys"))
            remove(bg_wallet_file.string() + ".keys");

        tools::wallet2 w;
        w.generate(bg_wallet_file.string(), password1);
        ASSERT_TRUE(w.is_key_encryption_enabled());
        w.setup_background_sync(tools::wallet2::BackgroundSyncCustomPassword, password1, password2);
        ASSERT_FALSE(w.is_background_syncing());
        w.start_background_sync();
        ASSERT_TRUE(w.is_background_syncing());
        ASSERT_FALSE(verify_wallet_privkeys(w));
        ASSERT_FALSE(w.is_key_encryption_enabled()); // because background syncing
    }
}

TEST(wallet_keys_unlocker, simple_nonce)
{
    // Test that encrypted keys are different each time, i.e. that a nonce may actually be used

    const epee::wipeable_string password("1612");
    tools::wallet2 w;
    w.generate("", password);
    ASSERT_TRUE(w.is_key_encryption_enabled());
    ASSERT_FALSE(w.is_unattended());
    ASSERT_FALSE(verify_wallet_privkeys(w));
    std::unordered_set<crypto::secret_key> encrypted_spendkeys;
    encrypted_spendkeys.insert(w.get_account().get_keys().m_spend_secret_key);

    const int n_locks = 10;
    for (int i = 0; i < n_locks; ++i)
    {
        {
            tools::wallet_keys_unlocker ul(w, &password);
        }
        const crypto::secret_key enc_spendkey = w.get_account().get_keys().m_spend_secret_key;
        ASSERT_FALSE(encrypted_spendkeys.count(enc_spendkey));
        encrypted_spendkeys.insert(enc_spendkey);
    }

    ASSERT_EQ(n_locks + 1, encrypted_spendkeys.size());
}

TEST(wallet_keys_unlocker, mutiple_attended)
{
    // Test that multiple non-unattended wallets can be decrypted at the same time.

    const epee::wipeable_string password1("https://www.justice.gov/archives/opa/pr/bitcoin-fog-operator-convicted-money-laundering-conspiracy");
    const epee::wipeable_string password2("https://cointelegraph.com/news/bad-blockchain-forensics-convict-roman-sterlingov");

    tools::wallet2 w1;
    w1.generate("", password1);
    ASSERT_TRUE(w1.is_key_encryption_enabled());
    ASSERT_FALSE(w1.is_unattended());
    ASSERT_FALSE(verify_wallet_privkeys(w1));
    const crypto::secret_key w1_ks_encrypted = w1.get_account().get_keys().m_spend_secret_key;
    const crypto::public_key w1_spend_pubkey = w1.get_account().get_keys().m_account_address.m_spend_public_key;

    tools::wallet2 w2;
    w2.generate("", password2);
    ASSERT_TRUE(w2.is_key_encryption_enabled());
    ASSERT_FALSE(w2.is_unattended());
    ASSERT_FALSE(verify_wallet_privkeys(w2));
    const crypto::secret_key w2_ks_encrypted = w2.get_account().get_keys().m_spend_secret_key;
    const crypto::public_key w2_spend_pubkey = w2.get_account().get_keys().m_account_address.m_spend_public_key;

    ASSERT_NE(w1_ks_encrypted, w2_ks_encrypted);
    ASSERT_NE(w1_spend_pubkey, w2_spend_pubkey);

    crypto::secret_key w1_ks_unencrypted;
    crypto::secret_key w2_ks_unencrypted;
    for (size_t i = 0; i < 2; ++i)
    {
        tools::wallet_keys_unlocker ul1 = WALLET_KEYS_UNLOCKER_CTOR(w1, &password1);
        w1_ks_unencrypted = w1.get_account().get_keys().m_spend_secret_key;
        ASSERT_NE(w1_ks_encrypted, w1_ks_unencrypted);
        EXPECT_TRUE(verify_wallet_privkeys(w1));

        tools::wallet_keys_unlocker ul2 = WALLET_KEYS_UNLOCKER_CTOR(w2, &password2);
        w2_ks_unencrypted = w2.get_account().get_keys().m_spend_secret_key;
        ASSERT_NE(w2_ks_encrypted, w2_ks_unencrypted);
        EXPECT_TRUE(verify_wallet_privkeys(w2));
    }

    ASSERT_NE(w1_ks_unencrypted, w1.get_account().get_keys().m_spend_secret_key);
    ASSERT_NE(w2_ks_unencrypted, w2.get_account().get_keys().m_spend_secret_key);
}

TEST(wallet_keys_unlocker, non_concentric_lifetime)
{
    // Test that wallet unlock-ers which aren't concentric still keep the wallet decrypted as
    // long as one of them is alive.

    const epee::wipeable_string password1("540fa389d7cf4476b061f6443215583d739b01b5d7d9b972a9c0600084bb3694");

    tools::wallet2 w1;
    w1.generate("", password1);
    ASSERT_TRUE(w1.is_key_encryption_enabled());
    ASSERT_FALSE(w1.is_unattended());
    ASSERT_FALSE(verify_wallet_privkeys(w1));
    const crypto::secret_key w1_ks_encrypted = w1.get_account().get_keys().m_spend_secret_key;

    std::unique_ptr<tools::wallet_keys_unlocker> ul1(new WALLET_KEYS_UNLOCKER_CTOR(w1, &password1));
    const crypto::secret_key w1_ks_unencrypted_1 = w1.get_account().get_keys().m_spend_secret_key;
    ASSERT_NE(w1_ks_encrypted, w1_ks_unencrypted_1);
    ASSERT_TRUE(verify_wallet_privkeys(w1));

    std::unique_ptr<tools::wallet_keys_unlocker> ul2(new WALLET_KEYS_UNLOCKER_CTOR(w1, &password1));
    const crypto::secret_key w1_ks_unencrypted_2 = w1.get_account().get_keys().m_spend_secret_key;
    ASSERT_EQ(w1_ks_unencrypted_1, w1_ks_unencrypted_2);

    ul1.reset(); // call ul1 destructor before ul2 destructor

    const crypto::secret_key w1_ks_unencrypted_3 = w1.get_account().get_keys().m_spend_secret_key;
    ASSERT_EQ(w1_ks_unencrypted_1, w1_ks_unencrypted_3);

    ul2.reset(); // call ul2 destructor

    ASSERT_NE(w1_ks_unencrypted_1, w1.get_account().get_keys().m_spend_secret_key);
    ASSERT_NE(w1_ks_encrypted, w1.get_account().get_keys().m_spend_secret_key); // should use a unique nonce

    // test that wallets were re-encrypted correctly & recoverable after non-concentric dtors
    ASSERT_FALSE(verify_wallet_privkeys(w1));
    tools::wallet_keys_unlocker ul3(w1, &password1);
    ASSERT_TRUE(verify_wallet_privkeys(w1));
}

TEST(wallet_keys_unlocker, first_not_locked)
{
    // Test that if the first unlock-er is disabled, then subsequent unlock-ers decrypt successfully

    const epee::wipeable_string password1("Ashigaru");

    tools::wallet2 w1;
    w1.generate("", password1);
    ASSERT_TRUE(w1.is_key_encryption_enabled());
    ASSERT_FALSE(w1.is_unattended());
    ASSERT_FALSE(verify_wallet_privkeys(w1));
    const crypto::secret_key w1_ks_encrypted = w1.get_account().get_keys().m_spend_secret_key;

    {
        tools::wallet_keys_unlocker ul1 = WALLET_KEYS_UNLOCKER_CTOR(w1, nullptr);
        const crypto::secret_key w1_ks_encrypted_2 = w1.get_account().get_keys().m_spend_secret_key;
        ASSERT_EQ(w1_ks_encrypted_2, w1_ks_encrypted); // is disabled ?

        tools::wallet_keys_unlocker ul2 = WALLET_KEYS_UNLOCKER_CTOR(w1, &password1);
        const crypto::secret_key w1_ks_unencrypted = w1.get_account().get_keys().m_spend_secret_key;
        ASSERT_NE(w1_ks_encrypted_2, w1_ks_unencrypted);
        ASSERT_TRUE(verify_wallet_privkeys(w1));
    }
}

TEST(wallet_keys_unlocker, construction_failure_rolls_back_lock_count)
{
    const epee::wipeable_string password("correct horse battery staple");
    const epee::wipeable_string wrong_password("correct horse battery stable");

    tools::wallet2 w;
    w.generate("", password);
    ASSERT_TRUE(w.is_key_encryption_enabled());
    ASSERT_FALSE(w.is_unattended());
    ASSERT_FALSE(verify_wallet_privkeys(w));

    ASSERT_ANY_THROW({
        tools::wallet_keys_unlocker ul(w, &wrong_password);
    });
    ASSERT_FALSE(verify_wallet_privkeys(w));

    {
        tools::wallet_keys_unlocker ul(w, &password);
        ASSERT_TRUE(verify_wallet_privkeys(w));
    }
    ASSERT_FALSE(verify_wallet_privkeys(w));
}
