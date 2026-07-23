// Copyright (c) 2025-2026, The Monero Project
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
#include "core_types.h"
#include "ringct/rctOps.h"
#include "string_tools.h"

using namespace carrot;

namespace
{
bool operator==(const mx25519_pubkey &lhs, const mx25519_pubkey &rhs)
{
    return 0 == memcmp(lhs.data, rhs.data, sizeof(mx25519_pubkey));
}

template <typename T>
const T &pod_unwrap(const T &v) { return v; }
template <typename T>
T &pod_unwrap(T &v) { return v; }
const crypto::ec_scalar &pod_unwrap(const crypto::secret_key &v) { return v; }
crypto::ec_scalar &pod_unwrap(crypto::secret_key &v) { return v; }

template <typename T>
std::string pod_to_hex(const T &v)
{
    return epee::string_tools::pod_to_hex(pod_unwrap(v));
}

template <typename T>
T hex_to_pod(const boost::string_ref hex)
{
    T val{};
    if (!epee::string_tools::hex_to_pod(hex, pod_unwrap(val)))
        throw std::runtime_error("hex failed to decode to value");
    return val;
}

template <typename T>
struct hex_value_t
{
    hex_value_t(const boost::string_ref hex):
        hex(hex),
        value(hex_to_pod<T>(hex))
    {}

    const std::string hex;
    const T value;

    testing::AssertionResult matches(const T &actual) const
    {
        const std::string actual_hex = pod_to_hex(actual);

        const bool matches = this->value == actual;
        const bool matches_hex = this->hex == actual_hex;
        if (matches != matches_hex)
            throw std::logic_error("Type is not practically POD-like");

        if (!matches)
        {
            return testing::AssertionFailure() << "actual hex is " << actual_hex;
        }

        return testing::AssertionSuccess();
    }
};

static const hex_value_t<crypto::secret_key> s_master("6e02e67b303dc713276bb1a4d70b0083b78e4f50e34e209da9f0377cdc3d376e"); // chosen by fair dice roll.
                                                                                                                     // guaranteed to be random.
static const hex_value_t<crypto::secret_key> k_prove_spend("c9651fc906015afeefdb8d3bf7be621c36e035de2a85cb22dd4b869a22086f0e");
static const hex_value_t<crypto::public_key> partial_spend_pubkey("0ea83cfaa5a4b8e3fcfd1073e84459b8ecbcfa4affe6a6bf5688c113b9e9ecc3");
static const hex_value_t<crypto::secret_key> s_view_balance("59b2ee8646923309384704613418f5982b0167eb3cd87c6c067ee10700c3af91");
static const hex_value_t<crypto::secret_key> s_generate_image_preimage("0f3bf96a0642ab4cd10e8c64fba1cc535379ec18dbc7d304d50eb753197e266f");
static const hex_value_t<crypto::secret_key> k_generate_image("b5d11670cbbbf7a70aa3a6eb88723d49f645162eb3603f70854a80a8dbceb404");
static const hex_value_t<crypto::secret_key> k_view_incoming("12624c702b4c1a22fd710a836894ed0705955502e6498e5c6e3ad6f5920bb00f");
static const hex_value_t<crypto::secret_key> s_generate_address("039f0744fb138954072ee6bcbda4b5c085fd05e09b476a7b34ad20bf9ad440bc");
static const hex_value_t<crypto::public_key> account_spend_pubkey("905f67e69c39948e03dacbcfaeb2e766bfb407cdae53f1b11a813df99d9444e5");
static const hex_value_t<crypto::public_key> account_view_pubkey("34e4a36c249e3e0d22a4ee4d6a4da5ee89b12dc42223a12195af8dd727eb35fc");
static const std::uint32_t address_index_major = 5;
static const std::uint32_t address_index_minor = 16;
static const hex_value_t<crypto::secret_key> address_index_preimage_1("9c21bf89635102f5379f97b5d08074e6ed36084544262f92a93d7644945475f1");
static const hex_value_t<crypto::secret_key> address_index_preimage_2("97d954f490a552e9faa5db4866441c0973d0f42aec56c8c2d553f6b43b4e9580");
static const hex_value_t<crypto::secret_key> subaddress_scalar("fe701755513860e77ae77bdb4f3161714635e0d48cea3f4f330895e910670103");
static const hex_value_t<crypto::public_key> subaddress_spend_pubkey("97d227d0ff67e521b805d05d3a51390fd889181e334bf68156cb1e2d5aee6dd4");
static const hex_value_t<crypto::public_key> subaddress_view_pubkey("99a684cd429d88815cb1f90b794522b32812388a9f35120cfc08c7435f7bd51f");

static const hex_value_t<carrot::janus_anchor_t> anchor_norm("caee1381775487a0982557f0d2680b55");
static const hex_value_t<carrot::janus_anchor_t> anchor_special("dea14ab8268ba491238f1c554ab60d83");
static const hex_value_t<carrot::input_context_t> input_context("9423f74f3e869dc8427d8b35bb24c917480409c3f4750bff3c742f8e4d5af7bef7");
static const hex_value_t<carrot::payment_id_t> payment_id("4321734f56621440");
static const hex_value_t<crypto::secret_key> enote_ephemeral_privkey("6bd72042c79d9532a3b90b3689ee53c22725a11169ac2d251337bc4a69b2340d");
static const hex_value_t<mx25519_pubkey> enote_ephemeral_pubkey_cryptonote("65b42ef1ed3bd2ab3e6e86d17a52d832bcb6c820a8987306bedd9f6453693869");
static const hex_value_t<mx25519_pubkey> enote_ephemeral_pubkey_subaddress("d8e787047bb21d7dd348524741c78f311f549554b6dffd71c86ecc4a98a15720");
static const hex_value_t<mx25519_pubkey> s_sender_receiver("513ee79c0c8d76fdd95665a36d607b618e2f76a4806cfdba340fafe64b7f805f");
static const hex_value_t<crypto::hash> s_sender_receiver_ctx("6d4288869ce44ed5c38d4016b33083a1a0200daa2d8afc16625702d2108b62ae");
static const rct::xmr_amount amount = 67000000000000;
static const hex_value_t<crypto::secret_key> amount_blinding_factor_payment("2943f1f7cdabfcfef4803fe6a36df414065e912089ebcee5dbdad32a9685060a");
static const hex_value_t<crypto::secret_key> amount_blinding_factor_change("b9787d10298d13e34cf21513bce84dd1b13ae8aad6ab79794ac06a4c936e2b09");
static const hex_value_t<carrot::amount_commitment_t> amount_commitment("95f818f40a41665950d90db8f790cd8a135403624bac284d1059aeb29652fafe");
static const hex_value_t<crypto::public_key> onetime_address_coinbase("eb6d42a2cabe4e71a0d61172ed8b528b62c0ab398cde9373dab35cc774ba7c05");
static const hex_value_t<crypto::public_key> onetime_address("5e8f18d1dd3aba72d6ea2c7cfb217573ff4baa878300bbf97b4e32c5c566050d");
static const hex_value_t<carrot::view_tag_t> view_tag("98ad1e");
static const hex_value_t<carrot::encrypted_janus_anchor_t> anchor_encryption_mask("8d769d8417759007792f824e83115408");
static const hex_value_t<carrot::encrypted_amount_t> amount_encryption_mask("ee875c495435d1c3");
static const hex_value_t<carrot::encrypted_payment_id_t> payment_id_encryption_mask("736350182e1e0840");
} //anonymous namespace

