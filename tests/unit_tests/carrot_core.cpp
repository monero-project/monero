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

#include "carrot_core/output_set_finalization.h"
#include "carrot_core/payment_proposal.h"
#include "carrot_core/scan.h"
#include "carrot_core/scan_unsafe.h"
#include "carrot_mock_helpers.h"

using namespace carrot;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "unit_tests.carrot"

//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, ECDH_cryptonote_completeness)
{
    crypto::secret_key k_view = rct::rct2sk(rct::skGen());
    crypto::public_key view_pubkey = rct::rct2pk(rct::scalarmultBase(rct::sk2rct(k_view)));
    crypto::secret_key k_ephem = rct::rct2sk(rct::skGen());
    ASSERT_NE(k_view, k_ephem);

    mx25519_pubkey enote_ephemeral_pubkey;
    make_carrot_enote_ephemeral_pubkey_cryptonote(k_ephem, enote_ephemeral_pubkey);

    mx25519_pubkey s_sr_sender;
    ASSERT_TRUE(make_carrot_uncontextualized_shared_key_sender(k_ephem, view_pubkey, s_sr_sender));

    mx25519_pubkey s_sr_receiver;
    ASSERT_TRUE(make_carrot_uncontextualized_shared_key_receiver(k_view, enote_ephemeral_pubkey, s_sr_receiver));

    EXPECT_EQ(s_sr_sender, s_sr_receiver);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, ECDH_subaddress_completeness)
{
    crypto::secret_key k_view = rct::rct2sk(rct::skGen());
    crypto::public_key spend_pubkey = rct::rct2pk(rct::pkGen());
    crypto::public_key view_pubkey = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(spend_pubkey), rct::sk2rct(k_view)));
    crypto::secret_key k_ephem = rct::rct2sk(rct::skGen());
    ASSERT_NE(k_view, k_ephem);

    mx25519_pubkey enote_ephemeral_pubkey;
    make_carrot_enote_ephemeral_pubkey_subaddress(k_ephem, spend_pubkey, enote_ephemeral_pubkey);

    mx25519_pubkey s_sr_sender;
    ASSERT_TRUE(make_carrot_uncontextualized_shared_key_sender(k_ephem, view_pubkey, s_sr_sender));

    mx25519_pubkey s_sr_receiver;
    ASSERT_TRUE(make_carrot_uncontextualized_shared_key_receiver(k_view, enote_ephemeral_pubkey, s_sr_receiver));

    EXPECT_EQ(s_sr_sender, s_sr_receiver);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, ECDH_mx25519_convergence)
{
    const mx25519_pubkey P = gen_x25519_pubkey();
    const crypto::secret_key a = rct::rct2sk(rct::skGen());

    const mx25519_impl *impl = mx25519_select_impl(MX25519_TYPE_AUTO);
    ASSERT_NE(nullptr, impl);

    // do Q = a * P using mx25519
    mx25519_pubkey Q_mx25519;
    mx25519_scmul_key(impl, &Q_mx25519, reinterpret_cast<const mx25519_privkey*>(&a), &P);

    // do Q = a * P using make_carrot_uncontextualized_shared_key_receiver()
    mx25519_pubkey Q_carrot;
    ASSERT_TRUE(make_carrot_uncontextualized_shared_key_receiver(a, P, Q_carrot));

    // check equal
    EXPECT_EQ(Q_mx25519, Q_carrot);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, main_address_normal_scan_completeness)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    const CarrotDestinationV1 main_address = keys.cryptonote_address();

    const CarrotPaymentProposalV1 proposal = CarrotPaymentProposalV1{
        .destination = main_address,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

    RCTOutputEnoteProposal enote_proposal;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_normal_v1(proposal,
        tx_first_key_image,
        enote_proposal,
        encrypted_payment_id);

    ASSERT_EQ(proposal.amount, enote_proposal.amount);
    const rct::key recomputed_amount_commitment = rct::commit(enote_proposal.amount, rct::sk2rct(enote_proposal.amount_blinding_factor));
    ASSERT_EQ(enote_proposal.enote.amount_commitment, recomputed_amount_commitment);

    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(keys.legacy_acb.get_keys().m_view_secret_key,
        enote_proposal.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external_receiver(enote_proposal.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&keys.carrot_account_spend_pubkey, 1},
        keys.k_view_incoming_dev,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        recovered_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_payment_id,
        recovered_enote_type);
    
    ASSERT_TRUE(scan_success);

    // check recovered data
    EXPECT_EQ(proposal.destination.address_spend_pubkey, recovered_address_spend_pubkey);
    EXPECT_EQ(proposal.amount, recovered_amount);
    EXPECT_EQ(enote_proposal.amount_blinding_factor, recovered_amount_blinding_factor);
    EXPECT_EQ(null_payment_id, recovered_payment_id);
    EXPECT_EQ(CarrotEnoteType::PAYMENT, recovered_enote_type);

    // check spendability
    EXPECT_TRUE(keys.can_open_fcmp_onetime_address(recovered_address_spend_pubkey,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote_proposal.enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, subaddress_normal_scan_completeness)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    const uint32_t j_major = crypto::rand_idx(mock::MAX_SUBADDRESS_MAJOR_INDEX);
    const uint32_t j_minor = crypto::rand_idx(mock::MAX_SUBADDRESS_MINOR_INDEX);

    const CarrotDestinationV1 subaddress = keys.subaddress({{j_major, j_minor}});

    const CarrotPaymentProposalV1 proposal = CarrotPaymentProposalV1{
        .destination = subaddress,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

    RCTOutputEnoteProposal enote_proposal;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_normal_v1(proposal,
        tx_first_key_image,
        enote_proposal,
        encrypted_payment_id);

    ASSERT_EQ(proposal.amount, enote_proposal.amount);
    const rct::key recomputed_amount_commitment = rct::commit(enote_proposal.amount, rct::sk2rct(enote_proposal.amount_blinding_factor));
    ASSERT_EQ(enote_proposal.enote.amount_commitment, recomputed_amount_commitment);

    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(keys.legacy_acb.get_keys().m_view_secret_key,
        enote_proposal.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external_receiver(enote_proposal.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&keys.carrot_account_spend_pubkey, 1},
        keys.k_view_incoming_dev,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        recovered_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_payment_id,
        recovered_enote_type);
    
    ASSERT_TRUE(scan_success);

    // check recovered data
    EXPECT_EQ(proposal.destination.address_spend_pubkey, recovered_address_spend_pubkey);
    EXPECT_EQ(proposal.amount, recovered_amount);
    EXPECT_EQ(enote_proposal.amount_blinding_factor, recovered_amount_blinding_factor);
    EXPECT_EQ(null_payment_id, recovered_payment_id);
    EXPECT_EQ(CarrotEnoteType::PAYMENT, recovered_enote_type);

    // check spendability
    EXPECT_TRUE(keys.can_open_fcmp_onetime_address(recovered_address_spend_pubkey,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote_proposal.enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, integrated_address_normal_scan_completeness)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    const CarrotDestinationV1 integrated_address = keys.cryptonote_address(gen_payment_id());

    const CarrotPaymentProposalV1 proposal = CarrotPaymentProposalV1{
        .destination = integrated_address,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

    RCTOutputEnoteProposal enote_proposal;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_normal_v1(proposal,
        tx_first_key_image,
        enote_proposal,
        encrypted_payment_id);

    ASSERT_EQ(proposal.amount, enote_proposal.amount);
    const rct::key recomputed_amount_commitment = rct::commit(enote_proposal.amount, rct::sk2rct(enote_proposal.amount_blinding_factor));
    ASSERT_EQ(enote_proposal.enote.amount_commitment, recomputed_amount_commitment);

    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(keys.legacy_acb.get_keys().m_view_secret_key,
        enote_proposal.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external_receiver(enote_proposal.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&keys.carrot_account_spend_pubkey, 1},
        keys.k_view_incoming_dev,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        recovered_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_payment_id,
        recovered_enote_type);
    
    ASSERT_TRUE(scan_success);

    // check recovered data
    EXPECT_EQ(proposal.destination.address_spend_pubkey, recovered_address_spend_pubkey);
    EXPECT_EQ(proposal.amount, recovered_amount);
    EXPECT_EQ(enote_proposal.amount_blinding_factor, recovered_amount_blinding_factor);
    EXPECT_EQ(integrated_address.payment_id, recovered_payment_id);
    EXPECT_EQ(CarrotEnoteType::PAYMENT, recovered_enote_type);

    // check spendability
    EXPECT_TRUE(keys.can_open_fcmp_onetime_address(recovered_address_spend_pubkey,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote_proposal.enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, main_address_special_scan_completeness)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    // try once with PAYMENT, once with CHANGE
    for (int i = 0; i < 2; ++i)
    {
        const CarrotEnoteType enote_type = i ? CarrotEnoteType::PAYMENT : CarrotEnoteType::CHANGE;

        const CarrotPaymentProposalSelfSendV1 proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = keys.carrot_account_spend_pubkey,
            .amount = crypto::rand<rct::xmr_amount>(),
            .enote_type = enote_type,
            .enote_ephemeral_pubkey = gen_x25519_pubkey(),
        };

        const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

        RCTOutputEnoteProposal enote_proposal;
        get_output_proposal_special_v1(proposal,
            keys.k_view_incoming_dev,
            tx_first_key_image,
            std::nullopt,
            enote_proposal);

        ASSERT_EQ(proposal.amount, enote_proposal.amount);
        const rct::key recomputed_amount_commitment = rct::commit(enote_proposal.amount, rct::sk2rct(enote_proposal.amount_blinding_factor));
        ASSERT_EQ(enote_proposal.enote.amount_commitment, recomputed_amount_commitment);

        mx25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(keys.legacy_acb.get_keys().m_view_secret_key,
            enote_proposal.enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        payment_id_t recovered_payment_id;
        CarrotEnoteType recovered_enote_type;
        const bool scan_success = try_scan_carrot_enote_external_receiver(enote_proposal.enote,
            std::nullopt,
            s_sender_receiver_unctx,
            {&keys.carrot_account_spend_pubkey, 1},
            keys.k_view_incoming_dev,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            recovered_address_spend_pubkey,
            recovered_amount,
            recovered_amount_blinding_factor,
            recovered_payment_id,
            recovered_enote_type);
        
        ASSERT_TRUE(scan_success);

        // check recovered data
        EXPECT_EQ(proposal.destination_address_spend_pubkey, recovered_address_spend_pubkey);
        EXPECT_EQ(proposal.amount, recovered_amount);
        EXPECT_EQ(enote_proposal.amount_blinding_factor, recovered_amount_blinding_factor);
        EXPECT_EQ(null_payment_id, recovered_payment_id);
        EXPECT_EQ(enote_type, recovered_enote_type);

        // check spendability
        EXPECT_TRUE(keys.can_open_fcmp_onetime_address(recovered_address_spend_pubkey,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote_proposal.enote.onetime_address));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, subaddress_special_scan_completeness)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    const uint32_t j_major = crypto::rand_idx(mock::MAX_SUBADDRESS_MAJOR_INDEX);
    const uint32_t j_minor = crypto::rand_idx(mock::MAX_SUBADDRESS_MINOR_INDEX);

    const CarrotDestinationV1 subaddress = keys.subaddress({{j_major, j_minor}});

    // try once with PAYMENT, once with CHANGE
    for (int i = 0; i < 2; ++i)
    {
        const CarrotEnoteType enote_type = i ? CarrotEnoteType::PAYMENT : CarrotEnoteType::CHANGE;

        const CarrotPaymentProposalSelfSendV1 proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = subaddress.address_spend_pubkey,
            .amount = crypto::rand<rct::xmr_amount>(),
            .enote_type = enote_type,
            .enote_ephemeral_pubkey = gen_x25519_pubkey(),
        };

        const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

        RCTOutputEnoteProposal enote_proposal;
        get_output_proposal_special_v1(proposal,
            keys.k_view_incoming_dev,
            tx_first_key_image,
            std::nullopt,
            enote_proposal);

        ASSERT_EQ(proposal.amount, enote_proposal.amount);
        const rct::key recomputed_amount_commitment = rct::commit(enote_proposal.amount, rct::sk2rct(enote_proposal.amount_blinding_factor));
        ASSERT_EQ(enote_proposal.enote.amount_commitment, recomputed_amount_commitment);

        mx25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(keys.legacy_acb.get_keys().m_view_secret_key,
            enote_proposal.enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        payment_id_t recovered_payment_id;
        CarrotEnoteType recovered_enote_type;
        const bool scan_success = try_scan_carrot_enote_external_receiver(enote_proposal.enote,
            std::nullopt,
            s_sender_receiver_unctx,
            {&keys.carrot_account_spend_pubkey, 1},
            keys.k_view_incoming_dev,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            recovered_address_spend_pubkey,
            recovered_amount,
            recovered_amount_blinding_factor,
            recovered_payment_id,
            recovered_enote_type);
        
        ASSERT_TRUE(scan_success);

        // check recovered data
        EXPECT_EQ(proposal.destination_address_spend_pubkey, recovered_address_spend_pubkey);
        EXPECT_EQ(proposal.amount, recovered_amount);
        EXPECT_EQ(enote_proposal.amount_blinding_factor, recovered_amount_blinding_factor);
        EXPECT_EQ(null_payment_id, recovered_payment_id);
        EXPECT_EQ(enote_type, recovered_enote_type);

        // check spendability
        EXPECT_TRUE(keys.can_open_fcmp_onetime_address(recovered_address_spend_pubkey,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote_proposal.enote.onetime_address));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, main_address_internal_scan_completeness)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    const CarrotDestinationV1 main_address = keys.cryptonote_address();

    // try once with PAYMENT, once with CHANGE
    for (int i = 0; i < 2; ++i)
    {
        const CarrotEnoteType enote_type = i ? CarrotEnoteType::PAYMENT : CarrotEnoteType::CHANGE;

        const CarrotPaymentProposalSelfSendV1 proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = main_address.address_spend_pubkey,
            .amount = crypto::rand<rct::xmr_amount>(),
            .enote_type = enote_type,
            .enote_ephemeral_pubkey = gen_x25519_pubkey(),
            .internal_message = gen_janus_anchor()
        };

        const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

        RCTOutputEnoteProposal enote_proposal;
        get_output_proposal_internal_v1(proposal,
            keys.s_view_balance_dev,
            tx_first_key_image,
            std::nullopt,
            enote_proposal);

        ASSERT_EQ(proposal.amount, enote_proposal.amount);
        const rct::key recomputed_amount_commitment = rct::commit(enote_proposal.amount, rct::sk2rct(enote_proposal.amount_blinding_factor));
        ASSERT_EQ(enote_proposal.enote.amount_commitment, recomputed_amount_commitment);

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        CarrotEnoteType recovered_enote_type;
        janus_anchor_t recovered_internal_message;
        const bool scan_success = try_scan_carrot_enote_internal_receiver(enote_proposal.enote,
            keys.s_view_balance_dev,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            recovered_address_spend_pubkey,
            recovered_amount,
            recovered_amount_blinding_factor,
            recovered_enote_type,
            recovered_internal_message);

        ASSERT_TRUE(scan_success);

        // check recovered data
        EXPECT_EQ(proposal.destination_address_spend_pubkey, recovered_address_spend_pubkey);
        EXPECT_EQ(proposal.amount, recovered_amount);
        EXPECT_EQ(enote_proposal.amount_blinding_factor, recovered_amount_blinding_factor);
        EXPECT_EQ(enote_type, recovered_enote_type);
        EXPECT_EQ(proposal.internal_message, recovered_internal_message);

        // check spendability
        EXPECT_TRUE(keys.can_open_fcmp_onetime_address(recovered_address_spend_pubkey,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote_proposal.enote.onetime_address));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, subaddress_internal_scan_completeness)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    const uint32_t j_major = crypto::rand_idx(mock::MAX_SUBADDRESS_MAJOR_INDEX);
    const uint32_t j_minor = crypto::rand_idx(mock::MAX_SUBADDRESS_MINOR_INDEX);

    const CarrotDestinationV1 subaddress = keys.subaddress({{j_major, j_minor}});

    // try once with PAYMENT, once with CHANGE
    for (int i = 0; i < 2; ++i)
    {
        const CarrotEnoteType enote_type = i ? CarrotEnoteType::PAYMENT : CarrotEnoteType::CHANGE;

        const CarrotPaymentProposalSelfSendV1 proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = subaddress.address_spend_pubkey,
            .amount = crypto::rand<rct::xmr_amount>(),
            .enote_type = enote_type,
            .enote_ephemeral_pubkey = gen_x25519_pubkey(),
            .internal_message = gen_janus_anchor()
        };

        const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

        RCTOutputEnoteProposal enote_proposal;
        get_output_proposal_internal_v1(proposal,
            keys.s_view_balance_dev,
            tx_first_key_image,
            std::nullopt,
            enote_proposal);

        ASSERT_EQ(proposal.amount, enote_proposal.amount);
        const rct::key recomputed_amount_commitment = rct::commit(enote_proposal.amount, rct::sk2rct(enote_proposal.amount_blinding_factor));
        ASSERT_EQ(enote_proposal.enote.amount_commitment, recomputed_amount_commitment);

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        CarrotEnoteType recovered_enote_type;
        janus_anchor_t recovered_internal_message;
        const bool scan_success = try_scan_carrot_enote_internal_receiver(enote_proposal.enote,
            keys.s_view_balance_dev,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            recovered_address_spend_pubkey,
            recovered_amount,
            recovered_amount_blinding_factor,
            recovered_enote_type,
            recovered_internal_message);

        ASSERT_TRUE(scan_success);

        // check recovered data
        EXPECT_EQ(proposal.destination_address_spend_pubkey, recovered_address_spend_pubkey);
        EXPECT_EQ(proposal.amount, recovered_amount);
        EXPECT_EQ(enote_proposal.amount_blinding_factor, recovered_amount_blinding_factor);
        EXPECT_EQ(enote_type, recovered_enote_type);
        EXPECT_EQ(proposal.internal_message, recovered_internal_message);

        // check spendability
        EXPECT_TRUE(keys.can_open_fcmp_onetime_address(recovered_address_spend_pubkey,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote_proposal.enote.onetime_address));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, main_address_coinbase_scan_completeness)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();

    const CarrotDestinationV1 main_address = keys.cryptonote_address();

    const CarrotPaymentProposalV1 proposal = CarrotPaymentProposalV1{
        .destination = main_address,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    const uint64_t block_index = crypto::rand<uint64_t>();

    CarrotCoinbaseEnoteV1 enote;
    get_coinbase_output_proposal_v1(proposal,
        block_index,
        enote);

    ASSERT_EQ(proposal.amount, enote.amount);

    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(keys.k_view_incoming_dev,
        enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    const bool scan_success = try_scan_carrot_coinbase_enote_receiver(enote,
        s_sender_receiver_unctx,
        keys.carrot_account_spend_pubkey,
        recovered_sender_extension_g,
        recovered_sender_extension_t);

    ASSERT_TRUE(scan_success);

    // check spendability
    EXPECT_TRUE(keys.can_open_fcmp_onetime_address(
        keys.carrot_account_spend_pubkey,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, keys_opening_for_main_address)
{
    mock::mock_carrot_and_legacy_keys keys;
    keys.generate();
    crypto::secret_key x;
    crypto::secret_key y;
    crypto::public_key address_spend_pubkey;
    keys.opening_for_subaddress({{0, 0}}, x, y, address_spend_pubkey);
    EXPECT_EQ(keys.carrot_account_spend_pubkey, address_spend_pubkey);
}
//----------------------------------------------------------------------------------------------------------------------
static void subtest_2out_transfer_get_output_enote_proposals_completeness(const bool alice_subaddress,
    const bool bob_subaddress,
    const bool bob_integrated,
    const CarrotEnoteType alice_selfsend_type,
    const bool alice_internal_selfsends)
{
    // generate alice keys and address
    mock::mock_carrot_and_legacy_keys alice;
    alice.generate();
    const uint32_t alice_j_major = crypto::rand_idx(mock::MAX_SUBADDRESS_MAJOR_INDEX);
    const uint32_t alice_j_minor = crypto::rand_idx(mock::MAX_SUBADDRESS_MINOR_INDEX);
    CarrotDestinationV1 alice_address;
    if (alice_subaddress)
    {
        alice_address = alice.subaddress({{alice_j_major, alice_j_minor}});
    }
    else // alice main address
    {
        alice_address = alice.cryptonote_address();
    }

    // generate bob keys and address
    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();
    const uint32_t bob_j_major = crypto::rand_idx(mock::MAX_SUBADDRESS_MAJOR_INDEX);
    const uint32_t bob_j_minor = crypto::rand_idx(mock::MAX_SUBADDRESS_MINOR_INDEX);
    CarrotDestinationV1 bob_address;
    if (bob_subaddress)
    {
        bob_address = bob.subaddress({{bob_j_major, bob_j_minor}});
    }
    else if (bob_integrated)
    {
        bob_address = bob.cryptonote_address(gen_payment_id());
    }
    else // bob main address
    {
        bob_address = bob.cryptonote_address();
    }

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
        .enote_ephemeral_pubkey = get_enote_ephemeral_pubkey(bob_payment_proposal, input_context),
        .internal_message = alice_internal_selfsends ? std::make_optional(gen_janus_anchor()) : std::nullopt
    };

    // calculate dummy encrypted pid
    const std::optional<encrypted_payment_id_t> dummy_encrypted_pid = bob_integrated
        ? std::optional<encrypted_payment_id_t>{}
        : gen_encrypted_payment_id();

    // turn payment proposals into enotes, passing dummy pid_enc if bob isn't integrated
    std::vector<RCTOutputEnoteProposal> enote_proposals;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_enote_proposals({bob_payment_proposal},
        {alice_payment_proposal},
        dummy_encrypted_pid,
        alice_internal_selfsends ? &alice.s_view_balance_dev : nullptr,
        &alice.k_view_incoming_dev,
        tx_first_key_image,
        enote_proposals,
        encrypted_payment_id);

    ASSERT_EQ(2, enote_proposals.size()); // 2-out tx

    // collect enotes
    std::vector<CarrotEnoteV1> enotes;
    for (const RCTOutputEnoteProposal &enote_proposal : enote_proposals)
        enotes.push_back(enote_proposal.enote);

    // check that alice scanned 1 enote
    std::vector<mock::mock_scan_result_t> alice_scan_vec;
    mock::mock_scan_enote_set(enotes, encrypted_payment_id, alice, alice_scan_vec);
    ASSERT_EQ(1, alice_scan_vec.size());
    mock::mock_scan_result_t alice_scan = alice_scan_vec.front();

    // check that bob scanned 1 enote
    std::vector<mock::mock_scan_result_t> bob_scan_vec;
    mock::mock_scan_enote_set(enotes, encrypted_payment_id, bob, bob_scan_vec);
    ASSERT_EQ(1, bob_scan_vec.size());
    mock::mock_scan_result_t bob_scan = bob_scan_vec.front();

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
    if (alice_internal_selfsends)
    {
        EXPECT_EQ(alice_payment_proposal.internal_message, alice_scan.internal_message);
    }

    // check Bob's recovered data
    EXPECT_EQ(bob_payment_proposal.destination.address_spend_pubkey, bob_scan.address_spend_pubkey);
    EXPECT_EQ(bob_payment_proposal.amount, bob_scan.amount);
    EXPECT_EQ(bob_enote.amount_commitment, rct::commit(bob_scan.amount, rct::sk2rct(bob_scan.amount_blinding_factor)));
    EXPECT_EQ(bob_integrated ? bob_address.payment_id : null_payment_id, bob_scan.payment_id);
    EXPECT_EQ(CarrotEnoteType::PAYMENT, bob_scan.enote_type);

    // check Alice spendability
    EXPECT_TRUE(alice.can_open_fcmp_onetime_address(alice_payment_proposal.destination_address_spend_pubkey,
        alice_scan.sender_extension_g,
        alice_scan.sender_extension_t,
        alice_enote.onetime_address));
    
    // check Bob spendability
    EXPECT_TRUE(bob.can_open_fcmp_onetime_address(bob_payment_proposal.destination.address_spend_pubkey,
        bob_scan.sender_extension_g,
        bob_scan.sender_extension_t,
        bob_enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_internal_ss_main2main_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, false, false, CarrotEnoteType::PAYMENT, true);
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, false, false, CarrotEnoteType::CHANGE, true);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_internal_ss_main2sub_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, true, false, CarrotEnoteType::PAYMENT, true);
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, true, false, CarrotEnoteType::CHANGE, true);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_internal_ss_main2integ_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, false, true, CarrotEnoteType::PAYMENT, true);
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, false, true, CarrotEnoteType::CHANGE, true);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_internal_ss_sub2main_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, false, false, CarrotEnoteType::PAYMENT, true);
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, false, false, CarrotEnoteType::CHANGE, true);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_internal_ss_sub2sub_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, true, false, CarrotEnoteType::PAYMENT, true);
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, true, false, CarrotEnoteType::CHANGE, true);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_internal_ss_sub2integ_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, false, true, CarrotEnoteType::PAYMENT, true);
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, false, true, CarrotEnoteType::CHANGE, true);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_external_ss_main2main_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, false, false, CarrotEnoteType::PAYMENT, false);
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, false, false, CarrotEnoteType::CHANGE, false);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_external_ss_main2sub_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, true, false, CarrotEnoteType::PAYMENT, false);
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, true, false, CarrotEnoteType::CHANGE, false);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_external_ss_main2integ_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, false, true, CarrotEnoteType::PAYMENT, false);
    subtest_2out_transfer_get_output_enote_proposals_completeness(false, false, true, CarrotEnoteType::CHANGE, false);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_external_ss_sub2main_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, false, false, CarrotEnoteType::PAYMENT, false);
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, false, false, CarrotEnoteType::CHANGE, false);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_external_ss_sub2sub_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, true, false, CarrotEnoteType::PAYMENT, false);
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, true, false, CarrotEnoteType::CHANGE, false);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, get_enote_output_proposals_external_ss_sub2integ_completeness)
{
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, false, true, CarrotEnoteType::PAYMENT, false);
    subtest_2out_transfer_get_output_enote_proposals_completeness(true, false, true, CarrotEnoteType::CHANGE, false);
}
//----------------------------------------------------------------------------------------------------------------------
struct JanusAttackProposalV1
{
    /// The address provided in the normal proposal is used to scale the enote ephemeral pubkey and derive s_sr.
    /// The `readjusted_opening_subaddress_spend_pubkey` is what is added to the sender extensions to derive K_o.
    /// If `use_readjusted_spend_pubkey_in_ephemeral_privkey_hash=true`, then d_e is calculated as
    /// d_e = H_n(anchor_norm, input_context, K^i_s, pid)), else it is normally calculated as
    /// d_e = H_n(anchor_norm, input_context, K^j_s, pid)). It shouldn't make a difference to the attack either
    /// way, but we will test that below.

