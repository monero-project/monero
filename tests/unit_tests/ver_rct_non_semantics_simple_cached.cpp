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

#include <sstream>

#define IN_UNIT_TESTS // To access Blockchain::{expand_transaction_2, verRctNonSemanticsSimpleCached}

#include "gtest/gtest.h"
#include "unit_tests_utils.h"

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/blockchain.h"
#include "file_io_utils.h"
#include "misc_log_ex.h"
#include "ringct/rctSigs.h"

namespace cryptonote
{
// declaration not provided in cryptonote_format_utils.h, but definition is not static ;)
bool expand_transaction_1(transaction &tx, bool base_only);
}

namespace
{
/**
 * @brief Make rct::ctkey from hex string representation of destionation and mask
 *
 * @param dest_hex
 * @param mask_hex
 * @return rct::ctkey
 */
static rct::ctkey make_ctkey(const char* dest_hex, const char* mask_hex)
{
    rct::key dest;
    rct::key mask;
    CHECK_AND_ASSERT_THROW_MES(epee::from_hex::to_buffer(epee::as_mut_byte_span(dest), dest_hex), "dest bad hex: " << dest_hex);
    CHECK_AND_ASSERT_THROW_MES(epee::from_hex::to_buffer(epee::as_mut_byte_span(mask), mask_hex), "mask bad hex: " << mask_hex);
    return {dest, mask};
}

template <typename T>
static std::string stringify_with_do_serialize(const T& t)
{
    std::stringstream ss;
    binary_archive<true> ar(ss);
    CHECK_AND_ASSERT_THROW_MES(ar.good(), "Archiver is not in a good state. This shouldn't happen!");
    ::do_serialize(ar, const_cast<T&>(t));
    return ss.str();
}

static bool check_tx_is_expanded(const cryptonote::transaction& tx, const rct::ctkeyM& pubkeys)
{
    // Ripped from cryptonote_core/blockchain.cpp

    const rct::rctSig& rv = tx.rct_signatures;

    if (pubkeys.size() != rv.mixRing.size())
    {
        MERROR("Failed to check ringct signatures: mismatched pubkeys/mixRing size");
        return false;
    }
    for (size_t i = 0; i < pubkeys.size(); ++i)
    {
        if (pubkeys[i].size() != rv.mixRing[i].size())
        {
            MERROR("Failed to check ringct signatures: mismatched pubkeys/mixRing size");
            return false;
        }
    }

    for (size_t n = 0; n < pubkeys.size(); ++n)
    {
        for (size_t m = 0; m < pubkeys[n].size(); ++m)
        {
            if (pubkeys[n][m].dest != rct::rct2pk(rv.mixRing[n][m].dest))
            {
                MERROR("Failed to check ringct signatures: mismatched pubkey at vin " << n << ", index " << m);
                return false;
            }
            if (pubkeys[n][m].mask != rct::rct2pk(rv.mixRing[n][m].mask))
            {
                MERROR("Failed to check ringct signatures: mismatched commitment at vin " << n << ", index " << m);
                return false;
            }
        }
    }

    const size_t n_sigs = rct::is_rct_clsag(rv.type) ? rv.p.CLSAGs.size() : rv.p.MGs.size();
    if (n_sigs != tx.vin.size())
    {
        MERROR("Failed to check ringct signatures: mismatched MGs/vin sizes");
        return false;
    }
    for (size_t n = 0; n < tx.vin.size(); ++n)
    {
        bool error;
        if (rct::is_rct_clsag(rv.type))
            error = memcmp(&boost::get<cryptonote::txin_to_key>(tx.vin[n]).k_image, &rv.p.CLSAGs[n].I, 32);
        else
            error = rv.p.MGs[n].II.empty() || memcmp(&boost::get<cryptonote::txin_to_key>(tx.vin[n]).k_image, &rv.p.MGs[n].II[0], 32);
        if (error)
        {
            MERROR("Failed to check ringct signatures: mismatched key image");
            return false;
        }
    }

    return true;
}

/**
 * @brief Perform expand_transaction_1 and Blockchain::expand_transaction_2 on a certain transaction
 */
static void expand_transaction_fully(cryptonote::transaction& tx, const rct::ctkeyM& input_pubkeys)
{
    const crypto::hash tx_prefix_hash = cryptonote::get_transaction_prefix_hash(tx);
    CHECK_AND_ASSERT_THROW_MES(cryptonote::expand_transaction_1(tx, false), "expand 1 failed");
    CHECK_AND_ASSERT_THROW_MES
    (
        cryptonote::Blockchain::expand_transaction_2(tx, tx_prefix_hash, input_pubkeys),
        "expand 2 failed"
    );
    CHECK_AND_ASSERT_THROW_MES(!memcmp(&tx_prefix_hash, &tx.rct_signatures.message, 32), "message check failed");
    CHECK_AND_ASSERT_THROW_MES(input_pubkeys == tx.rct_signatures.mixRing, "mixring check failed");
    CHECK_AND_ASSERT_THROW_MES(check_tx_is_expanded(tx, input_pubkeys), "tx expansion check 2 failed");
}

/**
 * @brief Mostly construct transaction from binary file and provided mix ring pubkeys
 *
 * Most important to us, this should populate the .rct_signatures.message and
 * .rct_signatures.mixRings fields of the transaction.
 *
 * @param file_name relative file path in unit test data directory
 * @param input_pubkeys manually retrived input pubkey destination / masks for each ring
 * @return cryptonote::transaction the expanded transaction
 */
static cryptonote::transaction expand_transaction_from_bin_file_and_pubkeys
(
    const char* file_name,
    const rct::ctkeyM& input_pubkeys
)
{
    cryptonote::transaction transaction;

    const boost::filesystem::path tx_json_path = unit_test::data_dir / file_name;
    std::string tx_blob;
    CHECK_AND_ASSERT_THROW_MES
    (
        epee::file_io_utils::load_file_to_string(tx_json_path.string(), tx_blob),
        "loading file to string failed"
    );

    CHECK_AND_ASSERT_THROW_MES
    (
        cryptonote::parse_and_validate_tx_from_blob(tx_blob, transaction),
        "TX blob could not be parsed"
    );

    expand_transaction_fully(transaction, input_pubkeys);

    return transaction;
}

/**
 * @brief Return whether a modification changes blob resulting from do_serialize()
 */
template <typename T, class TModifier>
static bool modification_changes_do_serialize
(
    const T& og_obj,
    TModifier& obj_modifier_func,
    bool expected_change
)
{
    T modded_obj = og_obj;
    obj_modifier_func(modded_obj);
    const std::string og_blob = stringify_with_do_serialize(og_obj);
    const std::string modded_blob = stringify_with_do_serialize(modded_obj);
    const bool did_change = modded_blob != og_blob;
    if (did_change != expected_change)
    {
        const std::string og_hex = epee::to_hex::string(epee::strspan<uint8_t>(og_blob));
        const std::string modded_hex = epee::to_hex::string(epee::strspan<uint8_t>(modded_blob));
        MERROR("unexpected: modded_blob '" << modded_hex << "' vs og_blob ' << " << og_hex << "'");
    }
    return did_change;
}

// Contains binary representation of mainnet transaction (height 2777777):
// e89415b95564aa7e3587c91422756ba5303e727996e19c677630309a0d52a7ca
static constexpr const char* tx1_file_name = "txs/bpp_tx_e89415.bin";

// This contains destination key / mask pairs for each output in the input ring of the above tx
static const rct::ctkeyM tx1_input_pubkeys =
{{
    make_ctkey("e50f476129d40af31e0938743f7f2d60e867aab31294f7acaf6e38f0976f0228", "51e788ddf5c95c124a7314d45a91b52d60db25a0572de9c2b4ec515aca3d4481"),
    make_ctkey("804245d067fcfe6cd66376db0571869989bc68b3e22a0f902109c7530df47a59", "c3cc65d3b3a05defaa05213dc3b0496f9b86dbeeefbff28db34b134b6ee3230b"),
    make_ctkey("527563a03b498e47732b815f5f0c5875a70e0fb71a37c88123f0f8686349fae4", "04417c03b397cd11e403275ec89cb0ab5b8476bb88470e9ae7208ea63dacf073"),
    make_ctkey("bffca8b5c7fe4235ba7136d6b5325f63df343dc147940b677f50217f8953bca6", "5cd8c5e54e07275422c9c5a9f4a7268d26c494ffba419e878b7e873a02ae2e76"),
    make_ctkey("1f73385ea74308aa78b5abf585faac14a5e78a6e23f0f68c9c14681108b28ef0", "5c02b3156daaa8ec476d3244439d90efa266f3e51cb9c8eb384d8b9a8efaa024"),
    make_ctkey("a2421eae8bb256644b34feeab48c6086c2c9feb40d2643436dc45e303eee8ab2", "787823abffa988b56d4a7b4a834630f71520220fd82fad035955e616ec095788"),
    make_ctkey("17d8d8dc1e1c25b7295f2eab44c4ccc08a629b8e8d781bbb6f9a51a9561bcd4c", "db1ea24be6947e03176a297160dba16d65f37751bb0ef2ba71a4590d12b61dfc"),
    make_ctkey("2c39348a9ab04dbabe3b5249819b7845ed8aaebd0d8eddd98bda0bf40753a398", "4e6cd25fbd10e2e040be84e3bf8043c612daeef625e66a5e5bcff88c9c46e82c"),
    make_ctkey("c4c97157f23b45c7084526aaa9958fe858bebe446a7efa22c491c439b74271b1", "e251db2c86193a11a5bffefffe48c20e3d92a8dc98cb3a2f41704e565bcd860a"),
    make_ctkey("d342045525139a8551bcdfa7aa0117d2ac2327cb6cf449ca59420c300e4471a5", "789c11f72060ad80f4cda5d89b24d49f9435bf765598dea7a91776e99f05f87c"),
    make_ctkey("9a972ccf2c74f648070b0be839749c98eca87166de401a6c1f59e64b938a46c1", "5444cbed5cec31fb6ed1612f815d292f2bf3d2ff584bbcd8e5201ec59670d414"),
    make_ctkey("49ccb806ccf5cbd74bae8d9fb2da8918ab61d0774ee6a6c3a6ccd237db22a088", "0c5db942fb44f29f6ef956e24db91f98a6de6e7288b0b04d01b8f260453d1431"),
    make_ctkey("74417e8d1483df2df6fe68c88fc9a72639c35d765b38351b838521addf45dadc", "a1a606d6c4762ef51c1759bcb8b5c88be1d323025400c41fe6885431064b64dc"),
    make_ctkey("48c4c349adaf7b3be27656ea70d1c83b93e1511bb0aac987861a4da9689b0e95", "ad14ffd5edac199ea7c5437d558089b0f2f03aa74bde43611322d769968b5a1c"),
    make_ctkey("2d2ffade0f85ddd83a036469e49542e93cad94f9bea535f0ea2eb2f56304517e", "bcc48d00bd06dc5439200e749d0caf8a062b072d0c0eb1f78f6a4d8f2373e5f4"),
    make_ctkey("4ee857d0ce17f66eca9c81eb326e404ceb50c8198248f2f827c440ee7aa0c0d7", "a8a9d61d4abbfb123630ffd214c834cc45113eaa51dd2f904cc6ae0c3c5d70e3")
}};
} // anonymous namespace

