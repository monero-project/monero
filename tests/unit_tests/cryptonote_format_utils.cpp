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
#include <boost/filesystem/path.hpp>
#include <cstdlib>

#include "crypto/crypto.h"
#include "crypto/generators.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "crypto/hash.h"
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_config.h"
#include "file_io_utils.h"
#include "ringct/rctOps.h"
#include "ringct/rctTypes.h"
#include "serialization/binary_utils.h"
#include "serialization/string.h"
#include "unit_tests_utils.h"

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

TEST(cn_format_utils, calculate_transaction_hash_from_blob_real)
{
    // load tx from file
    const boost::filesystem::path tx_path = unit_test::data_dir / "txs" / "bpp_tx_e89415.bin";
    cryptonote::blobdata tx_blob;
    ASSERT_TRUE(epee::file_io_utils::load_file_to_string(tx_path.string(), tx_blob));
    cryptonote::transaction tx;
    ASSERT_TRUE(cryptonote::parse_and_validate_tx_from_blob(tx_blob, tx));

    // hash from tx structure
    const crypto::hash expected_tx_hash = cryptonote::get_transaction_hash(tx);

    // hash directly from blob
    crypto::hash tx_hash_from_blob;
    ASSERT_TRUE(cryptonote::calculate_transaction_hash_from_blob(tx_blob, 
        crypto::null_hash, tx_hash_from_blob));

    EXPECT_EQ(expected_tx_hash, tx_hash_from_blob);
}

static const std::vector<std::size_t> n_inputs_vals = {1, 2, 3, 4, 7, 8, 15, 16, 31,
    32, 63, 64, 127, 128};
static const std::vector<std::size_t> n_outputs_vals = {0, 1, 2, 3, 4, 7, 8, 15, 16};
static const std::vector<std::size_t> ring_size_vals = {1, 2, 3, 4, 7, 8, 15, 16};
static const std::vector<std::size_t> extra_len_vals = {0, 1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 127, 128, 512, MAX_TX_EXTRA_SIZE};

/**
 * @brief: generate a random transaction prefix with the given parameters
 */
static void gen_tx_prefix(cryptonote::transaction &tx,
    const std::size_t tx_version,
    const std::size_t n_inputs,
    const std::size_t n_outputs,
    const std::size_t extra_len,
    const std::size_t ring_size,
    const bool use_view_tags)
{
    static const std::uint64_t unlock_time = crypto::rand<std::uint64_t>();
    static const std::uint64_t inp_amount = crypto::rand<std::uint64_t>();
    static const std::uint64_t key_offset = crypto::rand_idx<std::uint64_t>(CRYPTONOTE_MAX_BLOCK_NUMBER);
    static const crypto::key_image ki = crypto::rand<crypto::key_image>();
    static const unsigned char extra_char = crypto::rand<unsigned char>();
    static const cryptonote::tx_out output_untagged = cryptonote::tx_out{
        .amount = crypto::rand<std::uint64_t>(),
        .target = cryptonote::txout_to_key(crypto::rand<crypto::public_key>())
    };
    static const cryptonote::tx_out output_tagged = cryptonote::tx_out{
        .amount = crypto::rand<std::uint64_t>(),
        .target = cryptonote::txout_to_tagged_key(
            crypto::rand<crypto::public_key>(), crypto::rand<crypto::view_tag>())
    };

    tx.invalidate_hashes();
    tx.unprunable_size = 0;
    tx.prefix_size = 0;
    tx.version = tx_version;
    tx.unlock_time = unlock_time;
    tx.vin.resize(n_inputs, cryptonote::txin_to_key{.amount = inp_amount,
        .key_offsets = {},
        .k_image = ki});
    for (cryptonote::txin_v &in_v : tx.vin)
        boost::get<cryptonote::txin_to_key>(in_v).key_offsets.resize(ring_size, key_offset);
    tx.vout.resize(n_outputs, use_view_tags ? output_tagged : output_untagged);
    tx.extra.resize(extra_len, extra_char);
    tx.rct_signatures.type = rct::RCTTypeNull;
}

