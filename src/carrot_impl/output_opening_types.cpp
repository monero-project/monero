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

//paired header
#include "output_opening_types.h"

//local headers
#include "carrot_core/enote_utils.h"
#include "carrot_core/scan.h"
#include "ringct/rctOps.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot_impl"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static CarrotEnoteV1 make_carrot_enote_from_v2_opening_hint(const CarrotOutputOpeningHintV2 &opening_hint,
    const crypto::hash &s_sender_receiver)
{
    return CarrotEnoteV1{
        .onetime_address = opening_hint.onetime_address,
        .amount_commitment = opening_hint.amount_commitment,
        .amount_enc = encrypt_carrot_amount(opening_hint.amount, s_sender_receiver, opening_hint.onetime_address),
        .anchor_enc = opening_hint.anchor_enc,
        .view_tag = opening_hint.view_tag,
        .enote_ephemeral_pubkey = opening_hint.enote_ephemeral_pubkey,
        .tx_first_key_image = opening_hint.tx_first_key_image
    };
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool try_opening_hint_scan_on_carrot_enote(const CarrotEnoteV1 &enote,
    const std::optional<encrypted_payment_id_t> &encrypted_payment_id,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    const view_incoming_key_device *k_view_incoming_dev,
    const view_balance_secret_device *s_view_balance_dev,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    rct::xmr_amount &amount_out,
    rct::key &amount_blinding_factor_out)
{
    crypto::public_key dummy_address_spend_pubkey;
    crypto::secret_key amount_blinding_factor_sk;
    CarrotEnoteType dummy_enote_type;
    janus_anchor_t dummy_internal_message;
    payment_id_t dummy_payment_id;
    if (s_view_balance_dev != nullptr)
    {
        if (try_scan_carrot_enote_internal_receiver(enote,
                *s_view_balance_dev,
                sender_extension_g_out,
                sender_extension_t_out,
                dummy_address_spend_pubkey,
                amount_out,
                amount_blinding_factor_sk,
                dummy_enote_type,
                dummy_internal_message))
        {
            amount_blinding_factor_out = rct::sk2rct(amount_blinding_factor_sk);
            return true;
        }
    }
    if (k_view_incoming_dev != nullptr)
    {
        mx25519_pubkey s_sender_receiver_unctx;
        if (make_carrot_uncontextualized_shared_key_receiver(*k_view_incoming_dev,
            enote.enote_ephemeral_pubkey,
            s_sender_receiver_unctx))
        {
            if (try_scan_carrot_enote_external_receiver(enote,
                    encrypted_payment_id,
                    s_sender_receiver_unctx,
                    main_address_spend_pubkeys,
                    *k_view_incoming_dev,
                    sender_extension_g_out,
                    sender_extension_t_out,
                    dummy_address_spend_pubkey,
                    amount_out,
                    amount_blinding_factor_sk,
                    dummy_payment_id,
                    dummy_enote_type))
            {
                amount_blinding_factor_out = rct::sk2rct(amount_blinding_factor_sk);
                return true;
            }
        }
    }

    return false;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool try_scan_opening_hint(const OutputOpeningHintVariant &opening_hint,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    const view_incoming_key_device *k_view_incoming_dev,
    const view_balance_secret_device *s_view_balance_dev,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out,
    rct::xmr_amount &amount_out,
    rct::key &amount_blinding_factor_out)
{
    struct try_scan_opening_hint_visitor
    {
        bool operator()(const LegacyOutputOpeningHintV1 &hint) const
        {
            sender_extension_g_out = crypto::null_skey;
            sender_extension_t_out = crypto::null_skey;
            amount_out = hint.amount;
            amount_blinding_factor_out = hint.amount_blinding_factor;

            if (k_view_incoming_dev == nullptr)
                return false;

            // k_v K_e
            crypto::public_key kd_tors;
            if (!k_view_incoming_dev->view_key_scalar_mult_ed25519(hint.ephemeral_tx_pubkey, kd_tors))
                return false;

            // 8 k_v K_e
            kd_tors = rct::rct2pk(rct::scalarmult8(rct::pk2rct(kd_tors)));
            crypto::key_derivation kd;
            memcpy(&kd, &kd_tors, sizeof(kd));

            // d = k_o = Hs1(8 k_v K_e, i)
            crypto::derivation_to_scalar(kd, hint.local_output_index, unwrap(unwrap(sender_extension_g_out)));

            return true;
        }

        bool operator()(const CarrotOutputOpeningHintV1 &hint) const
        {
            return try_opening_hint_scan_on_carrot_enote(hint.source_enote,
                hint.encrypted_payment_id,
                main_address_spend_pubkeys,
                k_view_incoming_dev,
                s_view_balance_dev,
                sender_extension_g_out,
                sender_extension_t_out,
                amount_out,
                amount_blinding_factor_out);
        }

        bool operator()(const CarrotOutputOpeningHintV2 &hint) const
        {
            // input_context = "R" || KI_1
            const input_context_t input_context = carrot::make_carrot_input_context(hint.tx_first_key_image);

            // s^ctx_sr = H_32(s_sr, D_e, input_context) for internal&external s_sr
            crypto::hash s_sender_receiver[2]; //! @TODO: wipe
            std::size_t n_keys_available = 0;
            if (s_view_balance_dev != nullptr)
            {
                // s^ctx_sr = H_32(s_vb, D_e, input_context)
                s_view_balance_dev->make_internal_sender_receiver_secret(hint.enote_ephemeral_pubkey,
                    input_context, s_sender_receiver[n_keys_available++]);
            }
            if (k_view_incoming_dev != nullptr)
            {
                // s_sr = k_v D_e
                mx25519_pubkey s_sender_receiver_unctx;
                carrot::make_carrot_uncontextualized_shared_key_receiver(*k_view_incoming_dev,
                    hint.enote_ephemeral_pubkey,
                    s_sender_receiver_unctx);

                // s^ctx_sr = H_32(k_v D_e, D_e, input_context)
                carrot::make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
                    hint.enote_ephemeral_pubkey,
                    input_context,
                    s_sender_receiver[n_keys_available++]);
            }

            for (std::size_t i = 0; i < n_keys_available; ++i)
            {
                if (try_opening_hint_scan_on_carrot_enote(
                    make_carrot_enote_from_v2_opening_hint(hint, s_sender_receiver[i]),
                    hint.encrypted_payment_id,
                    main_address_spend_pubkeys,
                    k_view_incoming_dev,
                    s_view_balance_dev,
                    sender_extension_g_out,
                    sender_extension_t_out,
                    amount_out,
                    amount_blinding_factor_out))
                return true;
            }

            return false;
        }

        bool operator()(const CarrotCoinbaseOutputOpeningHintV1 &hint) const
        {
            amount_out = hint.source_enote.amount;
            amount_blinding_factor_out = rct::I;

            mx25519_pubkey s_sender_receiver_unctx;
            if (make_carrot_uncontextualized_shared_key_receiver(*k_view_incoming_dev,
                hint.source_enote.enote_ephemeral_pubkey,
                s_sender_receiver_unctx))
            {
                crypto::public_key dummy_address_spend_pubkey;
                return try_scan_carrot_coinbase_enote_receiver(hint.source_enote,
                    s_sender_receiver_unctx,
                    main_address_spend_pubkeys,
                    sender_extension_g_out,
                    sender_extension_t_out,
                    dummy_address_spend_pubkey);
            }

            return false;
        }

        epee::span<const crypto::public_key> main_address_spend_pubkeys;
        const view_incoming_key_device *k_view_incoming_dev;
        const view_balance_secret_device *s_view_balance_dev;
        crypto::secret_key &sender_extension_g_out;
        crypto::secret_key &sender_extension_t_out;
        rct::xmr_amount &amount_out;
        rct::key &amount_blinding_factor_out;
    };

    return std::visit(try_scan_opening_hint_visitor{
            main_address_spend_pubkeys,
            k_view_incoming_dev,
            s_view_balance_dev,
            sender_extension_g_out,
            sender_extension_t_out,
            amount_out,
            amount_blinding_factor_out},
        opening_hint);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const LegacyOutputOpeningHintV1 &a, const LegacyOutputOpeningHintV1 &b)
{
    return a.onetime_address == b.onetime_address
        && a.ephemeral_tx_pubkey == b.ephemeral_tx_pubkey
        && a.subaddr_index == b.subaddr_index
        && a.amount == b.amount
        && a.amount_blinding_factor == b.amount_blinding_factor
        && a.local_output_index == b.local_output_index;
}
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const CarrotOutputOpeningHintV1 &a, const CarrotOutputOpeningHintV1 &b)
{
    return a.source_enote == b.source_enote
        && a.encrypted_payment_id == b.encrypted_payment_id
        && a.subaddr_index == b.subaddr_index;
}
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const CarrotOutputOpeningHintV2 &a, const CarrotOutputOpeningHintV2 &b)
{
    return a.onetime_address == b.onetime_address
        && a.amount_commitment == b.amount_commitment
        && a.anchor_enc == b.anchor_enc
        && a.view_tag == b.view_tag
        && memcmp(&a.enote_ephemeral_pubkey, &b.enote_ephemeral_pubkey, sizeof(a.enote_ephemeral_pubkey)) == 0
        && a.tx_first_key_image == b.tx_first_key_image
        && a.amount == b.amount
        && a.encrypted_payment_id == b.encrypted_payment_id
        && a.subaddr_index == b.subaddr_index;
}
//-------------------------------------------------------------------------------------------------------------------
bool operator==(const CarrotCoinbaseOutputOpeningHintV1 &a, const CarrotCoinbaseOutputOpeningHintV1 &b)
{
    return a.source_enote == b.source_enote
        && a.derive_type == b.derive_type;
}
//-------------------------------------------------------------------------------------------------------------------
const crypto::public_key &onetime_address_ref(const OutputOpeningHintVariant &opening_hint)
{
    struct onetime_address_ref_visitor
    {
        const crypto::public_key &operator()(const LegacyOutputOpeningHintV1 &h) const
        { return h.onetime_address; }
        const crypto::public_key &operator()(const CarrotOutputOpeningHintV1 &h) const
        { return h.source_enote.onetime_address; }
        const crypto::public_key &operator()(const CarrotOutputOpeningHintV2 &h) const
        { return h.onetime_address; }
        const crypto::public_key &operator()(const CarrotCoinbaseOutputOpeningHintV1 &h) const
        { return h.source_enote.onetime_address; }
    };

    return std::visit(onetime_address_ref_visitor{}, opening_hint);
}
//-------------------------------------------------------------------------------------------------------------------
rct::key amount_commitment_ref(const OutputOpeningHintVariant &opening_hint)
{
    struct amount_commitment_ref_visitor
    {
        rct::key operator()(const LegacyOutputOpeningHintV1 &h) const
        { return rct::commit(h.amount, h.amount_blinding_factor); }
        rct::key operator()(const CarrotOutputOpeningHintV1 &h) const
        { return h.source_enote.amount_commitment; }
        rct::key operator()(const CarrotOutputOpeningHintV2 &h) const
        { return h.amount_commitment; }
        rct::key operator()(const CarrotCoinbaseOutputOpeningHintV1 &h) const
        { return rct::zeroCommitVartime(h.source_enote.amount); }
    };

    return std::visit(amount_commitment_ref_visitor{}, opening_hint);
}
//-------------------------------------------------------------------------------------------------------------------
subaddress_index_extended subaddress_index_ref(const OutputOpeningHintVariant &opening_hint)
{
    struct subaddress_index_ref_visitor
    {
        subaddress_index_extended operator()(const LegacyOutputOpeningHintV1 &h) const
        { return {h.subaddr_index, AddressDeriveType::PreCarrot}; }
        subaddress_index_extended operator()(const CarrotOutputOpeningHintV1 &h) const
        { return h.subaddr_index; }
        subaddress_index_extended operator()(const CarrotOutputOpeningHintV2 &h) const
        { return h.subaddr_index; }
        subaddress_index_extended operator()(const CarrotCoinbaseOutputOpeningHintV1 &h) const
        { return {{0, 0}, h.derive_type}; }
    };

    return std::visit(subaddress_index_ref_visitor{}, opening_hint);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_opening_hint_sender_extensions(const OutputOpeningHintVariant &opening_hint,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    const view_incoming_key_device *k_view_incoming_dev,
    const view_balance_secret_device *s_view_balance_dev,
    crypto::secret_key &sender_extension_g_out,
    crypto::secret_key &sender_extension_t_out)
{
    rct::xmr_amount amount;
    rct::key amount_blinding_factor;
    return try_scan_opening_hint(opening_hint,
        main_address_spend_pubkeys,
        k_view_incoming_dev,
        s_view_balance_dev,
        sender_extension_g_out,
        sender_extension_t_out,
        amount,
        amount_blinding_factor);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_scan_opening_hint_amount(const OutputOpeningHintVariant &opening_hint,
    const epee::span<const crypto::public_key> main_address_spend_pubkeys,
    const view_incoming_key_device *k_view_incoming_dev,
    const view_balance_secret_device *s_view_balance_dev,
    rct::xmr_amount &amount_out,
    rct::key &amount_blinding_factor_out)
{
    crypto::secret_key sender_extension_g, sender_extension_t;
    return try_scan_opening_hint(opening_hint,
        main_address_spend_pubkeys,
        k_view_incoming_dev,
        s_view_balance_dev,
        sender_extension_g,
        sender_extension_t,
        amount_out,
        amount_blinding_factor_out);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