TEST(verRctNonSemanticsSimple, tx1_preconditions)
{
    // If this unit test fails, something changed about transaction deserialization / expansion or
    // something changed about RingCT signature verification.

    cryptonote::rct_ver_cache_t rct_ver_cache;

    cryptonote::transaction tx = expand_transaction_from_bin_file_and_pubkeys
        (tx1_file_name, tx1_input_pubkeys);
    const rct::rctSig& rs = tx.rct_signatures;

    const crypto::hash tx_prefix_hash = cryptonote::get_transaction_prefix_hash(tx);

    EXPECT_EQ(1, tx.vin.size());
    EXPECT_EQ(2, tx.vout.size());
    const rct::key expected_sig_msg = rct::hash2rct(tx_prefix_hash);
    EXPECT_EQ(expected_sig_msg, rs.message);
    EXPECT_EQ(1, rs.mixRing.size());
    EXPECT_EQ(16, rs.mixRing[0].size());
    EXPECT_EQ(0, rs.pseudoOuts.size());
    EXPECT_EQ(0, rs.p.rangeSigs.size());
    EXPECT_EQ(0, rs.p.bulletproofs.size());
    EXPECT_EQ(1, rs.p.bulletproofs_plus.size());
    EXPECT_EQ(2, rs.p.bulletproofs_plus[0].V.size());
    EXPECT_EQ(7, rs.p.bulletproofs_plus[0].L.size());
    EXPECT_EQ(7, rs.p.bulletproofs_plus[0].R.size());
    EXPECT_EQ(0, rs.p.MGs.size());
    EXPECT_EQ(1, rs.p.CLSAGs.size());
    EXPECT_EQ(16, rs.p.CLSAGs[0].s.size());
    EXPECT_EQ(1, rs.p.pseudoOuts.size());
    EXPECT_EQ(tx1_input_pubkeys, rs.mixRing);
    EXPECT_EQ(2, rs.outPk.size());

    EXPECT_TRUE(rct::verRctSemanticsSimple(rs));
    EXPECT_TRUE(rct::verRctNonSemanticsSimple(rs));
    EXPECT_TRUE(rct::verRctSimple(rs));
    EXPECT_TRUE(cryptonote::ver_rct_non_semantics_simple_cached(tx, tx1_input_pubkeys, rct_ver_cache, rct::RCTTypeBulletproofPlus));
    EXPECT_TRUE(cryptonote::ver_rct_non_semantics_simple_cached(tx, tx1_input_pubkeys, rct_ver_cache, rct::RCTTypeBulletproofPlus));
}

