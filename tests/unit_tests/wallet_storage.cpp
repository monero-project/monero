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
#include <functional>

#include "file_io_utils.h"
#include "wallet/wallet2.h"
#include "common/util.h"

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
    static tools::wallet2::unconfirmed_transfer_details &unconfirmed_tx(tools::wallet2 &wallet, const crypto::hash &txid)
    {
        return wallet.m_unconfirmed_txs[txid];
    }

    static const tools::wallet2::confirmed_transfer_details &confirm_outgoing(tools::wallet2 &wallet,
        const crypto::hash &txid, const cryptonote::transaction &tx, uint64_t spent, uint64_t received,
        const std::set<uint32_t> &indices)
    {
        wallet.process_unconfirmed(txid, tx, 0);
        wallet.process_outgoing(txid, tx, 0, 200, spent, received, 0, indices);
        return wallet.m_confirmed_txs.at(txid);
    }

    static void scan_transaction(tools::wallet2 &wallet, const cryptonote::transaction &tx)
    {
        wallet.process_new_transaction(cryptonote::get_transaction_hash(tx), tx,
            std::vector<uint64_t>(tx.vout.size()), 1, 1, 1, false, false, false, {});
    }

    static void detach_incoming(tools::wallet2 &wallet)
    {
        wallet.detach_blockchain(1);
    }

    static uint64_t pool_query_time(const tools::wallet2 &wallet)
    {
        return wallet.m_pool_info_query_time;
    }

    static cryptonote::block genesis(tools::wallet2 &wallet)
    {
        cryptonote::block block;
        wallet.generate_genesis(block);
        return block;
    }

    static void prepare_refresh(tools::wallet2 &wallet)
    {
        wallet.set_offline(true);
        wallet.generate("", "");
        wallet.set_offline(false);
        wallet.set_refresh_from_block_height(0);
        wallet.m_first_refresh_done = true;
        wallet.m_pool_info_query_time = 1;
    }

    static const tools::wallet2::transfer_details &add_spent_output(tools::wallet2 &wallet, cryptonote::transaction &tx)
    {
        cryptonote::txin_to_key input = AUTO_VAL_INIT(input);
        input.k_image = crypto::rand<crypto::key_image>();
        tx.vin.push_back(input);
        wallet.m_transfers.emplace_back();
        auto &transfer = wallet.m_transfers.back();
        transfer.m_amount = 9;
        transfer.m_spent = true;
        transfer.m_spent_height = 100;
        transfer.m_key_image = input.k_image;
        transfer.m_key_image_known = true;
        transfer.m_subaddr_index = {0, 0};
        wallet.m_key_images[input.k_image] = wallet.m_transfers.size() - 1;
        return transfer;
    }
};

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

TEST(wallet_pool, reconciles_partial_outgoing_on_confirmation)
{
    tools::wallet2 w;
    cryptonote::transaction tx;
    tx.version = 2;
    tx.rct_signatures.txnFee = 1;
    const crypto::hash txid = crypto::null_hash;
    auto &pending = wallet_accessor_test::unconfirmed_tx(w, txid);
    pending.m_tx = tx;
    pending.m_amount_in = 4;
    pending.m_amount_out = 3;
    pending.m_change = 2;
    pending.m_subaddr_account = 0;
    pending.m_subaddr_indices = {0};
    pending.m_dests.resize(1);
    pending.m_dests[0].amount = 6;
    pending.m_payment_id = crypto::rand<crypto::hash>();
    const crypto::hash payment_id = pending.m_payment_id;

    const std::set<uint32_t> indices = {1};
    const auto &confirmed = wallet_accessor_test::confirm_outgoing(w, txid, tx, 9, 2, indices);
    EXPECT_EQ(9, confirmed.m_amount_in);
    EXPECT_EQ(8, confirmed.m_amount_out);
    EXPECT_EQ(6, confirmed.m_amount_out - confirmed.m_change);
    EXPECT_EQ(std::set<uint32_t>({0, 1}), confirmed.m_subaddr_indices);
    ASSERT_EQ(1, confirmed.m_dests.size());
    EXPECT_EQ(6, confirmed.m_dests[0].amount);
    EXPECT_EQ(payment_id, confirmed.m_payment_id);

    // A later scan with fewer known key images must preserve the complete totals.
    wallet_accessor_test::confirm_outgoing(w, txid, tx, 4, 2, {0});
    EXPECT_EQ(9, confirmed.m_amount_in);
    EXPECT_EQ(8, confirmed.m_amount_out);
    EXPECT_EQ(2, confirmed.m_change);
}

