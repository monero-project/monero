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

//paired header
#include "carrot_tx_builder_inputs.h"

//local headers
#include "carrot_core/account_secrets.h"
#include "carrot_core/address_utils.h"
#include "carrot_core/config.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/scan.h"
#include "crypto/generators.h"
#include "fcmp_pp/prove.h"
#include "misc_log_ex.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers
#include <algorithm>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
rct::key load_key(const std::uint8_t bytes[32])
{
    rct::key k;
    memcpy(k.bytes, bytes, sizeof(k));
    return k;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
void store_key(std::uint8_t bytes[32], const rct::key &k)
{
    memcpy(bytes, k.bytes, 32);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static FcmpInputCompressed calculate_fcmp_input_for_rerandomizations(const crypto::public_key &onetime_address,
    const rct::key &amount_commitment,
    const rct::key &r_o,
    const rct::key &r_i,
    const rct::key &r_r_i,
    const rct::key &r_c)
{
    return fcmp_pp::calculate_fcmp_input_for_rerandomizations(onetime_address,
        rct::rct2pt(amount_commitment),
        rct::rct2sk(r_o),
        rct::rct2sk(r_i),
        rct::rct2sk(r_r_i),
        rct::rct2sk(r_c));
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void make_sal_proof_nominal_address_naive(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const crypto::secret_key &address_privkey_g,
    const crypto::secret_key &address_privkey_t,
    const crypto::secret_key &sender_extension_g,
    const crypto::secret_key &sender_extension_t,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    // O = x G + y T

    // x = k^{j,g}_addr + k^g_o
    crypto::secret_key x;
    sc_add(to_bytes(x),
        to_bytes(address_privkey_g),
        to_bytes(sender_extension_g));

    // y = k^{j,t}_addr + k^t_o
    crypto::secret_key y;
    sc_add(to_bytes(y),
        to_bytes(address_privkey_t),
        to_bytes(sender_extension_t));

    std::tie(sal_proof_out, key_image_out) = fcmp_pp::prove_sal(signable_tx_hash,
        x,
        y,
        rerandomized_output);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void make_sal_proof_nominal_address_carrot_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &address_privkey_g,
    const crypto::secret_key &address_privkey_t,
    const crypto::public_key &account_spend_pubkey,
    const view_balance_secret_device *s_view_balance_dev,
    const view_incoming_key_device *k_view_incoming_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    CHECK_AND_ASSERT_THROW_MES(verify_rerandomized_output_basic(rerandomized_output,
            opening_hint.source_enote.onetime_address,
            opening_hint.source_enote.amount_commitment),
        "make sal proof nominal address carrot v1: rerandomized output does not verify");

    // We scan scan here as a defensive programming measure against naive-scanner burning bugs,
    // malicious-scanner burning bugs, and malicious-scanner subaddress swaps. However, if you want
    // a user to confirm other details about the enote they're spending (e.g. amount, payment ID,
    // subaddress index, internal message, enote type, TXID), you're going to have to pre-scan this
    // enote and implement the checks yourself before calling this function. Hardware wallet
    // developers: if you want your users to keep their hard-earned funds, don't skip cold-side
    // enote scanning in Carrot enotes! Legacy enotes aren't SAFU from malicious-scanner burning
    // anyways since K_o doesn't bind to C_a.

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;
    crypto::public_key address_spend_pubkey;
    rct::xmr_amount amount;
    crypto::secret_key amount_blinding_factor;
    payment_id_t payment_id;
    CarrotEnoteType enote_type;
    janus_anchor_t internal_message;

    // first, try do an internal scan of the enote
    bool scanned = false;
    if (s_view_balance_dev)
    {
        scanned = try_scan_carrot_enote_internal_receiver(opening_hint.source_enote,
            *s_view_balance_dev,
            sender_extension_g,
            sender_extension_t,
            address_spend_pubkey,
            amount,
            amount_blinding_factor,
            enote_type,
            internal_message);
        payment_id = null_payment_id;
    }
    else
    {
        internal_message = janus_anchor_t{};
    }

    // if that didn't work, try an external scan
    if (!scanned && k_view_incoming_dev)
    {
        mx25519_pubkey s_sender_receiver_unctx;
        CHECK_AND_ASSERT_THROW_MES(make_carrot_uncontextualized_shared_key_receiver(*k_view_incoming_dev,
                opening_hint.source_enote.enote_ephemeral_pubkey,
                s_sender_receiver_unctx),
            "make sal proof nominal address carrot v1: failed to do ECDH");

        scanned = try_scan_carrot_enote_external_receiver(opening_hint.source_enote,
            opening_hint.encrypted_payment_id,
            s_sender_receiver_unctx,
            {&account_spend_pubkey, 1},
            *k_view_incoming_dev,
            sender_extension_g,
            sender_extension_t,
            address_spend_pubkey,
            amount,
            amount_blinding_factor,
            payment_id,
            enote_type);
    }

    CHECK_AND_ASSERT_THROW_MES(scanned,
        "make sal proof nominal address carrot v1: cannot spend enote because of a scan failure");

    make_sal_proof_nominal_address_naive(signable_tx_hash,
        rerandomized_output,
        address_privkey_g,
        address_privkey_t,
        sender_extension_g,
        sender_extension_t,
        sal_proof_out,
        key_image_out);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void make_sal_proof_nominal_address_carrot_coinbase_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotCoinbaseOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &address_privkey_g,
    const crypto::secret_key &address_privkey_t,
    const crypto::public_key &account_spend_pubkey,
    const view_incoming_key_device &k_view_incoming_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    const rct::key coinbase_amount_commitment = rct::zeroCommitVartime(opening_hint.source_enote.amount);

    CHECK_AND_ASSERT_THROW_MES(verify_rerandomized_output_basic(rerandomized_output,
            opening_hint.source_enote.onetime_address,
            coinbase_amount_commitment),
        "make sal proof nominal address carrot coinbase v1: rerandomized output does not verify");

    // We scan scan here as a defensive programming measure against naive-scanner burning bugs and
    // malicious-scanner burning bugs. However, if you want a user to confirm other details about
    // the coinbase enote they're spending (e.g. amount, block index), you're going to have to
    // pre-scan this enote and implement the checks yourself before calling this function. Hardware
    // wallet developers: if you want your users to keep their hard-earned funds, don't skip
    // cold-side enote scanning in Carrot enotes! Legacy enotes aren't SAFU from malicious-scanner
    // burning anyways since K_o doesn't bind to C_a.

    crypto::secret_key sender_extension_g;
    crypto::secret_key sender_extension_t;

    // first, try do a scan of the enote
    mx25519_pubkey s_sender_receiver_unctx;
    CHECK_AND_ASSERT_THROW_MES(make_carrot_uncontextualized_shared_key_receiver(k_view_incoming_dev,
            opening_hint.source_enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx),
        "make sal proof nominal address carrot coinbase v1: failed to do ECDH");

    const bool scanned = try_scan_carrot_coinbase_enote_receiver(opening_hint.source_enote,
        s_sender_receiver_unctx,
        account_spend_pubkey,
        sender_extension_g,
        sender_extension_t);

    CHECK_AND_ASSERT_THROW_MES(scanned,
        "make sal proof nominal address carrot coinbase v1: cannot spend enote because of a scan failure");

    make_sal_proof_nominal_address_naive(signable_tx_hash,
        rerandomized_output,
        address_privkey_g,
        address_privkey_t,
        sender_extension_g,
        sender_extension_t,
        sal_proof_out,
        key_image_out);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
const crypto::public_key &onetime_address_ref(const OutputOpeningHintVariant &opening_hint)
{
    struct onetime_address_ref_visitor: public tools::variant_static_visitor<const crypto::public_key &>
    {
        const crypto::public_key &operator()(const LegacyOutputOpeningHintV1 &h) const
        { return h.onetime_address; }
        const crypto::public_key &operator()(const CarrotOutputOpeningHintV1 &h) const 
        { return h.source_enote.onetime_address; }
        const crypto::public_key &operator()(const CarrotCoinbaseOutputOpeningHintV1 &h) const
        { return h.source_enote.onetime_address; }
    };

    return opening_hint.visit(onetime_address_ref_visitor{});
}
//-------------------------------------------------------------------------------------------------------------------
rct::key amount_commitment_ref(const OutputOpeningHintVariant &opening_hint)
{
    struct amount_commitment_ref_visitor: public tools::variant_static_visitor<rct::key>
    {
        rct::key operator()(const LegacyOutputOpeningHintV1 &h) const
        { return rct::commit(h.amount, rct::sk2rct(h.amount_blinding_factor)); }
        rct::key operator()(const CarrotOutputOpeningHintV1 &h) const 
        { return h.source_enote.amount_commitment; }
        rct::key operator()(const CarrotCoinbaseOutputOpeningHintV1 &h) const
        { return rct::zeroCommitVartime(h.source_enote.amount); }
    };

    return opening_hint.visit(amount_commitment_ref_visitor{});
}
//-------------------------------------------------------------------------------------------------------------------
void make_carrot_rerandomized_outputs_nonrefundable(const std::vector<crypto::public_key> &input_onetime_addresses,
    const std::vector<rct::key> &input_amount_commitments,
    const std::vector<rct::key> &input_amount_blinding_factors,
    const std::vector<rct::key> &output_amount_blinding_factors,
    std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs_out)
{
    // collect input_amount_commitments as crypto::ec_point
    std::vector<crypto::ec_point> input_amount_commitments_pt;
    input_amount_commitments_pt.reserve(input_amount_commitments.size());
    for (const rct::key &input_amount_commitment : input_amount_commitments)
        input_amount_commitments_pt.push_back(rct::rct2pt(input_amount_commitment));

    // collect input_amount_blinding_factors as crypto::secret_key
    std::vector<crypto::secret_key> input_amount_blinding_factors_sk;
    input_amount_blinding_factors_sk.reserve(input_amount_blinding_factors_sk.size());
    for (const rct::key &input_amount_blinding_factor : input_amount_blinding_factors)
        input_amount_blinding_factors_sk.push_back(rct::rct2sk(input_amount_blinding_factor));

    // generate random r_o
    std::vector<crypto::secret_key> r_o(input_onetime_addresses.size());
    for (size_t i = 0; i < input_onetime_addresses.size(); ++i)
        crypto::random32_unbiased(to_bytes(r_o[i]));

    // calculate output_amount_blinding_factor_sum = sum(output_amount_blinding_factors)
    crypto::secret_key output_amount_blinding_factor_sum;
    sc_0(to_bytes(output_amount_blinding_factor_sum));
    for (const rct::key &output_amount_blinding_factor : output_amount_blinding_factors)
        sc_add(to_bytes(output_amount_blinding_factor_sum),
            to_bytes(output_amount_blinding_factor_sum),
            output_amount_blinding_factor.bytes);

    fcmp_pp::make_balanced_rerandomized_output_set(input_onetime_addresses,
        input_amount_commitments_pt,
        input_amount_blinding_factors_sk,
        r_o,
        output_amount_blinding_factor_sum,
        rerandomized_outputs_out);
}
//-------------------------------------------------------------------------------------------------------------------
bool verify_rerandomized_output_basic(const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const crypto::public_key &onetime_address,
    const rct::key &amount_commitment)
{
    const FcmpInputCompressed recomputed_input = calculate_fcmp_input_for_rerandomizations(
        onetime_address,
        amount_commitment,
        load_key(rerandomized_output.r_o),
        load_key(rerandomized_output.r_i),
        load_key(rerandomized_output.r_r_i),
        load_key(rerandomized_output.r_c));

    return 0 == memcmp(&recomputed_input, &rerandomized_output.input, sizeof(FcmpInputCompressed));
}
//-------------------------------------------------------------------------------------------------------------------
bool verify_openable_rerandomized_output_basic(const CarrotOpenableRerandomizedOutputV1 &openable_rerandomized_output)
{
    return verify_rerandomized_output_basic(openable_rerandomized_output.rerandomized_output,
        onetime_address_ref(openable_rerandomized_output.opening_hint),
        amount_commitment_ref(openable_rerandomized_output.opening_hint));
}
//-------------------------------------------------------------------------------------------------------------------
void make_sal_proof_legacy_to_legacy_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const LegacyOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_spend,
    const cryptonote_hierarchy_address_device &addr_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    CHECK_AND_ASSERT_THROW_MES(verify_rerandomized_output_basic(rerandomized_output,
            opening_hint.onetime_address,
            rct::commit(opening_hint.amount, rct::sk2rct(opening_hint.amount_blinding_factor))),
        "make sal proof legacy to legacy v1: rerandomized output does not verify");

    // k^{j,}g_addr = k_s + k^j_subext
    crypto::secret_key address_privkey_g;
    addr_dev.make_legacy_subaddress_extension(opening_hint.subaddr_index.major,
        opening_hint.subaddr_index.minor,
        address_privkey_g);
    sc_add(to_bytes(address_privkey_g),
        to_bytes(address_privkey_g),
        to_bytes(k_spend));

    // note that we pass k_spend as k_generate_image, and leave k_prove_spend as 0
    make_sal_proof_nominal_address_naive(signable_tx_hash,
        rerandomized_output,
        address_privkey_g,
        crypto::null_skey,
        opening_hint.sender_extension_g,
        crypto::null_skey,
        sal_proof_out,
        key_image_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_sal_proof_carrot_to_legacy_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_spend,
    const cryptonote_hierarchy_address_device &addr_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    // check that the opening hint tells us to open as a legacy address
    const AddressDeriveType derive_type = opening_hint.subaddr_index.derive_type;
    CHECK_AND_ASSERT_THROW_MES(derive_type == AddressDeriveType::PreCarrot,
        "make sal proof carrot to carrot v1: invalid subaddr derive type: " << static_cast<int>(derive_type));

    // k^{j, g}_addr = k_s + k^j_subext
    crypto::secret_key address_privkey_g;
    addr_dev.make_legacy_subaddress_extension(opening_hint.subaddr_index.index.major,
        opening_hint.subaddr_index.index.minor,
        address_privkey_g);
    sc_add(to_bytes(address_privkey_g),
        to_bytes(address_privkey_g),
        to_bytes(k_spend));

    make_sal_proof_nominal_address_carrot_v1(signable_tx_hash,
        rerandomized_output,
        opening_hint,
        address_privkey_g,
        /*address_privkey_t=*/crypto::null_skey,
        addr_dev.get_cryptonote_account_spend_pubkey(),
        /*s_view_balance_dev=*/nullptr,
        &addr_dev,
        sal_proof_out,
        key_image_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_sal_proof_carrot_to_carrot_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_prove_spend,
    const crypto::secret_key &k_generate_image,
    const view_balance_secret_device &s_view_balance_dev,
    const view_incoming_key_device &k_view_incoming_dev,
    const generate_address_secret_device &s_generate_address_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    // check that the opening hint tells us to open as a Carrot address
    const AddressDeriveType derive_type = opening_hint.subaddr_index.derive_type;
    CHECK_AND_ASSERT_THROW_MES(derive_type == AddressDeriveType::Carrot,
        "make sal proof carrot to carrot v1: invalid subaddr derive type: " << static_cast<int>(derive_type));

    // K_s = k_gi G + k_ps T
    crypto::public_key account_spend_pubkey;
    make_carrot_spend_pubkey(k_generate_image, k_prove_spend, account_spend_pubkey);

    const std::uint32_t major_index = opening_hint.subaddr_index.index.major;
    const std::uint32_t minor_index = opening_hint.subaddr_index.index.minor;
    const bool is_subaddress = major_index || minor_index;
    crypto::secret_key k_subaddress_scalar;
    if (is_subaddress)
    {
        // s^j_gen = H_32[s_ga](j_major, j_minor)
        crypto::secret_key s_address_generator;
        s_generate_address_dev.make_index_extension_generator(major_index, minor_index, s_address_generator);

        // k^j_subscal = H_n(K_s, j_major, j_minor, s^j_gen)
        make_carrot_subaddress_scalar(account_spend_pubkey,
            s_address_generator,
            major_index,
            minor_index,
            k_subaddress_scalar);
    }
    else
    {
        // k^j_subscal = 1
        sc_1(to_bytes(k_subaddress_scalar));
    }

    // k^{j, g}_addr = k_gi * k^j_subscal
    crypto::secret_key address_privkey_g;
    sc_mul(to_bytes(address_privkey_g), to_bytes(k_generate_image), to_bytes(k_subaddress_scalar));

    // k^{j, t}_addr = k_ps * k^j_subscal
    crypto::secret_key address_privkey_t;
    sc_mul(to_bytes(address_privkey_t), to_bytes(k_prove_spend), to_bytes(k_subaddress_scalar));

    make_sal_proof_nominal_address_carrot_v1(signable_tx_hash,
        rerandomized_output,
        opening_hint,
        address_privkey_g,
        address_privkey_t,
        account_spend_pubkey,
        &s_view_balance_dev,
        &k_view_incoming_dev,
        sal_proof_out,
        key_image_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_sal_proof_carrot_coinbase_to_legacy_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotCoinbaseOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_spend,
    const cryptonote_hierarchy_address_device &addr_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    // check that the opening hint tells us to open as a legacy address
    const AddressDeriveType derive_type = opening_hint.derive_type;
    CHECK_AND_ASSERT_THROW_MES(derive_type == AddressDeriveType::PreCarrot,
        "make sal proof carrot coinbase to legacy v1: invalid subaddr derive type: " << static_cast<int>(derive_type));

    make_sal_proof_nominal_address_carrot_coinbase_v1(signable_tx_hash,
        rerandomized_output,
        opening_hint,
        k_spend,
        crypto::null_skey,
        addr_dev.get_cryptonote_account_spend_pubkey(),
        addr_dev,
        sal_proof_out,
        key_image_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_sal_proof_carrot_coinbase_to_carrot_v1(const crypto::hash &signable_tx_hash,
    const FcmpRerandomizedOutputCompressed &rerandomized_output,
    const CarrotCoinbaseOutputOpeningHintV1 &opening_hint,
    const crypto::secret_key &k_prove_spend,
    const crypto::secret_key &k_generate_image,
    const view_incoming_key_device &k_view_incoming_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    // check that the opening hint tells us to open as a Carrot address
    const AddressDeriveType derive_type = opening_hint.derive_type;
    CHECK_AND_ASSERT_THROW_MES(derive_type == AddressDeriveType::Carrot,
        "make sal proof carrot coinbase to carrot v1: invalid subaddr derive type: " << static_cast<int>(derive_type));

    // K_s = k_gi G + k_ps T
    crypto::public_key account_spend_pubkey;
    make_carrot_spend_pubkey(k_generate_image, k_prove_spend, account_spend_pubkey);

    make_sal_proof_nominal_address_carrot_coinbase_v1(signable_tx_hash,
        rerandomized_output,
        opening_hint,
        k_generate_image,
        k_prove_spend,
        account_spend_pubkey,
        k_view_incoming_dev,
        sal_proof_out,
        key_image_out);
}
//-------------------------------------------------------------------------------------------------------------------
void make_sal_proof_any_to_legacy_v1(const crypto::hash &signable_tx_hash,
    const CarrotOpenableRerandomizedOutputV1 &openable_rerandomized_output,
    const crypto::secret_key &k_spend,
    const cryptonote_hierarchy_address_device &addr_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    struct make_sal_proof_any_to_legacy_v1_visitor
    {
        void operator()(const LegacyOutputOpeningHintV1 &hint)
        {
            make_sal_proof_legacy_to_legacy_v1(signable_tx_hash,
                rerandomized_output,
                hint,
                k_spend,
                addr_dev,
                sal_proof_out,
                key_image_out);
        }

        void operator()(const CarrotOutputOpeningHintV1 &hint)
        {
            make_sal_proof_carrot_to_legacy_v1(signable_tx_hash,
                rerandomized_output,
                hint,
                k_spend,
                addr_dev,
                sal_proof_out,
                key_image_out);
        }

        void operator()(const CarrotCoinbaseOutputOpeningHintV1 &hint)
        {
            make_sal_proof_carrot_coinbase_to_legacy_v1(signable_tx_hash,
                rerandomized_output,
                hint,
                k_spend,
                addr_dev,
                sal_proof_out,
                key_image_out);
        }

        const crypto::hash &signable_tx_hash;
        const FcmpRerandomizedOutputCompressed &rerandomized_output;
        const crypto::secret_key &k_spend;
        const cryptonote_hierarchy_address_device &addr_dev;
        fcmp_pp::FcmpPpSalProof &sal_proof_out;
        crypto::key_image &key_image_out;
    };

    return openable_rerandomized_output.opening_hint.visit(
        make_sal_proof_any_to_legacy_v1_visitor{
            signable_tx_hash,
            openable_rerandomized_output.rerandomized_output,
            k_spend,
            addr_dev,
            sal_proof_out,
            key_image_out
        }
    );
}
//-------------------------------------------------------------------------------------------------------------------
void make_sal_proof_any_to_carrot_v1(const crypto::hash &signable_tx_hash,
    const CarrotOpenableRerandomizedOutputV1 &openable_rerandomized_output,
    const crypto::secret_key &k_prove_spend,
    const crypto::secret_key &k_generate_image,
    const view_balance_secret_device &s_view_balance_dev,
    const view_incoming_key_device &k_view_incoming_dev,
    const generate_address_secret_device &s_generate_address_dev,
    fcmp_pp::FcmpPpSalProof &sal_proof_out,
    crypto::key_image &key_image_out)
{
    struct make_sal_proof_any_to_carrot_v1_visitor
    {
        void operator()(const LegacyOutputOpeningHintV1 &hint)
        {
            ASSERT_MES_AND_THROW("make sal proof any to carrot v1: legacy unsupported");
        }

        void operator()(const CarrotOutputOpeningHintV1 &hint)
        {
            make_sal_proof_carrot_to_carrot_v1(signable_tx_hash,
                rerandomized_output,
                hint,
                k_prove_spend,
                k_generate_image,
                s_view_balance_dev,
                k_view_incoming_dev,
                s_generate_address_dev,
                sal_proof_out,
                key_image_out);
        }

        void operator()(const CarrotCoinbaseOutputOpeningHintV1 &hint)
        {
            make_sal_proof_carrot_coinbase_to_carrot_v1(signable_tx_hash,
                rerandomized_output,
                hint,
                k_prove_spend,
                k_generate_image,
                k_view_incoming_dev,
                sal_proof_out,
                key_image_out);
        }

        const crypto::hash &signable_tx_hash;
        const FcmpRerandomizedOutputCompressed &rerandomized_output;
        const crypto::secret_key &k_prove_spend;
        const crypto::secret_key &k_generate_image;
        const view_balance_secret_device &s_view_balance_dev;
        const view_incoming_key_device &k_view_incoming_dev;
        const generate_address_secret_device &s_generate_address_dev;
        fcmp_pp::FcmpPpSalProof &sal_proof_out;
        crypto::key_image &key_image_out;
    };

    return openable_rerandomized_output.opening_hint.visit(
        make_sal_proof_any_to_carrot_v1_visitor{
            signable_tx_hash,
            openable_rerandomized_output.rerandomized_output,
            k_prove_spend,
            k_generate_image,
            s_view_balance_dev,
            k_view_incoming_dev,
            s_generate_address_dev,
            sal_proof_out,
            key_image_out
        }
    );
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