/**
 * @brief: generate random RingCT sig base fields with the given type, and given sizes in the prefix
 */
template <std::uint8_t rct_type>
static void gen_tx_rctsig_base(cryptonote::transaction &tx)
{
    const std::size_t n_inputs = tx.vin.size();
    const std::size_t n_outputs = tx.vout.size();

    static const rct::ecdhTuple ecdh_tuple = crypto::rand<rct::ecdhTuple>();
    static const rct::key dest = crypto::rand<rct::key>();
    static const rct::key mask = rct::pkGen();
    static const rct::ctkey ct = {dest, mask};
    static const rct::xmr_amount fee = crypto::rand<std::uint64_t>();

    switch (rct_type)
    {
    case rct::RCTTypeNull:
        break;
    case rct::RCTTypeSimple:
        tx.rct_signatures.pseudoOuts.resize(n_inputs, dest);
        [[fallthrough]];
    case rct::RCTTypeFull:
    case rct::RCTTypeBulletproof:
    case rct::RCTTypeBulletproof2:
    case rct::RCTTypeCLSAG:
    case rct::RCTTypeBulletproofPlus:
        tx.rct_signatures.ecdhInfo.resize(n_outputs, ecdh_tuple);
        tx.rct_signatures.outPk.resize(n_outputs, ct);
        tx.rct_signatures.txnFee = fee;
        break;
    default:
        throw std::logic_error("Unrecognized RingCT type: " + std::to_string(rct_type));
    }

    tx.rct_signatures.type = rct_type;
}

/**
 * @brief: test that a serialized transaction gets the same TXID from parsing and hashing, as direct hashing from blob
 */
static void assert_tx_to_blob_to_hash_from_blob(cryptonote::transaction &tx, cryptonote::blobdata &tx_blob)
{
    // do typical blob -> tx -> hash
    ASSERT_TRUE(cryptonote::tx_to_blob(tx, tx_blob));
    ASSERT_TRUE(cryptonote::parse_and_validate_tx_from_blob(tx_blob, tx));
    const crypto::hash expected_tx_hash = cryptonote::get_transaction_hash(tx);

    // do direct blob -> hash
    crypto::hash tx_hash_from_blob;
    ASSERT_TRUE(cryptonote::calculate_transaction_hash_from_blob(tx_blob,
        crypto::null_hash, tx_hash_from_blob));

    // expect equality
    ASSERT_EQ(expected_tx_hash, tx_hash_from_blob);
}

TEST(cn_format_utils, calculate_transaction_hash_from_blob_v1)
{
    cryptonote::transaction tx;
    cryptonote::blobdata tx_blob;
    for (std::size_t n_inputs : n_inputs_vals)
    {
        for (std::size_t n_outputs : n_outputs_vals)
        {
            for (std::size_t ring_size : ring_size_vals)
            {
                for (std::size_t extra_len : extra_len_vals)
                {
                    // make v1 tx
                    gen_tx_prefix(tx, 1, n_inputs, n_outputs, extra_len, ring_size, false);
                    tx.signatures.resize(n_inputs);
                    for (std::vector<crypto::signature> &sig : tx.signatures)
                        sig.resize(ring_size);

                    ASSERT_NO_FATAL_FAILURE(assert_tx_to_blob_to_hash_from_blob(tx, tx_blob));
                }
            }
        }
    }
}

