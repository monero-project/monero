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

#include "cryptonote_core/cryptonote_tx_utils.h"
#include "cryptonote_core/tx_verification_utils.h"

TEST(tx_verification_utils, make_input_verification_id_ring)
{
    rct::key key1, key2, key3;
    epee::from_hex::to_buffer(epee::as_mut_byte_span(key1), "e50f476129d40af31e0938743f7f2d60e867aab31294f7acaf6e38f0976f0228");
    epee::from_hex::to_buffer(epee::as_mut_byte_span(key2), "e50f476129d40af31e0938743f7f2d60e867aab31294f7acaf6e38f0976f0227");
    epee::from_hex::to_buffer(epee::as_mut_byte_span(key3), "d50f476129d40af31e0938743f7f2d60e867aab31294f7acaf6e38f0976f0228");

    const crypto::hash hash1 = cryptonote::make_input_verification_id(rct::rct2hash(key1), rct::ctkeyM{});
    const crypto::hash hash2 = cryptonote::make_input_verification_id(rct::rct2hash(key2), rct::ctkeyM{});    
    const crypto::hash hash3 = cryptonote::make_input_verification_id(rct::rct2hash(key3), rct::ctkeyM{});
    ASSERT_NE(hash1, hash2);
    ASSERT_NE(hash1, hash3);
    ASSERT_NE(hash2, hash3);

    const crypto::hash hash4 = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key1}}});
    const crypto::hash hash5 = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key2}}});
    const crypto::hash hash6 = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key3}}});
    ASSERT_NE(hash4, hash5);
    ASSERT_NE(hash4, hash6);
    ASSERT_NE(hash5, hash6);

    const crypto::hash hash7 = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key1},{key1, key1}}});
    const crypto::hash hash8 = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key1}},{{key1, key1}}});
    ASSERT_NE(hash7, hash8);

    const crypto::hash hash1_eq = cryptonote::make_input_verification_id(rct::rct2hash(key1), rct::ctkeyM{});
    const crypto::hash hash2_eq = cryptonote::make_input_verification_id(rct::rct2hash(key2), rct::ctkeyM{});    
    const crypto::hash hash3_eq = cryptonote::make_input_verification_id(rct::rct2hash(key3), rct::ctkeyM{});
    const crypto::hash hash4_eq = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key1}}});
    const crypto::hash hash5_eq = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key2}}});
    const crypto::hash hash6_eq = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key3}}});
    const crypto::hash hash7_eq = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key1},{key1, key1}}});
    const crypto::hash hash8_eq = cryptonote::make_input_verification_id(rct::rct2hash(key1), {{{key1, key1}},{{key1, key1}}});

    ASSERT_EQ(hash1, hash1_eq);
    ASSERT_EQ(hash2, hash2_eq);
    ASSERT_EQ(hash3, hash3_eq);
    ASSERT_EQ(hash4, hash4_eq);
    ASSERT_EQ(hash5, hash5_eq);
    ASSERT_EQ(hash6, hash6_eq);
    ASSERT_EQ(hash7, hash7_eq);
    ASSERT_EQ(hash8, hash8_eq);
}

