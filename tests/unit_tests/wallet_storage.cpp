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

#include "unit_tests_utils.h"
#include "gtest/gtest.h"

#include <cctype>

#include "file_io_utils.h"
#include "wallet/wallet2.h"
#include "common/util.h"

using namespace boost::filesystem;
using namespace epee::file_io_utils;

static constexpr const char WALLET_00fd416a_PRIMARY_ADDRESS[] =
    "45p2SngJAPSJbqSiUvYfS3BfhEdxZmv8pDt25oW1LzxrZv9Uq6ARagiFViMGUE3gJk5VPWingCXVf1p2tyAy6SUeSHPhbve";

// https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L156
static constexpr const char WALLET2_ASCII_OUTPUT_MAGIC[] = "MoneroAsciiDataV1";

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
            ASSERT_TRUE(std::isprint(c) || c == '\n' || c == '\r');
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
            ASSERT_TRUE(epee::file_io_utils::load_file_to_string(target_wallet_file.string() + ".keys", key_file_contents));
            EXPECT_EQ(std::string::npos, key_file_contents.find(WALLET2_ASCII_OUTPUT_MAGIC));
            bool only_printable = true;
            for (const char c : key_file_contents)
            {
                if (!std::isprint(c) && c != '\n' && c != '\r')
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
            ASSERT_TRUE(std::isprint(c) || c == '\n' || c == '\r');
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