#define SERIALIZABLE_SIG_CHANGES_SUBTEST(fieldmodifyclause)                                    \
    do {                                                                                       \
        const auto sig_modifier_func = [](rct::rctSig& rs) { rs.fieldmodifyclause; };          \
        EXPECT_TRUE(modification_changes_do_serialize(original_sig, sig_modifier_func, true)); \
    } while (0);                                                                               \

TEST(verRctNonSemanticsSimple, serializable_sig_changes)
{
    // Hello, future visitors. If this unit test fails, then fields of rctSig have been dropped from
    // serialization.

    const cryptonote::transaction tx = expand_transaction_from_bin_file_and_pubkeys
        (tx1_file_name, tx1_input_pubkeys);
    const rct::rctSig& original_sig = tx.rct_signatures;

    // These are the subtests most likely to fail. Fields 'message' and 'mixRing' are not serialized
    // when sent over the wire, since they can be reconstructed from transaction data. However, they
    // are serialized by ::do_serialize(rctSig).
    // How signatures are serialized for the blockchain can be found in the methods
    // rct::rctSigBase::serialize_rctsig_base and rct::rctSigPrunable::serialize_rctsig_prunable.
    SERIALIZABLE_SIG_CHANGES_SUBTEST(message.bytes[31]++)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(mixRing.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(mixRing[0].push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(mixRing[0][8].dest[10]--)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(mixRing[0][15].mask[3]--)

    // rctSigBase changes. These subtests are less likely to break
    SERIALIZABLE_SIG_CHANGES_SUBTEST(type ^= 23)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(pseudoOuts.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(ecdhInfo.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(outPk.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(outPk[0].dest[14]--)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(outPk[1].dest[14]--)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(outPk[0].mask[14]--)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(outPk[1].mask[14]--)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(txnFee *= 2023)

    // rctSigPrunable changes
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.rangeSigs.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].A[13] -= 7)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].A1[13] -= 7)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].B[13] -= 7)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].r1[13] -= 7)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].s1[13] -= 7)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].d1[13] -= 7)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].L.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].L[2][13] -= 7)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].R.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].R[2][13] -= 7)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.MGs.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.CLSAGs.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.CLSAGs[0].s.push_back({}))
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.CLSAGs[0].s[15][31] ^= 69)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.CLSAGs[0].c1[0] /= 3)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.CLSAGs[0].D[0] /= 3)
    SERIALIZABLE_SIG_CHANGES_SUBTEST(p.pseudoOuts.push_back({}))

    // Uncomment line below to sanity check SERIALIZABLE_SIG_CHANGES_SUBTEST
    // SERIALIZABLE_SIG_CHANGES_SUBTEST(message) // should fail
}

