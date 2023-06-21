// Copyright (c) 2023, The Monero Project
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

#include "unit_tests_utils.h"
#include "wallet/wallet2_basic/wallet2_storage.h"

TEST(wallet2_storage, read_old_wallet)
{
    const boost::filesystem::path wallet_file = unit_test::data_dir / "wallet_9svHk1";
    const epee::wipeable_string password = "test";

    wallet2_basic::cache c;
    wallet2_basic::keys_data k;
    wallet2_basic::load_keys_and_cache_from_file(wallet_file.string(), password, c, k);

    /*
    This test suite is adapated from unit test Serialization.portability_wallet
    Cache fields to be checked:
        std::vector<crypto::hash>                                       m_blockchain
        std::vector<transfer_details>                                   m_transfers
        cryptonote::account_public_address                              m_account_public_address
        std::unordered_map<crypto::key_image, size_t>                   m_key_images
        std::unordered_map<crypto::hash, unconfirmed_transfer_details>  m_unconfirmed_txs
        std::unordered_multimap<crypto::hash, payment_details>          m_payments
        std::unordered_map<crypto::hash, crypto::secret_key>            m_tx_keys
        std::unordered_map<crypto::hash, confirmed_transfer_details>    m_confirmed_txs
        std::unordered_map<crypto::hash, std::string>                   m_tx_notes
        std::unordered_map<crypto::hash, payment_details>               m_unconfirmed_payments
        std::unordered_map<crypto::public_key, size_t>                  m_pub_keys
        std::vector<tools::wallet2::address_book_row>                   m_address_book
    */

    // blockchain
    EXPECT_TRUE(c.m_blockchain.size() == 1);
    EXPECT_TRUE(epee::string_tools::pod_to_hex(c.m_blockchain[0]) == "48ca7cd3c8de5b6a4d53d2861fbdaedca141553559f9be9520068053cda8430b");
    // transfers (TODO)
    EXPECT_TRUE(c.m_transfers.size() == 3);
    // account public address
    EXPECT_TRUE(epee::string_tools::pod_to_hex(c.m_account_public_address.m_view_public_key) == "e47d4b6df6ab7339539148c2a03ad3e2f3434e5ab2046848e1f21369a3937cad");
    EXPECT_TRUE(epee::string_tools::pod_to_hex(c.m_account_public_address.m_spend_public_key) == "13daa2af00ad26a372d317195de0bdd716f7a05d33bc4d7aff1664b6ee93c060");
    // key images
    ASSERT_TRUE(c.m_key_images.size() == 3);
    {
        crypto::key_image ki[3];
        epee::string_tools::hex_to_pod("c5680d3735b90871ca5e3d90cd82d6483eed1151b9ab75c2c8c3a7d89e00a5a8", ki[0]);
        epee::string_tools::hex_to_pod("d54cbd435a8d636ad9b01b8d4f3eb13bd0cf1ce98eddf53ab1617f9b763e66c0", ki[1]);
        epee::string_tools::hex_to_pod("6c3cd6af97c4070a7aef9b1344e7463e29c7cd245076fdb65da447a34da3ca76", ki[2]);
        EXPECT_EQ_MAP(0, c.m_key_images, ki[0]);
        EXPECT_EQ_MAP(1, c.m_key_images, ki[1]);
        EXPECT_EQ_MAP(2, c.m_key_images, ki[2]);
    }
    // unconfirmed txs
    EXPECT_TRUE(c.m_unconfirmed_txs.size() == 0);
    // payments
    ASSERT_TRUE(c.m_payments.size() == 2);
    {
        auto pd0 = c.m_payments.begin();
        auto pd1 = pd0;
        ++pd1;
        EXPECT_TRUE(epee::string_tools::pod_to_hex(pd0->first) == "0000000000000000000000000000000000000000000000000000000000000000");
        EXPECT_TRUE(epee::string_tools::pod_to_hex(pd1->first) == "0000000000000000000000000000000000000000000000000000000000000000");
        if (epee::string_tools::pod_to_hex(pd0->second.m_tx_hash) == "ec34c9bb12b99af33d49691384eee5bed9171498ff04e59516505f35d1fc5efc")
            swap(pd0, pd1);
        EXPECT_TRUE(epee::string_tools::pod_to_hex(pd0->second.m_tx_hash) == "15024343b38e77a1a9860dfed29921fa17e833fec837191a6b04fa7cb9605b8e");
        EXPECT_TRUE(epee::string_tools::pod_to_hex(pd1->second.m_tx_hash) == "ec34c9bb12b99af33d49691384eee5bed9171498ff04e59516505f35d1fc5efc");
        EXPECT_TRUE(pd0->second.m_amount == 13400845012231);
        EXPECT_TRUE(pd1->second.m_amount == 1200000000000);
        EXPECT_TRUE(pd0->second.m_block_height == 818424);
        EXPECT_TRUE(pd1->second.m_block_height == 818522);
        EXPECT_TRUE(pd0->second.m_unlock_time == 818484);
        EXPECT_TRUE(pd1->second.m_unlock_time == 0);
        EXPECT_TRUE(pd0->second.m_timestamp == 1483263366);
        EXPECT_TRUE(pd1->second.m_timestamp == 1483272963);
    }
    // tx keys
    ASSERT_TRUE(c.m_tx_keys.size() == 2);
    {
        const std::vector<std::pair<std::string, std::string>> txid_txkey =
        {
            {"b9aac8c020ab33859e0c0b6331f46a8780d349e7ac17b067116e2d87bf48daad", "bf3614c6de1d06c09add5d92a5265d8c76af706f7bc6ac830d6b0d109aa87701"},
            {"6e7013684d35820f66c6679197ded9329bfe0e495effa47e7b25258799858dba", "e556884246df5a787def6732c6ea38f1e092fa13e5ea98f732b99c07a6332003"},
        };
        for (size_t i = 0; i < txid_txkey.size(); ++i)
        {
            crypto::hash txid;
            crypto::secret_key txkey;
            epee::string_tools::hex_to_pod(txid_txkey[i].first, txid);
            epee::string_tools::hex_to_pod(txid_txkey[i].second, txkey);
            EXPECT_EQ_MAP(txkey, c.m_tx_keys, txid);
        }
    }
    // confirmed txs
    EXPECT_TRUE(c.m_confirmed_txs.size() == 1);
    // tx notes
    ASSERT_TRUE(c.m_tx_notes.size() == 2);
    {
        crypto::hash h[2];
        epee::string_tools::hex_to_pod("15024343b38e77a1a9860dfed29921fa17e833fec837191a6b04fa7cb9605b8e", h[0]);
        epee::string_tools::hex_to_pod("6e7013684d35820f66c6679197ded9329bfe0e495effa47e7b25258799858dba", h[1]);
        EXPECT_EQ_MAP("sample note", c.m_tx_notes, h[0]);
        EXPECT_EQ_MAP("sample note 2", c.m_tx_notes, h[1]);
    }
    // unconfirmed payments
    EXPECT_TRUE(c.m_unconfirmed_payments.size() == 0);
    // pub keys
    ASSERT_TRUE(c.m_pub_keys.size() == 3);
    {
        crypto::public_key pubkey[3];
        epee::string_tools::hex_to_pod("33f75f264574cb3a9ea5b24220a5312e183d36dc321c9091dfbb720922a4f7b0", pubkey[0]);
        epee::string_tools::hex_to_pod("5066ff2ce9861b1d131cf16eeaa01264933a49f28242b97b153e922ec7b4b3cb", pubkey[1]);
        epee::string_tools::hex_to_pod("0d8467e16e73d16510452b78823e082e05ee3a63788d40de577cf31eb555f0c8", pubkey[2]);
        EXPECT_EQ_MAP(0, c.m_pub_keys, pubkey[0]);
        EXPECT_EQ_MAP(1, c.m_pub_keys, pubkey[1]);
        EXPECT_EQ_MAP(2, c.m_pub_keys, pubkey[2]);
    }
    // address book
    ASSERT_TRUE(c.m_address_book.size() == 1);
    {
        auto address_book_row = c.m_address_book.begin();
        EXPECT_TRUE(epee::string_tools::pod_to_hex(address_book_row->m_address.m_spend_public_key) == "9bc53a6ff7b0831c9470f71b6b972dbe5ad1e8606f72682868b1dda64e119fb3");
        EXPECT_TRUE(epee::string_tools::pod_to_hex(address_book_row->m_address.m_view_public_key) == "49fece1ef97dc0c0f7a5e2106e75e96edd910f7e86b56e1e308cd0cf734df191");
        EXPECT_TRUE(address_book_row->m_description == "testnet wallet 9y52S6");
    }
}