    carrot::CarrotPaymentProposalV1 normal; // contains address (is_subaddress, K^j_s, K^j_v, pid)
    crypto::public_key readjusted_opening_subaddress_spend_pubkey; // K^i_s
    bool use_readjusted_spend_pubkey_in_ephemeral_privkey_hash;
};
//----------------------------------------------------------------------------------------------------------------------
static void get_output_proposal_janus_attack_v1(const JanusAttackProposalV1 &proposal,
    const crypto::key_image &tx_first_key_image,
    carrot::RCTOutputEnoteProposal &output_enote_out,
    carrot::encrypted_payment_id_t &encrypted_payment_id_out)
{
    // 1. sanity checks
    CHECK_AND_ASSERT_THROW_MES(proposal.normal.randomness != carrot::janus_anchor_t{},
        "jamtis payment proposal: invalid randomness for janus anchor (zero).");

    const carrot::CarrotDestinationV1 &destination = proposal.normal.destination;

    // 2. input context: input_context = "R" || KI_1
    const input_context_t input_context = make_carrot_input_context(tx_first_key_image);

    // 3. decide if K^x_s = K^j_s XOR K^x_s = K^i_s
    const crypto::public_key &spend_pubkey_used_in_ephemeral_privkey_hash =
        proposal.use_readjusted_spend_pubkey_in_ephemeral_privkey_hash
            ? proposal.readjusted_opening_subaddress_spend_pubkey
            : destination.address_spend_pubkey;

    // 4. d_e = H_n(anchor_norm, input_context, K^x_s, pid)
    crypto::secret_key enote_ephemeral_privkey;
    carrot::make_carrot_enote_ephemeral_privkey(proposal.normal.randomness,
        input_context,
        spend_pubkey_used_in_ephemeral_privkey_hash,
        destination.payment_id,
        enote_ephemeral_privkey);

    // 5. make D_e
    make_carrot_enote_ephemeral_pubkey(enote_ephemeral_privkey,
        destination.address_spend_pubkey,
        destination.is_subaddress,
        output_enote_out.enote.enote_ephemeral_pubkey);

    // 6. s_sr = d_e ConvertPointE(K^j_v)
    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_sender(enote_ephemeral_privkey,
        destination.address_view_pubkey,
        s_sender_receiver_unctx);

    // 7. build the output enote address pieces
    crypto::hash s_sender_receiver;
    make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
        output_enote_out.enote.enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver);

    // 8. k_a = H_n(s^ctx_sr, a, K^i_s, enote_type)
    make_carrot_amount_blinding_factor(s_sender_receiver,
        proposal.normal.amount,
        proposal.readjusted_opening_subaddress_spend_pubkey,
        carrot::CarrotEnoteType::PAYMENT,
        output_enote_out.amount_blinding_factor);

    // 9. C_a = k_a G + a H
    output_enote_out.enote.amount_commitment = rct::commit(proposal.normal.amount,
        rct::sk2rct(output_enote_out.amount_blinding_factor));

    // 10. Ko = K^i_s + K^o_ext = K^i_s + (k^o_g G + k^o_t T)
    make_carrot_onetime_address(proposal.readjusted_opening_subaddress_spend_pubkey,
        s_sender_receiver,
        output_enote_out.enote.amount_commitment,
        output_enote_out.enote.onetime_address);
    
    // 11. a_enc = a XOR m_a
    output_enote_out.enote.amount_enc = carrot::encrypt_carrot_amount(proposal.normal.amount,
        s_sender_receiver,
        output_enote_out.enote.onetime_address);

    // 12. pid_enc = pid XOR m_pid
    encrypted_payment_id_out = encrypt_legacy_payment_id(destination.payment_id,
        s_sender_receiver,
        output_enote_out.enote.onetime_address);

    // 13. vt = H_3(s_sr || input_context || Ko)
    make_carrot_view_tag(s_sender_receiver_unctx.data,
        input_context,
        output_enote_out.enote.onetime_address,
        output_enote_out.enote.view_tag);

    // 14. anchor_enc = anchor XOR m_anchor
    output_enote_out.enote.anchor_enc = encrypt_carrot_anchor(proposal.normal.randomness,
        s_sender_receiver,
        output_enote_out.enote.onetime_address);

    // 14. save the amount and first key image
    output_enote_out.amount                   = proposal.normal.amount;
    output_enote_out.enote.tx_first_key_image = tx_first_key_image;

    // Notice steps 8 & 10 specifically for where `readjusted_opening_subaddress_spend_pubkey` is
    // substituted instead of the actual address spend pubkey used for deriving D_e and s_sr.
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, janus_protection_non_coinbase_main_sub_readjust_NOT_in_d_e)
{
    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 bob_main = bob.cryptonote_address();
    const CarrotDestinationV1 bob_subaddr = bob.subaddress({{2, 4}});

    const rct::xmr_amount amount = rct::randXmrAmount(COIN);

    const JanusAttackProposalV1 proposal{
        .normal = carrot::CarrotPaymentProposalV1{
            .destination = bob_main,
            .amount = amount,
            .randomness = carrot::gen_janus_anchor()
        },
        .readjusted_opening_subaddress_spend_pubkey = bob_subaddr.address_spend_pubkey,
        .use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = false,
    };

    carrot::RCTOutputEnoteProposal output_enote;
    carrot::encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_janus_attack_v1(proposal, tx_first_key_image, output_enote, encrypted_payment_id);

    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    carrot::make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    crypto::public_key nominal_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    carrot::payment_id_t nominal_payment_id;
    carrot::CarrotEnoteType recovered_enote_type;
    carrot::janus_anchor_t nominal_janus_anchor;
    const bool scanned = carrot::try_scan_carrot_enote_external_no_janus(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type,
        nominal_janus_anchor);
    ASSERT_TRUE(scanned);
    ASSERT_EQ(proposal.readjusted_opening_subaddress_spend_pubkey, nominal_address_spend_pubkey);
    ASSERT_EQ(amount, recovered_amount);
    ASSERT_EQ(carrot::CarrotEnoteType::PAYMENT, recovered_enote_type);
    ASSERT_EQ(output_enote.enote.amount_commitment, rct::commit(recovered_amount, rct::sk2rct(recovered_amount_blinding_factor)));
    ASSERT_TRUE(bob.can_open_fcmp_onetime_address(nominal_address_spend_pubkey,
        sender_extension_g,
        sender_extension_t,
        output_enote.enote.onetime_address));

    EXPECT_FALSE(verify_carrot_normal_janus_protection(
        carrot::make_carrot_input_context(tx_first_key_image),
        nominal_address_spend_pubkey,
        bob.cryptonote_address().address_spend_pubkey != nominal_address_spend_pubkey,
        output_enote.enote.enote_ephemeral_pubkey,
        nominal_janus_anchor,
        nominal_payment_id));

    EXPECT_FALSE(try_scan_carrot_enote_external_receiver(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        bob.k_view_incoming_dev,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type));

    EXPECT_FALSE(try_scan_carrot_enote_external_sender(output_enote.enote,
        encrypted_payment_id,
        bob_main,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_enote_type));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, janus_protection_non_coinbase_main_sub_readjust_in_d_e)
{
    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 bob_main = bob.cryptonote_address();
    const CarrotDestinationV1 bob_subaddr = bob.subaddress({{2, 4}});

    const rct::xmr_amount amount = rct::randXmrAmount(COIN);

    const JanusAttackProposalV1 proposal{
        .normal = carrot::CarrotPaymentProposalV1{
            .destination = bob_main,
            .amount = amount,
            .randomness = carrot::gen_janus_anchor()
        },
        .readjusted_opening_subaddress_spend_pubkey = bob_subaddr.address_spend_pubkey,
        .use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = true,
    };

    carrot::RCTOutputEnoteProposal output_enote;
    carrot::encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_janus_attack_v1(proposal, tx_first_key_image, output_enote, encrypted_payment_id);

    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    carrot::make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    crypto::public_key nominal_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    carrot::payment_id_t nominal_payment_id;
    carrot::CarrotEnoteType recovered_enote_type;
    carrot::janus_anchor_t nominal_janus_anchor;
    const bool scanned = carrot::try_scan_carrot_enote_external_no_janus(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type,
        nominal_janus_anchor);
    ASSERT_TRUE(scanned);
    ASSERT_EQ(proposal.readjusted_opening_subaddress_spend_pubkey, nominal_address_spend_pubkey);
    ASSERT_EQ(amount, recovered_amount);
    ASSERT_EQ(carrot::CarrotEnoteType::PAYMENT, recovered_enote_type);
    ASSERT_EQ(output_enote.enote.amount_commitment, rct::commit(recovered_amount, rct::sk2rct(recovered_amount_blinding_factor)));
    ASSERT_TRUE(bob.can_open_fcmp_onetime_address(nominal_address_spend_pubkey,
        sender_extension_g,
        sender_extension_t,
        output_enote.enote.onetime_address));

    EXPECT_FALSE(verify_carrot_normal_janus_protection(
        carrot::make_carrot_input_context(tx_first_key_image),
        nominal_address_spend_pubkey,
        bob.cryptonote_address().address_spend_pubkey != nominal_address_spend_pubkey,
        output_enote.enote.enote_ephemeral_pubkey,
        nominal_janus_anchor,
        nominal_payment_id));

    EXPECT_FALSE(try_scan_carrot_enote_external_receiver(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        bob.k_view_incoming_dev,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type));

    EXPECT_FALSE(try_scan_carrot_enote_external_sender(output_enote.enote,
        encrypted_payment_id,
        bob_main,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_enote_type));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, janus_protection_non_coinbase_sub_main_readjust_NOT_in_d_e)
{
    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 bob_main = bob.cryptonote_address();
    const CarrotDestinationV1 bob_subaddr = bob.subaddress({{2, 4}});

    const rct::xmr_amount amount = rct::randXmrAmount(COIN);

    const JanusAttackProposalV1 proposal{
        .normal = carrot::CarrotPaymentProposalV1{
            .destination = bob_subaddr,
            .amount = amount,
            .randomness = carrot::gen_janus_anchor()
        },
        .readjusted_opening_subaddress_spend_pubkey = bob_main.address_spend_pubkey,
        .use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = false,
    };

    carrot::RCTOutputEnoteProposal output_enote;
    carrot::encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_janus_attack_v1(proposal, tx_first_key_image, output_enote, encrypted_payment_id);

    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    carrot::make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    crypto::public_key nominal_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    carrot::payment_id_t nominal_payment_id;
    carrot::CarrotEnoteType recovered_enote_type;
    carrot::janus_anchor_t nominal_janus_anchor;
    const bool scanned = carrot::try_scan_carrot_enote_external_no_janus(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type,
        nominal_janus_anchor);
    ASSERT_TRUE(scanned);
    ASSERT_EQ(proposal.readjusted_opening_subaddress_spend_pubkey, nominal_address_spend_pubkey);
    ASSERT_EQ(amount, recovered_amount);
    ASSERT_EQ(carrot::CarrotEnoteType::PAYMENT, recovered_enote_type);
    ASSERT_EQ(output_enote.enote.amount_commitment, rct::commit(recovered_amount, rct::sk2rct(recovered_amount_blinding_factor)));
    ASSERT_TRUE(bob.can_open_fcmp_onetime_address(nominal_address_spend_pubkey,
        sender_extension_g,
        sender_extension_t,
        output_enote.enote.onetime_address));

    EXPECT_FALSE(verify_carrot_normal_janus_protection(
        carrot::make_carrot_input_context(tx_first_key_image),
        nominal_address_spend_pubkey,
        bob.cryptonote_address().address_spend_pubkey != nominal_address_spend_pubkey,
        output_enote.enote.enote_ephemeral_pubkey,
        nominal_janus_anchor,
        nominal_payment_id));

    EXPECT_FALSE(try_scan_carrot_enote_external_receiver(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        bob.k_view_incoming_dev,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type));

    EXPECT_FALSE(try_scan_carrot_enote_external_sender(output_enote.enote,
        encrypted_payment_id,
        bob_subaddr,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_enote_type));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, janus_protection_non_coinbase_sub_main_readjust_in_d_e)
{
    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 bob_main = bob.cryptonote_address();
    const CarrotDestinationV1 bob_subaddr = bob.subaddress({{2, 4}});

    const rct::xmr_amount amount = rct::randXmrAmount(COIN);

    const JanusAttackProposalV1 proposal{
        .normal = carrot::CarrotPaymentProposalV1{
            .destination = bob_subaddr,
            .amount = amount,
            .randomness = carrot::gen_janus_anchor()
        },
        .readjusted_opening_subaddress_spend_pubkey = bob_main.address_spend_pubkey,
        .use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = true,
    };

    carrot::RCTOutputEnoteProposal output_enote;
    carrot::encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_janus_attack_v1(proposal, tx_first_key_image, output_enote, encrypted_payment_id);

    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    carrot::make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    crypto::public_key nominal_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    carrot::payment_id_t nominal_payment_id;
    carrot::CarrotEnoteType recovered_enote_type;
    carrot::janus_anchor_t nominal_janus_anchor;
    const bool scanned = carrot::try_scan_carrot_enote_external_no_janus(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type,
        nominal_janus_anchor);
    ASSERT_TRUE(scanned);
    ASSERT_EQ(proposal.readjusted_opening_subaddress_spend_pubkey, nominal_address_spend_pubkey);
    ASSERT_EQ(amount, recovered_amount);
    ASSERT_EQ(carrot::CarrotEnoteType::PAYMENT, recovered_enote_type);
    ASSERT_EQ(output_enote.enote.amount_commitment, rct::commit(recovered_amount, rct::sk2rct(recovered_amount_blinding_factor)));
    ASSERT_TRUE(bob.can_open_fcmp_onetime_address(nominal_address_spend_pubkey,
        sender_extension_g,
        sender_extension_t,
        output_enote.enote.onetime_address));

    EXPECT_FALSE(verify_carrot_normal_janus_protection(
        carrot::make_carrot_input_context(tx_first_key_image),
        nominal_address_spend_pubkey,
        bob.cryptonote_address().address_spend_pubkey != nominal_address_spend_pubkey,
        output_enote.enote.enote_ephemeral_pubkey,
        nominal_janus_anchor,
        nominal_payment_id));

    EXPECT_FALSE(try_scan_carrot_enote_external_receiver(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        bob.k_view_incoming_dev,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type));

    EXPECT_FALSE(try_scan_carrot_enote_external_sender(output_enote.enote,
        encrypted_payment_id,
        bob_subaddr,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_enote_type));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, janus_protection_non_coinbase_sub_sub_readjust_NOT_in_d_e)
{
    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 bob_subaddr1 = bob.subaddress({{4, 2}});
    const CarrotDestinationV1 bob_subaddr2 = bob.subaddress({{2, 4}});

    const rct::xmr_amount amount = rct::randXmrAmount(COIN);

    const JanusAttackProposalV1 proposal{
        .normal = carrot::CarrotPaymentProposalV1{
            .destination = bob_subaddr1,
            .amount = amount,
            .randomness = carrot::gen_janus_anchor()
        },
        .readjusted_opening_subaddress_spend_pubkey = bob_subaddr2.address_spend_pubkey,
        .use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = false,
    };

    carrot::RCTOutputEnoteProposal output_enote;
    carrot::encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_janus_attack_v1(proposal, tx_first_key_image, output_enote, encrypted_payment_id);

    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    carrot::make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    crypto::public_key nominal_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    carrot::payment_id_t nominal_payment_id;
    carrot::CarrotEnoteType recovered_enote_type;
    carrot::janus_anchor_t nominal_janus_anchor;
    const bool scanned = carrot::try_scan_carrot_enote_external_no_janus(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type,
        nominal_janus_anchor);
    ASSERT_TRUE(scanned);
    ASSERT_EQ(proposal.readjusted_opening_subaddress_spend_pubkey, nominal_address_spend_pubkey);
    ASSERT_EQ(amount, recovered_amount);
    ASSERT_EQ(carrot::CarrotEnoteType::PAYMENT, recovered_enote_type);
    ASSERT_EQ(output_enote.enote.amount_commitment, rct::commit(recovered_amount, rct::sk2rct(recovered_amount_blinding_factor)));
    ASSERT_TRUE(bob.can_open_fcmp_onetime_address(nominal_address_spend_pubkey,
        sender_extension_g,
        sender_extension_t,
        output_enote.enote.onetime_address));

    EXPECT_FALSE(verify_carrot_normal_janus_protection(
        carrot::make_carrot_input_context(tx_first_key_image),
        nominal_address_spend_pubkey,
        bob.cryptonote_address().address_spend_pubkey != nominal_address_spend_pubkey,
        output_enote.enote.enote_ephemeral_pubkey,
        nominal_janus_anchor,
        nominal_payment_id));

    EXPECT_FALSE(try_scan_carrot_enote_external_receiver(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        bob.k_view_incoming_dev,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type));

    EXPECT_FALSE(try_scan_carrot_enote_external_sender(output_enote.enote,
        encrypted_payment_id,
        bob_subaddr1,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_enote_type));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, janus_protection_non_coinbase_sub_sub_readjust_in_d_e)
{
    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 bob_subaddr1 = bob.subaddress({{4, 2}});
    const CarrotDestinationV1 bob_subaddr2 = bob.subaddress({{2, 4}});

    const rct::xmr_amount amount = rct::randXmrAmount(COIN);

    const JanusAttackProposalV1 proposal{
        .normal = carrot::CarrotPaymentProposalV1{
            .destination = bob_subaddr1,
            .amount = amount,
            .randomness = carrot::gen_janus_anchor()
        },
        .readjusted_opening_subaddress_spend_pubkey = bob_subaddr2.address_spend_pubkey,
        .use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = true,
    };

    carrot::RCTOutputEnoteProposal output_enote;
    carrot::encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_janus_attack_v1(proposal, tx_first_key_image, output_enote, encrypted_payment_id);

    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    carrot::make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    crypto::public_key nominal_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    carrot::payment_id_t nominal_payment_id;
    carrot::CarrotEnoteType recovered_enote_type;
    carrot::janus_anchor_t nominal_janus_anchor;
    const bool scanned = carrot::try_scan_carrot_enote_external_no_janus(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type,
        nominal_janus_anchor);
    ASSERT_TRUE(scanned);
    ASSERT_EQ(proposal.readjusted_opening_subaddress_spend_pubkey, nominal_address_spend_pubkey);
    ASSERT_EQ(amount, recovered_amount);
    ASSERT_EQ(carrot::CarrotEnoteType::PAYMENT, recovered_enote_type);
    ASSERT_EQ(output_enote.enote.amount_commitment, rct::commit(recovered_amount, rct::sk2rct(recovered_amount_blinding_factor)));
    ASSERT_TRUE(bob.can_open_fcmp_onetime_address(nominal_address_spend_pubkey,
        sender_extension_g,
        sender_extension_t,
        output_enote.enote.onetime_address));

    EXPECT_FALSE(verify_carrot_normal_janus_protection(
        carrot::make_carrot_input_context(tx_first_key_image),
        nominal_address_spend_pubkey,
        bob.cryptonote_address().address_spend_pubkey != nominal_address_spend_pubkey,
        output_enote.enote.enote_ephemeral_pubkey,
        nominal_janus_anchor,
        nominal_payment_id));

    EXPECT_FALSE(try_scan_carrot_enote_external_receiver(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        bob.k_view_incoming_dev,
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        nominal_payment_id,
        recovered_enote_type));

    EXPECT_FALSE(try_scan_carrot_enote_external_sender(output_enote.enote,
        encrypted_payment_id,
        bob_subaddr1,
        s_sender_receiver_unctx,
        sender_extension_g,
        sender_extension_t,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_enote_type));
}
//----------------------------------------------------------------------------------------------------------------------
static void get_coinbase_output_proposal_janus_attack_v1(const JanusAttackProposalV1 &proposal,
    const std::uint64_t block_index,
    carrot::CarrotCoinbaseEnoteV1 &output_enote_out)
{
    // 1. sanity checks
    CHECK_AND_ASSERT_THROW_MES(proposal.normal.randomness != carrot::janus_anchor_t{},
        "jamtis payment proposal: invalid randomness for janus anchor (zero).");
    CHECK_AND_ASSERT_THROW_MES(proposal.normal.destination.is_subaddress,
        "jamtis payment proposal: we need OG address to be a subaddress so we can make do ECDH against that");

    const carrot::CarrotDestinationV1 &destination = proposal.normal.destination;

    // 2. input context: input_context = "R" || KI_1
    const input_context_t input_context = make_carrot_input_context_coinbase(block_index);

    // 3. decide if K^x_s = K^j_s XOR K^x_s = K^i_s
    const crypto::public_key &spend_pubkey_used_in_ephemeral_privkey_hash =
        proposal.use_readjusted_spend_pubkey_in_ephemeral_privkey_hash
            ? proposal.readjusted_opening_subaddress_spend_pubkey
            : destination.address_spend_pubkey;

    // 4. d_e = H_n(anchor_norm, input_context, K^x_s, pid)
    crypto::secret_key enote_ephemeral_privkey;
    carrot::make_carrot_enote_ephemeral_privkey(proposal.normal.randomness,
        input_context,
        spend_pubkey_used_in_ephemeral_privkey_hash,
        destination.payment_id,
        enote_ephemeral_privkey);

    // 5. make D_e
    make_carrot_enote_ephemeral_pubkey(enote_ephemeral_privkey,
        destination.address_spend_pubkey,
        destination.is_subaddress,
        output_enote_out.enote_ephemeral_pubkey);

    // 6. s_sr = d_e ConvertPointE(K^j_v)
    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_sender(enote_ephemeral_privkey,
        destination.address_view_pubkey,
        s_sender_receiver_unctx);

    // 7. build the output enote address pieces
    crypto::hash s_sender_receiver;
    make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
        output_enote_out.enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver);

    // 8. Ko = K^i_s + K^o_ext = K^i_s + (k^o_g G + k^o_t T)
    make_carrot_onetime_address_coinbase(
        proposal.readjusted_opening_subaddress_spend_pubkey,
        s_sender_receiver,
        proposal.normal.amount,
        output_enote_out.onetime_address);

    // 9. vt = H_3(s_sr || input_context || Ko)
    make_carrot_view_tag(s_sender_receiver_unctx.data,
        input_context,
        output_enote_out.onetime_address,
        output_enote_out.view_tag);

    // 10. anchor_enc = anchor XOR m_anchor
    output_enote_out.anchor_enc = encrypt_carrot_anchor(proposal.normal.randomness,
        s_sender_receiver,
        output_enote_out.onetime_address);

    // 11. save the amount and first key image
    output_enote_out.amount      = proposal.normal.amount;
    output_enote_out.block_index = block_index;

    // Notice step 8 specifically for where `readjusted_opening_subaddress_spend_pubkey` is
    // substituted instead of the actual address spend pubkey used for deriving D_e and s_sr.
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, janus_protection_coinbase_main_sub_readjust_NOT_in_d_e)
{
    const std::uint64_t block_index = mock::gen_block_index();

    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 bob_main = bob.cryptonote_address();
    const CarrotDestinationV1 bob_subaddr = bob.subaddress({{2, 4}});

    const rct::xmr_amount amount = rct::randXmrAmount(COIN);

    const JanusAttackProposalV1 proposal{
        .normal = carrot::CarrotPaymentProposalV1{
            .destination = bob_subaddr,
            .amount = amount,
            .randomness = carrot::gen_janus_anchor()
        },
        .readjusted_opening_subaddress_spend_pubkey = bob_main.address_spend_pubkey,
        .use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = false,
    };

    carrot::CarrotCoinbaseEnoteV1 output_enote;
    get_coinbase_output_proposal_janus_attack_v1(proposal, block_index, output_enote);

    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    carrot::make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    crypto::public_key nominal_address_spend_pubkey;
    carrot::janus_anchor_t nominal_janus_anchor;
    const bool scanned = carrot::try_scan_carrot_coinbase_enote_no_janus(output_enote,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        nominal_janus_anchor);
    ASSERT_TRUE(scanned);
    ASSERT_EQ(proposal.readjusted_opening_subaddress_spend_pubkey, nominal_address_spend_pubkey);
    ASSERT_TRUE(bob.can_open_fcmp_onetime_address(nominal_address_spend_pubkey,
        sender_extension_g,
        sender_extension_t,
        output_enote.onetime_address));

    EXPECT_FALSE(carrot::verify_carrot_normal_janus_protection(nominal_janus_anchor,
        make_carrot_input_context_coinbase(block_index),
        nominal_address_spend_pubkey,
        bob.cryptonote_address().address_spend_pubkey != nominal_address_spend_pubkey,
        null_payment_id,
        output_enote.enote_ephemeral_pubkey));

    EXPECT_FALSE(try_scan_carrot_coinbase_enote_receiver(output_enote,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey));

    EXPECT_FALSE(try_scan_carrot_coinbase_enote_sender(output_enote,
        bob_main,
        proposal.normal.randomness,
        sender_extension_g,
        sender_extension_t));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, janus_protection_coinbase_main_sub_readjust_in_d_e)
{
    const std::uint64_t block_index = mock::gen_block_index();

    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 bob_main = bob.cryptonote_address();
    const CarrotDestinationV1 bob_subaddr = bob.subaddress({{2, 4}});

    const rct::xmr_amount amount = rct::randXmrAmount(COIN);

    const JanusAttackProposalV1 proposal{
        .normal = carrot::CarrotPaymentProposalV1{
            .destination = bob_subaddr,
            .amount = amount,
            .randomness = carrot::gen_janus_anchor()
        },
        .readjusted_opening_subaddress_spend_pubkey = bob_main.address_spend_pubkey,
        .use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = true,
    };

    carrot::CarrotCoinbaseEnoteV1 output_enote;
    get_coinbase_output_proposal_janus_attack_v1(proposal, block_index, output_enote);

    // s_sr = k_v D_e
    mx25519_pubkey s_sender_receiver_unctx;
    carrot::make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    crypto::public_key nominal_address_spend_pubkey;
    carrot::janus_anchor_t nominal_janus_anchor;
    const bool scanned = carrot::try_scan_carrot_coinbase_enote_no_janus(output_enote,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey,
        nominal_janus_anchor);
    ASSERT_TRUE(scanned);
    ASSERT_EQ(proposal.readjusted_opening_subaddress_spend_pubkey, nominal_address_spend_pubkey);
    ASSERT_TRUE(bob.can_open_fcmp_onetime_address(nominal_address_spend_pubkey,
        sender_extension_g,
        sender_extension_t,
        output_enote.onetime_address));

    EXPECT_FALSE(carrot::verify_carrot_normal_janus_protection(nominal_janus_anchor,
        make_carrot_input_context_coinbase(block_index),
        nominal_address_spend_pubkey,
        bob.cryptonote_address().address_spend_pubkey != nominal_address_spend_pubkey,
        null_payment_id,
        output_enote.enote_ephemeral_pubkey));

    EXPECT_FALSE(try_scan_carrot_coinbase_enote_receiver(output_enote,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        sender_extension_g,
        sender_extension_t,
        nominal_address_spend_pubkey));

    EXPECT_FALSE(try_scan_carrot_coinbase_enote_sender(output_enote,
        bob_main,
        proposal.normal.randomness,
        sender_extension_g,
        sender_extension_t));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, janus_protection_use_readjusted_spend_pubkey_in_ephemeral_privkey_hash)
{
    const std::uint64_t block_index = mock::gen_block_index();

    mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 bob_main = bob.cryptonote_address();
    const CarrotDestinationV1 bob_subaddr = bob.subaddress({{2, 4}});

    const rct::xmr_amount amount = rct::randXmrAmount(COIN);

    const JanusAttackProposalV1 proposal1{
        .normal = carrot::CarrotPaymentProposalV1{
            .destination = bob_subaddr,
            .amount = amount,
            .randomness = carrot::gen_janus_anchor()
        },
        .readjusted_opening_subaddress_spend_pubkey = bob_main.address_spend_pubkey,
        .use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = false,
    };

    JanusAttackProposalV1 proposal2 = proposal1;
    proposal2.use_readjusted_spend_pubkey_in_ephemeral_privkey_hash = true;

    carrot::CarrotCoinbaseEnoteV1 output_enote1;
    get_coinbase_output_proposal_janus_attack_v1(proposal1, block_index, output_enote1);

    carrot::CarrotCoinbaseEnoteV1 output_enote2;
    get_coinbase_output_proposal_janus_attack_v1(proposal2, block_index, output_enote2);

    EXPECT_NE(0, memcmp(&output_enote1.enote_ephemeral_pubkey,
        &output_enote2.enote_ephemeral_pubkey,
        sizeof(output_enote1.enote_ephemeral_pubkey)));
}
//----------------------------------------------------------------------------------------------------------------------
struct s_comm_T
{
    crypto::hash h;
    crypto::ec_point key;
    crypto::ec_point comm;
};
//----------------------------------------------------------------------------------------------------------------------
static const ge_p3 T_p3 = crypto::get_T_p3();
//----------------------------------------------------------------------------------------------------------------------
static void generate_signature_T(const crypto::hash &prefix_hash, const crypto::public_key &pub,
    const crypto::secret_key &sec, crypto::signature &sig)
{
    // modified from crypto::generate_signature() to use T as a base

    ge_p2 tmp2;
    crypto::ec_scalar k;
    s_comm_T buf;
    buf.h = prefix_hash;
    buf.key = pub;
try_again:
    crypto::random32_unbiased(to_bytes(k));
    ge_scalarmult(&tmp2, to_bytes(k), &T_p3);
    ge_tobytes(to_bytes(buf.comm), &tmp2);
    crypto::hash_to_scalar(&buf, sizeof(s_comm_T), sig.c);
    if (!sc_isnonzero((const unsigned char*)sig.c.data))
        goto try_again;
    sc_mulsub(to_bytes(sig.r), to_bytes(sig.c), to_bytes(unwrap(sec)), to_bytes(k));
    if (!sc_isnonzero((const unsigned char*)sig.r.data))
        goto try_again;
    memwipe(&k, sizeof(k));
}
//----------------------------------------------------------------------------------------------------------------------
static bool check_signature_T(const crypto::hash &prefix_hash, const crypto::public_key &pub,
    const crypto::signature &sig)
{
    // modified from crypto::check_signature() to use T as a base

    ge_p3 tmp3;
    crypto::ec_scalar c;
    s_comm_T buf;
    assert(check_key(pub));
    buf.h = prefix_hash;
    buf.key = pub;
    if (ge_frombytes_vartime(&tmp3, to_bytes(pub)) != 0) {
        return false;
    }
    if (sc_check(to_bytes(sig.c)) != 0 || sc_check(to_bytes(sig.r)) != 0 || !sc_isnonzero(to_bytes(sig.c))) {
        return false;
    }
    {
        // buf.comm = (sig.c) * pub + sig.r * T
        ge_p3 tmp_a;
        ge_scalarmult_p3(&tmp_a, to_bytes(sig.c), &tmp3);
        ge_p3 tmp_b;
        ge_scalarmult_p3(&tmp_b, to_bytes(sig.r), &T_p3);
        ge_cached tmp_c;
        ge_p3_to_cached(&tmp_c, &tmp_a);
        ge_p1p1 tmp_d;
        ge_add(&tmp_d, &tmp_b, &tmp_c);
        ge_p2 tmp_e;
        ge_p1p1_to_p2(&tmp_e, &tmp_d);
        ge_tobytes(to_bytes(buf.comm), &tmp_e);
    }    
    static const crypto::ec_point infinity = {{ 1 }};
    if (memcmp(&buf.comm, &infinity, 32) == 0)
        return false;
    hash_to_scalar(&buf, sizeof(s_comm_T), c);
    sc_sub(to_bytes(c), to_bytes(c), to_bytes(sig.c));
    return sc_isnonzero(to_bytes(c)) == 0;
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, generate_signature_T)
{
    crypto::secret_key sec;
    crypto::random32_unbiased(to_bytes(sec));

    crypto::public_key pub;
    make_carrot_partial_spend_pubkey(sec, pub);

    const crypto::hash h = crypto::rand<crypto::hash>();

    crypto::signature sig;
    generate_signature_T(h, pub, sec, sig);

    EXPECT_TRUE(check_signature_T(h, pub, sig));
}
//----------------------------------------------------------------------------------------------------------------------
// https://gist.github.com/jeffro256/146bfd5306ea3a8a2a0ea4d660cd2243
static bool pq_turnstile_verify_coinbase(const crypto::hash &txid,
    const std::size_t local_output_index,
    const crypto::public_key &partial_spend_pubkey,
    const crypto::secret_key &s_generate_image_preimage,
    const crypto::hash &s_sender_receiver,
    const rct::xmr_amount amount,
    const crypto::hash &migration_tx_signable_hash,
    const crypto::signature &sig,
    const std::function<std::pair<crypto::public_key, rct::key>(const crypto::hash&, std::size_t)> &fetch_output,
    const std::function<bool(const crypto::key_image&)> &is_key_image_spent
)
{
    // step 1
    crypto::public_key output_pubkey;
    rct::key amount_commitment;
    std::tie(output_pubkey, amount_commitment) = fetch_output(txid, local_output_index);

    // step 2
    const bool is_in_main_subgroup = rct::isInMainSubgroup(rct::pk2rct(output_pubkey))
        && rct::isInMainSubgroup(amount_commitment)
        && rct::isInMainSubgroup(rct::pk2rct(partial_spend_pubkey));
    CHECK_AND_ASSERT_MES(is_in_main_subgroup, false, "one of input points not in main subgroup");

    // step 3
    crypto::secret_key k_generate_image;
    make_carrot_generateimage_key(s_generate_image_preimage, partial_spend_pubkey, k_generate_image);

    // step 4
    rct::key account_spend_pubkey_rct;
    rct::addKeys1(account_spend_pubkey_rct, rct::sk2rct(k_generate_image), rct::pk2rct(partial_spend_pubkey));
    const crypto::public_key account_spend_pubkey = rct::rct2pk(account_spend_pubkey_rct);

    // step 5
    const rct::key amount_commitment_rc = rct::zeroCommitVartime(amount);

    // step 6
    CHECK_AND_ASSERT_MES(amount_commitment_rc == amount_commitment,
        false, "failed to re-compute amount commitment");

    // step 7-9
    crypto::public_key output_pubkey_rc;
    make_carrot_onetime_address_coinbase(account_spend_pubkey, s_sender_receiver, amount, output_pubkey_rc);

    // step 10
    CHECK_AND_ASSERT_MES(output_pubkey_rc == output_pubkey, false, "failed to re-compute output pubkey");

    // step 11
    CHECK_AND_ASSERT_MES(check_signature_T(migration_tx_signable_hash, partial_spend_pubkey, sig),
        false, "signature for partial spend pubkey did not validate");

    // step 12
    crypto::ec_point I;
    crypto::derive_key_image_generator(output_pubkey, I);

    crypto::secret_key sender_extension_g;
    make_carrot_sender_extension_g_coinbase(s_sender_receiver,
        amount,
        account_spend_pubkey,
        sender_extension_g);

    crypto::secret_key ki_priv = k_generate_image;
    sc_add(to_bytes(ki_priv), to_bytes(ki_priv), to_bytes(sender_extension_g));

    const crypto::key_image ki = rct::rct2ki(rct::scalarmultKey(rct::pt2rct(I), rct::sk2rct(ki_priv)));

    // step 13
    CHECK_AND_ASSERT_MES(!is_key_image_spent(ki), false, "key image already spent in chain");

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
// https://gist.github.com/jeffro256/146bfd5306ea3a8a2a0ea4d660cd2243
static bool pq_turnstile_verify(const crypto::hash &txid,
    const std::size_t local_output_index,
    const crypto::public_key &partial_spend_pubkey,
    const crypto::secret_key &s_generate_image_preimage,
    const bool is_subaddress,
    const crypto::secret_key &address_index_preimage_2,
    const crypto::hash &s_sender_receiver,
    const rct::xmr_amount amount,
    const CarrotEnoteType enote_type,
    const crypto::hash &migration_tx_signable_hash,
    const crypto::signature &sig,
    const std::function<std::pair<crypto::public_key, rct::key>(const crypto::hash&, std::size_t)> &fetch_output,
    const std::function<bool(const crypto::key_image&)> &is_key_image_spent
)
{
    // step 1
    crypto::public_key output_pubkey;
    rct::key amount_commitment;
    std::tie(output_pubkey, amount_commitment) = fetch_output(txid, local_output_index);

    // step 2
    const bool is_in_main_subgroup = rct::isInMainSubgroup(rct::pk2rct(output_pubkey))
        && rct::isInMainSubgroup(amount_commitment)
        && rct::isInMainSubgroup(rct::pk2rct(partial_spend_pubkey));
    CHECK_AND_ASSERT_MES(is_in_main_subgroup, false, "one of input points not in main subgroup");

    // step 3
    crypto::secret_key k_generate_image;
    make_carrot_generateimage_key(s_generate_image_preimage, partial_spend_pubkey, k_generate_image);

    // step 4
    rct::key account_spend_pubkey_rct;
    rct::addKeys1(account_spend_pubkey_rct, rct::sk2rct(k_generate_image), rct::pk2rct(partial_spend_pubkey));
    const crypto::public_key account_spend_pubkey = rct::rct2pk(account_spend_pubkey_rct);

    // step 5
    crypto::secret_key subaddress_scalar;
    if (is_subaddress)
    {
        make_carrot_subaddress_scalar(address_index_preimage_2, account_spend_pubkey, subaddress_scalar);
    }
    else // main
    {
        subaddress_scalar = {{1}};
    }

    // step 6
    const crypto::public_key address_spend_pubkey = rct::rct2pk(rct::scalarmultKey(account_spend_pubkey_rct,
        rct::sk2rct(subaddress_scalar)));

    // step 7
    crypto::secret_key amount_blinding_factor;
    make_carrot_amount_blinding_factor(s_sender_receiver, amount, address_spend_pubkey, enote_type,
        amount_blinding_factor);

    // step 8
    const rct::key amount_commitment_rc = rct::commit(amount, rct::sk2rct(amount_blinding_factor));

    // step 9
    CHECK_AND_ASSERT_MES(amount_commitment_rc == amount_commitment,
        false, "failed to re-compute amount commitment");

    // step 10-12
    crypto::public_key output_pubkey_rc;
    make_carrot_onetime_address(address_spend_pubkey, s_sender_receiver, amount_commitment, output_pubkey_rc);

    // step 13
    CHECK_AND_ASSERT_MES(output_pubkey_rc == output_pubkey, false, "failed to re-compute output pubkey");

    // step 14
    CHECK_AND_ASSERT_MES(check_signature_T(migration_tx_signable_hash, partial_spend_pubkey, sig),
        false, "signature for partial spend pubkey did not validate");

    // step 15
    crypto::ec_point I;
    crypto::derive_key_image_generator(output_pubkey, I);

    crypto::secret_key sender_extension_g;
    make_carrot_sender_extension_g(s_sender_receiver, amount_commitment, sender_extension_g);

    crypto::secret_key ki_priv = k_generate_image;
    sc_muladd(to_bytes(ki_priv), to_bytes(ki_priv), to_bytes(subaddress_scalar), to_bytes(sender_extension_g));

    const crypto::key_image ki = rct::rct2ki(rct::scalarmultKey(rct::pt2rct(I), rct::sk2rct(ki_priv)));

    // step 16
    CHECK_AND_ASSERT_MES(!is_key_image_spent(ki), false, "key image already spent in chain");

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, pq_turnstile_completeness_main)
{
    carrot::mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 main_addr = bob.cryptonote_address();
    const CarrotPaymentProposalV1 payment_proposal{
        .destination = main_addr,
        .amount = COIN,
        .randomness = gen_janus_anchor()
    };
    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    RCTOutputEnoteProposal output_enote;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_normal_v1(payment_proposal, tx_first_key_image, output_enote, encrypted_payment_id);

    const crypto::hash migration_tx_signable_hash = crypto::rand<crypto::hash>();
    crypto::signature migration_sig;
    generate_signature_T(migration_tx_signable_hash, bob.carrot_partial_spend_pubkey, bob.k_prove_spend, migration_sig);

    crypto::hash s_sender_receiver;
    {
        // s_sr = k_v D_e
        mx25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
            output_enote.enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        // input_context = "R" || KI_1
        const input_context_t input_context = make_carrot_input_context(output_enote.enote.tx_first_key_image);

        // s^ctx_sr = H_32(s_sr, D_e, input_context)
        make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
            output_enote.enote.enote_ephemeral_pubkey,
            input_context,
            s_sender_receiver);
    }

    crypto::key_image ki;
    const bool pq_ver = pq_turnstile_verify(/*txid=*/{},
        /*local_output_index=*/{},
        bob.carrot_partial_spend_pubkey,
        bob.s_generate_image_preimage,
        /*is_subaddress=*/false,
        /*address_index_preimage_2=*/{},
        s_sender_receiver,
        payment_proposal.amount,
        CarrotEnoteType::PAYMENT,
        migration_tx_signable_hash,
        migration_sig,
        [&output_enote](auto,auto){ return std::pair(output_enote.enote.onetime_address, output_enote.enote.amount_commitment); },
        [&ki](const crypto::key_image &derived_key_img){ ki = derived_key_img; return false;});
    EXPECT_TRUE(pq_ver);

    // scan and compute key image to test pq_turnstile_verify()
    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);
    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external_receiver(output_enote.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        bob.k_view_incoming_dev,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        recovered_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_payment_id,
        recovered_enote_type);
    ASSERT_TRUE(scan_success);
    const crypto::key_image scanned_ki = bob.derive_key_image(
        recovered_address_spend_pubkey,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        output_enote.enote.onetime_address);
    ASSERT_EQ(scanned_ki, ki);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, pq_turnstile_completeness_sub)
{
    carrot::mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const subaddress_index subaddr_index = mock::gen_subaddress_index();
    ASSERT_TRUE(subaddr_index.is_subaddress());
    const CarrotDestinationV1 sub_addr = bob.subaddress({subaddr_index});
    const CarrotPaymentProposalV1 payment_proposal{
        .destination = sub_addr,
        .amount = COIN,
        .randomness = gen_janus_anchor()
    };
    const crypto::key_image tx_first_key_image = mock::gen_key_image();

    RCTOutputEnoteProposal output_enote;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_proposal_normal_v1(payment_proposal, tx_first_key_image, output_enote, encrypted_payment_id);

    const crypto::hash migration_tx_signable_hash = crypto::rand<crypto::hash>();
    crypto::signature migration_sig;
    generate_signature_T(migration_tx_signable_hash, bob.carrot_partial_spend_pubkey, bob.k_prove_spend, migration_sig);
    
    crypto::secret_key address_index_preimage_2;
    {
        crypto::secret_key address_index_preimage_1;
        make_carrot_address_index_preimage_1(bob.s_generate_address, subaddr_index.major, subaddr_index.minor,
            address_index_preimage_1);
        make_carrot_address_index_preimage_2(address_index_preimage_1, subaddr_index.major, subaddr_index.minor,
            bob.carrot_account_spend_pubkey, bob.carrot_account_view_pubkey, address_index_preimage_2);
    }

    crypto::hash s_sender_receiver;
    {
        // s_sr = k_v D_e
        mx25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
            output_enote.enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        // input_context = "R" || KI_1
        const input_context_t input_context = make_carrot_input_context(output_enote.enote.tx_first_key_image);

        // s^ctx_sr = H_32(s_sr, D_e, input_context)
        make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
            output_enote.enote.enote_ephemeral_pubkey,
            input_context,
            s_sender_receiver);
    }

    crypto::key_image ki;
    const bool pq_ver = pq_turnstile_verify(/*txid=*/{},
        /*local_output_index=*/{},
        bob.carrot_partial_spend_pubkey,
        bob.s_generate_image_preimage,
        /*is_subaddress=*/true,
        address_index_preimage_2,
        s_sender_receiver,
        payment_proposal.amount,
        CarrotEnoteType::PAYMENT,
        migration_tx_signable_hash,
        migration_sig,
        [&output_enote](auto,auto){ return std::pair(output_enote.enote.onetime_address, output_enote.enote.amount_commitment); },
        [&ki](const crypto::key_image &derived_key_img){ ki = derived_key_img; return false;});
    EXPECT_TRUE(pq_ver);

    // scan and compute key image to test pq_turnstile_verify()
    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);
    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external_receiver(output_enote.enote,
        std::nullopt,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        bob.k_view_incoming_dev,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        recovered_address_spend_pubkey,
        recovered_amount,
        recovered_amount_blinding_factor,
        recovered_payment_id,
        recovered_enote_type);
    ASSERT_TRUE(scan_success);
    const crypto::key_image scanned_ki = bob.derive_key_image(
        recovered_address_spend_pubkey,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        output_enote.enote.onetime_address);
    ASSERT_EQ(scanned_ki, ki);
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, pq_turnstile_completeness_coinbase)
{
    carrot::mock::mock_carrot_and_legacy_keys bob;
    bob.generate();

    const CarrotDestinationV1 main_addr = bob.cryptonote_address();
    const CarrotPaymentProposalV1 payment_proposal{
        .destination = main_addr,
        .amount = COIN,
        .randomness = gen_janus_anchor()
    };
    const std::uint64_t block_index = mock::gen_block_index();

    CarrotCoinbaseEnoteV1 output_enote;
    get_coinbase_output_proposal_v1(payment_proposal, block_index, output_enote);

    const crypto::hash migration_tx_signable_hash = crypto::rand<crypto::hash>();
    crypto::signature migration_sig;
    generate_signature_T(migration_tx_signable_hash, bob.carrot_partial_spend_pubkey, bob.k_prove_spend, migration_sig);

    crypto::hash s_sender_receiver;
    {
        // s_sr = k_v D_e
        mx25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
            output_enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        // input_context = "R" || KI_1
        const input_context_t input_context = make_carrot_input_context_coinbase(output_enote.block_index);

        // s^ctx_sr = H_32(s_sr, D_e, input_context)
        make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
            output_enote.enote_ephemeral_pubkey,
            input_context,
            s_sender_receiver);
    }

    crypto::key_image ki;
    const bool pq_ver = pq_turnstile_verify_coinbase(/*txid=*/{},
        /*local_output_index=*/{},
        bob.carrot_partial_spend_pubkey,
        bob.s_generate_image_preimage,
        s_sender_receiver,
        payment_proposal.amount,
        migration_tx_signable_hash,
        migration_sig,
        [&output_enote](auto,auto){ return std::pair(output_enote.onetime_address, rct::zeroCommitVartime(output_enote.amount)); },
        [&ki](const crypto::key_image &derived_key_img){ ki = derived_key_img; return false;});
    EXPECT_TRUE(pq_ver);

    // scan and compute key image to test pq_turnstile_verify()
    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(bob.k_view_incoming_dev,
        output_enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);
    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    const bool scan_success = try_scan_carrot_coinbase_enote_receiver(output_enote,
        s_sender_receiver_unctx,
        {&bob.carrot_account_spend_pubkey, 1},
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        recovered_address_spend_pubkey);
    ASSERT_TRUE(scan_success);
    const crypto::key_image scanned_ki = bob.derive_key_image(
        recovered_address_spend_pubkey,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        output_enote.onetime_address);
    ASSERT_EQ(scanned_ki, ki);
}
//----------------------------------------------------------------------------------------------------------------------