#define UNSERIALIZABLE_SIG_CHANGES_SUBTEST(fieldmodifyclause)                                    \
    do {                                                                                         \
        const auto sig_modifier_func = [](rct::rctSig& rs) { rs.fieldmodifyclause; };            \
        EXPECT_FALSE(modification_changes_do_serialize(original_sig, sig_modifier_func, false)); \
    } while (0);                                                                                 \

TEST(verRctNonSemanticsSimple, unserializable_sig_changes)
{
    // Hello, future visitors. If this unit test fails, then congrats! ::do_serialize(rctSig) became
    // better at uniquely representing rctSig.
    const cryptonote::transaction tx = expand_transaction_from_bin_file_and_pubkeys
        (tx1_file_name, tx1_input_pubkeys);
    const rct::rctSig& original_sig = tx.rct_signatures;

    UNSERIALIZABLE_SIG_CHANGES_SUBTEST(p.CLSAGs[0].I[14]++)
    UNSERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].V.push_back({}))
    UNSERIALIZABLE_SIG_CHANGES_SUBTEST(p.bulletproofs_plus[0].V[1][31]--)

    // Uncomment line below to sanity check UNSERIALIZABLE_SIG_CHANGES_SUBTEST_SHORTCUT
    // UNSERIALIZABLE_SIG_CHANGES_SUBTEST_SHORTCUT(message[2]++) // should fail
}