TEST(wallet_pool, skips_confirmed_outgoing_in_stale_snapshot)
{
    tools::wallet2 w(cryptonote::MAINNET, 1, true);
    w.set_offline(true);
    w.generate("", "");
    cryptonote::transaction tx;
    tx.version = 2;
    tx.rct_signatures.txnFee = 1;
    const crypto::hash txid = crypto::null_hash;
    const auto &transfer = wallet_accessor_test::add_spent_output(w, tx);
    wallet_accessor_test::confirm_outgoing(w, txid, tx, 9, 2, {0});
    w.process_pool_state({std::make_tuple(tx, txid, false)});
    EXPECT_EQ(100, transfer.m_spent_height);
    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> pending;
    w.get_unconfirmed_payments_out(pending);
    EXPECT_TRUE(pending.empty());
}

namespace
{
    cryptonote::transaction make_pool_payment(const tools::wallet2 &wallet, size_t outputs = 1)
    {
        cryptonote::transaction tx;
        tx.version = 1;
        cryptonote::txin_to_key input = AUTO_VAL_INIT(input);
        input.amount = 8 * outputs + 1;
        input.k_image = crypto::rand<crypto::key_image>();
        input.key_offsets = {0};
        tx.vin.push_back(input);
        const auto &address = wallet.get_account().get_keys().m_account_address;
        crypto::public_key public_key;
        crypto::secret_key secret_key;
        crypto::generate_keys(public_key, secret_key);
        EXPECT_TRUE(cryptonote::add_tx_pub_key_to_extra(tx, public_key));
        crypto::key_derivation derivation;
        EXPECT_TRUE(crypto::generate_key_derivation(address.m_view_public_key, secret_key, derivation));
        for (size_t i = 0; i < outputs; ++i)
        {
            cryptonote::txout_to_key output;
            EXPECT_TRUE(crypto::derive_public_key(derivation, i, address.m_spend_public_key, output.key));
            tx.vout.push_back({8, output});
        }
        return tx;
    }

    class pool_callback : public tools::i_wallet2_callback
    {
    public:
        size_t pool_receipts = 0;
        size_t password_requests = 0;
        std::function<void()> on_pool_receipt;
        void on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid,
            const cryptonote::transaction &tx, uint64_t amount,
            const cryptonote::subaddress_index &index) override
        {
            ++pool_receipts;
            if (on_pool_receipt)
                on_pool_receipt();
        }

        boost::optional<epee::wipeable_string> on_get_password(const char *reason) override
        {
            ++password_requests;
            return boost::none;
        }
    };
}

TEST(wallet_pool, skips_confirmed_incoming_in_stale_snapshot)
{
    tools::wallet2 w(cryptonote::MAINNET, 1, true);
    w.set_offline(true);
    w.generate("", "");
    const auto tx = make_pool_payment(w);
    const auto txid = cryptonote::get_transaction_hash(tx);
    wallet_accessor_test::scan_transaction(w, tx);
    ASSERT_EQ(8, w.balance_all(true));
    pool_callback callback;
    w.callback(&callback);

    w.process_pool_state({std::make_tuple(tx, txid, false)});
    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pending;
    w.get_unconfirmed_payments(pending);
    EXPECT_TRUE(pending.empty());
    EXPECT_EQ(0, callback.pool_receipts);
    EXPECT_EQ(8, w.balance_all(true));
}

TEST(wallet_pool, processes_incoming_with_reused_output_key)
{
    tools::wallet2 w(cryptonote::MAINNET, 1, true);
    w.set_offline(true);
    w.generate("", "");
    const auto pool_tx = make_pool_payment(w, 2);
    auto confirmed_tx = pool_tx;
    confirmed_tx.vout.pop_back();
    wallet_accessor_test::scan_transaction(w, confirmed_tx);
    ASSERT_EQ(8, w.balance_all(true));
    const auto txid = cryptonote::get_transaction_hash(pool_tx);
    ASSERT_NE(cryptonote::get_transaction_hash(confirmed_tx), txid);
    pool_callback callback;
    w.callback(&callback);

    w.process_pool_state({std::make_tuple(pool_tx, txid, false)});
    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pending;
    w.get_unconfirmed_payments(pending);
    ASSERT_EQ(1, pending.size());
    EXPECT_EQ(txid, pending.front().second.m_pd.m_tx_hash);
    EXPECT_EQ(8, pending.front().second.m_pd.m_amount);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(8, w.balance_all(true));
}

