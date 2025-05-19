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

#include "carrot_impl/address_device_ram_borrowed.h"
#include "carrot_impl/tx_builder_inputs.h"
#include "carrot_mock_helpers.h"
#include "fcmp_pp/prove.h"

using namespace carrot;

//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_legacy_to_legacy_v1_mainaddr)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate(AddressDeriveType::PreCarrot);

    const cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        keys.legacy_acb.get_keys().m_account_address.m_spend_public_key,
        keys.legacy_acb.get_keys().m_view_secret_key);

    // (K^0_s, K^0_v)
    const CarrotDestinationV1 addr = keys.cryptonote_address();

    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // a
    const rct::xmr_amount amount = crypto::rand<rct::xmr_amount>();
    // z
    const rct::key amount_blinding_factor = rct::skGen();
    // k^g_o
    const crypto::secret_key sender_extension_g = mock::gen_secret_key();

    // K_o = K^0_s + k^g_o G
    rct::key onetime_address;
    rct::addKeys1(onetime_address,
        rct::sk2rct(sender_extension_g),
        rct::pk2rct(addr.address_spend_pubkey));

    // C_a = z G + a H
    const rct::key amount_commitment = rct::commit(amount, amount_blinding_factor);

    const LegacyOutputOpeningHintV1 opening_hint{
        .onetime_address = rct::rct2pk(onetime_address),
        .sender_extension_g = sender_extension_g,
        .subaddr_index = {0, 0},
        .amount = amount,
        .amount_blinding_factor = rct::rct2sk(amount_blinding_factor)
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        sender_extension_g,
        crypto::null_skey,
        rct::rct2pk(onetime_address));

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({opening_hint.onetime_address},
        {amount_commitment},
        {amount_blinding_factor},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_legacy_to_legacy_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.legacy_acb.get_keys().m_spend_secret_key,
        addr_dev,
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_legacy_to_legacy_v1_subaddr)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate(AddressDeriveType::PreCarrot);

    const cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        keys.legacy_acb.get_keys().m_account_address.m_spend_public_key,
        keys.legacy_acb.get_keys().m_view_secret_key);

    // j
    const subaddress_index_extended subaddress_index = mock::gen_subaddress_index_extended();
    
    // (K^j_s, K^j_v)
    const CarrotDestinationV1 addr = keys.subaddress(subaddress_index);

    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // a
    const rct::xmr_amount amount = crypto::rand<rct::xmr_amount>();
    // z
    const rct::key amount_blinding_factor = rct::skGen();
    // k^g_o
    const crypto::secret_key sender_extension_g = mock::gen_secret_key();

    // K_o = K^0_s + k^g_o G
    rct::key onetime_address;
    rct::addKeys1(onetime_address,
        rct::sk2rct(sender_extension_g),
        rct::pk2rct(addr.address_spend_pubkey));

    // C_a = z G + a H
    const rct::key amount_commitment = rct::commit(amount, amount_blinding_factor);

    const LegacyOutputOpeningHintV1 opening_hint{
        .onetime_address = rct::rct2pk(onetime_address),
        .sender_extension_g = sender_extension_g,
        .subaddr_index = subaddress_index.index,
        .amount = amount,
        .amount_blinding_factor = rct::rct2sk(amount_blinding_factor)
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        sender_extension_g,
        crypto::null_skey,
        rct::rct2pk(onetime_address));

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({opening_hint.onetime_address},
        {amount_commitment},
        {amount_blinding_factor},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_legacy_to_legacy_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.legacy_acb.get_keys().m_spend_secret_key,
        addr_dev,
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_legacy_v1_mainaddr_normal)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate(AddressDeriveType::PreCarrot);

    const cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        keys.legacy_acb.get_keys().m_account_address.m_spend_public_key,
        keys.legacy_acb.get_keys().m_view_secret_key);

    // (K^0_s, K^0_v)
    const CarrotDestinationV1 addr = keys.cryptonote_address();

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalV1 normal_payment_proposal{
        .destination = addr,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    RCTOutputEnoteProposal output_enote_proposal;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_normal_v1(normal_payment_proposal,
        tx_first_key_image,
        output_enote_proposal,
        encrypted_payment_id);
    
    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        encrypted_payment_id,
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = encrypted_payment_id,
        .subaddr_index = {{0, 0}, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_legacy_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.legacy_acb.get_keys().m_spend_secret_key,
        addr_dev,
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_legacy_v1_subaddr_normal)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate(AddressDeriveType::PreCarrot);

    const cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        keys.legacy_acb.get_keys().m_account_address.m_spend_public_key,
        keys.legacy_acb.get_keys().m_view_secret_key);

    // j
    const subaddress_index subaddr_index = mock::gen_subaddress_index();

    // (K^j_s, K^j_v)
    const CarrotDestinationV1 addr = keys.subaddress({subaddr_index});

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalV1 normal_payment_proposal{
        .destination = addr,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    RCTOutputEnoteProposal output_enote_proposal;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_normal_v1(normal_payment_proposal,
        tx_first_key_image,
        output_enote_proposal,
        encrypted_payment_id);
    
    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        encrypted_payment_id,
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = encrypted_payment_id,
        .subaddr_index = {subaddr_index, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_legacy_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.legacy_acb.get_keys().m_spend_secret_key,
        addr_dev,
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_legacy_v1_subaddr_special)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate(AddressDeriveType::PreCarrot);

    const cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        keys.legacy_acb.get_keys().m_account_address.m_spend_public_key,
        keys.legacy_acb.get_keys().m_view_secret_key);

    // j
    const subaddress_index subaddr_index = mock::gen_subaddress_index();

    // (K^j_s, K^j_v)
    const CarrotDestinationV1 addr = keys.subaddress({subaddr_index});

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalSelfSendV1 selfsend_payment_proposal{
        .destination_address_spend_pubkey = addr.address_spend_pubkey,
        .amount = crypto::rand<rct::xmr_amount>(),
        .enote_type = CarrotEnoteType::CHANGE
    };

    RCTOutputEnoteProposal output_enote_proposal;
    get_output_proposal_special_v1(selfsend_payment_proposal,
        keys.k_view_incoming_dev,
        tx_first_key_image,
        gen_x25519_pubkey(),
        output_enote_proposal);

    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        gen_encrypted_payment_id(),
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = std::nullopt,
        .subaddr_index = {subaddr_index, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_legacy_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.legacy_acb.get_keys().m_spend_secret_key,
        addr_dev,
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_legacy_v1_mainaddr_special)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate(AddressDeriveType::PreCarrot);

    const cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        keys.legacy_acb.get_keys().m_account_address.m_spend_public_key,
        keys.legacy_acb.get_keys().m_view_secret_key);

    // (K^0_s, K^0_v)
    const CarrotDestinationV1 addr = keys.cryptonote_address();

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalSelfSendV1 selfsend_payment_proposal{
        .destination_address_spend_pubkey = addr.address_spend_pubkey,
        .amount = crypto::rand<rct::xmr_amount>(),
        .enote_type = CarrotEnoteType::CHANGE
    };

    RCTOutputEnoteProposal output_enote_proposal;
    get_output_proposal_special_v1(selfsend_payment_proposal,
        keys.k_view_incoming_dev,
        tx_first_key_image,
        gen_x25519_pubkey(),
        output_enote_proposal);

    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        gen_encrypted_payment_id(),
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = std::nullopt,
        .subaddr_index = {{0, 0}, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_legacy_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.legacy_acb.get_keys().m_spend_secret_key,
        addr_dev,
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_carrot_v1_mainaddr_normal)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    // (K^0_s, K^0_v)
    const CarrotDestinationV1 addr = keys.cryptonote_address();

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalV1 normal_payment_proposal{
        .destination = addr,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    RCTOutputEnoteProposal output_enote_proposal;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_normal_v1(normal_payment_proposal,
        tx_first_key_image,
        output_enote_proposal,
        encrypted_payment_id);
    
    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        encrypted_payment_id,
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = encrypted_payment_id,
        .subaddr_index = {{0, 0}, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_carrot_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.k_prove_spend,
        keys.k_generate_image,
        keys.s_view_balance_dev,
        keys.k_view_incoming_dev,
        keys.s_generate_address_dev, 
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_carrot_v1_subaddr_normal)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    // j
    const subaddress_index subaddr_index = mock::gen_subaddress_index();

    // (K^j_s, K^j_v)
    const CarrotDestinationV1 addr = keys.subaddress({subaddr_index});

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalV1 normal_payment_proposal{
        .destination = addr,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    RCTOutputEnoteProposal output_enote_proposal;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_normal_v1(normal_payment_proposal,
        tx_first_key_image,
        output_enote_proposal,
        encrypted_payment_id);
    
    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        encrypted_payment_id,
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = encrypted_payment_id,
        .subaddr_index = {subaddr_index, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_carrot_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.k_prove_spend,
        keys.k_generate_image,
        keys.s_view_balance_dev,
        keys.k_view_incoming_dev,
        keys.s_generate_address_dev, 
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_carrot_v1_mainaddr_special)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    // (K^0_s, K^0_v)
    const CarrotDestinationV1 addr = keys.cryptonote_address();

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalSelfSendV1 selfsend_payment_proposal{
        .destination_address_spend_pubkey = addr.address_spend_pubkey,
        .amount = crypto::rand<rct::xmr_amount>(),
        .enote_type = CarrotEnoteType::PAYMENT
    };

    RCTOutputEnoteProposal output_enote_proposal;
    get_output_proposal_special_v1(selfsend_payment_proposal,
        keys.k_view_incoming_dev,
        tx_first_key_image,
        gen_x25519_pubkey(),
        output_enote_proposal);

    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        gen_encrypted_payment_id(),
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = std::nullopt,
        .subaddr_index = {{0, 0}, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_carrot_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.k_prove_spend,
        keys.k_generate_image,
        keys.s_view_balance_dev,
        keys.k_view_incoming_dev,
        keys.s_generate_address_dev, 
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_carrot_v1_subaddr_special)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    // j
    const subaddress_index subaddr_index = mock::gen_subaddress_index();

    // (K^j_s, K^j_v)
    const CarrotDestinationV1 addr = keys.subaddress({subaddr_index});

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalSelfSendV1 selfsend_payment_proposal{
        .destination_address_spend_pubkey = addr.address_spend_pubkey,
        .amount = crypto::rand<rct::xmr_amount>(),
        .enote_type = CarrotEnoteType::CHANGE
    };

    RCTOutputEnoteProposal output_enote_proposal;
    get_output_proposal_special_v1(selfsend_payment_proposal,
        keys.k_view_incoming_dev,
        tx_first_key_image,
        gen_x25519_pubkey(),
        output_enote_proposal);

    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        gen_encrypted_payment_id(),
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = std::nullopt,
        .subaddr_index = {subaddr_index, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_carrot_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.k_prove_spend,
        keys.k_generate_image,
        keys.s_view_balance_dev,
        keys.k_view_incoming_dev,
        keys.s_generate_address_dev, 
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_carrot_v1_mainaddr_internal)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    // (K^0_s, K^0_v)
    const CarrotDestinationV1 addr = keys.cryptonote_address();

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalSelfSendV1 selfsend_payment_proposal{
        .destination_address_spend_pubkey = addr.address_spend_pubkey,
        .amount = crypto::rand<rct::xmr_amount>(),
        .enote_type = CarrotEnoteType::PAYMENT
    };

    RCTOutputEnoteProposal output_enote_proposal;
    get_output_proposal_internal_v1(selfsend_payment_proposal,
        keys.s_view_balance_dev,
        tx_first_key_image,
        gen_x25519_pubkey(),
        output_enote_proposal);

    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        gen_encrypted_payment_id(),
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = std::nullopt,
        .subaddr_index = {{0, 0}, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_carrot_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.k_prove_spend,
        keys.k_generate_image,
        keys.s_view_balance_dev,
        keys.k_view_incoming_dev,
        keys.s_generate_address_dev, 
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_to_carrot_v1_subaddr_internal)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    // j
    const subaddress_index subaddr_index = mock::gen_subaddress_index();

    // (K^j_s, K^j_v)
    const CarrotDestinationV1 addr = keys.subaddress({subaddr_index});

    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    const CarrotPaymentProposalSelfSendV1 selfsend_payment_proposal{
        .destination_address_spend_pubkey = addr.address_spend_pubkey,
        .amount = crypto::rand<rct::xmr_amount>(),
        .enote_type = CarrotEnoteType::CHANGE
    };

    RCTOutputEnoteProposal output_enote_proposal;
    get_output_proposal_internal_v1(selfsend_payment_proposal,
        keys.s_view_balance_dev,
        tx_first_key_image,
        gen_x25519_pubkey(),
        output_enote_proposal);

    const CarrotEnoteV1 &enote = output_enote_proposal.enote;

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_enote_set({enote},
        gen_encrypted_payment_id(),
        keys,
        scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotOutputOpeningHintV1 opening_hint{
        .source_enote = enote,
        .encrypted_payment_id = std::nullopt,
        .subaddr_index = {subaddr_index, keys.default_derive_type}
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({enote.onetime_address},
        {enote.amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_to_carrot_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.k_prove_spend,
        keys.k_generate_image,
        keys.s_view_balance_dev,
        keys.k_view_incoming_dev,
        keys.s_generate_address_dev, 
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_coinbase_to_legacy_v1)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate(AddressDeriveType::PreCarrot);

    const cryptonote_hierarchy_address_device_ram_borrowed addr_dev(
        keys.legacy_acb.get_keys().m_account_address.m_spend_public_key,
        keys.legacy_acb.get_keys().m_view_secret_key);

    // (K^0_s, K^0_v)
    const CarrotDestinationV1 addr = keys.cryptonote_address();

    const CarrotPaymentProposalV1 normal_payment_proposal{
        .destination = addr,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    const uint64_t block_index = mock::gen_block_index();

    CarrotCoinbaseEnoteV1 coinbase_enote;
    get_coinbase_output_proposal_v1(normal_payment_proposal,
        block_index,
        coinbase_enote);
    const rct::key coinbase_enote_amount_commitment = rct::zeroCommitVartime(coinbase_enote.amount);

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_coinbase_enote_set({coinbase_enote}, keys, scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotCoinbaseOutputOpeningHintV1 opening_hint{
        .source_enote = coinbase_enote,
        .derive_type = keys.default_derive_type
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        coinbase_enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({coinbase_enote.onetime_address},
        {coinbase_enote_amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_coinbase_to_legacy_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.legacy_acb.get_keys().m_spend_secret_key,
        addr_dev,
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_tx_builder, make_sal_proof_carrot_coinbase_to_carrot_v1)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    // (K^0_s, K^0_v)
    const CarrotDestinationV1 addr = keys.cryptonote_address();

    const CarrotPaymentProposalV1 normal_payment_proposal{
        .destination = addr,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    const uint64_t block_index = mock::gen_block_index();

    CarrotCoinbaseEnoteV1 coinbase_enote;
    get_coinbase_output_proposal_v1(normal_payment_proposal,
        block_index,
        coinbase_enote);
    const rct::key coinbase_enote_amount_commitment = rct::zeroCommitVartime(coinbase_enote.amount);

    // scan enote to get sender extensions and calculate expected key image
    std::vector<mock::mock_scan_result_t> scan_results;
    mock::mock_scan_coinbase_enote_set({coinbase_enote}, keys, scan_results);

    ASSERT_EQ(1, scan_results.size());

    const mock::mock_scan_result_t &scan_result = scan_results.front();

    ASSERT_EQ(addr.address_spend_pubkey, scan_result.address_spend_pubkey);

    const CarrotCoinbaseOutputOpeningHintV1 opening_hint{
        .source_enote = coinbase_enote,
        .derive_type = keys.default_derive_type
    };

    const crypto::key_image expected_key_image = keys.derive_key_image(addr.address_spend_pubkey,
        scan_result.sender_extension_g,
        scan_result.sender_extension_t,
        coinbase_enote.onetime_address);

    // fake output amount blinding factor in a hypothetical tx where we spent the aforementioned output
    const rct::key output_amount_blinding_factor = rct::skGen();
    const crypto::hash signable_tx_hash = crypto::rand<crypto::hash>();

    // make rerandomized outputs
    std::vector<FcmpRerandomizedOutputCompressed> rerandomized_outputs;
    make_carrot_rerandomized_outputs_nonrefundable({coinbase_enote.onetime_address},
        {coinbase_enote_amount_commitment},
        {rct::sk2rct(scan_result.amount_blinding_factor)},
        {output_amount_blinding_factor},
        rerandomized_outputs);

    ASSERT_EQ(1, rerandomized_outputs.size());

    // make SA/L proof for spending aforementioned enote
    fcmp_pp::FcmpPpSalProof sal_proof;
    crypto::key_image actual_key_image;
    make_sal_proof_carrot_coinbase_to_carrot_v1(signable_tx_hash,
        rerandomized_outputs.front(),
        opening_hint,
        keys.k_prove_spend,
        keys.k_generate_image,
        keys.k_view_incoming_dev,
        sal_proof,
        actual_key_image);

    ASSERT_EQ(expected_key_image, actual_key_image);

    // verify SA/L
    EXPECT_TRUE(fcmp_pp::verify_sal(signable_tx_hash,
        rerandomized_outputs.front().input,
        actual_key_image,
        sal_proof));
}
//----------------------------------------------------------------------------------------------------------------------