TEST(cn_format_utils, calculate_transaction_hash_from_blob_RCTTypeFull)
{
    cryptonote::transaction tx;
    cryptonote::blobdata tx_blob;
    for (std::size_t n_inputs : n_inputs_vals)
    {
        for (std::size_t n_outputs : n_outputs_vals)
        {
            for (std::size_t ring_size : ring_size_vals)
            {
                for (std::size_t extra_len : extra_len_vals)
                {
                    // make RingCT non-simple tx
                    gen_tx_prefix(tx, 2, n_inputs, n_outputs, extra_len, ring_size, false);
                    gen_tx_rctsig_base<rct::RCTTypeFull>(tx);
                    tx.rct_signatures.p.rangeSigs.resize(n_outputs);
                    tx.rct_signatures.p.MGs = {rct::mgSig{
                        .ss = rct::keyM(ring_size, rct::keyV(n_inputs + 1, crypto::rand<rct::key>())),
                        .cc = crypto::rand<rct::key>(),
                        .II = rct::keyV(n_inputs, crypto::rand<rct::key>())
                    }};

                    ASSERT_NO_FATAL_FAILURE(assert_tx_to_blob_to_hash_from_blob(tx, tx_blob));
                }
            }
        }
    }
}

TEST(cn_format_utils, calculate_transaction_hash_from_blob_RCTTypeSimple)
{
    cryptonote::transaction tx;
    cryptonote::blobdata tx_blob;
    for (std::size_t n_inputs : n_inputs_vals)
    {
        for (std::size_t n_outputs : n_outputs_vals)
        {
            for (std::size_t ring_size : ring_size_vals)
            {
                for (std::size_t extra_len : extra_len_vals)
                {
                    // make RingCT simple tx, MLSAGs, Borromean, long-amount
                    gen_tx_prefix(tx, 2, n_inputs, n_outputs, extra_len, ring_size, false);
                    gen_tx_rctsig_base<rct::RCTTypeSimple>(tx);
                    tx.rct_signatures.p.rangeSigs.resize(n_outputs);
                    tx.rct_signatures.p.MGs.resize(n_inputs);
                    for (rct::mgSig &mlsag : tx.rct_signatures.p.MGs)
                    {
                        mlsag.ss.resize(ring_size, rct::keyV(2, crypto::rand<rct::key>()));
                        mlsag.cc = crypto::rand<rct::key>();
                        mlsag.II.resize(1, crypto::rand<rct::key>());
                    };

                    ASSERT_NO_FATAL_FAILURE(assert_tx_to_blob_to_hash_from_blob(tx, tx_blob));
                }
            }
        }
    }
}

TEST(cn_format_utils, calculate_transaction_hash_from_blob_RCTTypeBulletproof2)
{
    cryptonote::transaction tx;
    cryptonote::blobdata tx_blob;
    for (std::size_t n_inputs : n_inputs_vals)
    {
        for (std::size_t n_outputs : n_outputs_vals)
        {
            if (n_outputs < 1)
                continue;

            std::size_t n_lr = 0;
            while ((1 << n_lr) < n_outputs) { ++n_lr; }
            n_lr += 6;

            for (std::size_t ring_size : ring_size_vals)
            {
                for (std::size_t extra_len : extra_len_vals)
                {
                    // make RingCT simple tx, MLSAGs, Bulletproof, short-amount
                    gen_tx_prefix(tx, 2, n_inputs, n_outputs, extra_len, ring_size, false);
                    gen_tx_rctsig_base<rct::RCTTypeBulletproof2>(tx);
                    tx.rct_signatures.p.bulletproofs.resize(1);
                    tx.rct_signatures.p.bulletproofs.front().L.resize(n_lr);
                    tx.rct_signatures.p.bulletproofs.front().R.resize(n_lr);
                    tx.rct_signatures.p.MGs.resize(n_inputs);
                    for (rct::mgSig &mlsag : tx.rct_signatures.p.MGs)
                    {
                        mlsag.ss.resize(ring_size, rct::keyV(2, crypto::rand<rct::key>()));
                        mlsag.cc = crypto::rand<rct::key>();
                        mlsag.II.resize(1, crypto::rand<rct::key>());
                    };
                    tx.rct_signatures.p.pseudoOuts.resize(n_inputs, crypto::rand<rct::key>());

                    ASSERT_NO_FATAL_FAILURE(assert_tx_to_blob_to_hash_from_blob(tx, tx_blob));
                }
            }
        }
    }
}