TEST(wallet_pool, processes_incoming_after_detach)
{
    tools::wallet2 w(cryptonote::MAINNET, 1, true);
    w.set_offline(true);
    w.generate("", "");
    const auto tx = make_pool_payment(w);
    const auto txid = cryptonote::get_transaction_hash(tx);
    wallet_accessor_test::scan_transaction(w, tx);
    ASSERT_EQ(8, w.balance_all(true));
    wallet_accessor_test::detach_incoming(w);
    ASSERT_EQ(0, w.balance_all(true));
    pool_callback callback;
    w.callback(&callback);

    w.process_pool_state({std::make_tuple(tx, txid, false)});
    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pending;
    w.get_unconfirmed_payments(pending);
    ASSERT_EQ(1, pending.size());
    EXPECT_EQ(txid, pending.front().second.m_pd.m_tx_hash);
    EXPECT_EQ(8, pending.front().second.m_pd.m_amount);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(0, w.balance_all(true));
}

namespace
{
    using pool_reply = std::function<void(const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &)>;

    class pool_http_client : public net::http::client
    {
        epee::net_utils::http::http_response_info response;
        pool_reply reply;

    public:
        explicit pool_http_client(const pool_reply &reply) : reply(reply) {}

        bool invoke(const boost::string_ref uri, const boost::string_ref method, const boost::string_ref body,
            std::chrono::milliseconds timeout, const epee::net_utils::http::http_response_info **result,
            const epee::net_utils::http::fields_list &headers) override
        {
            EXPECT_EQ("/getblocks.bin", uri);
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request request = AUTO_VAL_INIT(request);
            EXPECT_TRUE(epee::serialization::load_t_from_binary(request, std::string(body.data(), body.size())));
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response pool = AUTO_VAL_INIT(pool);
            pool.status = CORE_RPC_STATUS_OK;
            pool.pool_info_extent = request.pool_info_since
                ? cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::INCREMENTAL
                : cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::FULL;
            pool.daemon_time = 2;
            reply(request, pool);
            response.m_response_code = 200;
            const auto blob = epee::serialization::store_t_to_binary(pool);
            response.m_body.assign(reinterpret_cast<const char *>(blob.data()), blob.size());
            *result = &response;
            return true;
        }
    };

    class pool_http_client_factory : public epee::net_utils::http::http_client_factory
    {
        pool_reply reply;

    public:
        explicit pool_http_client_factory(const pool_reply &reply) : reply(reply) {}

        std::unique_ptr<epee::net_utils::http::abstract_http_client> create() override
        {
            return std::unique_ptr<epee::net_utils::http::abstract_http_client>(new pool_http_client(reply));
        }
    };

    cryptonote::block make_refresh_block(uint64_t height, const crypto::hash &previous,
        const cryptonote::account_public_address &address)
    {
        cryptonote::block block;
        block.major_version = 1;
        block.minor_version = 1;
        block.timestamp = std::time(nullptr);
        block.prev_id = previous;
        EXPECT_TRUE(cryptonote::construct_miner_tx(height, 0, 0, 0, 0, address,
            block.miner_tx));
        return block;
    }

    void add_refresh_block(cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response,
        const cryptonote::block &block)
    {
        if (response.blocks.empty())
            response.start_height = cryptonote::get_block_height(block);
        response.blocks.emplace_back();
        response.blocks.back().block = cryptonote::block_to_blob(block);
        response.output_indices.emplace_back();
        response.output_indices.back().indices.resize(1);
        response.output_indices.back().indices[0].indices.resize(block.miner_tx.vout.size());
    }

    void add_pool_payment(cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response,
        const cryptonote::transaction &tx)
    {
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::pool_tx_info info = AUTO_VAL_INIT(info);
        info.tx_hash = cryptonote::get_transaction_hash(tx);
        info.tx_blob = cryptonote::tx_to_blob(tx);
        response.added_pool_txs.push_back(info);
    }
}

TEST(wallet_pool, commits_refresh_cursor_after_pool_processing)
{
    cryptonote::transaction pool_tx;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        EXPECT_EQ(cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::BLOCKS_AND_POOL, request.requested_info);
        EXPECT_EQ(1, request.pool_info_since);
        response.start_height = response.current_height = 1;
        add_pool_payment(response, pool_tx);
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, true,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet_accessor_test::prepare_refresh(w);
    pool_tx = make_pool_payment(w);
    pool_callback callback;
    callback.on_pool_receipt = [&] { EXPECT_EQ(1, wallet_accessor_test::pool_query_time(w)); };
    w.callback(&callback);

    uint64_t fetched = 0;
    bool received = false;
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(2, wallet_accessor_test::pool_query_time(w));
}

