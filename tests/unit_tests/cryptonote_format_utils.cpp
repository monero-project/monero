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

#include "crypto/generators.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "file_io_utils.h"
#include "serialization/binary_utils.h"
#include "serialization/string.h"
#include "string_tools.h"
#include "unit_tests_utils.h"

namespace
{
static constexpr const char EXISTING_BLOCK_POW_HASH_202612[] =
    "84f64766475d51837ac9efbef1926486e58563c95a19fef4aec3254f03000000";
} //anonymous namespace

TEST(cn_format_utils, add_extra_nonce_to_tx_extra)
{
    static constexpr std::size_t max_nonce_size = TX_EXTRA_NONCE_MAX_COUNT + 1; // we *can* test higher if desired

    for (int empty_prefix = 0; empty_prefix < 2; ++empty_prefix)
    {
        std::vector<std::uint8_t> extra_prefix;
        if (!empty_prefix)
            cryptonote::add_tx_pub_key_to_extra(extra_prefix, crypto::get_H());

        std::vector<std::uint8_t> extra;
        std::string nonce;
        std::vector<cryptonote::tx_extra_field> tx_extra_fields;
        extra.reserve(extra_prefix.size() + max_nonce_size + 1 + 10);
        nonce.reserve(max_nonce_size);
        tx_extra_fields.reserve(2);
        for (std::size_t nonce_size = 0; nonce_size <= max_nonce_size; ++nonce_size)
        {
            extra = extra_prefix;
            nonce.resize(nonce_size);
            if (nonce.size())
                memset(&nonce[0], '%', nonce.size());
            tx_extra_fields.clear();

            const std::size_t expected_extra_size = extra_prefix.size() + 1
                + tools::get_varint_byte_size(nonce_size) + nonce_size;
            const bool expected_success = nonce_size <= TX_EXTRA_NONCE_MAX_COUNT;

            // add nonce and do detailed test
            const bool add_success = cryptonote::add_extra_nonce_to_tx_extra(extra, nonce);
            ASSERT_EQ(expected_success, add_success);
            if (!expected_success)
                continue;
            ASSERT_EQ(expected_extra_size, extra.size());
            ASSERT_EQ(0, memcmp(extra_prefix.data(), extra.data(), extra_prefix.size()));
            const std::uint8_t *p = extra.data() + extra_prefix.size();
            ASSERT_EQ(TX_EXTRA_NONCE, *p);
            ++p;
            std::size_t read_nonce_size = 0;
            const int varint_size = tools::read_varint((const uint8_t*)(p), // copy p
                (const uint8_t*) extra.data() + extra.size(),
                read_nonce_size);
            ASSERT_EQ(tools::get_varint_byte_size(nonce_size), varint_size);
            p += varint_size;
            for (std::size_t i = 0; i < nonce_size; ++i)
            {
                ASSERT_EQ('%', *p);
                ++p;
            }
            ASSERT_EQ(extra.data() + extra.size(), p);

            // do integration test with higher-level tx_extra parsing code
            ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
            if (empty_prefix)
            {
                ASSERT_EQ(1, tx_extra_fields.size());
                const auto &nonce_field = boost::get<cryptonote::tx_extra_nonce>(tx_extra_fields.at(0));
                ASSERT_EQ(nonce, nonce_field.nonce);
            }
            else
            {
                ASSERT_EQ(2, tx_extra_fields.size());
                const auto &pk_field = boost::get<cryptonote::tx_extra_pub_key>(tx_extra_fields.at(0));
                ASSERT_EQ(crypto::get_H(), pk_field.pub_key);
                const auto &nonce_field = boost::get<cryptonote::tx_extra_nonce>(tx_extra_fields.at(1));
                ASSERT_EQ(nonce, nonce_field.nonce);
            }
        }
    }
}

