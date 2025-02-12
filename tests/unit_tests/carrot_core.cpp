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

#include "carrot_core/account_secrets.h"
#include "carrot_core/address_utils.h"
#include "carrot_core/carrot_enote_scan.h"
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/output_set_finalization.h"
#include "carrot_core/payment_proposal.h"
#include "crypto/crypto.h"
#include "crypto/generators.h"
#include "ringct/rctOps.h"

using namespace carrot;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
namespace
{
// https://github.com/jedisct1/libsodium/blob/master/src/libsodium/crypto_scalarmult/curve25519/ref10/x25519_ref10.c#L17
static const mx25519_pubkey x25519_small_order_points[7] = {
    /* 0 (order 4) */
    {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    /* 1 (order 1) */
    {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    /* 325606250916557431795983626356110631294008115727848805560023387167927233504
        (order 8) */
    {{ 0xe0, 0xeb, 0x7a, 0x7c, 0x3b, 0x41, 0xb8, 0xae, 0x16, 0x56, 0xe3,
        0xfa, 0xf1, 0x9f, 0xc4, 0x6a, 0xda, 0x09, 0x8d, 0xeb, 0x9c, 0x32,
        0xb1, 0xfd, 0x86, 0x62, 0x05, 0x16, 0x5f, 0x49, 0xb8, 0x00 }},
    /* 39382357235489614581723060781553021112529911719440698176882885853963445705823
        (order 8) */
    {{ 0x5f, 0x9c, 0x95, 0xbc, 0xa3, 0x50, 0x8c, 0x24, 0xb1, 0xd0, 0xb1,
        0x55, 0x9c, 0x83, 0xef, 0x5b, 0x04, 0x44, 0x5c, 0xc4, 0x58, 0x1c,
        0x8e, 0x86, 0xd8, 0x22, 0x4e, 0xdd, 0xd0, 0x9f, 0x11, 0x57 }},
    /* p-1 (order 2) */
    {{ 0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f }},
    /* p (=0, order 4) */
    {{ 0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f }},
    /* p+1 (=1, order 1) */
    {{ 0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f }}
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct mock_carrot_keys
{
    crypto::secret_key s_master;
    crypto::secret_key k_prove_spend;
    crypto::secret_key s_view_balance;
    crypto::secret_key k_generate_image;
    crypto::secret_key k_view;
    crypto::secret_key s_generate_address;
    crypto::public_key account_spend_pubkey;
    crypto::public_key account_view_pubkey;
    crypto::public_key main_address_view_pubkey;

    view_incoming_key_ram_borrowed_device k_view_dev;
    view_balance_secret_ram_borrowed_device s_view_balance_dev;
    generate_address_secret_ram_borrowed_device s_generate_address_dev;

    mock_carrot_keys(): k_view_dev(k_view),
        s_view_balance_dev(s_view_balance),
        s_generate_address_dev(s_generate_address)
    {}

    static mock_carrot_keys generate()
    {
        mock_carrot_keys k;
        crypto::generate_random_bytes_thread_safe(sizeof(crypto::secret_key), to_bytes(k.s_master));
        make_carrot_provespend_key(k.s_master, k.k_prove_spend);
        make_carrot_viewbalance_secret(k.s_master, k.s_view_balance);
        make_carrot_generateimage_key(k.s_view_balance, k.k_generate_image);
        make_carrot_viewincoming_key(k.s_view_balance, k.k_view);
        make_carrot_generateaddress_secret(k.s_view_balance, k.s_generate_address);
        make_carrot_spend_pubkey(k.k_generate_image, k.k_prove_spend, k.account_spend_pubkey);
        k.account_view_pubkey = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(k.account_spend_pubkey),
            rct::sk2rct(k.k_view)));
        k.main_address_view_pubkey = rct::rct2pk(rct::scalarmultBase(rct::sk2rct(k.k_view)));
        return k;
    }
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool can_open_fcmp_onetime_address(const crypto::secret_key &k_prove_spend,
    const crypto::secret_key &k_generate_image,
    const crypto::secret_key &subaddr_scalar,
    const crypto::secret_key &sender_extension_g,
    const crypto::secret_key &sender_extension_t,
    const crypto::public_key &onetime_address)
{
    // K_s = k_gi G + k_ps T
    // K^j_s = k^j_subscal * K_s
    // Ko = K^j_s + k^o_g G + k^o_t T
    //    = (k^o_g + k^j_subscal * k_gi) G + (k^o_t + k^j_subscal * k_ps) T

    // combined_g = k^o_g + k^j_subscal * k_gi
    rct::key combined_g;
    sc_muladd(combined_g.bytes, to_bytes(subaddr_scalar), to_bytes(k_generate_image), to_bytes(sender_extension_g));

    // combined_t = k^o_t + k^j_subscal * k_ps
    rct::key combined_t;
    sc_muladd(combined_t.bytes, to_bytes(subaddr_scalar), to_bytes(k_prove_spend), to_bytes(sender_extension_t));

    // Ko' = combined_g G + combined_t T
    rct::key recomputed_onetime_address;
    rct::addKeys2(recomputed_onetime_address, combined_g, combined_t, rct::pk2rct(crypto::get_T()));

    // Ko' ?= Ko
    return recomputed_onetime_address == onetime_address;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct unittest_carrot_scan_result_t
{
    crypto::public_key address_spend_pubkey = rct::rct2pk(rct::I);
    crypto::secret_key sender_extension_g = rct::rct2sk(rct::I);
    crypto::secret_key sender_extension_t = rct::rct2sk(rct::I);

    rct::xmr_amount amount = 0;
    crypto::secret_key amount_blinding_factor = rct::rct2sk(rct::I);

    CarrotEnoteType enote_type = CarrotEnoteType::PAYMENT;

    payment_id_t payment_id = null_payment_id;

    janus_anchor_t internal_message = janus_anchor_t{};

    size_t output_index = 0;
};
static void unittest_scan_enote_set(const std::vector<CarrotEnoteV1> &enotes,
    const encrypted_payment_id_t encrypted_payment_id,
    const mock_carrot_keys keys,
    std::vector<unittest_carrot_scan_result_t> &res)
{
    res.clear();

    // external scans
    for (size_t output_index = 0; output_index < enotes.size(); ++output_index)
    {
        const CarrotEnoteV1 &enote = enotes.at(output_index);

        // s_sr = k_v D_e
        mx25519_pubkey s_sr;
        make_carrot_uncontextualized_shared_key_receiver(keys.k_view, enote.enote_ephemeral_pubkey, s_sr);

        unittest_carrot_scan_result_t scan_result{};
        const bool r = try_scan_carrot_enote_external(enote,
            encrypted_payment_id,
            s_sr,
            keys.k_view_dev,
            keys.account_spend_pubkey,
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

    // internal scans
    for (size_t output_index = 0; output_index < enotes.size(); ++output_index)
    {
        const CarrotEnoteV1 &enote = enotes.at(output_index);

        unittest_carrot_scan_result_t scan_result{};
        const bool r = try_scan_carrot_enote_internal(enote,
            keys.s_view_balance_dev,
            scan_result.sender_extension_g,
            scan_result.sender_extension_t,
            scan_result.address_spend_pubkey,
            scan_result.amount,
            scan_result.amount_blinding_factor,
            scan_result.enote_type,
            scan_result.internal_message);

        scan_result.output_index = output_index;

        if (r)
            res.push_back(scan_result);
    }
}
} // namespace
static inline bool operator==(const mx25519_pubkey &a, const mx25519_pubkey &b)
{
    return 0 == memcmp(&a, &b, sizeof(mx25519_pubkey));
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
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
    const mock_carrot_keys keys = mock_carrot_keys::generate();

    CarrotDestinationV1 main_address;
    make_carrot_main_address_v1(keys.account_spend_pubkey, keys.main_address_view_pubkey, main_address);

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
    make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
        enote_proposal.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    encrypted_payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external(enote_proposal.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        keys.k_view_dev,
        keys.account_spend_pubkey,
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
    EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
        keys.k_generate_image,
        rct::rct2sk(rct::I),
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote_proposal.enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, subaddress_normal_scan_completeness)
{
    const mock_carrot_keys keys = mock_carrot_keys::generate();

    const uint32_t j_major = crypto::rand<uint32_t>();
    const uint32_t j_minor = crypto::rand<uint32_t>();

    CarrotDestinationV1 subaddress;
    make_carrot_subaddress_v1(keys.account_spend_pubkey,
        keys.account_view_pubkey,
        keys.s_generate_address_dev,
        j_major,
        j_minor,
        subaddress);

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
    make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
        enote_proposal.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    encrypted_payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external(enote_proposal.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        keys.k_view_dev,
        keys.account_spend_pubkey,
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
    crypto::secret_key address_generator;
    make_carrot_index_extension_generator(keys.s_generate_address,
        j_major,
        j_minor,
        address_generator);

    crypto::secret_key subaddr_scalar;
    make_carrot_subaddress_scalar(keys.account_spend_pubkey,
        address_generator,
        j_major,
        j_minor,
        subaddr_scalar);

    EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
        keys.k_generate_image,
        subaddr_scalar,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote_proposal.enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, integrated_address_normal_scan_completeness)
{
    const mock_carrot_keys keys = mock_carrot_keys::generate();

    CarrotDestinationV1 integrated_address;
    make_carrot_integrated_address_v1(keys.account_spend_pubkey,
        keys.main_address_view_pubkey,
        gen_payment_id(),
        integrated_address);

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
    make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
        enote_proposal.enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    encrypted_payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external(enote_proposal.enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        keys.k_view_dev,
        keys.account_spend_pubkey,
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
    EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
        keys.k_generate_image,
        rct::rct2sk(rct::I),
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote_proposal.enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, main_address_special_scan_completeness)
{
    const mock_carrot_keys keys = mock_carrot_keys::generate();

    CarrotDestinationV1 main_address;
    make_carrot_main_address_v1(keys.account_spend_pubkey, keys.main_address_view_pubkey, main_address);

    // try once with PAYMENT, once with CHANGE
    for (int i = 0; i < 2; ++i)
    {
        const CarrotEnoteType enote_type = i ? CarrotEnoteType::PAYMENT : CarrotEnoteType::CHANGE;

        const CarrotPaymentProposalSelfSendV1 proposal = CarrotPaymentProposalSelfSendV1{
            .destination_address_spend_pubkey = keys.account_spend_pubkey,
            .amount = crypto::rand<rct::xmr_amount>(),
            .enote_type = enote_type,
            .enote_ephemeral_pubkey = gen_x25519_pubkey(),
        };

        const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

        RCTOutputEnoteProposal enote_proposal;
        get_output_proposal_special_v1(proposal,
            keys.k_view_dev,
            keys.account_spend_pubkey,
            tx_first_key_image,
            std::nullopt,
            enote_proposal);

        ASSERT_EQ(proposal.amount, enote_proposal.amount);
        const rct::key recomputed_amount_commitment = rct::commit(enote_proposal.amount, rct::sk2rct(enote_proposal.amount_blinding_factor));
        ASSERT_EQ(enote_proposal.enote.amount_commitment, recomputed_amount_commitment);

        mx25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
            enote_proposal.enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        encrypted_payment_id_t recovered_payment_id;
        CarrotEnoteType recovered_enote_type;
        const bool scan_success = try_scan_carrot_enote_external(enote_proposal.enote,
            std::nullopt,
            s_sender_receiver_unctx,
            keys.k_view_dev,
            keys.account_spend_pubkey,
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
        EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
            keys.k_generate_image,
            rct::rct2sk(rct::I),
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote_proposal.enote.onetime_address));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, subaddress_special_scan_completeness)
{
    const mock_carrot_keys keys = mock_carrot_keys::generate();

    const uint32_t j_major = crypto::rand<uint32_t>();
    const uint32_t j_minor = crypto::rand<uint32_t>();

    CarrotDestinationV1 subaddress;
    make_carrot_subaddress_v1(keys.account_spend_pubkey,
        keys.account_view_pubkey,
        keys.s_generate_address_dev,
        j_major,
        j_minor,
        subaddress);

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
            keys.k_view_dev,
            keys.account_spend_pubkey,
            tx_first_key_image,
            std::nullopt,
            enote_proposal);

        ASSERT_EQ(proposal.amount, enote_proposal.amount);
        const rct::key recomputed_amount_commitment = rct::commit(enote_proposal.amount, rct::sk2rct(enote_proposal.amount_blinding_factor));
        ASSERT_EQ(enote_proposal.enote.amount_commitment, recomputed_amount_commitment);

        mx25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
            enote_proposal.enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        encrypted_payment_id_t recovered_payment_id;
        CarrotEnoteType recovered_enote_type;
        const bool scan_success = try_scan_carrot_enote_external(enote_proposal.enote,
            std::nullopt,
            s_sender_receiver_unctx,
            keys.k_view_dev,
            keys.account_spend_pubkey,
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
        crypto::secret_key address_generator;
        make_carrot_index_extension_generator(keys.s_generate_address,
            j_major,
            j_minor,
            address_generator);

        crypto::secret_key subaddr_scalar;
        make_carrot_subaddress_scalar(keys.account_spend_pubkey,
            address_generator,
            j_major,
            j_minor,
            subaddr_scalar);

        EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
            keys.k_generate_image,
            subaddr_scalar,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote_proposal.enote.onetime_address));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, main_address_internal_scan_completeness)
{
    const mock_carrot_keys keys = mock_carrot_keys::generate();

    CarrotDestinationV1 main_address;
    make_carrot_main_address_v1(keys.account_spend_pubkey, keys.main_address_view_pubkey, main_address);

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
        const bool scan_success = try_scan_carrot_enote_internal(enote_proposal.enote,
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
        EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
            keys.k_generate_image,
            rct::rct2sk(rct::I),
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote_proposal.enote.onetime_address));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, subaddress_internal_scan_completeness)
{
    const mock_carrot_keys keys = mock_carrot_keys::generate();

    const uint32_t j_major = crypto::rand<uint32_t>();
    const uint32_t j_minor = crypto::rand<uint32_t>();

    CarrotDestinationV1 subaddress;
    make_carrot_subaddress_v1(keys.account_spend_pubkey,
        keys.account_view_pubkey,
        keys.s_generate_address_dev,
        j_major,
        j_minor,
        subaddress);

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
        const bool scan_success = try_scan_carrot_enote_internal(enote_proposal.enote,
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
        crypto::secret_key address_generator;
        make_carrot_index_extension_generator(keys.s_generate_address,
            j_major,
            j_minor,
            address_generator);

        crypto::secret_key subaddr_scalar;
        make_carrot_subaddress_scalar(keys.account_spend_pubkey,
            address_generator,
            j_major,
            j_minor,
            subaddr_scalar);

        EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
            keys.k_generate_image,
            subaddr_scalar,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote_proposal.enote.onetime_address));
    }
}
//----------------------------------------------------------------------------------------------------------------------
TEST(carrot_core, main_address_coinbase_scan_completeness)
{
    const mock_carrot_keys keys = mock_carrot_keys::generate();

    CarrotDestinationV1 main_address;
    make_carrot_main_address_v1(keys.account_spend_pubkey, keys.main_address_view_pubkey, main_address);

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
    make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
        enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    const bool scan_success = try_scan_carrot_coinbase_enote(enote,
        s_sender_receiver_unctx,
        keys.k_view_dev,
        keys.account_spend_pubkey,
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        recovered_address_spend_pubkey);
    
    ASSERT_TRUE(scan_success);

    // check recovered data
    EXPECT_EQ(proposal.destination.address_spend_pubkey, recovered_address_spend_pubkey);

    // check spendability
    EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
        keys.k_generate_image,
        rct::rct2sk(rct::I),
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote.onetime_address));
}
//----------------------------------------------------------------------------------------------------------------------
static void subtest_2out_transfer_get_output_enote_proposals_completeness(const bool alice_subaddress,
    const bool bob_subaddress,
    const bool bob_integrated,
    const CarrotEnoteType alice_selfsend_type,
    const bool alice_internal_selfsends)
{
    // generate alice keys and address
    const mock_carrot_keys alice = mock_carrot_keys::generate();
    const uint32_t alice_j_major = crypto::rand<uint32_t>();
    const uint32_t alice_j_minor = crypto::rand<uint32_t>();
    CarrotDestinationV1 alice_address;
    if (alice_subaddress)
    {
        make_carrot_subaddress_v1(alice.account_spend_pubkey,
            alice.account_view_pubkey,
            alice.s_generate_address_dev,
            alice_j_major,
            alice_j_minor,
            alice_address);
    }
    else // alice main address
    {
        make_carrot_main_address_v1(alice.account_spend_pubkey,
            alice.main_address_view_pubkey,
            alice_address);
    }

    // generate bob keys and address
    const mock_carrot_keys bob = mock_carrot_keys::generate();
    const uint32_t bob_j_major = crypto::rand<uint32_t>();
    const uint32_t bob_j_minor = crypto::rand<uint32_t>();
    CarrotDestinationV1 bob_address;
    if (bob_subaddress)
    {
        make_carrot_subaddress_v1(bob.account_spend_pubkey,
            bob.account_view_pubkey,
            bob.s_generate_address_dev,
            bob_j_major,
            bob_j_minor,
            bob_address);
    }
    else if (bob_integrated)
    {
        make_carrot_integrated_address_v1(bob.account_spend_pubkey,
            bob.main_address_view_pubkey,
            gen_payment_id(),
            bob_address);
    }
    else // bob main address
    {
        make_carrot_main_address_v1(bob.account_spend_pubkey,
            bob.main_address_view_pubkey,
            bob_address);
    }

    // generate input context
    const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen());
    input_context_t input_context;
    make_carrot_input_context(tx_first_key_image, input_context);

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
        : gen_payment_id();