TEST(wallet_pool, retries_pool_after_stopped_refresh)
{
    tools::wallet2 *wallet = nullptr;
    size_t queries = 0;
    cryptonote::transaction pool_tx;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        EXPECT_EQ(1, request.pool_info_since);
        response.start_height = response.current_height = 1;
        add_pool_payment(response, pool_tx);
        if (queries++ == 0)
            wallet->stop();
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, true,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet = &w;
    wallet_accessor_test::prepare_refresh(w);
    pool_tx = make_pool_payment(w);
    pool_callback callback;
    w.callback(&callback);

    uint64_t fetched = 0;
    bool received = false;
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(0, callback.pool_receipts);
    EXPECT_EQ(1, wallet_accessor_test::pool_query_time(w));
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(2, queries);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(2, wallet_accessor_test::pool_query_time(w));
}

TEST(wallet_pool, retries_pool_after_password_failure)
{
    size_t queries = 0;
    cryptonote::transaction pool_tx;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        EXPECT_EQ(1, request.pool_info_since);
        ++queries;
        response.start_height = response.current_height = 1;
        add_pool_payment(response, pool_tx);
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, false,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet_accessor_test::prepare_refresh(w);
    pool_tx = make_pool_payment(w);
    pool_callback callback;
    w.callback(&callback);

    uint64_t fetched = 0;
    bool received = false;
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(1, callback.password_requests);
    EXPECT_EQ(0, callback.pool_receipts);
    EXPECT_EQ(1, wallet_accessor_test::pool_query_time(w));
    w.decrypt_keys(epee::wipeable_string(""));
    w.ask_password(tools::wallet2::AskPasswordNever);
    w.refresh(true, 0, fetched, received);
    EXPECT_EQ(2, queries);
    EXPECT_EQ(1, callback.pool_receipts);
    EXPECT_EQ(2, wallet_accessor_test::pool_query_time(w));
}