TEST(cn_format_utils, payment_id_tx_extra_nonce_roundtrip)
{
    const crypto::hash payment_id = crypto::rand<crypto::hash>();
    std::string extra_nonce;
    cryptonote::set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
    ASSERT_EQ(sizeof(payment_id) + 1, extra_nonce.size());

    crypto::hash parsed_payment_id = crypto::null_hash;
    ASSERT_TRUE(cryptonote::get_payment_id_from_tx_extra_nonce(extra_nonce, parsed_payment_id));
    EXPECT_EQ(payment_id, parsed_payment_id);

    const crypto::hash8 encrypted_payment_id = crypto::rand<crypto::hash8>();
    std::string encrypted_extra_nonce;
    cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(encrypted_extra_nonce, encrypted_payment_id);
    ASSERT_EQ(sizeof(encrypted_payment_id) + 1, encrypted_extra_nonce.size());

    crypto::hash8 parsed_encrypted_payment_id = crypto::null_hash8;
    ASSERT_TRUE(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(encrypted_extra_nonce, parsed_encrypted_payment_id));
    EXPECT_EQ(encrypted_payment_id, parsed_encrypted_payment_id);
}

TEST(cn_format_utils, payment_id_tx_extra_nonce_rejects_wrong_tag_or_size)
{
    const crypto::hash payment_id = crypto::rand<crypto::hash>();
    std::string extra_nonce;
    cryptonote::set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);

    crypto::hash parsed_payment_id = crypto::null_hash;
    std::string wrong_tag = extra_nonce;
    wrong_tag[0] = TX_EXTRA_NONCE_ENCRYPTED_PAYMENT_ID;
    EXPECT_FALSE(cryptonote::get_payment_id_from_tx_extra_nonce(wrong_tag, parsed_payment_id));

    std::string wrong_size = extra_nonce;
    wrong_size.resize(wrong_size.size() - 1);
    EXPECT_FALSE(cryptonote::get_payment_id_from_tx_extra_nonce(wrong_size, parsed_payment_id));
    wrong_size = extra_nonce;
    wrong_size.push_back('\0');
    EXPECT_FALSE(cryptonote::get_payment_id_from_tx_extra_nonce(wrong_size, parsed_payment_id));

    const crypto::hash8 encrypted_payment_id = crypto::rand<crypto::hash8>();
    std::string encrypted_extra_nonce;
    cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(encrypted_extra_nonce, encrypted_payment_id);

    crypto::hash8 parsed_encrypted_payment_id = crypto::null_hash8;
    wrong_tag = encrypted_extra_nonce;
    wrong_tag[0] = TX_EXTRA_NONCE_PAYMENT_ID;
    EXPECT_FALSE(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(wrong_tag, parsed_encrypted_payment_id));

    wrong_size = encrypted_extra_nonce;
    wrong_size.resize(wrong_size.size() - 1);
    EXPECT_FALSE(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(wrong_size, parsed_encrypted_payment_id));
    wrong_size = encrypted_extra_nonce;
    wrong_size.push_back('\0');
    EXPECT_FALSE(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(wrong_size, parsed_encrypted_payment_id));
}

TEST(cn_format_utils, block_longhash_202612_arbitrary_blob)
{
    const cryptonote::blobdata blob = "not the historical block 202612 hashing blob";
    const crypto::hash pow_hash = cryptonote::get_block_longhash(blob, 202612, 1, crypto::null_hash);

    ASSERT_NE(EXISTING_BLOCK_POW_HASH_202612, epee::string_tools::pod_to_hex(pow_hash));
}

TEST(cn_format_utils, block_longhash_202612_mainnet_blob)
{
    // load existing mainnet block 202612 from file
    const auto block_202612_blob_path = unit_test::data_dir / "blocks" / "block_202612_mainnet.bin";
    cryptonote::blobdata block_blob;
    ASSERT_TRUE(epee::file_io_utils::load_file_to_string(block_202612_blob_path.string(), block_blob));

    // parse and validate existing mainnet block 202612 and check block ID (you can see bbd604d2... on an explorer)
    cryptonote::block block;
    crypto::hash block_id = crypto::null_hash;
    ASSERT_TRUE(cryptonote::parse_and_validate_block_from_blob(block_blob, block, &block_id));
    ASSERT_EQ("bbd604d2ba11ba27935e006ed39c9bfdd99b76bf4a50654bc1e1e61217962698", epee::string_tools::pod_to_hex(block_id));

    // get hashing blob for 202612 and check raw hash (you can see 426d16cf... in cryptonote::get_block_longhash() impl)
    const cryptonote::blobdata hashing_blob = cryptonote::get_block_hashing_blob(block);
    ASSERT_TRUE(cryptonote::get_object_hash(hashing_blob, block_id));
    ASSERT_EQ("426d16cff04c71f8b16340b722dc4010a2dd3831c22041431f772547ba6e331a", epee::string_tools::pod_to_hex(block_id));

    // call get_block_longhash() and check that we get the existing PoW hash
    const crypto::hash pow_hash = cryptonote::get_block_longhash(hashing_blob, 202612, 1, crypto::null_hash);
    ASSERT_EQ(EXISTING_BLOCK_POW_HASH_202612, epee::string_tools::pod_to_hex(pow_hash));
}