#define SERIALIZABLE_MIXRING_CHANGES_SUBTEST(fieldmodifyclause)                                   \
    do {                                                                                          \
        using mr_mod_func_t = std::function<void(rct::ctkeyM&)>;                                  \
        const mr_mod_func_t mr_modifier_func = [&](rct::ctkeyM& mr) { mr fieldmodifyclause; };    \
        EXPECT_TRUE(modification_changes_do_serialize(original_mixring, mr_modifier_func, true)); \
    } while (0);                                                                                  \

TEST(verRctNonSemanticsSimple, serializable_mixring_changes)
{
    // Hello, future Monero devs! If this unit test fails, a huge concensus-related assumption has
    // been broken and verRctNonSemanticsSimpleCached needs to be reevalulated for validity. If it
    // is not, there may be an exploit which allows for double-spending. See the implementation for
    // more comments on the uniqueness of the internal cache hash.

    const rct::ctkeyM original_mixring = tx1_input_pubkeys;

    const size_t mlen = tx1_input_pubkeys.size();
    ASSERT_EQ(1, mlen);
    const size_t nlen = tx1_input_pubkeys[0].size();
    ASSERT_EQ(16, nlen);

    SERIALIZABLE_MIXRING_CHANGES_SUBTEST(.clear())
    SERIALIZABLE_MIXRING_CHANGES_SUBTEST(.push_back({}))
    SERIALIZABLE_MIXRING_CHANGES_SUBTEST([0].clear())
    SERIALIZABLE_MIXRING_CHANGES_SUBTEST([0].push_back({}))
    SERIALIZABLE_MIXRING_CHANGES_SUBTEST([0][0].dest[4]--)
    SERIALIZABLE_MIXRING_CHANGES_SUBTEST([0][15].mask[31]--)

    // Loop through all bytes of the mixRing and check for serialiable changes
    for (size_t i = 0; i < mlen; ++i)
    {
        for (size_t j = 0; j < nlen; ++j)
        {
            static_assert(sizeof(rct::key) == 32, "rct::key size wrong");
            for (size_t k = 0; k < sizeof(rct::key); ++k)
            {
                SERIALIZABLE_MIXRING_CHANGES_SUBTEST([i][j].dest[k]++)
                SERIALIZABLE_MIXRING_CHANGES_SUBTEST([i][j].mask[k]++)
            }
        }
    }
}

#define EXPAND_TRANSACTION_2_FAILURES_SUBTEST(fieldmodifyclause)       \
    do {                                                               \
        cryptonote::transaction test_tx = original_tx;                 \
        test_tx.fieldmodifyclause;                                     \
        test_tx.invalidate_hashes();                                   \
        EXPECT_FALSE(check_tx_is_expanded(test_tx, original_mixring)); \
    } while (0);                                                       \

TEST(verRctNonSemanticsSimple, expand_transaction_2_failures)
{
    cryptonote::transaction original_tx = expand_transaction_from_bin_file_and_pubkeys
        (tx1_file_name, tx1_input_pubkeys);
    rct::ctkeyM original_mixring = tx1_input_pubkeys;

    EXPAND_TRANSACTION_2_FAILURES_SUBTEST(rct_signatures.p.CLSAGs[0].I[0]++)
    EXPAND_TRANSACTION_2_FAILURES_SUBTEST(rct_signatures.mixRing[0][15].dest[31]++)
    EXPAND_TRANSACTION_2_FAILURES_SUBTEST(rct_signatures.mixRing[0][15].mask[31]++)
}