TEST(tx_verification_utils, ver_input_proofs_rings)
{
    // constants
    static constexpr size_t N_INPUTS = 2;
    static constexpr size_t N_OUTPUTS = 10;
    static constexpr size_t N_RING_MEMBERS = 16;
    static constexpr bool USE_VIEW_TAGS = true;
    static constexpr rct::RCTConfig RCT_CONFIG{ rct::RangeProofPaddedBulletproof, 4 }; // CLSAG, BP+
    static constexpr uint8_t HF_VERSION = HF_VERSION_VIEW_TAGS + 1; // CLSAG, BP+, after grace period

    // generate accounts
    hw::device &hwdev = hw::get_device("default");

    cryptonote::account_base alice;
    alice.generate();
    const cryptonote::account_public_address &alice_main_addr = alice.get_keys().m_account_address;
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> alice_subaddresses{
        {alice_main_addr.m_spend_public_key, {}}
    };

    cryptonote::account_base bob;
    bob.generate();
    const cryptonote::account_public_address &bob_main_addr = bob.get_keys().m_account_address;

    cryptonote::account_base aether;
    aether.generate();

    // populate inputs
    rct::xmr_amount total_input_amounts = 0;
    std::vector<cryptonote::tx_source_entry> sources;
    sources.reserve(N_INPUTS);
    for (size_t i = 0; i < N_INPUTS; ++i)
    {
        const rct::xmr_amount in_amount = crypto::rand_range<rct::xmr_amount>(0, COIN) + COIN; // [1, 2] XMR
        const size_t real_in_ring_idx = crypto::rand_idx(N_RING_MEMBERS);

        // generate one-time address from derivation
        crypto::secret_key in_main_tx_privkey;
        crypto::public_key in_main_tx_pubkey;
        crypto::generate_keys(in_main_tx_pubkey, in_main_tx_privkey); // (r, R)
        crypto::secret_key_to_public_key(in_main_tx_privkey, in_main_tx_pubkey);
        crypto::key_derivation ecdh;
        ASSERT_TRUE(hwdev.generate_key_derivation(in_main_tx_pubkey, alice.get_keys().m_view_secret_key, ecdh));
        const size_t real_output_in_tx_index = crypto::rand_idx(N_OUTPUTS);

        crypto::public_key in_onetime_address;
        crypto::view_tag in_view_tag;
        std::vector<crypto::public_key> in_additional_tx_public_keys;
        std::vector<rct::key> in_amount_keys;
        ASSERT_TRUE(hwdev.generate_output_ephemeral_keys(/*tx_version=*/2,
            aether.get_keys(), in_main_tx_pubkey, in_main_tx_privkey,
            {0, alice_main_addr, false}, /*change_addr=*/boost::none, real_output_in_tx_index,
            /*need_additional_txkeys=*/false, /*additional_tx_keys=*/{},
            in_additional_tx_public_keys,
            in_amount_keys, in_onetime_address,
            USE_VIEW_TAGS, in_view_tag));
        ASSERT_EQ(1, in_amount_keys.size());

        const rct::key in_amount_blinding_factor = rct::genCommitmentMask(in_amount_keys.at(0));
        const rct::key in_amount_commitment = rct::commit(in_amount, in_amount_blinding_factor);

        // randomly populate decoys and insert real spend
        auto &tx_source = sources.emplace_back();
        tx_source.outputs.reserve(N_RING_MEMBERS);
        for (size_t j = 0; j < N_RING_MEMBERS; ++j)
        {
            const size_t ring_member_global_output_idx = 20 * j;
            if (j == real_in_ring_idx)
            {
                tx_source.outputs.emplace_back(ring_member_global_output_idx,
                    rct::ctkey{rct::pk2rct(in_onetime_address), in_amount_commitment});
            }
            else // decoy
            {
                tx_source.outputs.emplace_back(ring_member_global_output_idx,
                    rct::ctkey{rct::pkGen(), rct::pkGen()});
            }
        }

        tx_source.real_output = real_in_ring_idx;
        tx_source.real_out_tx_key = in_main_tx_pubkey;
        tx_source.real_out_additional_tx_keys = in_additional_tx_public_keys;
        tx_source.real_output_in_tx_index = real_output_in_tx_index;
        tx_source.amount = in_amount;
        tx_source.rct = true;
        tx_source.mask = in_amount_blinding_factor;
        tx_source.multisig_kLRki = {};

        total_input_amounts += in_amount;
    }

    // populate destinations
    const rct::xmr_amount approx_fee = 500000000000; // 0.5 XMR
    const rct::xmr_amount dest_amount = (total_input_amounts - approx_fee) / N_OUTPUTS;
    std::vector<cryptonote::tx_destination_entry> destinations;
    destinations.reserve(N_OUTPUTS);
    for (size_t i = 0; i < N_OUTPUTS - 1; ++i)
        destinations.push_back(cryptonote::tx_destination_entry(dest_amount, bob_main_addr, false));
    destinations.push_back(cryptonote::tx_destination_entry(dest_amount, alice_main_addr, false));

    // construct transaction
    cryptonote::transaction tx;
    crypto::secret_key main_tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    ASSERT_TRUE(cryptonote::construct_tx_and_get_tx_key(alice.get_keys(),
        alice_subaddresses,
        sources,
        destinations,
        alice_main_addr,
        /*extra=*/{},
        tx,
        main_tx_key,
        additional_tx_keys,
        /*rct=*/true,
        RCT_CONFIG,
        USE_VIEW_TAGS));
    ASSERT_EQ(N_INPUTS, tx.vin.size());
    ASSERT_EQ(N_OUTPUTS, tx.vout.size());
    ASSERT_EQ(N_RING_MEMBERS, boost::get<cryptonote::txin_to_key>(tx.vin.at(0)).key_offsets.size());
    ASSERT_GE(tx.rct_signatures.txnFee, approx_fee);
    ASSERT_LE(tx.rct_signatures.txnFee, approx_fee + N_OUTPUTS);

    // collect mix rings
    rct::ctkeyM mixrings(N_INPUTS);
    for (size_t i = 0; i < N_INPUTS; ++i)
    {
        mixrings.at(i).resize(N_RING_MEMBERS);
        for (size_t j = 0; j < N_RING_MEMBERS; ++j)
        {
            mixrings.at(i).at(j) = sources.at(i).outputs.at(j).second;
        }
    }

    // serialize transaction to blob
    const cryptonote::blobdata tx_blob = cryptonote::tx_to_blob(tx);

    // de-serialize transaction from blob
    cryptonote::transaction deserialized_tx;
    ASSERT_TRUE(cryptonote::parse_and_validate_tx_from_blob(tx_blob, deserialized_tx));

    // test non-input consensus rules
    cryptonote::tx_verification_context tvc{};
    ASSERT_TRUE(cryptonote::ver_non_input_consensus(deserialized_tx, tvc, HF_VERSION));
    ASSERT_FALSE(tvc.m_verifivation_failed);

    // test verify input rings [positive]
    EXPECT_TRUE(cryptonote::ver_input_proofs_rings(deserialized_tx, mixrings));

    // test verify input rings again (already expanded) [positive]
    EXPECT_TRUE(cryptonote::ver_input_proofs_rings(deserialized_tx, mixrings));

    // test verify input rings after modify to expansion [positive]
    deserialized_tx.rct_signatures.mixRing.at(0).at(0) = {rct::pkGen(), rct::pkGen()};
    EXPECT_TRUE(cryptonote::ver_input_proofs_rings(deserialized_tx, mixrings));

    // test verify input rings after modify to dereferenced mixring [negative]
    rct::ctkeyM modified_mixrings = mixrings;
    modified_mixrings.at(0).at(0) = {rct::pkGen(), rct::pkGen()};
    EXPECT_FALSE(cryptonote::ver_input_proofs_rings(deserialized_tx, modified_mixrings));
    modified_mixrings = mixrings;
    modified_mixrings.at(0).at(1) = {rct::pkGen(), rct::pkGen()};
    EXPECT_FALSE(cryptonote::ver_input_proofs_rings(deserialized_tx, modified_mixrings));

    // test verify input rings after add dereferenced mixring [negative]
    modified_mixrings = mixrings;
    modified_mixrings.emplace_back();
    EXPECT_FALSE(cryptonote::ver_input_proofs_rings(deserialized_tx, modified_mixrings));

    // test verify input rings after remove dereferenced mixring [negative]
    modified_mixrings = mixrings;
    modified_mixrings.pop_back();
    EXPECT_FALSE(cryptonote::ver_input_proofs_rings(deserialized_tx, modified_mixrings));

    // test verify input rings after add dereferenced decoy [negative]
    modified_mixrings = mixrings;
    modified_mixrings.at(0).push_back({rct::pkGen(), rct::pkGen()});
    EXPECT_FALSE(cryptonote::ver_input_proofs_rings(deserialized_tx, modified_mixrings));

    // test verify input rings after remove dereferenced decoy [negative]
    {
        modified_mixrings = mixrings;
        rct::ctkeyV &mixring0 = modified_mixrings.at(0);
        mixring0.erase(mixring0.begin());
        EXPECT_FALSE(cryptonote::ver_input_proofs_rings(deserialized_tx, modified_mixrings));
    }
    {
        modified_mixrings = mixrings;
        rct::ctkeyV &mixring0 = modified_mixrings.at(0);
        mixring0.erase(mixring0.begin() + 1);
        EXPECT_FALSE(cryptonote::ver_input_proofs_rings(deserialized_tx, modified_mixrings));
    }
}