TEST(cn_format_utils, block_longhash_202612_testnet_blob)
{
    // load existing testnet block 202612 from file
    const auto block_202612_blob_path = unit_test::data_dir / "blocks" / "block_202612_testnet.bin";
    cryptonote::blobdata block_blob;
    ASSERT_TRUE(epee::file_io_utils::load_file_to_string(block_202612_blob_path.string(), block_blob));

    // parse and validate existing testnet block 202612 and check block ID (you can see 248fde4b... on an explorer)
    cryptonote::block block;
    crypto::hash block_id;
    ASSERT_TRUE(cryptonote::parse_and_validate_block_from_blob(block_blob, block, block_id));
    ASSERT_EQ("248fde4b96b829c4ddbd00e3f76d35b03d01257898bc1b5578bc9e04b379a676", epee::string_tools::pod_to_hex(block_id));

    // get hashing blob for 202612 and check raw hash (you can see 248fde4b... in cryptonote::get_block_longhash() impl)
    const cryptonote::blobdata hashing_blob = cryptonote::get_block_hashing_blob(block);
    ASSERT_TRUE(cryptonote::get_object_hash(hashing_blob, block_id));
    ASSERT_EQ("248fde4b96b829c4ddbd00e3f76d35b03d01257898bc1b5578bc9e04b379a676", epee::string_tools::pod_to_hex(block_id));

    // call get_block_longhash() and check that we get the existing PoW hash
    const crypto::hash pow_hash = cryptonote::get_block_longhash(hashing_blob, 202612, 9, crypto::null_hash);
    ASSERT_EQ(EXISTING_BLOCK_POW_HASH_202612, epee::string_tools::pod_to_hex(pow_hash));
}

TEST(cn_format_utils, block_longhash_202612_stagenet_blob)
{
    // load existing stagenet block 202612 from file
    const auto block_202612_blob_path = unit_test::data_dir / "blocks" / "block_202612_stagenet.bin";
    cryptonote::blobdata block_blob;
    ASSERT_TRUE(epee::file_io_utils::load_file_to_string(block_202612_blob_path.string(), block_blob));

    // parse and validate existing stagenet block 202612 and check block ID (you can see f3449e65... on an explorer)
    cryptonote::block block;
    crypto::hash block_id;
    ASSERT_TRUE(cryptonote::parse_and_validate_block_from_blob(block_blob, block, block_id));
    ASSERT_EQ("f3449e658b5f880c4b0e69007ed5d092c9c883ac3a518166fa652d5cc505e7b1", epee::string_tools::pod_to_hex(block_id));

    // get hashing blob for 202612 and check raw hash (you can see f3449e65... in cryptonote::get_block_longhash() impl)
    const cryptonote::blobdata hashing_blob = cryptonote::get_block_hashing_blob(block);
    ASSERT_TRUE(cryptonote::get_object_hash(hashing_blob, block_id));
    ASSERT_EQ("f3449e658b5f880c4b0e69007ed5d092c9c883ac3a518166fa652d5cc505e7b1", epee::string_tools::pod_to_hex(block_id));

    // call get_block_longhash() and check that we get the existing PoW hash
    const crypto::hash pow_hash = cryptonote::get_block_longhash(hashing_blob, 202612, 9, crypto::null_hash);
    ASSERT_EQ(EXISTING_BLOCK_POW_HASH_202612, epee::string_tools::pod_to_hex(pow_hash));
}