    // turn payment proposals into enotes, passing dummy pid_enc if bob isn't integrated
    std::vector<RCTOutputEnoteProposal> enote_proposals;
    encrypted_payment_id_t encrypted_payment_id;
    get_output_enote_proposals({bob_payment_proposal},
        {alice_payment_proposal},
        dummy_encrypted_pid,
        alice_internal_selfsends ? &alice.s_view_balance_dev : nullptr,
        &alice.k_view_dev,
        alice.account_spend_pubkey,
        tx_first_key_image,
        enote_proposals,
        encrypted_payment_id);

    ASSERT_EQ(2, enote_proposals.size()); // 2-out tx

    // collect enotes
    std::vector<CarrotEnoteV1> enotes;
    for (const RCTOutputEnoteProposal &enote_proposal : enote_proposals)
        enotes.push_back(enote_proposal.enote);

    // check that alice scanned 1 enote
    std::vector<unittest_carrot_scan_result_t> alice_scan_vec;
    unittest_scan_enote_set(enotes, encrypted_payment_id, alice, alice_scan_vec);
    ASSERT_EQ(1, alice_scan_vec.size());
    unittest_carrot_scan_result_t alice_scan = alice_scan_vec.front();

    // check that bob scanned 1 enote
    std::vector<unittest_carrot_scan_result_t> bob_scan_vec;
    unittest_scan_enote_set(enotes, encrypted_payment_id, bob, bob_scan_vec);
    ASSERT_EQ(1, bob_scan_vec.size());
    unittest_carrot_scan_result_t bob_scan = bob_scan_vec.front();

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
    crypto::secret_key alice_address_generator;
    make_carrot_index_extension_generator(alice.s_generate_address,
        alice_j_major,
        alice_j_minor,
        alice_address_generator);

    crypto::secret_key alice_subaddr_scalar;
    make_carrot_subaddress_scalar(alice.account_spend_pubkey,
        alice_address_generator,
        alice_j_major,
        alice_j_minor,
        alice_subaddr_scalar);

    EXPECT_TRUE(can_open_fcmp_onetime_address(alice.k_prove_spend,
        alice.k_generate_image,
        alice_subaddress ? alice_subaddr_scalar : crypto::secret_key{{1}},
        alice_scan.sender_extension_g,
        alice_scan.sender_extension_t,
        alice_enote.onetime_address));
    
    // check Bob spendability
    crypto::secret_key bob_address_generator;
    make_carrot_index_extension_generator(bob.s_generate_address,
        bob_j_major,
        bob_j_minor,
        bob_address_generator);

    crypto::secret_key bob_subaddr_scalar;
    make_carrot_subaddress_scalar(bob.account_spend_pubkey,
        bob_address_generator,
        bob_j_major,
        bob_j_minor,
        bob_subaddr_scalar);

    EXPECT_TRUE(can_open_fcmp_onetime_address(bob.k_prove_spend,
        bob.k_generate_image,
        bob_subaddress ? bob_subaddr_scalar : crypto::secret_key{{1}},
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
