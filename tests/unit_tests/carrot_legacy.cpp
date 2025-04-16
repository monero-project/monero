// Copyright (c) 2024, The Monero Project
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

#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/output_set_finalization.h"
#include "carrot_core/payment_proposal.h"
#include "carrot_core/scan.h"
#include "crypto/crypto.h"
#include "crypto/generators.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/subaddress_index.h"
#include "device/device_default.hpp"
#include "ringct/rctOps.h"

using namespace carrot;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool can_open_fcmp_onetime_address_from_legacy_addr(const cryptonote::account_keys &keys,
    const uint32_t j_major,
    const uint32_t j_minor,
    const crypto::secret_key &sender_extension_g,
    const crypto::secret_key &sender_extension_t,
    const crypto::public_key &onetime_address)
{
    // K_s = k_s G
    // m = Hn(k_v || j_major || j_minor) if j else 0
    // K^j_s = K_s + m G
    // Ko = K^j_s + k^o_g G + k^o_t T
    //    = (k^o_g + m + k_s) G + k^o_t T

    // m = Hn(k_v || j_major || j_minor) if j else 0
    crypto::secret_key subaddress_ext{};
    if (j_major || j_minor)
        subaddress_ext = keys.get_device().get_subaddress_secret_key(keys.m_view_secret_key, {j_major, j_minor});

    // combined_g = k^o_g + m + k_s
    rct::key combined_g;
    sc_add(combined_g.bytes, to_bytes(sender_extension_g), to_bytes(subaddress_ext));
    sc_add(combined_g.bytes, combined_g.bytes, to_bytes(keys.m_spend_secret_key));

    // Ko' = combined_g G + k^o_t T
    rct::key recomputed_onetime_address;
    rct::addKeys2(recomputed_onetime_address, combined_g, rct::sk2rct(sender_extension_t), rct::pk2rct(crypto::get_T()));

    // Ko' ?= Ko
    return recomputed_onetime_address == onetime_address;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct unittest_legacy_scan_result_t
{
    crypto::public_key address_spend_pubkey = rct::rct2pk(rct::I);
    crypto::secret_key sender_extension_g = rct::rct2sk(rct::I);
    crypto::secret_key sender_extension_t = rct::rct2sk(rct::I);

    rct::xmr_amount amount = 0;
    crypto::secret_key amount_blinding_factor = rct::rct2sk(rct::I);

    CarrotEnoteType enote_type = CarrotEnoteType::PAYMENT;

    payment_id_t payment_id = null_payment_id;

    size_t output_index = 0;
};
static void unittest_legacy_scan_enote_set(const std::vector<CarrotEnoteV1> &enotes,
    const encrypted_payment_id_t encrypted_payment_id,
    const cryptonote::account_base acb,
    std::vector<unittest_legacy_scan_result_t> &res)
{
    res.clear();

    // external scans
    for (size_t output_index = 0; output_index < enotes.size(); ++output_index)
    {
        const CarrotEnoteV1 &enote = enotes.at(output_index);

        // s_sr = k_v D_e
        mx25519_pubkey s_sr;
        make_carrot_uncontextualized_shared_key_receiver(acb.get_keys().m_view_secret_key,
            enote.enote_ephemeral_pubkey,
            s_sr);

        unittest_legacy_scan_result_t scan_result{};
        const bool r = try_scan_carrot_enote_external_receiver(enote,
            encrypted_payment_id,
            s_sr,
            {&acb.get_keys().m_account_address.m_spend_public_key, 1},
            view_incoming_key_ram_borrowed_device(acb.get_keys().m_view_secret_key),
            scan_result.sender_extension_g,
            scan_result.sender_extension_t,
            scan_result.address_spend_pubkey,
            scan_result.amount,
            scan_result.amount_blinding_factor,
            scan_result.payment_id,
            scan_result.enote_type);
        
        scan_result.output_index = output_index;

        if (r)
            res.push_back(scan_result);
    }
}
//----------------------------------------------------------------------------------------------------------------------
static void subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(const bool alice_subaddress,
    const bool bob_subaddress,
    const bool bob_integrated,
    const CarrotEnoteType alice_selfsend_type)
{
    hw::device &hwdev = hw::get_device("default");

    // generate alice keys and address
    cryptonote::account_base alice;
    alice.generate();
    const uint32_t alice_j_major = alice_subaddress ? crypto::rand<uint32_t>() : 0;
    const uint32_t alice_j_minor = alice_subaddress ? crypto::rand<uint32_t>() : 0;
    CarrotDestinationV1 alice_address{};
    cryptonote::account_public_address subaddr = hwdev.get_subaddress(alice.get_keys(),
            {alice_j_major, alice_j_minor});
    alice_address.address_spend_pubkey = subaddr.m_spend_public_key;
    alice_address.address_view_pubkey = subaddr.m_view_public_key;
    alice_address.is_subaddress = alice_subaddress;
    alice_address.payment_id = null_payment_id;

    // generate bob keys and address
    cryptonote::account_base bob;
    bob.generate();
    const uint32_t bob_j_major = bob_subaddress ? crypto::rand<uint32_t>() : 0;
    const uint32_t bob_j_minor = bob_subaddress ? crypto::rand<uint32_t>() : 0;
    CarrotDestinationV1 bob_address{};
    subaddr = hwdev.get_subaddress(bob.get_keys(), {bob_j_major, bob_j_minor});
    bob_address.address_spend_pubkey = subaddr.m_spend_public_key;
    bob_address.address_view_pubkey = subaddr.m_view_public_key;
    bob_address.is_subaddress = bob_subaddress;
    bob_address.payment_id = bob_integrated ? gen_payment_id() : null_payment_id;

    // generate input context
    const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen());
    const input_context_t input_context = make_carrot_input_context(tx_first_key_image);

    // outgoing payment proposal to bob
    const CarrotPaymentProposalV1 bob_payment_proposal = CarrotPaymentProposalV1{
        .destination = bob_address,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .randomness = gen_janus_anchor()
    };

    // selfsend payment proposal to alice
    const CarrotPaymentProposalSelfSendV1 alice_payment_proposal = CarrotPaymentProposalSelfSendV1{
        .destination_address_spend_pubkey = alice_address.address_spend_pubkey,
        .amount = crypto::rand_idx<rct::xmr_amount>(1000000),
        .enote_type = CarrotEnoteType::CHANGE,
        .enote_ephemeral_pubkey = get_enote_ephemeral_pubkey(bob_payment_proposal, input_context)
    };

    // alice mem devices
    view_incoming_key_ram_borrowed_device alive_k_v_dev(alice.get_keys().m_view_secret_key);

    // turn payment proposals into enotes, always pass dummy pid
    std::vector<RCTOutputEnoteProposal> enote_proposals;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_enote_proposals({bob_payment_proposal},
        {alice_payment_proposal},
        gen_payment_id(),
        nullptr,
        &alive_k_v_dev,
        tx_first_key_image,
        enote_proposals,
        encrypted_payment_id);

    ASSERT_EQ(2, enote_proposals.size()); // 2-out tx

    // collect enotes
    std::vector<CarrotEnoteV1> enotes;
    for (const RCTOutputEnoteProposal &enote_proposal : enote_proposals)
        enotes.push_back(enote_proposal.enote);

    // check that alice scanned 1 enote
    std::vector<unittest_legacy_scan_result_t> alice_scan_vec;
    unittest_legacy_scan_enote_set(enotes, encrypted_payment_id, alice, alice_scan_vec);
    ASSERT_EQ(1, alice_scan_vec.size());
    unittest_legacy_scan_result_t alice_scan = alice_scan_vec.front();

    // check that bob scanned 1 enote
    std::vector<unittest_legacy_scan_result_t> bob_scan_vec;
    unittest_legacy_scan_enote_set(enotes, encrypted_payment_id, bob, bob_scan_vec);
    ASSERT_EQ(1, bob_scan_vec.size());
    unittest_legacy_scan_result_t bob_scan = bob_scan_vec.front();

    // set named references to enotes
    ASSERT_TRUE((alice_scan.output_index == 0 && bob_scan.output_index == 1) ||
        (alice_scan.output_index == 1 && bob_scan.output_index == 0));
    const CarrotEnoteV1 &alice_enote = enotes.at(alice_scan.output_index);
    const CarrotEnoteV1 &bob_enote = enotes.at(bob_scan.output_index);

    // check Alice's recovered data
    EXPECT_EQ(alice_payment_proposal.destination_address_spend_pubkey, alice_scan.address_spend_pubkey);
    EXPECT_EQ(alice_payment_proposal.amount, alice_scan.amount);
    EXPECT_EQ(alice_enote.amount_commitment, rct::commit(alice_scan.amount, rct::sk2rct(alice_scan.amount_blinding_factor)));
    EXPECT_EQ(null_payment_id, alice_scan.payment_id);
    EXPECT_EQ(alice_payment_proposal.enote_type, alice_scan.enote_type);

    // check Bob's recovered data
    EXPECT_EQ(bob_payment_proposal.destination.address_spend_pubkey, bob_scan.address_spend_pubkey);
    EXPECT_EQ(bob_payment_proposal.amount, bob_scan.amount);
    EXPECT_EQ(bob_enote.amount_commitment, rct::commit(bob_scan.amount, rct::sk2rct(bob_scan.amount_blinding_factor)));
    EXPECT_EQ(bob_integrated ? bob_address.payment_id : null_payment_id, bob_scan.payment_id);
    EXPECT_EQ(CarrotEnoteType::PAYMENT, bob_scan.enote_type);

    // check Alice spendability
    EXPECT_TRUE(can_open_fcmp_onetime_address_from_legacy_addr(alice.get_keys(),
        alice_j_major,
        alice_j_minor,
        alice_scan.sender_extension_g,
        alice_scan.sender_extension_t,
        alice_enote.onetime_address));
    
    // check Bob spendability
    EXPECT_TRUE(can_open_fcmp_onetime_address_from_legacy_addr(bob.get_keys(),
        bob_j_major,
        bob_j_minor,
        bob_scan.sender_extension_g,
        bob_scan.sender_extension_t,
        bob_enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_legacy, legacy_get_enote_output_proposals_main2main_completeness)
{
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(false, false, false, CarrotEnoteType::PAYMENT);
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(false, false, false, CarrotEnoteType::CHANGE);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_legacy, legacy_get_enote_output_proposals_main2sub_completeness)
{
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(false, true, false, CarrotEnoteType::PAYMENT);
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(false, true, false, CarrotEnoteType::CHANGE);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_legacy, legacy_get_enote_output_proposals_main2integ_completeness)
{
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(false, false, true, CarrotEnoteType::PAYMENT);
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(false, false, true, CarrotEnoteType::CHANGE);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_legacy, legacy_get_enote_output_proposals_sub2main_completeness)
{
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(true, false, false, CarrotEnoteType::PAYMENT);
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(true, false, false, CarrotEnoteType::CHANGE);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_legacy, legacy_get_enote_output_proposals_sub2sub_completeness)
{
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(true, true, false, CarrotEnoteType::PAYMENT);
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(true, true, false, CarrotEnoteType::CHANGE);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_legacy, legacy_get_enote_output_proposals_sub2integ_completeness)
{
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(true, false, true, CarrotEnoteType::PAYMENT);
    subtest_legacy_2out_transfer_get_output_enote_proposals_completeness(true, false, true, CarrotEnoteType::CHANGE);
}
//----------------------------------------------------------------------------------------------------------------------