TEST(cn_format_utils, block_longhash_reconstruct_existing_202612)
{
    // load existing mainnet block 202612 from file
    const auto block_202612_blob_path = unit_test::data_dir / "blocks" / "block_202612_mainnet.bin";
    cryptonote::blobdata block_blob;
    ASSERT_TRUE(epee::file_io_utils::load_file_to_string(block_202612_blob_path.string(), block_blob));

    // parse and validate existing mainnet block 202612
    cryptonote::block block;
    ASSERT_TRUE(cryptonote::parse_and_validate_block_from_blob(block_blob, block));
    ASSERT_EQ(513, block.tx_hashes.size());

    // modify block and hashing blob s.t. it matches buggy behavior of tree hash before counting fix
    block.tx_hashes.pop_back();
    block.tx_hashes.pop_back();
    cryptonote::blobdata hashing_blob = cryptonote::get_block_hashing_blob(block);
    ASSERT_EQ(77, hashing_blob.size());
    hashing_blob.at(75) = (char) -126; // mangle total tx count from 512 to 514

    // check that we can reconstruct the existing PoW hash
    crypto::hash pow_hash;
    crypto::cn_slow_hash(hashing_blob.data(), hashing_blob.size(), pow_hash, /*variant=*/0, /*height=*/202612);
    ASSERT_EQ(EXISTING_BLOCK_POW_HASH_202612, epee::string_tools::pod_to_hex(pow_hash));
}

TEST(cn_format_utils, add_mm_merkle_root_to_tx_extra)
{
    const std::vector<std::uint64_t> depths{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 63, 64, 127, 128, 16383, 16384};

    const crypto::hash mm_merkle_root = crypto::rand<crypto::hash>();

    for (int empty_prefix = 0; empty_prefix < 2; ++empty_prefix)
    {
        std::vector<std::uint8_t> extra_prefix;
        if (!empty_prefix)
            cryptonote::add_tx_pub_key_to_extra(extra_prefix, crypto::get_H());

        std::vector<std::uint8_t> extra;
        std::vector<cryptonote::tx_extra_field> tx_extra_fields;
        extra.reserve(extra_prefix.size() + 1 + 1 + 10 + 32);
        tx_extra_fields.reserve(2);
        for (std::uint64_t mm_merkle_tree_depth : depths)
        {
            extra = extra_prefix;
            tx_extra_fields.clear();

            const std::size_t expected_extra_size = extra_prefix.size() + 1 + 1
                + tools::get_varint_byte_size(mm_merkle_tree_depth) + 32;

            // add nonce and do detailed test
            const bool add_success = cryptonote::add_mm_merkle_root_to_tx_extra(extra, mm_merkle_root, mm_merkle_tree_depth);
            ASSERT_TRUE(add_success);
            ASSERT_EQ(expected_extra_size, extra.size());
            ASSERT_EQ(0, memcmp(extra_prefix.data(), extra.data(), extra_prefix.size()));
            const std::uint8_t *p = extra.data() + extra_prefix.size();
            ASSERT_EQ(TX_EXTRA_MERGE_MINING_TAG, *p);
            ++p;
            ASSERT_EQ(32 + tools::get_varint_byte_size(mm_merkle_tree_depth), *p);
            ++p;
            std::uint64_t read_depth = 0;
            const int varint_size = tools::read_varint((const uint8_t*)(p), // copy p
                (const uint8_t*) extra.data() + extra.size(),
                read_depth);
            ASSERT_EQ(tools::get_varint_byte_size(mm_merkle_tree_depth), varint_size);
            ASSERT_EQ(mm_merkle_tree_depth, read_depth);
            p += varint_size;
            ASSERT_EQ(0, memcmp(p, mm_merkle_root.data, sizeof(mm_merkle_root)));
            p += sizeof(crypto::hash);
            ASSERT_EQ(extra.data() + extra.size(), p);

            // do integration test with higher-level tx_extra parsing code
            ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
            if (empty_prefix)
            {
                ASSERT_EQ(1, tx_extra_fields.size());
                const auto &mm_field = boost::get<cryptonote::tx_extra_merge_mining_tag>(tx_extra_fields.at(0));
                ASSERT_EQ(mm_merkle_root, mm_field.merkle_root);
                ASSERT_EQ(mm_merkle_tree_depth, mm_field.depth);
            }
            else
            {
                ASSERT_EQ(2, tx_extra_fields.size());
                const auto &pk_field = boost::get<cryptonote::tx_extra_pub_key>(tx_extra_fields.at(0));
                ASSERT_EQ(crypto::get_H(), pk_field.pub_key);
                const auto &mm_field = boost::get<cryptonote::tx_extra_merge_mining_tag>(tx_extra_fields.at(1));
                ASSERT_EQ(mm_merkle_root, mm_field.merkle_root);
                ASSERT_EQ(mm_merkle_tree_depth, mm_field.depth);
            }
        }
    }
}