TEST(wallet_pool, advances_skipped_pool_before_block_password_failure)
{
    for (const bool check_pool : {false, true})
    {
        size_t queries = 0;
        std::vector<cryptonote::block> blocks;
        const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
        {
            EXPECT_EQ(cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::BLOCKS_AND_POOL, request.requested_info);
            EXPECT_EQ(queries == 0 || check_pool ? 1 : 2, request.pool_info_since);
            ++queries;
            response.current_height = blocks.size();
            for (const auto &block : blocks)
                add_refresh_block(response, block);
        };
        tools::wallet2 w(cryptonote::MAINNET, 1, false,
            std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
        wallet_accessor_test::prepare_refresh(w);
        blocks.push_back(wallet_accessor_test::genesis(w));
        blocks.push_back(make_refresh_block(1, cryptonote::get_block_hash(blocks[0]),
            w.get_account().get_keys().m_account_address));
        pool_callback callback;
        w.callback(&callback);

        uint64_t fetched = 0;
        bool received = false;
        EXPECT_THROW(w.refresh(true, 0, fetched, received, check_pool), tools::error::password_needed);
        EXPECT_EQ(check_pool ? 1 : 2, wallet_accessor_test::pool_query_time(w));
        EXPECT_THROW(w.refresh(true, 0, fetched, received, check_pool), tools::error::password_needed);
        EXPECT_EQ(2, queries);
        EXPECT_EQ(2, callback.password_requests);
        EXPECT_EQ(0, callback.pool_receipts);
    }
}

TEST(wallet_pool, preserves_daemon_reset_during_pool_processing)
{
    for (const bool switch_back : {false, true})
    {
        size_t queries = 0;
        cryptonote::transaction pool_tx;
        const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
            cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
        {
            EXPECT_EQ(queries++ == 0 ? 1 : 0, request.pool_info_since);
            response.start_height = response.current_height = 1;
            add_pool_payment(response, pool_tx);
        };
        tools::wallet2 w(cryptonote::MAINNET, 1, true,
            std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
        ASSERT_TRUE(w.set_daemon("http://127.0.0.1:18081"));
        wallet_accessor_test::prepare_refresh(w);
        pool_tx = make_pool_payment(w);
        pool_callback callback;
        callback.on_pool_receipt = [&]
        {
            EXPECT_TRUE(w.set_daemon("http://127.0.0.1:28081"));
            if (switch_back)
                EXPECT_TRUE(w.set_daemon("http://127.0.0.1:18081"));
        };
        w.callback(&callback);

        uint64_t fetched = 0;
        bool received = false;
        w.refresh(true, 0, fetched, received);
        EXPECT_EQ(1, callback.pool_receipts);
        EXPECT_EQ(0, wallet_accessor_test::pool_query_time(w));
        callback.on_pool_receipt = nullptr;
        w.refresh(true, 0, fetched, received);
        EXPECT_EQ(2, queries);
        EXPECT_EQ(2, wallet_accessor_test::pool_query_time(w));
    }
}

TEST(wallet_pool, preserves_pool_membership_when_blobs_are_malformed)
{
    size_t queries = 0;
    cryptonote::transaction pool_tx, outgoing_tx;
    std::vector<cryptonote::block> blocks;
    const auto reply = [&](const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request &request,
        cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &response)
    {
        response.pool_info_extent = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::FULL;
        response.start_height = response.current_height = blocks.size();
        if (queries == 0)
            for (const auto &block : blocks)
                add_refresh_block(response, block);
        add_pool_payment(response, pool_tx);
        add_pool_payment(response, outgoing_tx);
        if (queries++ == 1)
            for (auto &info : response.added_pool_txs)
                info.tx_blob.clear();
        response.added_pool_txs.emplace_back();
    };
    tools::wallet2 w(cryptonote::MAINNET, 1, true,
        std::unique_ptr<epee::net_utils::http::http_client_factory>(new pool_http_client_factory(reply)));
    wallet_accessor_test::prepare_refresh(w);
    pool_tx = make_pool_payment(w);
    outgoing_tx.version = 1;
    wallet_accessor_test::add_spent_output(w, outgoing_tx);
    const auto outgoing_txid = cryptonote::get_transaction_hash(outgoing_tx);
    auto &pending_out = wallet_accessor_test::unconfirmed_tx(w, outgoing_txid);
    pending_out.m_tx = outgoing_tx;
    pending_out.m_sent_time = 0;
    blocks.push_back(wallet_accessor_test::genesis(w));
    blocks.push_back(make_refresh_block(1, cryptonote::get_block_hash(blocks[0]),
        w.get_account().get_keys().m_account_address));
    pool_callback callback;
    w.callback(&callback);

    for (size_t i = 0; i < 3; ++i)
    {
        uint64_t fetched = 0;
        bool received = false;
        w.refresh(true, 0, fetched, received);
        EXPECT_EQ(i + 1, queries);
        EXPECT_EQ(i == 0 ? 1 : 0, fetched);
        EXPECT_EQ(cryptonote::get_outs_money_amount(blocks[1].miner_tx), w.balance_all(true));
        EXPECT_EQ(1, callback.pool_receipts);
        EXPECT_EQ(2, wallet_accessor_test::pool_query_time(w));
        std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pending;
        w.get_unconfirmed_payments(pending);
        ASSERT_EQ(1, pending.size());
        EXPECT_EQ(8, pending.front().second.m_pd.m_amount);
        EXPECT_EQ(tools::wallet2::unconfirmed_transfer_details::pending_in_pool, pending_out.m_state);
        tools::wallet2::transfer_container transfers;
        w.get_transfers(transfers);
        EXPECT_TRUE(transfers.front().m_spent);
    }
}

TEST(wallet_pool, restores_pending_from_unappended_block)
{
    for (const bool with_change : {false, true})
    {
        tools::wallet2 w(cryptonote::MAINNET, 1, true);
        w.set_offline(true);
        w.generate("", "");
        auto tx = make_pool_payment(w, with_change ? 1 : 0);
        tx.vin.clear();
        wallet_accessor_test::add_spent_output(w, tx);
        boost::get<cryptonote::txin_to_key>(tx.vin[0]).amount = 9;
        const auto txid = cryptonote::get_transaction_hash(tx);
        wallet_accessor_test::unconfirmed_tx(w, txid).m_tx = tx;
        wallet_accessor_test::scan_transaction(w, tx);
        ASSERT_EQ(1, w.get_blockchain_current_height());
        std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> pending;
        w.get_unconfirmed_payments_out(pending);
        ASSERT_TRUE(pending.empty());

        w.process_pool_state({std::make_tuple(tx, txid, false)});
        w.get_unconfirmed_payments_out(pending);
        ASSERT_EQ(1, pending.size());
        EXPECT_EQ(txid, pending.front().first);
        std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> incoming;
        w.get_unconfirmed_payments(incoming);
        EXPECT_TRUE(incoming.empty());
    }
}