//---------------------------------------------------------------------------------------------------------------------
// ACCOUNT
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_provespend_key)
{
    crypto::secret_key k_prove_spend_rc;
    make_carrot_provespend_key(s_master.value, k_prove_spend_rc);
    EXPECT_TRUE(k_prove_spend.matches(k_prove_spend_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_viewbalance_secret)
{
    crypto::secret_key s_view_balance_rc;
    make_carrot_viewbalance_secret(s_master.value, s_view_balance_rc);
    EXPECT_TRUE(s_view_balance.matches(s_view_balance_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_partial_spend_pubkey)
{
    crypto::public_key partial_spend_pubkey_rc;
    make_carrot_partial_spend_pubkey(k_prove_spend.value, partial_spend_pubkey_rc);
    EXPECT_TRUE(partial_spend_pubkey.matches(partial_spend_pubkey_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_generateimage_preimage)
{
    crypto::secret_key s_generate_image_preimage_rc;
    make_carrot_generateimage_preimage(s_view_balance.value, s_generate_image_preimage_rc);
    EXPECT_TRUE(s_generate_image_preimage.matches(s_generate_image_preimage_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_generateimage_key)
{
    crypto::secret_key k_generate_image_rc;
    make_carrot_generateimage_key(s_generate_image_preimage.value, partial_spend_pubkey.value, k_generate_image_rc);
    EXPECT_TRUE(k_generate_image.matches(k_generate_image_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_viewincoming_key)
{
    crypto::secret_key k_view_incoming_rc;
    make_carrot_viewincoming_key(s_view_balance.value, k_view_incoming_rc);
    EXPECT_TRUE(k_view_incoming.matches(k_view_incoming_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_generateaddress_secret)
{
    crypto::secret_key s_generate_address_rc;
    make_carrot_generateaddress_secret(s_view_balance.value, s_generate_address_rc);
    EXPECT_TRUE(s_generate_address.matches(s_generate_address_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_spend_pubkey)
{
    crypto::public_key account_spend_pubkey_rc;
    make_carrot_spend_pubkey(k_generate_image.value, k_prove_spend.value, account_spend_pubkey_rc);
    EXPECT_TRUE(account_spend_pubkey.matches(account_spend_pubkey_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_view_pubkey)
{
    const crypto::public_key account_view_pubkey_rc = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(account_spend_pubkey.value),
        rct::sk2rct(k_view_incoming.value)));
    EXPECT_TRUE(account_view_pubkey.matches(account_view_pubkey_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_address_index_preimage_1)
{
    crypto::secret_key address_index_preimage_1_rc;
    make_carrot_address_index_preimage_1(s_generate_address.value, address_index_major, address_index_minor,
        address_index_preimage_1_rc);
    EXPECT_TRUE(address_index_preimage_1.matches(address_index_preimage_1_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_address_index_preimage_2)
{
    crypto::secret_key address_index_preimage_2_rc;
    make_carrot_address_index_preimage_2(address_index_preimage_1.value, address_index_major, address_index_minor,
        account_spend_pubkey.value, account_view_pubkey.value, address_index_preimage_2_rc);
    EXPECT_TRUE(address_index_preimage_2.matches(address_index_preimage_2_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_subaddress_scalar)
{
    crypto::secret_key subaddress_scalar_rc;
    make_carrot_subaddress_scalar(address_index_preimage_2.value, account_spend_pubkey.value, subaddress_scalar_rc);
    EXPECT_TRUE(subaddress_scalar.matches(subaddress_scalar_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_subaddress_v1)
{
    CarrotDestinationV1 subaddress_rc;
    make_carrot_subaddress_v1(account_spend_pubkey.value,
        account_view_pubkey.value,
        generate_address_secret_ram_borrowed_device(s_generate_address.value),
        address_index_major,
        address_index_minor,
        subaddress_rc);
    EXPECT_TRUE(subaddress_spend_pubkey.matches(subaddress_rc.address_spend_pubkey));
    EXPECT_TRUE(subaddress_view_pubkey.matches(subaddress_rc.address_view_pubkey));
}
//---------------------------------------------------------------------------------------------------------------------
// ENOTE
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_enote_ephemeral_privkey)
{
    crypto::secret_key enote_ephemeral_privkey_rc;
    make_carrot_enote_ephemeral_privkey(anchor_norm.value,
        input_context.value,
        subaddress_spend_pubkey.value,
        payment_id.value,
        enote_ephemeral_privkey_rc);
    EXPECT_TRUE(enote_ephemeral_privkey.matches(enote_ephemeral_privkey_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_enote_ephemeral_pubkey_cryptonote)
{
    mx25519_pubkey enote_ephemeral_pubkey_rc;
    make_carrot_enote_ephemeral_pubkey_cryptonote(enote_ephemeral_privkey.value,
        enote_ephemeral_pubkey_rc);
    EXPECT_TRUE(enote_ephemeral_pubkey_cryptonote.matches(enote_ephemeral_pubkey_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_enote_ephemeral_pubkey_subaddress)
{
    mx25519_pubkey enote_ephemeral_pubkey_rc;
    const bool r = try_make_carrot_enote_ephemeral_pubkey_subaddress(enote_ephemeral_privkey.value,
        subaddress_spend_pubkey.value,
        enote_ephemeral_pubkey_rc);
    ASSERT_TRUE(r);
    EXPECT_TRUE(enote_ephemeral_pubkey_subaddress.matches(enote_ephemeral_pubkey_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, try_make_carrot_shared_key_receiver)
{
    mx25519_pubkey s_sender_receiver_rc;
    try_make_carrot_shared_key_receiver(k_view_incoming.value,
        enote_ephemeral_pubkey_subaddress.value,
        s_sender_receiver_rc);
    EXPECT_TRUE(s_sender_receiver.matches(s_sender_receiver_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, try_make_carrot_shared_key_sender)
{
    mx25519_pubkey s_sender_receiver_rc;
    try_make_carrot_shared_key_sender(enote_ephemeral_privkey.value,
        subaddress_view_pubkey.value,
        s_sender_receiver_rc);
    EXPECT_TRUE(s_sender_receiver.matches(s_sender_receiver_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_contextualized_sender_receiver_secret)
{
    crypto::hash s_sender_receiver_ctx_rc;
    make_carrot_contextualized_sender_receiver_secret(s_sender_receiver.value.data,
        enote_ephemeral_pubkey_subaddress.value,
        input_context.value,
        s_sender_receiver_ctx_rc);
    EXPECT_TRUE(s_sender_receiver_ctx.matches(s_sender_receiver_ctx_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_amount_blinding_factor_payment)
{
    crypto::secret_key amount_blinding_factor_rc;
    make_carrot_amount_blinding_factor(s_sender_receiver_ctx.value,
        amount,
        subaddress_spend_pubkey.value,
        CarrotEnoteType::PAYMENT,
        amount_blinding_factor_rc);
    EXPECT_TRUE(amount_blinding_factor_payment.matches(amount_blinding_factor_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_amount_blinding_factor_change)
{
    crypto::secret_key amount_blinding_factor_rc;
    make_carrot_amount_blinding_factor(s_sender_receiver_ctx.value,
        amount,
        subaddress_spend_pubkey.value,
        CarrotEnoteType::CHANGE,
        amount_blinding_factor_rc);
    EXPECT_TRUE(amount_blinding_factor_change.matches(amount_blinding_factor_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, commit)
{
    const amount_commitment_t amount_commitment_rc = commit_carrot_amount(amount,
        amount_blinding_factor_payment.value);
    EXPECT_TRUE(amount_commitment.matches(amount_commitment_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, try_make_carrot_onetime_address_coinbase)
{
    crypto::public_key onetime_address_coinbase_rc;
    ASSERT_TRUE(try_make_carrot_onetime_address_coinbase(
        subaddress_spend_pubkey.value,
        s_sender_receiver_ctx.value,
        amount,
        onetime_address_coinbase_rc));
    EXPECT_TRUE(onetime_address_coinbase.matches(onetime_address_coinbase_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, try_make_carrot_onetime_address)
{
    crypto::public_key onetime_address_rc;
    ASSERT_TRUE(try_make_carrot_onetime_address(
        subaddress_spend_pubkey.value,
        s_sender_receiver_ctx.value,
        amount_commitment.value,
        onetime_address_rc));
    EXPECT_TRUE(onetime_address.matches(onetime_address_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_view_tag)
{
    view_tag_t view_tag_rc;
    make_carrot_view_tag(s_sender_receiver.value.data,
        input_context.value,
        onetime_address.value,
        view_tag_rc);
    EXPECT_TRUE(view_tag.matches(view_tag_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_anchor_encryption_mask)
{
    encrypted_janus_anchor_t anchor_encryption_mask_rc;
    make_carrot_anchor_encryption_mask(s_sender_receiver_ctx.value,
        onetime_address.value,
        anchor_encryption_mask_rc);
    EXPECT_TRUE(anchor_encryption_mask.matches(anchor_encryption_mask_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_amount_encryption_mask)
{
    encrypted_amount_t amount_encryption_mask_rc;
    make_carrot_amount_encryption_mask(s_sender_receiver_ctx.value,
        onetime_address.value,
        amount_encryption_mask_rc);
    EXPECT_TRUE(amount_encryption_mask.matches(amount_encryption_mask_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_payment_id_encryption_mask)
{
    encrypted_payment_id_t payment_id_encryption_mask_rc;
    make_carrot_payment_id_encryption_mask(s_sender_receiver_ctx.value,
        onetime_address.value,
        payment_id_encryption_mask_rc);
    EXPECT_TRUE(payment_id_encryption_mask.matches(payment_id_encryption_mask_rc));
}
//---------------------------------------------------------------------------------------------------------------------
TEST(carrot_convergence, make_carrot_janus_anchor_special)
{
    janus_anchor_t anchor_special_rc;
    make_carrot_janus_anchor_special(enote_ephemeral_pubkey_cryptonote.value,
        input_context.value,
        onetime_address.value,
        k_view_incoming.value,
        anchor_special_rc);
    EXPECT_TRUE(anchor_special.matches(anchor_special_rc));
}
//---------------------------------------------------------------------------------------------------------------------