TEST(cn_format_utils, tx_extra_merge_mining_tag_store_load)
{
    const std::vector<std::uint64_t> depths{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 63, 64, 127, 128, 16383, 16384};

    const crypto::hash mm_merkle_root = crypto::rand<crypto::hash>();

    for (int empty_prefix = 0; empty_prefix < 2; ++empty_prefix)
    {
        std::vector<std::uint8_t> extra_prefix;
        if (!empty_prefix)
            cryptonote::add_tx_pub_key_to_extra(extra_prefix, crypto::get_H());

        std::vector<std::uint8_t> extra;
        std::vector<cryptonote::tx_extra_field> tx_extra_fields;
        extra.reserve(extra_prefix.size() + 1 + 1 + 10 + 32);
        tx_extra_fields.reserve(2);
        for (std::uint64_t mm_merkle_tree_depth : depths)
        {
            extra = extra_prefix;
            tx_extra_fields.clear();

            const std::size_t expected_extra_size = extra_prefix.size() + 1 + 1
                + tools::get_varint_byte_size(mm_merkle_tree_depth) + 32;

            // add nonce and do detailed test
            cryptonote::tx_extra_merge_mining_tag mm;
            mm.depth = mm_merkle_tree_depth;
            mm.merkle_root = mm_merkle_root;
            cryptonote::tx_extra_field extra_field = mm;
            std::string mm_blob;
            ASSERT_TRUE(::serialization::dump_binary(extra_field, mm_blob));
            extra.resize(extra.size() + mm_blob.size());
            memcpy(extra.data() + extra.size() - mm_blob.size(), mm_blob.data(), mm_blob.size());
            ASSERT_EQ(expected_extra_size, extra.size());
            ASSERT_EQ(0, memcmp(extra_prefix.data(), extra.data(), extra_prefix.size()));
            const std::uint8_t *p = extra.data() + extra_prefix.size();
            ASSERT_EQ(TX_EXTRA_MERGE_MINING_TAG, *p);
            ++p;
            ASSERT_EQ(32 + tools::get_varint_byte_size(mm_merkle_tree_depth), *p);
            ++p;
            std::uint64_t read_depth = 0;
            const int varint_size = tools::read_varint((const uint8_t*)(p), // copy p
                (const uint8_t*) extra.data() + extra.size(),
                read_depth);
            ASSERT_EQ(tools::get_varint_byte_size(mm_merkle_tree_depth), varint_size);
            ASSERT_EQ(mm_merkle_tree_depth, read_depth);
            p += varint_size;
            ASSERT_EQ(0, memcmp(p, mm_merkle_root.data, sizeof(mm_merkle_root)));
            p += sizeof(crypto::hash);
            ASSERT_EQ(extra.data() + extra.size(), p);

            // do integration test with higher-level tx_extra parsing code
            ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
            if (empty_prefix)
            {
                ASSERT_EQ(1, tx_extra_fields.size());
                const auto &mm_field = boost::get<cryptonote::tx_extra_merge_mining_tag>(tx_extra_fields.at(0));
                ASSERT_EQ(mm_merkle_root, mm_field.merkle_root);
                ASSERT_EQ(mm_merkle_tree_depth, mm_field.depth);
            }
            else
            {
                ASSERT_EQ(2, tx_extra_fields.size());
                const auto &pk_field = boost::get<cryptonote::tx_extra_pub_key>(tx_extra_fields.at(0));
                ASSERT_EQ(crypto::get_H(), pk_field.pub_key);
                const auto &mm_field = boost::get<cryptonote::tx_extra_merge_mining_tag>(tx_extra_fields.at(1));
                ASSERT_EQ(mm_merkle_root, mm_field.merkle_root);
                ASSERT_EQ(mm_merkle_tree_depth, mm_field.depth);
            }
        }
    }
}
