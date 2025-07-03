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
#include "scanning_tools.h"

//local headers
#include "carrot_core/destination.h"
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/lazy_amount_commitment.h"
#include "carrot_core/scan.h"
#include "carrot_impl/format_utils.h"
#include "common/container_helpers.h"
#include "crypto/generators.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.scanning_tools"

namespace tools
{
namespace wallet
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool parse_tx_extra_for_scanning(const std::vector<std::uint8_t> &tx_extra,
    const std::size_t n_outputs,
    std::vector<crypto::public_key> &main_tx_ephemeral_pubkeys_out,
    std::vector<crypto::public_key> &additional_tx_ephemeral_pubkeys_out,
    cryptonote::blobdata &tx_extra_nonce_out)
{
    // 1. parse extra fields
    std::vector<cryptonote::tx_extra_field> tx_extra_fields;
    bool fully_parsed = cryptonote::parse_tx_extra(tx_extra, tx_extra_fields);

    // 2. extract main tx pubkey
    cryptonote::tx_extra_pub_key field_main_pubkey;
    size_t field_main_pubkey_index = 0;
    while (cryptonote::find_tx_extra_field_by_type(tx_extra_fields, field_main_pubkey, field_main_pubkey_index++))
        main_tx_ephemeral_pubkeys_out.push_back(field_main_pubkey.pub_key);

    // 3. extract additional tx pubkeys
    cryptonote::tx_extra_additional_pub_keys field_additional_pubkeys;
    if (cryptonote::find_tx_extra_field_by_type(tx_extra_fields, field_additional_pubkeys))
    {
        if (field_additional_pubkeys.data.size() == n_outputs)
            additional_tx_ephemeral_pubkeys_out = std::move(field_additional_pubkeys.data);
        else
            fully_parsed = false;
    }

    // 4. extract nonce string
    cryptonote::tx_extra_nonce field_extra_nonce;
    if (cryptonote::find_tx_extra_field_by_type(tx_extra_fields, field_extra_nonce))
        tx_extra_nonce_out = std::move(field_extra_nonce.nonce);

    return fully_parsed;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool try_load_pre_carrot_enote(const bool is_coinbase,
    const cryptonote::transaction &tx,
    const std::size_t local_output_index,
    PreCarrotEnote &enote_out)
{
    CHECK_AND_ASSERT_MES(local_output_index < tx.vout.size(),
        false,
        "make_pre_carrot_enote: local_output_index out of range for vout");
    const cryptonote::tx_out &o = tx.vout.at(local_output_index);
    CHECK_AND_ASSERT_MES(cryptonote::get_output_public_key(o, enote_out.onetime_address),
        false,
        "make_pre_carrot_enote: missing output public key");
    enote_out.view_tag = cryptonote::get_output_view_tag(o);

    enote_out.local_output_index = local_output_index;

    enote_out.encrypted_amount = tx.version == 2 && !is_coinbase;
    enote_out.short_amount = rct::is_rct_short_amount(tx.rct_signatures.type);

    if (enote_out.encrypted_amount)
    {
        CHECK_AND_ASSERT_MES(local_output_index < tx.rct_signatures.outPk.size(),
            false,
            "make_pre_carrot_enote: local_output_index out of range for outPk");
        CHECK_AND_ASSERT_MES(local_output_index < tx.rct_signatures.ecdhInfo.size(),
            false,
            "make_pre_carrot_enote: local_output_index out of range for ecdhInfo");
        enote_out.amount_commitment = tx.rct_signatures.outPk.at(local_output_index).mask;
        enote_out.amount = tx.rct_signatures.ecdhInfo.at(local_output_index);
    }
    else // !enote_out.encrypted_amount
    {
        enote_out.amount.amount = rct::d2h(o.amount);
        enote_out.amount.mask = rct::I;

        // technically the implicit amount commitment should be rct::zeroCommitVartime(o.amount),
        // but it isn't used during scanning for non-RingCT enotes, so we will leave it blank
        enote_out.amount_commitment = rct::Z;
    }

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::optional<enote_view_incoming_scan_info_t> view_incoming_scan_pre_carrot_enote(
    const PreCarrotEnote &enote,
    const std::optional<carrot::encrypted_payment_id_t> &encrypted_payment_id,
    const epee::span<const crypto::key_derivation> main_derivations,
    const epee::span<const crypto::key_derivation> additional_derivations,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    hw::device &hwdev)
{
    boost::optional<cryptonote::subaddress_receive_info> receive_info;
    size_t main_deriv_idx;
    for (main_deriv_idx = 0; main_deriv_idx < std::max<size_t>(1, main_derivations.size()); ++main_deriv_idx)
    {
        const crypto::key_derivation kd = main_deriv_idx < main_derivations.size()
            ? main_derivations[main_deriv_idx] : crypto::key_derivation{};
        receive_info = cryptonote::is_out_to_acc_precomp(subaddress_map,
            enote.onetime_address,
            kd,
            main_deriv_idx ? epee::span<const crypto::key_derivation>{} : additional_derivations,
            enote.local_output_index,
            hwdev,
            enote.view_tag);
        if (receive_info)
            break;
    }

    if (!receive_info)
        return std::nullopt;

    crypto::ec_scalar amount_key;
    if (!hwdev.derivation_to_scalar(receive_info->derivation,
            enote.local_output_index,
            amount_key))
        return std::nullopt;

    // a
    rct::xmr_amount amount = 0;
    // z
    rct::key amount_blinding_factor = rct::I;

    if (enote.encrypted_amount)
    {
        rct::ecdhTuple decoded_ecdh_tuple = enote.amount;
        if (!hwdev.ecdhDecode(decoded_ecdh_tuple,
                carrot::raw_byte_convert<rct::key>(amount_key),
                enote.short_amount))
            return std::nullopt;

        // a
        amount = rct::h2d(decoded_ecdh_tuple.amount);
        // z
        amount_blinding_factor = decoded_ecdh_tuple.mask;

        // C' = z G + a H
        const rct::key recomputed_amount_commitment = rct::commit(amount, amount_blinding_factor);

        // C' ?= C
        if (!rct::equalKeys(enote.amount_commitment, recomputed_amount_commitment))
            return std::nullopt;
    }
    else // !is_rct_amount
    {
        // a
        amount = rct::h2d(enote.amount.amount);
        // z
        amount_blinding_factor = enote.amount.mask;
    }

    // derive k^g_o
    crypto::secret_key sender_extension_g;
    if (!hwdev.derivation_to_scalar(receive_info->derivation,
            enote.local_output_index,
            sender_extension_g))
        return std::nullopt;

    // K^j_s'
    crypto::public_key address_spend_pubkey;
    if (!hwdev.derive_subaddress_public_key(enote.onetime_address,
            receive_info->derivation,
            enote.local_output_index,
            address_spend_pubkey))
        return std::nullopt;

    //pid
    crypto::hash payment_id = crypto::null_hash;
    if (encrypted_payment_id)
    {
        const crypto::secret_key inv_8 = rct::rct2sk(rct::INV_EIGHT);
        const crypto::public_key fake_pubkey = carrot::raw_byte_convert<crypto::public_key>(receive_info->derivation);
        crypto::hash8 pid_hash8 = carrot::raw_byte_convert<crypto::hash8>(*encrypted_payment_id);
        if (hwdev.decrypt_payment_id(pid_hash8, fake_pubkey, inv_8))
            memcpy(&payment_id, &pid_hash8, sizeof(pid_hash8));
    }

    //j
    const carrot::subaddress_index_extended subaddr_index{
        .index = {receive_info->index.major, receive_info->index.minor},
        .derive_type = carrot::AddressDeriveType::PreCarrot
    };

    return enote_view_incoming_scan_info_t{
        .sender_extension_g = sender_extension_g,
        .sender_extension_t = crypto::null_skey,
        .address_spend_pubkey = address_spend_pubkey,
        .payment_id = payment_id,
        .subaddr_index = subaddr_index,
        .amount = amount,
        .amount_blinding_factor = amount_blinding_factor,
        .main_tx_pubkey_index = main_deriv_idx
    };
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::optional<enote_view_incoming_scan_info_t> view_incoming_scan_carrot_coinbase_enote(
    const carrot::CarrotCoinbaseEnoteV1 &enote,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const crypto::public_key &main_address_spend_pubkey)
{
    enote_view_incoming_scan_info_t res;

    if (!carrot::try_scan_carrot_coinbase_enote_receiver(enote,
            s_sender_receiver_unctx,
            main_address_spend_pubkey,
            res.sender_extension_g,
            res.sender_extension_t))
        return std::nullopt;

    res.address_spend_pubkey = main_address_spend_pubkey;
    res.payment_id = crypto::null_hash;
    res.subaddr_index = carrot::subaddress_index_extended{{0, 0}};
    res.amount = enote.amount;
    res.amount_blinding_factor = rct::I;
    res.main_tx_pubkey_index = 0;

    return res;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::optional<enote_view_incoming_scan_info_t> view_incoming_scan_carrot_enote_sender(
    const carrot::CarrotEnoteV1 &enote,
    const std::optional<carrot::encrypted_payment_id_t> &encrypted_payment_id,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const cryptonote::account_public_address &cn_addr)
{
    for (unsigned is_subaddress = 0; is_subaddress < 2; ++is_subaddress)
    {
        const carrot::CarrotDestinationV1 destination{
            .address_spend_pubkey = cn_addr.m_spend_public_key,
            .address_view_pubkey = cn_addr.m_view_public_key,
            .is_subaddress = bool(is_subaddress)
        };

        enote_view_incoming_scan_info_t res;

        crypto::secret_key amount_blinding_factor_sk;
        carrot::payment_id_t payment_id;
        carrot::CarrotEnoteType dummy_enote_type;
        if (!carrot::try_scan_carrot_enote_external_sender(enote,
                encrypted_payment_id,
                destination,
                s_sender_receiver_unctx,
                res.sender_extension_g,
                res.sender_extension_t,
                res.amount,
                amount_blinding_factor_sk,
                dummy_enote_type,
                /*check_payment_id=*/false))
            continue;

        memset(&res.payment_id, 0, sizeof(res.payment_id));
        memcpy(&res.payment_id, &payment_id, sizeof(carrot::payment_id_t));

        res.address_spend_pubkey= destination.address_spend_pubkey;
        res.subaddr_index.reset();
        res.amount_blinding_factor = rct::sk2rct(amount_blinding_factor_sk);
        res.main_tx_pubkey_index = 0;

        return res;
    }

    return std::nullopt;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static std::optional<enote_view_incoming_scan_info_t> view_incoming_scan_carrot_enote_receiver(
    const carrot::CarrotEnoteV1 &enote,
    const std::optional<carrot::encrypted_payment_id_t> &encrypted_payment_id,
    const mx25519_pubkey &s_sender_receiver_unctx,
    const crypto::public_key &main_address_spend_pubkey,
    const carrot::view_incoming_key_device &k_view_dev,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map)
{
    enote_view_incoming_scan_info_t res;

    crypto::secret_key amount_blinding_factor_sk;
    carrot::payment_id_t payment_id;
    carrot::CarrotEnoteType dummy_enote_type;
    if (!carrot::try_scan_carrot_enote_external_receiver(enote,
        encrypted_payment_id,
        s_sender_receiver_unctx,
        {&main_address_spend_pubkey, 1},
        k_view_dev,
        res.sender_extension_g,
        res.sender_extension_t,
        res.address_spend_pubkey,
        res.amount,
        amount_blinding_factor_sk,
        payment_id,
        dummy_enote_type))
    return std::nullopt;

    const auto subaddr_it = subaddress_map.find(res.address_spend_pubkey);
    CHECK_AND_ASSERT_MES(subaddr_it != subaddress_map.cend(),
        std::nullopt,
        "view_incoming_scan_carrot_enote: carrot enote scanned successfully, "
        "but the recovered address spend pubkey was not found in the subaddress map");

    const carrot::subaddress_index_extended subaddr_index = {{subaddr_it->second.major, subaddr_it->second.minor}};

    memset(&res.payment_id, 0, sizeof(res.payment_id));
    memcpy(&res.payment_id, &payment_id, sizeof(carrot::payment_id_t));

    res.subaddr_index = subaddr_index;
    res.amount_blinding_factor = rct::sk2rct(amount_blinding_factor_sk);
    res.main_tx_pubkey_index = 0;

    return res;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void perform_ecdh_derivations(const epee::span<const crypto::public_key> main_tx_ephemeral_pubkeys,
    const epee::span<const crypto::public_key> additional_tx_ephemeral_pubkeys,
    const crypto::secret_key &k_view_incoming,
    hw::device &hwdev,
    const bool is_carrot,
    std::vector<crypto::key_derivation> &main_derivations_out,
    std::vector<crypto::key_derivation> &additional_derivations_out)
{
    main_derivations_out.clear();
    additional_derivations_out.clear();
    main_derivations_out.reserve(main_tx_ephemeral_pubkeys.size());
    additional_derivations_out.reserve(additional_tx_ephemeral_pubkeys.size());

    if (is_carrot)
    {
        //! @TODO: HW device
        carrot::view_incoming_key_ram_borrowed_device k_view_dev(k_view_incoming);

        for (const crypto::public_key &main_tx_ephemeral_pubkey : main_tx_ephemeral_pubkeys)
        {
            mx25519_pubkey s_sender_receiver_unctx;
            k_view_dev.view_key_scalar_mult_x25519(
                carrot::raw_byte_convert<mx25519_pubkey>(main_tx_ephemeral_pubkey),
                s_sender_receiver_unctx);
            main_derivations_out.push_back(carrot::raw_byte_convert<crypto::key_derivation>(s_sender_receiver_unctx));
        }

        for (const crypto::public_key &additional_tx_ephemeral_pubkey : additional_tx_ephemeral_pubkeys)
        {
            mx25519_pubkey s_sender_receiver_unctx;
            k_view_dev.view_key_scalar_mult_x25519(
                carrot::raw_byte_convert<mx25519_pubkey>(additional_tx_ephemeral_pubkey),
                s_sender_receiver_unctx);
            additional_derivations_out.push_back(carrot::raw_byte_convert<crypto::key_derivation>(s_sender_receiver_unctx));
        }
    }
    else // !is_carrot
    {
        for (const crypto::public_key &main_tx_ephemeral_pubkey : main_tx_ephemeral_pubkeys)
        {
            hwdev.generate_key_derivation(main_tx_ephemeral_pubkey,
                k_view_incoming,
                tools::add_element(main_derivations_out));
        }

        for (const crypto::public_key &additional_tx_ephemeral_pubkey : additional_tx_ephemeral_pubkeys)
        {
            hwdev.generate_key_derivation(additional_tx_ephemeral_pubkey,
                k_view_incoming,
                tools::add_element(additional_derivations_out));
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
bool try_load_pre_carrot_enote_from_transaction_prefix(const cryptonote::transaction_prefix &tx_prefix,
    const std::size_t local_output_index,
    const rct::xmr_amount amount,
    const rct::key &amount_blinding_factor,
    PreCarrotEnote &enote_out)
{
    CHECK_AND_ASSERT_MES(local_output_index < tx_prefix.vout.size(),
        false,
        "make_pre_carrot_enote: local_output_index out of range for vout");
    const cryptonote::tx_out &o = tx_prefix.vout.at(local_output_index);
    CHECK_AND_ASSERT_MES(cryptonote::get_output_public_key(o, enote_out.onetime_address),
        false,
        "make_pre_carrot_enote: missing output public key");
    enote_out.view_tag = cryptonote::get_output_view_tag(o);

    enote_out.local_output_index = local_output_index;

    enote_out.encrypted_amount = false;
    enote_out.short_amount = false;
    enote_out.amount.amount = rct::d2h(amount);
    enote_out.amount.mask = amount_blinding_factor;

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
std::optional<enote_view_incoming_scan_info_t> view_incoming_scan_enote(
    const MoneroEnoteVariant &enote,
    const std::size_t local_output_index,
    const cryptonote::blobdata &tx_extra_nonce,
    const epee::span<const crypto::key_derivation> main_derivations,
    const epee::span<const crypto::key_derivation> additional_derivations,
    const cryptonote::account_public_address &address,
    const carrot::view_incoming_key_device *k_view_dev,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    hw::device &hwdev)
{
    CHECK_AND_ASSERT_MES(!main_derivations.empty() || !additional_derivations.empty(),
        std::nullopt,
        "view_incoming_scan_enote: no derivations provided");
    CHECK_AND_ASSERT_MES(additional_derivations.empty() || local_output_index < additional_derivations.size(),
        std::nullopt,
        "view_incoming_scan_enote: additional derivations wrong size");

    //pid_enc
    std::optional<carrot::encrypted_payment_id_t> encrypted_payment_id;
    {
        crypto::hash8 pid_hash8;
        if (cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(tx_extra_nonce, pid_hash8))
            encrypted_payment_id = carrot::raw_byte_convert<carrot::encrypted_payment_id_t>(pid_hash8);
    }

    struct view_incoming_scan_enote_visitor
    {
        std::optional<enote_view_incoming_scan_info_t> operator()(const PreCarrotEnote &enote) const
        {
            auto res = view_incoming_scan_pre_carrot_enote(enote,
                encrypted_payment_id,
                main_derivations,
                additional_derivations,
                subaddress_map,
                hwdev);

            // copy long plaintext payment ID, if applicable
            const bool could_have_long_pid = res && res->payment_id == crypto::null_hash;
            if (could_have_long_pid && !cryptonote::get_payment_id_from_tx_extra_nonce(tx_extra_nonce, res->payment_id))
                res->payment_id = crypto::hash{};

            return res;
        }

        std::optional<enote_view_incoming_scan_info_t> operator()(const carrot::CarrotCoinbaseEnoteV1 &enote) const
        {
            const crypto::key_derivation &kd = main_derivations.size()
                ? main_derivations[0]
                : additional_derivations[local_output_index];
            const mx25519_pubkey s_sender_receiver_unctx = carrot::raw_byte_convert<mx25519_pubkey>(kd);

            return view_incoming_scan_carrot_coinbase_enote(enote,
                s_sender_receiver_unctx,
                address.m_spend_public_key);
        }

        std::optional<enote_view_incoming_scan_info_t> operator()(const carrot::CarrotEnoteV1 &enote) const
        {
            const crypto::key_derivation &kd = main_derivations.size()
                ? main_derivations[0]
                : additional_derivations[local_output_index];
            const mx25519_pubkey s_sender_receiver_unctx = carrot::raw_byte_convert<mx25519_pubkey>(kd);

            const bool scan_as_sender = k_view_dev == nullptr;
            if (scan_as_sender)
            {
                return view_incoming_scan_carrot_enote_sender(enote,
                    encrypted_payment_id,
                    s_sender_receiver_unctx,
                    address);
            }
            else
            {
                return view_incoming_scan_carrot_enote_receiver(enote,
                    encrypted_payment_id,
                    s_sender_receiver_unctx,
                    address.m_spend_public_key,
                    *k_view_dev,
                    subaddress_map);
            }
        }

        const std::size_t local_output_index;
        const cryptonote::blobdata &tx_extra_nonce;
        const std::optional<carrot::encrypted_payment_id_t> encrypted_payment_id;
        const epee::span<const crypto::key_derivation> main_derivations;
        const epee::span<const crypto::key_derivation> additional_derivations;
        const cryptonote::account_public_address &address;
        const carrot::view_incoming_key_device *k_view_dev;
        const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map;
        hw::device &hwdev;
    };

    return std::visit(
        view_incoming_scan_enote_visitor{local_output_index,
            tx_extra_nonce,
            encrypted_payment_id,
            main_derivations,
            additional_derivations,
            address,
            k_view_dev,
            subaddress_map,
            hwdev
        },
        enote);
}
//-------------------------------------------------------------------------------------------------------------------
std::optional<enote_view_incoming_scan_info_t> view_incoming_scan_enote(
    const cryptonote::transaction &tx,
    const std::size_t local_output_index,
    const epee::span<const crypto::public_key> main_tx_ephemeral_pubkeys,
    const epee::span<const crypto::public_key> additional_tx_ephemeral_pubkeys,
    const cryptonote::blobdata &tx_extra_nonce,
    const epee::span<const crypto::key_derivation> main_derivations,
    const epee::span<const crypto::key_derivation> additional_derivations,
    const cryptonote::account_public_address &address,
    const carrot::view_incoming_key_device *k_view_dev,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    hw::device &hwdev)
{
    MoneroEnoteVariant enote;
    const bool is_coinbase = cryptonote::is_coinbase(tx);
    const bool is_carrot = carrot::is_carrot_transaction_v1(tx);

    if (is_carrot)
    {
        const epee::span<const crypto::public_key> enote_ephemeral_pubkeys_pk = main_tx_ephemeral_pubkeys.empty()
            ? additional_tx_ephemeral_pubkeys
            : main_tx_ephemeral_pubkeys;

        //! @TODO: breaks strict aliasing rules
        const epee::span<const mx25519_pubkey> enote_ephemeral_pubkeys = {
            reinterpret_cast<const mx25519_pubkey*>(enote_ephemeral_pubkeys_pk.data()),
            enote_ephemeral_pubkeys_pk.size()};

        if (is_coinbase)
        {
            carrot::CarrotCoinbaseEnoteV1 coinbase_enote;
            if (!carrot::try_load_carrot_coinbase_enote_from_transaction_v1(tx,
                    enote_ephemeral_pubkeys,
                    local_output_index,
                    coinbase_enote))
                return std::nullopt;

            enote = coinbase_enote;
        }
        else
        {
            carrot::CarrotEnoteV1 carrot_enote;
            if (!carrot::try_load_carrot_enote_from_transaction_v1(tx,
                    enote_ephemeral_pubkeys,
                    local_output_index,
                    carrot_enote))
                return std::nullopt;

            enote = carrot_enote;
        }
    }
    else // !is_carrot
    {
        PreCarrotEnote pre_carrot_enote;
        if (!try_load_pre_carrot_enote(is_coinbase, tx, local_output_index, pre_carrot_enote))
            return std::nullopt;

        enote = pre_carrot_enote;
    }

    return view_incoming_scan_enote(enote,
        local_output_index,
        tx_extra_nonce,
        main_derivations,
        additional_derivations,
        address,
        k_view_dev,
        subaddress_map,
        hwdev);
}
//-------------------------------------------------------------------------------------------------------------------
std::optional<enote_view_incoming_scan_info_t> view_incoming_scan_enote_from_prefix(
    const cryptonote::transaction_prefix &tx_prefix,
    const rct::xmr_amount amount,
    const rct::key &amount_blinding_factor,
    const std::size_t local_output_index,
    const cryptonote::account_public_address &address,
    const crypto::secret_key &k_view_incoming,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    hw::device &hwdev)
{
    const bool is_carrot = carrot::is_carrot_transaction_v1(tx_prefix);
    CHECK_AND_ASSERT_MES(!is_carrot,
        std::nullopt,
        "view_incoming_scan_enote_from_prefix: carrot not yet supported");

    // 1. parse enote at local_output_index using provided amount information
    PreCarrotEnote enote;
    CHECK_AND_ASSERT_MES(try_load_pre_carrot_enote_from_transaction_prefix(tx_prefix,
            local_output_index,
            amount,
            amount_blinding_factor,
            enote),
        std::nullopt,
        "view_incoming_scan_enote_from_prefix: failed to load enote from prefix");

    // 2. parse tx_extra
    std::vector<crypto::public_key> main_tx_ephemeral_pubkeys;
    std::vector<crypto::public_key> additional_tx_ephemeral_pubkeys;
    cryptonote::blobdata tx_extra_nonce;
    if (!parse_tx_extra_for_scanning(tx_prefix.extra,
        tx_prefix.vout.size(),
        main_tx_ephemeral_pubkeys,
        additional_tx_ephemeral_pubkeys,
        tx_extra_nonce))
    {
        MWARNING("view_incoming_scan_enote_from_prefix: tx extra has unsupported format");
    }

    // 3. perform ECDH
    std::vector<crypto::key_derivation> main_derivations;
    std::vector<crypto::key_derivation> additional_derivations;
    perform_ecdh_derivations(epee::to_span(main_tx_ephemeral_pubkeys),
        epee::to_span(additional_tx_ephemeral_pubkeys),
        k_view_incoming,
        hwdev,
        is_carrot,
        main_derivations,
        additional_derivations);

    //! @TODO: HW device
    const carrot::view_incoming_key_ram_borrowed_device k_view_dev(k_view_incoming);

    // 4. view scan enote
    return view_incoming_scan_enote(enote,
        local_output_index,
        tx_extra_nonce,
        epee::to_span(main_derivations),
        epee::to_span(additional_derivations),
        address,
        &k_view_dev,
        subaddress_map,
        hwdev);
}
//-------------------------------------------------------------------------------------------------------------------
void view_incoming_scan_transaction(
    const cryptonote::transaction &tx,
    const epee::span<const crypto::public_key> main_tx_ephemeral_pubkeys,
    const epee::span<const crypto::public_key> additional_tx_ephemeral_pubkeys,
    const cryptonote::blobdata &tx_extra_nonce,
    const epee::span<const crypto::key_derivation> main_derivations,
    const epee::span<const crypto::key_derivation> additional_derivations,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const epee::span<std::optional<enote_view_incoming_scan_info_t>> enote_scan_infos_out)
{
    const size_t n_outputs = tx.vout.size();

    CHECK_AND_ASSERT_THROW_MES(enote_scan_infos_out.size() == n_outputs,
        "view_incoming_scan_transaction: enote scan span wrong length");

    //! @TODO: HW device
    carrot::view_incoming_key_ram_borrowed_device k_view_dev(acc.m_view_secret_key);

    // do view-incoming scan for each output enotes
    for (size_t local_output_index = 0; local_output_index < n_outputs; ++local_output_index)
    {
        auto &enote_scan_info = enote_scan_infos_out[local_output_index];
        enote_scan_info = view_incoming_scan_enote(tx,
            local_output_index,
            main_tx_ephemeral_pubkeys,
            additional_tx_ephemeral_pubkeys,
            tx_extra_nonce,
            main_derivations,
            additional_derivations,
            acc.m_account_address,
            &k_view_dev,
            subaddress_map,
            acc.get_device());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void view_incoming_scan_transaction(
    const cryptonote::transaction &tx,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const epee::span<std::optional<enote_view_incoming_scan_info_t>> enote_scan_infos_out)
{
    // 1. parse tx extra
    std::vector<crypto::public_key> main_tx_ephemeral_pubkeys;
    std::vector<crypto::public_key> additional_tx_ephemeral_pubkeys;
    cryptonote::blobdata tx_extra_nonce;
    if (!parse_tx_extra_for_scanning(tx.extra,
            tx.vout.size(),
            main_tx_ephemeral_pubkeys,
            additional_tx_ephemeral_pubkeys,
            tx_extra_nonce))
        MWARNING("Transaction extra has unsupported format: " << cryptonote::get_transaction_hash(tx));

    CHECK_AND_ASSERT_MES(!main_tx_ephemeral_pubkeys.empty() || !additional_tx_ephemeral_pubkeys.empty(),,
        "Transaction missing ephemeral pubkeys");

    // 2. perform ECDH derivations
    std::vector<crypto::key_derivation> main_derivations;
    std::vector<crypto::key_derivation> additional_derivations;
    perform_ecdh_derivations(epee::to_span(main_tx_ephemeral_pubkeys),
        epee::to_span(additional_tx_ephemeral_pubkeys),
        acc.m_view_secret_key,
        acc.get_device(),
        carrot::is_carrot_transaction_v1(tx),
        main_derivations,
        additional_derivations);

    // 3. view-incoming scan output enotes
    view_incoming_scan_transaction(tx,
        epee::to_span(main_tx_ephemeral_pubkeys),
        epee::to_span(additional_tx_ephemeral_pubkeys),
        tx_extra_nonce,
        epee::to_span(main_derivations),
        epee::to_span(additional_derivations),
        acc,
        subaddress_map,
        enote_scan_infos_out);
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::optional<enote_view_incoming_scan_info_t>> view_incoming_scan_transaction(
    const cryptonote::transaction &tx,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map)
{
    std::vector<std::optional<enote_view_incoming_scan_info_t>> res(tx.vout.size());
    view_incoming_scan_transaction(tx, acc, subaddress_map, epee::to_mut_span(res));
    return res;
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::optional<enote_view_incoming_scan_info_t>> view_incoming_scan_transaction_as_sender(
    const cryptonote::transaction &tx,
    const epee::span<const crypto::key_derivation> custom_main_derivations,
    const epee::span<const crypto::key_derivation> custom_additional_derivations,
    const cryptonote::account_public_address &address)
{
    // 1. Resize output
    const size_t n_outputs = tx.vout.size();
    std::vector<std::optional<enote_view_incoming_scan_info_t>> res(n_outputs);

    // 2. parse tx extra
    std::vector<crypto::public_key> main_tx_ephemeral_pubkeys;
    std::vector<crypto::public_key> additional_tx_ephemeral_pubkeys;
    cryptonote::blobdata tx_extra_nonce;
    if (!parse_tx_extra_for_scanning(tx.extra,
            tx.vout.size(),
            main_tx_ephemeral_pubkeys,
            additional_tx_ephemeral_pubkeys,
            tx_extra_nonce))
        MWARNING("Transaction extra has unsupported format: " << cryptonote::get_transaction_hash(tx));

    // 3. do view-incoming scan for each output enotes
    hw::device &hwdev = hw::get_device("default");
    for (size_t local_output_index = 0; local_output_index < n_outputs; ++local_output_index)
    {
        auto &enote_scan_info = res[local_output_index];

        enote_scan_info = view_incoming_scan_enote(tx,
            local_output_index,
            epee::to_span(main_tx_ephemeral_pubkeys),
            epee::to_span(additional_tx_ephemeral_pubkeys),
            tx_extra_nonce,
            custom_main_derivations,
            custom_additional_derivations,
            address,
            /*k_view_dev=*/nullptr,
            {{address.m_spend_public_key, {}}},
            hwdev);
    }

    return res;
}
//-------------------------------------------------------------------------------------------------------------------
bool is_long_payment_id(const crypto::hash &pid)
{
    static_assert(sizeof(pid.data) / sizeof(pid.data[0]) == 32);
    char c = 0;
    for (size_t i = 8; i < 32; i++)
        c |= pid.data[i];
    return c != 0;
}
//-------------------------------------------------------------------------------------------------------------------
std::optional<crypto::key_image> try_derive_enote_key_image(
    const enote_view_incoming_scan_info_t &enote_scan_info,
    const cryptonote::account_keys &acc)
{
    if (!enote_scan_info.subaddr_index)
        return std::nullopt;

    // k^j_subext
    rct::key subaddress_extension;
    if (enote_scan_info.subaddr_index->index.is_subaddress())
    {
        const cryptonote::subaddress_index subaddr_index_cn{enote_scan_info.subaddr_index->index.major,
            enote_scan_info.subaddr_index->index.minor};
        subaddress_extension = rct::sk2rct(
            acc.get_device().get_subaddress_secret_key(acc.m_view_secret_key, subaddr_index_cn));
    }
    else // !subaddr_index_cn.is_zero()
    {
        subaddress_extension = rct::Z;
    }

    // O = K^j_s + k^g_o G + k^t_o T
    rct::key onetime_address = rct::scalarmultKey(rct::pk2rct(crypto::get_T()),
        rct::sk2rct(enote_scan_info.sender_extension_t));
    rct::addKeys1(onetime_address, rct::sk2rct(enote_scan_info.sender_extension_g), onetime_address);
    rct::addKeys(onetime_address, onetime_address, rct::pk2rct(enote_scan_info.address_spend_pubkey));

    // I = Hp(O)
    crypto::ec_point ki_generator;
    crypto::derive_key_image_generator(rct::rct2pk(onetime_address), ki_generator);

    //! @TODO: HW devices
    // x = k_s + k^j_subext + k^g_o
    rct::key x;
    sc_add(x.bytes,
        to_bytes(acc.m_spend_secret_key),
        to_bytes(enote_scan_info.sender_extension_g));
    sc_add(x.bytes, x.bytes, subaddress_extension.bytes);

    // L = x I = (k_s + k^j_subext + k^g_o) Hp(O)
    return rct::rct2ki(rct::scalarmultKey(rct::pt2rct(ki_generator), x));
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace wallet
} //namespace tools
