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

#include "carrot_core/account_secrets.h"
#include "carrot_core/address_utils.h"
#include "carrot_core/destination.h"
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "ringct/rctOps.h"
#include "string_tools.h"

using namespace carrot;
using epee::string_tools::hex_to_pod;
using epee::string_tools::pod_to_hex;

//---------------------------------------------------------------------------------------------------------------------
// ACCOUNT
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_provespend_key)
{
    crypto::secret_key s_master;
    ASSERT_TRUE(hex_to_pod("6e02e67b303dc713276bb1a4d70b0083b78e4f50e34e209da9f0377cdc3d376e",
        unwrap(unwrap(s_master))));

    crypto::secret_key k_prove_spend;
    make_carrot_provespend_key(s_master, k_prove_spend);
    ASSERT_EQ("f10bf01839ea216e5d70b7c9ceaa8b8e9a432b5e98e6e48a8043ffb3fa229f0b",
        pod_to_hex(unwrap(unwrap(k_prove_spend))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_viewbalance_secret)
{
    crypto::secret_key s_master;
    ASSERT_TRUE(hex_to_pod("6e02e67b303dc713276bb1a4d70b0083b78e4f50e34e209da9f0377cdc3d376e",
        unwrap(unwrap(s_master))));

    crypto::secret_key s_view_balance;
    make_carrot_viewbalance_secret(s_master, s_view_balance);
    ASSERT_EQ("154c5e01902b20acc8436c9aa06b40355d78dfda0fc6af3d53a2220f1363a0f5",
        pod_to_hex(unwrap(unwrap(s_view_balance))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_generateimage_key)
{
    crypto::secret_key s_view_balance;
    ASSERT_TRUE(hex_to_pod("154c5e01902b20acc8436c9aa06b40355d78dfda0fc6af3d53a2220f1363a0f5",
        unwrap(unwrap(s_view_balance))));

    crypto::secret_key k_generate_image;
    make_carrot_generateimage_key(s_view_balance, k_generate_image);
    ASSERT_EQ("336e3af233b3aa5bc95d5589aba67aab727727419899823acc6a6c4479e4ea04",
        pod_to_hex(unwrap(unwrap(k_generate_image))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_viewincoming_key)
{
    crypto::secret_key s_view_balance;
    ASSERT_TRUE(hex_to_pod("154c5e01902b20acc8436c9aa06b40355d78dfda0fc6af3d53a2220f1363a0f5",
        unwrap(unwrap(s_view_balance))));

    crypto::secret_key k_view_incoming;
    make_carrot_viewincoming_key(s_view_balance, k_view_incoming);
    ASSERT_EQ("60eff3ec120a12bb44d4258816e015952fc5651040da8c8af58c17676485f200",
        pod_to_hex(unwrap(unwrap(k_view_incoming))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_generateaddress_secret)
{
    crypto::secret_key s_view_balance;
    ASSERT_TRUE(hex_to_pod("154c5e01902b20acc8436c9aa06b40355d78dfda0fc6af3d53a2220f1363a0f5",
        unwrap(unwrap(s_view_balance))));

    crypto::secret_key s_generate_address;
    make_carrot_generateaddress_secret(s_view_balance, s_generate_address);
    ASSERT_EQ("593ece76c5d24cbfe3c7ac9e2d455cdd4b372c89584700bf1c2e7bef2b70a4d1",
        pod_to_hex(unwrap(unwrap(s_generate_address))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_spend_pubkey)
{
    crypto::secret_key k_generate_image;
    ASSERT_TRUE(hex_to_pod("336e3af233b3aa5bc95d5589aba67aab727727419899823acc6a6c4479e4ea04",
        unwrap(unwrap(k_generate_image))));

    crypto::secret_key k_prove_spend;
    ASSERT_TRUE(hex_to_pod("f10bf01839ea216e5d70b7c9ceaa8b8e9a432b5e98e6e48a8043ffb3fa229f0b",
        unwrap(unwrap(k_prove_spend))));

    crypto::public_key account_spend_pubkey;
    make_carrot_spend_pubkey(k_generate_image, k_prove_spend, account_spend_pubkey);
    ASSERT_EQ("c984806ae9be958800cfe04b5ed85279f48d78c3792b5abb2f5ce2b67adc491f",
        pod_to_hex(account_spend_pubkey));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_index_extension_generator)
{
    crypto::secret_key s_generate_address;
    ASSERT_TRUE(hex_to_pod("593ece76c5d24cbfe3c7ac9e2d455cdd4b372c89584700bf1c2e7bef2b70a4d1",
        unwrap(unwrap(s_generate_address))));

    const std::uint32_t major_index = 5;
    const std::uint32_t minor_index = 16;

    crypto::secret_key address_index_generator;
    make_carrot_index_extension_generator(s_generate_address, major_index, minor_index, address_index_generator);
    ASSERT_EQ("79ad2383f44b4d26413adb7ae79c5658b2a8c20b6f5046bfa9f229bfcf1744a7",
        pod_to_hex(unwrap(unwrap(address_index_generator))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_subaddress_scalar)
{
    crypto::public_key account_spend_pubkey;
    ASSERT_TRUE(hex_to_pod("c984806ae9be958800cfe04b5ed85279f48d78c3792b5abb2f5ce2b67adc491f",
        account_spend_pubkey));

    crypto::secret_key k_view_incoming;
    ASSERT_TRUE(hex_to_pod("60eff3ec120a12bb44d4258816e015952fc5651040da8c8af58c17676485f200",
        unwrap(unwrap(k_view_incoming))));

    const crypto::public_key account_view_pubkey = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(account_spend_pubkey),
        rct::sk2rct(k_view_incoming)));
    ASSERT_EQ("a30c1b720a66557c03a9784c6dd0902c95ee56670e04907d18eaa20608a72e7e", pod_to_hex(account_view_pubkey));

    crypto::secret_key address_index_generator;
    ASSERT_TRUE(hex_to_pod("79ad2383f44b4d26413adb7ae79c5658b2a8c20b6f5046bfa9f229bfcf1744a7",
        unwrap(unwrap(address_index_generator))));

    const std::uint32_t major_index = 5;
    const std::uint32_t minor_index = 16;

    crypto::secret_key subaddress_scalar;
    make_carrot_subaddress_scalar(account_spend_pubkey,
        account_view_pubkey, address_index_generator, major_index, minor_index, subaddress_scalar);
    ASSERT_EQ("824e9710a9ee164dcf225be9ced906ceb53a0e93326b199a79340f6c0c7e050d",
        pod_to_hex(unwrap(unwrap(subaddress_scalar))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_subaddress_v1)
{
    crypto::public_key account_spend_pubkey;
    ASSERT_TRUE(hex_to_pod("c984806ae9be958800cfe04b5ed85279f48d78c3792b5abb2f5ce2b67adc491f",
        account_spend_pubkey));

    crypto::secret_key k_view_incoming;
    ASSERT_TRUE(hex_to_pod("60eff3ec120a12bb44d4258816e015952fc5651040da8c8af58c17676485f200",
        unwrap(unwrap(k_view_incoming))));

    crypto::secret_key s_generate_address;
    ASSERT_TRUE(hex_to_pod("593ece76c5d24cbfe3c7ac9e2d455cdd4b372c89584700bf1c2e7bef2b70a4d1",
        unwrap(unwrap(s_generate_address))));

    const crypto::public_key account_view_pubkey = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(account_spend_pubkey),
        rct::sk2rct(k_view_incoming)));

    const std::uint32_t major_index = 5;
    const std::uint32_t minor_index = 16;

    CarrotDestinationV1 subaddress;
    make_carrot_subaddress_v1(account_spend_pubkey,
        account_view_pubkey,
        generate_address_secret_ram_borrowed_device(s_generate_address),
        major_index,
        minor_index,
        subaddress);
    ASSERT_EQ("cb84becce21364e6fc91f6cec459ae917287bc3d87791369f8ff0fc40e4fcc08",
        pod_to_hex(subaddress.address_spend_pubkey));
    ASSERT_EQ("82800b2b97f50a798768d3235eabe9d4b3d5bd6d12956975b79db53f29895bdd",
        pod_to_hex(subaddress.address_view_pubkey));
}
//---------------------------------------------------------------------------------------------------------------------
// ENOTE
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_enote_ephemeral_privkey)
{
    janus_anchor_t anchor_norm;
    ASSERT_TRUE(hex_to_pod("caee1381775487a0982557f0d2680b55",
        anchor_norm));
    
    input_context_t input_context;
    ASSERT_TRUE(hex_to_pod("9423f74f3e869dc8427d8b35bb24c917480409c3f4750bff3c742f8e4d5af7bef7",
        input_context));

    crypto::public_key address_spend_pubkey;
    ASSERT_TRUE(hex_to_pod("1ebcddd5d98e26788ed8d8510de7f520e973902238e107a070aad104e166b6a0",
        address_spend_pubkey));

    payment_id_t payment_id;
    ASSERT_TRUE(hex_to_pod("4321734f56621440",
        payment_id));

    crypto::secret_key enote_ephemeral_privkey;
    make_carrot_enote_ephemeral_privkey(anchor_norm,
        input_context,
        address_spend_pubkey,
        payment_id,
        enote_ephemeral_privkey);
    ASSERT_EQ("6d4645a0e398ff430f68eaa78240dd2c04051e9a50438cd9c9c3c0e12af68b0b",
        pod_to_hex(unwrap(unwrap(enote_ephemeral_privkey))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_enote_ephemeral_pubkey_cryptonote)
{
    crypto::secret_key enote_ephemeral_privkey;
    ASSERT_TRUE(hex_to_pod("f57ff2d7c898b755137b69e8d826801945ed72e9951850de908e9d645a0bb00d",
        unwrap(unwrap(enote_ephemeral_privkey))));

    mx25519_pubkey enote_ephemeral_pubkey;
    make_carrot_enote_ephemeral_pubkey_cryptonote(enote_ephemeral_privkey,
        enote_ephemeral_pubkey);
    ASSERT_EQ("2987777565c02409dfe871cc27b2334f5ade9d4ad014012c568367b80e99c666",
        pod_to_hex(enote_ephemeral_pubkey));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_enote_ephemeral_pubkey_subaddress)
{
    crypto::secret_key enote_ephemeral_privkey;
    ASSERT_TRUE(hex_to_pod("f57ff2d7c898b755137b69e8d826801945ed72e9951850de908e9d645a0bb00d",
        unwrap(unwrap(enote_ephemeral_privkey))));

    crypto::public_key address_spend_pubkey;
    ASSERT_TRUE(hex_to_pod("1ebcddd5d98e26788ed8d8510de7f520e973902238e107a070aad104e166b6a0",
        address_spend_pubkey));

    mx25519_pubkey enote_ephemeral_pubkey;
    make_carrot_enote_ephemeral_pubkey_subaddress(enote_ephemeral_privkey,
        address_spend_pubkey,
        enote_ephemeral_pubkey);
    ASSERT_EQ("d8b8ce01943edd05d7db66aeb15109c58ec270796f0c76c03d58a398926aca55",
        pod_to_hex(enote_ephemeral_pubkey));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_uncontextualized_shared_key_receiver)
{
    crypto::secret_key k_view_incoming;
    ASSERT_TRUE(hex_to_pod("60eff3ec120a12bb44d4258816e015952fc5651040da8c8af58c17676485f200",
        unwrap(unwrap(k_view_incoming))));

    mx25519_pubkey enote_ephemeral_pubkey;
    ASSERT_TRUE(hex_to_pod("d8b8ce01943edd05d7db66aeb15109c58ec270796f0c76c03d58a398926aca55",
        enote_ephemeral_pubkey));

    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_receiver(k_view_incoming,
        enote_ephemeral_pubkey,
        s_sender_receiver_unctx);
    ASSERT_EQ("baa47cfc380374b15cb5a3048099968962a66e287d78654c75b550d711e58451",
        pod_to_hex(s_sender_receiver_unctx));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_uncontextualized_shared_key_sender)
{
    crypto::secret_key enote_ephemeral_privkey;
    ASSERT_TRUE(hex_to_pod("f57ff2d7c898b755137b69e8d826801945ed72e9951850de908e9d645a0bb00d",
        unwrap(unwrap(enote_ephemeral_privkey))));

    crypto::public_key address_view_pubkey;
    ASSERT_TRUE(hex_to_pod("75b7bc7759da5d9ad5ff421650949b27a13ea369685eb4d1bd59abc518e25fe2",
        address_view_pubkey));

    mx25519_pubkey s_sender_receiver_unctx;
    make_carrot_uncontextualized_shared_key_sender(enote_ephemeral_privkey,
        address_view_pubkey,
        s_sender_receiver_unctx);
    ASSERT_EQ("baa47cfc380374b15cb5a3048099968962a66e287d78654c75b550d711e58451",
        pod_to_hex(s_sender_receiver_unctx));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_sender_receiver_secret)
{
    mx25519_pubkey s_sender_receiver_unctx;
    ASSERT_TRUE(hex_to_pod("baa47cfc380374b15cb5a3048099968962a66e287d78654c75b550d711e58451",
        s_sender_receiver_unctx));
    
    mx25519_pubkey enote_ephemeral_pubkey;
    ASSERT_TRUE(hex_to_pod("d8b8ce01943edd05d7db66aeb15109c58ec270796f0c76c03d58a398926aca55",
        enote_ephemeral_pubkey));

    input_context_t input_context;
    ASSERT_TRUE(hex_to_pod("9423f74f3e869dc8427d8b35bb24c917480409c3f4750bff3c742f8e4d5af7bef7",
        input_context));

    crypto::hash s_sender_receiver;
    make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
        enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver);
    ASSERT_EQ("232e62041ee1262cb3fce0d10fdbd018cca5b941ff92283676d6112aa426f76c",
        pod_to_hex(s_sender_receiver));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_amount_blinding_factor_payment)
{
    crypto::hash s_sender_receiver;
    ASSERT_TRUE(hex_to_pod("232e62041ee1262cb3fce0d10fdbd018cca5b941ff92283676d6112aa426f76c",
        s_sender_receiver));

    const rct::xmr_amount amount = 23000000000000;

    crypto::public_key address_spend_pubkey;
    ASSERT_TRUE(hex_to_pod("1ebcddd5d98e26788ed8d8510de7f520e973902238e107a070aad104e166b6a0",
        address_spend_pubkey));

    const CarrotEnoteType enote_type = CarrotEnoteType::PAYMENT;

    crypto::secret_key amount_blinding_factor;
    make_carrot_amount_blinding_factor(s_sender_receiver,
        amount,
        address_spend_pubkey,
        enote_type,
        amount_blinding_factor);
    ASSERT_EQ("9fc3581e926a844877479d829ff9deeae17ce77feaf2c3c972923510e04f1f02",
        pod_to_hex(unwrap(unwrap(amount_blinding_factor))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_amount_blinding_factor_change)
{
    crypto::hash s_sender_receiver;
    ASSERT_TRUE(hex_to_pod("232e62041ee1262cb3fce0d10fdbd018cca5b941ff92283676d6112aa426f76c",
        s_sender_receiver));

    const rct::xmr_amount amount = 23000000000000;

    crypto::public_key address_spend_pubkey;
    ASSERT_TRUE(hex_to_pod("1ebcddd5d98e26788ed8d8510de7f520e973902238e107a070aad104e166b6a0",
        address_spend_pubkey));

    const CarrotEnoteType enote_type = CarrotEnoteType::CHANGE;

    crypto::secret_key amount_blinding_factor;
    make_carrot_amount_blinding_factor(s_sender_receiver,
        amount,
        address_spend_pubkey,
        enote_type,
        amount_blinding_factor);
    ASSERT_EQ("dda34eac46030e4084f5a2c808d0a82ffaa82cbf01d4a74d7ee0d4fe72c31a0f",
        pod_to_hex(unwrap(unwrap(amount_blinding_factor))));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, commit)
{
    const rct::xmr_amount amount = 23000000000000;

    crypto::secret_key amount_blinding_factor;
    ASSERT_TRUE(hex_to_pod("9fc3581e926a844877479d829ff9deeae17ce77feaf2c3c972923510e04f1f02",
        unwrap(unwrap(amount_blinding_factor))));

    const rct::key amount_commitment = rct::commit(amount, rct::sk2rct(amount_blinding_factor));
    ASSERT_EQ("ca5f0fc2fe7a4fe628e6f08b2c0eb44f3af3b87e1619b2ed2de296f7e425512b",
        pod_to_hex(amount_commitment));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_onetime_address)
{
    crypto::public_key address_spend_pubkey;
    ASSERT_TRUE(hex_to_pod("1ebcddd5d98e26788ed8d8510de7f520e973902238e107a070aad104e166b6a0",
        address_spend_pubkey));

    crypto::hash s_sender_receiver;
    ASSERT_TRUE(hex_to_pod("232e62041ee1262cb3fce0d10fdbd018cca5b941ff92283676d6112aa426f76c",
        s_sender_receiver));

    rct::key amount_commitment;
    ASSERT_TRUE(hex_to_pod("ca5f0fc2fe7a4fe628e6f08b2c0eb44f3af3b87e1619b2ed2de296f7e425512b",
        amount_commitment));

    crypto::public_key onetime_address;
    make_carrot_onetime_address(address_spend_pubkey,
        s_sender_receiver,
        amount_commitment,
        onetime_address);
    ASSERT_EQ("4c93cf2d7ff8556eac73025ab3019a0db220b56bdf0387e0524724cc0e409d92",
        pod_to_hex(onetime_address));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_view_tag)
{
    mx25519_pubkey s_sender_receiver_unctx;
    ASSERT_TRUE(hex_to_pod("baa47cfc380374b15cb5a3048099968962a66e287d78654c75b550d711e58451",
        s_sender_receiver_unctx));

    input_context_t input_context;
    ASSERT_TRUE(hex_to_pod("9423f74f3e869dc8427d8b35bb24c917480409c3f4750bff3c742f8e4d5af7bef7",
        input_context));

    crypto::public_key onetime_address;
    ASSERT_TRUE(hex_to_pod("4c93cf2d7ff8556eac73025ab3019a0db220b56bdf0387e0524724cc0e409d92",
        onetime_address));

    view_tag_t view_tag;
    make_carrot_view_tag(s_sender_receiver_unctx.data,
        input_context,
        onetime_address,
        view_tag);
    ASSERT_EQ("0176f6",
        pod_to_hex(view_tag));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_anchor_encryption_mask)
{
    crypto::hash s_sender_receiver;
    ASSERT_TRUE(hex_to_pod("232e62041ee1262cb3fce0d10fdbd018cca5b941ff92283676d6112aa426f76c",
        s_sender_receiver));

    crypto::public_key onetime_address;
    ASSERT_TRUE(hex_to_pod("4c93cf2d7ff8556eac73025ab3019a0db220b56bdf0387e0524724cc0e409d92",
        onetime_address));

    encrypted_janus_anchor_t anchor_encryption_mask;
    make_carrot_anchor_encryption_mask(s_sender_receiver,
        onetime_address,
        anchor_encryption_mask);
    ASSERT_EQ("52d95a8e441f26a056f55094938cbfa8",
        pod_to_hex(anchor_encryption_mask));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_amount_encryption_mask)
{
    crypto::hash s_sender_receiver;
    ASSERT_TRUE(hex_to_pod("232e62041ee1262cb3fce0d10fdbd018cca5b941ff92283676d6112aa426f76c",
        s_sender_receiver));

    crypto::public_key onetime_address;
    ASSERT_TRUE(hex_to_pod("4c93cf2d7ff8556eac73025ab3019a0db220b56bdf0387e0524724cc0e409d92",
        onetime_address));

    encrypted_amount_t amount_encryption_mask;
    make_carrot_amount_encryption_mask(s_sender_receiver,
        onetime_address,
        amount_encryption_mask);
    ASSERT_EQ("98d25d1db65b6a3e",
        pod_to_hex(amount_encryption_mask));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_payment_id_encryption_mask)
{
    crypto::hash s_sender_receiver;
    ASSERT_TRUE(hex_to_pod("232e62041ee1262cb3fce0d10fdbd018cca5b941ff92283676d6112aa426f76c",
        s_sender_receiver));

    crypto::public_key onetime_address;
    ASSERT_TRUE(hex_to_pod("4c93cf2d7ff8556eac73025ab3019a0db220b56bdf0387e0524724cc0e409d92",
        onetime_address));

    encrypted_payment_id_t payment_id_encryption_mask;
    make_carrot_payment_id_encryption_mask(s_sender_receiver,
        onetime_address,
        payment_id_encryption_mask);
    ASSERT_EQ("b57a1560e82e2483",
        pod_to_hex(payment_id_encryption_mask));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_janus_anchor_special)
{
    mx25519_pubkey enote_ephemeral_pubkey;
    ASSERT_TRUE(hex_to_pod("d8b8ce01943edd05d7db66aeb15109c58ec270796f0c76c03d58a398926aca55",
        enote_ephemeral_pubkey));

    input_context_t input_context;
    ASSERT_TRUE(hex_to_pod("9423f74f3e869dc8427d8b35bb24c917480409c3f4750bff3c742f8e4d5af7bef7",
        input_context));

    crypto::public_key onetime_address;
    ASSERT_TRUE(hex_to_pod("4c93cf2d7ff8556eac73025ab3019a0db220b56bdf0387e0524724cc0e409d92",
        onetime_address));

    crypto::secret_key k_view_incoming;
    ASSERT_TRUE(hex_to_pod("60eff3ec120a12bb44d4258816e015952fc5651040da8c8af58c17676485f200",
        unwrap(unwrap(k_view_incoming))));

    janus_anchor_t anchor_special;
    make_carrot_janus_anchor_special(enote_ephemeral_pubkey,
        input_context,
        onetime_address,
        k_view_incoming,
        anchor_special);
    ASSERT_EQ("31afa8f580feaf736cd424ecc9ae5fd2",
        pod_to_hex(anchor_special));
}
//---------------------------------------------------------------------------------------------------------------------
