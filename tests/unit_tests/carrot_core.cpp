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
#include "carrot_core/payment_proposal.h"
#include "crypto/crypto.h"
#include "crypto/generators.h"
#include "ringct/rctOps.h"

using namespace carrot;

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

    mock_carrot_keys(): k_view_dev(k_view), s_view_balance_dev(s_view_balance)
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
    //    = (k^o_g + k^j_subscal * k_gi) + (k^o_t + k^j_subscal * k_ps)

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
TEST(carrot_core, ECDH_cryptonote_completeness)
{
    crypto::secret_key k_view = rct::rct2sk(rct::skGen());
    crypto::public_key view_pubkey = rct::rct2pk(rct::scalarmultBase(rct::sk2rct(k_view)));
    crypto::secret_key k_ephem = rct::rct2sk(rct::skGen());
    ASSERT_NE(k_view, k_ephem);

    crypto::x25519_pubkey enote_ephemeral_pubkey;
    make_carrot_enote_ephemeral_pubkey_cryptonote(k_ephem, enote_ephemeral_pubkey);

    crypto::x25519_pubkey s_sr_sender;
    ASSERT_TRUE(make_carrot_uncontextualized_shared_key_sender(k_ephem, view_pubkey, s_sr_sender));

    crypto::x25519_pubkey s_sr_receiver;
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

    crypto::x25519_pubkey enote_ephemeral_pubkey;
    make_carrot_enote_ephemeral_pubkey_subaddress(k_ephem, spend_pubkey, enote_ephemeral_pubkey);

    crypto::x25519_pubkey s_sr_sender;
    ASSERT_TRUE(make_carrot_uncontextualized_shared_key_sender(k_ephem, view_pubkey, s_sr_sender));

    crypto::x25519_pubkey s_sr_receiver;
    ASSERT_TRUE(make_carrot_uncontextualized_shared_key_receiver(k_view, enote_ephemeral_pubkey, s_sr_receiver));

    EXPECT_EQ(s_sr_sender, s_sr_receiver);
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

    CarrotEnoteV1 enote;
    encrypted_payment_id_t encrypted_payment_id;
    rct::xmr_amount amount;
    crypto::secret_key amount_blinding_factor;
    get_output_proposal_normal_v1(proposal,
        tx_first_key_image,
        enote,
        encrypted_payment_id,
        amount,
        amount_blinding_factor);

    ASSERT_EQ(proposal.amount, amount);
    ASSERT_EQ(enote.amount_commitment, rct::commit(amount, rct::sk2rct(amount_blinding_factor)));

    crypto::x25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
        enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    encrypted_payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external(enote,
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
    EXPECT_EQ(amount, recovered_amount);
    EXPECT_EQ(amount_blinding_factor, recovered_amount_blinding_factor);
    EXPECT_EQ(null_payment_id, recovered_payment_id);
    EXPECT_EQ(CarrotEnoteType::PAYMENT, recovered_enote_type);

    // check spendability
    EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
        keys.k_generate_image,
        rct::rct2sk(rct::I),
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote.onetime_address));
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
        keys.s_generate_address,
        j_major,
        j_minor,
        subaddress);

    const CarrotPaymentProposalV1 proposal = CarrotPaymentProposalV1{
        .destination = subaddress,
        .amount = crypto::rand<rct::xmr_amount>(),
        .randomness = gen_janus_anchor()
    };

    const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

    CarrotEnoteV1 enote;
    encrypted_payment_id_t encrypted_payment_id;
    rct::xmr_amount amount;
    crypto::secret_key amount_blinding_factor;
    get_output_proposal_normal_v1(proposal,
        tx_first_key_image,
        enote,
        encrypted_payment_id,
        amount,
        amount_blinding_factor);

    ASSERT_EQ(proposal.amount, amount);
    ASSERT_EQ(enote.amount_commitment, rct::commit(amount, rct::sk2rct(amount_blinding_factor)));

    crypto::x25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
        enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    encrypted_payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external(enote,
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
    EXPECT_EQ(amount, recovered_amount);
    EXPECT_EQ(amount_blinding_factor, recovered_amount_blinding_factor);
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
        enote.onetime_address));
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

    CarrotEnoteV1 enote;
    encrypted_payment_id_t encrypted_payment_id;
    rct::xmr_amount amount;
    crypto::secret_key amount_blinding_factor;
    get_output_proposal_normal_v1(proposal,
        tx_first_key_image,
        enote,
        encrypted_payment_id,
        amount,
        amount_blinding_factor);

    ASSERT_EQ(proposal.amount, amount);
    ASSERT_EQ(enote.amount_commitment, rct::commit(amount, rct::sk2rct(amount_blinding_factor)));

    crypto::x25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
        enote.enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    crypto::secret_key recovered_sender_extension_g;
    crypto::secret_key recovered_sender_extension_t;
    crypto::public_key recovered_address_spend_pubkey;
    rct::xmr_amount recovered_amount;
    crypto::secret_key recovered_amount_blinding_factor;
    encrypted_payment_id_t recovered_payment_id;
    CarrotEnoteType recovered_enote_type;
    const bool scan_success = try_scan_carrot_enote_external(enote,
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
    EXPECT_EQ(amount, recovered_amount);
    EXPECT_EQ(amount_blinding_factor, recovered_amount_blinding_factor);
    EXPECT_EQ(integrated_address.payment_id, recovered_payment_id);
    EXPECT_EQ(CarrotEnoteType::PAYMENT, recovered_enote_type);

    // check spendability
    EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
        keys.k_generate_image,
        rct::rct2sk(rct::I),
        recovered_sender_extension_g,
        recovered_sender_extension_t,
        enote.onetime_address));
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
            .enote_ephemeral_pubkey = crypto::x25519_pubkey_gen(),
        };

        const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

        CarrotEnoteV1 enote;
        rct::xmr_amount amount;
        crypto::secret_key amount_blinding_factor;
        get_output_proposal_special_v1(proposal,
            keys.k_view,
            keys.account_spend_pubkey,
            tx_first_key_image,
            enote,
            amount,
            amount_blinding_factor);

        ASSERT_EQ(proposal.amount, amount);
        ASSERT_EQ(enote.amount_commitment, rct::commit(amount, rct::sk2rct(amount_blinding_factor)));

        crypto::x25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
            enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        encrypted_payment_id_t recovered_payment_id;
        CarrotEnoteType recovered_enote_type;
        const bool scan_success = try_scan_carrot_enote_external(enote,
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
        EXPECT_EQ(amount, recovered_amount);
        EXPECT_EQ(amount_blinding_factor, recovered_amount_blinding_factor);
        EXPECT_EQ(null_payment_id, recovered_payment_id);
        EXPECT_EQ(enote_type, recovered_enote_type);

        // check spendability
        EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
            keys.k_generate_image,
            rct::rct2sk(rct::I),
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote.onetime_address));
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
        keys.s_generate_address,
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
            .enote_ephemeral_pubkey = crypto::x25519_pubkey_gen(),
        };

        const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

        CarrotEnoteV1 enote;
        rct::xmr_amount amount;
        crypto::secret_key amount_blinding_factor;
        get_output_proposal_special_v1(proposal,
            keys.k_view,
            keys.account_spend_pubkey,
            tx_first_key_image,
            enote,
            amount,
            amount_blinding_factor);

        ASSERT_EQ(proposal.amount, amount);
        ASSERT_EQ(enote.amount_commitment, rct::commit(amount, rct::sk2rct(amount_blinding_factor)));

        crypto::x25519_pubkey s_sender_receiver_unctx;
        make_carrot_uncontextualized_shared_key_receiver(keys.k_view,
            enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx);

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        encrypted_payment_id_t recovered_payment_id;
        CarrotEnoteType recovered_enote_type;
        const bool scan_success = try_scan_carrot_enote_external(enote,
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
        EXPECT_EQ(amount, recovered_amount);
        EXPECT_EQ(amount_blinding_factor, recovered_amount_blinding_factor);
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
            enote.onetime_address));
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
            .enote_ephemeral_pubkey = crypto::x25519_pubkey_gen()
        };

        const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

        CarrotEnoteV1 enote;
        rct::xmr_amount amount;
        crypto::secret_key amount_blinding_factor;
        get_output_proposal_internal_v1(proposal,
            keys.s_view_balance,
            tx_first_key_image,
            enote,
            amount,
            amount_blinding_factor);

        ASSERT_EQ(proposal.amount, amount);
        ASSERT_EQ(enote.amount_commitment, rct::commit(amount, rct::sk2rct(amount_blinding_factor)));

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        CarrotEnoteType recovered_enote_type;
        const bool scan_success = try_scan_carrot_enote_internal(enote,
            keys.s_view_balance_dev,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            recovered_address_spend_pubkey,
            recovered_amount,
            recovered_amount_blinding_factor,
            recovered_enote_type);

        ASSERT_TRUE(scan_success);

        // check recovered data
        EXPECT_EQ(proposal.destination_address_spend_pubkey, recovered_address_spend_pubkey);
        EXPECT_EQ(amount, recovered_amount);
        EXPECT_EQ(amount_blinding_factor, recovered_amount_blinding_factor);
        EXPECT_EQ(enote_type, recovered_enote_type);

        // check spendability
        EXPECT_TRUE(can_open_fcmp_onetime_address(keys.k_prove_spend,
            keys.k_generate_image,
            rct::rct2sk(rct::I),
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            enote.onetime_address));
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
        keys.s_generate_address,
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
            .enote_ephemeral_pubkey = crypto::x25519_pubkey_gen()
        };

        const crypto::key_image tx_first_key_image = rct::rct2ki(rct::pkGen()); 

        CarrotEnoteV1 enote;
        rct::xmr_amount amount;
        crypto::secret_key amount_blinding_factor;
        get_output_proposal_internal_v1(proposal,
            keys.s_view_balance,
            tx_first_key_image,
            enote,
            amount,
            amount_blinding_factor);

        ASSERT_EQ(proposal.amount, amount);
        ASSERT_EQ(enote.amount_commitment, rct::commit(amount, rct::sk2rct(amount_blinding_factor)));

        crypto::secret_key recovered_sender_extension_g;
        crypto::secret_key recovered_sender_extension_t;
        crypto::public_key recovered_address_spend_pubkey;
        rct::xmr_amount recovered_amount;
        crypto::secret_key recovered_amount_blinding_factor;
        CarrotEnoteType recovered_enote_type;
        const bool scan_success = try_scan_carrot_enote_internal(enote,
            keys.s_view_balance_dev,
            recovered_sender_extension_g,
            recovered_sender_extension_t,
            recovered_address_spend_pubkey,
            recovered_amount,
            recovered_amount_blinding_factor,
            recovered_enote_type);

        ASSERT_TRUE(scan_success);

        // check recovered data
        EXPECT_EQ(proposal.destination_address_spend_pubkey, recovered_address_spend_pubkey);
        EXPECT_EQ(amount, recovered_amount);
        EXPECT_EQ(amount_blinding_factor, recovered_amount_blinding_factor);
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
            enote.onetime_address));
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

    crypto::x25519_pubkey s_sender_receiver_unctx;
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