TEST(cn_format_utils, calculate_transaction_hash_from_blob_RCTTypeBulletproofPlus)
{
    cryptonote::transaction tx;
    cryptonote::blobdata tx_blob;
    for (std::size_t n_inputs : n_inputs_vals)
    {
        for (std::size_t n_outputs : n_outputs_vals)
        {
            if (n_outputs < 1)
                continue;

            std::size_t n_lr = 0;
            while ((1 << n_lr) < n_outputs) { ++n_lr; }
            n_lr += 6;

            for (std::size_t ring_size : ring_size_vals)
            {
                for (std::size_t extra_len : extra_len_vals)
                {
                    // make RingCT simple tx, CLSAGs, Bulletproof+, short-amount, view tags
                    gen_tx_prefix(tx, 2, n_inputs, n_outputs, extra_len, ring_size, true);
                    gen_tx_rctsig_base<rct::RCTTypeBulletproofPlus>(tx);
                    tx.rct_signatures.p.bulletproofs_plus.resize(1);
                    tx.rct_signatures.p.bulletproofs_plus.front().L.resize(n_lr);
                    tx.rct_signatures.p.bulletproofs_plus.front().R.resize(n_lr);
                    tx.rct_signatures.p.CLSAGs.resize(n_inputs);
                    for (rct::clsag &cl : tx.rct_signatures.p.CLSAGs)
                        cl.s.resize(ring_size, crypto::rand<rct::key>());
                    tx.rct_signatures.p.pseudoOuts.resize(n_inputs, crypto::rand<rct::key>());

                    ASSERT_NO_FATAL_FAILURE(assert_tx_to_blob_to_hash_from_blob(tx, tx_blob));
                }
            }
        }
    }
}

TEST(cn_format_utils, calculate_transaction_hash_from_blob_v1_coinbase)
{
    cryptonote::transaction tx;
    cryptonote::blobdata tx_blob;
    for (std::size_t n_outputs = 0; n_outputs <= 128; ++n_outputs)
    {
        for (std::size_t extra_len : extra_len_vals)
        {
            // make v1 coinbase tx
            tx.set_null();
            tx.version = 1;
            tx.unlock_time = crypto::rand<std::uint64_t>();
            tx.vin.emplace_back(cryptonote::txin_gen{
                .height = crypto::rand<std::uint64_t>()
            });
            tx.vout.resize(n_outputs, cryptonote::tx_out{
                .amount = crypto::rand<std::uint64_t>(),
                .target = cryptonote::txout_to_key(crypto::rand<crypto::public_key>())
            });
            tx.extra.resize(extra_len, crypto::rand<unsigned char>());

            ASSERT_NO_FATAL_FAILURE(assert_tx_to_blob_to_hash_from_blob(tx, tx_blob));
        }
    }
}

TEST(cn_format_utils, calculate_transaction_hash_from_blob_v2_coinbase)
{
    cryptonote::transaction tx;
    cryptonote::blobdata tx_blob;
    for (std::size_t n_outputs = 0; n_outputs <= 128; ++n_outputs)
    {
        for (std::size_t extra_len : extra_len_vals)
        {
            // make v2 coinbase tx
            tx.set_null();
            tx.version = 2;
            tx.unlock_time = 0;
            tx.vin.emplace_back(cryptonote::txin_gen{
                .height = crypto::rand<std::uint64_t>()
            });
            tx.vout.resize(n_outputs, cryptonote::tx_out{
                .amount = crypto::rand<std::uint64_t>(),
                .target = cryptonote::txout_to_tagged_key(
                    crypto::rand<crypto::public_key>(), crypto::rand<crypto::view_tag>())
            });
            tx.extra.resize(extra_len, crypto::rand<unsigned char>());

            ASSERT_NO_FATAL_FAILURE(assert_tx_to_blob_to_hash_from_blob(tx, tx_blob));
        }
    }
}
