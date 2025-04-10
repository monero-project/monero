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
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/lazy_amount_commitment.h"
#include "carrot_core/scan.h"
#include "carrot_impl/carrot_tx_format_utils.h"
#include "common/container_helpers.h"
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
    if (!cryptonote::find_tx_extra_field_by_type(tx_extra_fields, field_extra_nonce))
        field_extra_nonce.nonce.clear();
    
    return fully_parsed;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static bool parse_tx_extra_for_scanning(const cryptonote::transaction_prefix &tx_prefix,
    std::vector<crypto::public_key> &main_tx_ephemeral_pubkeys_out,
    std::vector<crypto::public_key> &additional_tx_ephemeral_pubkeys_out,
    cryptonote::blobdata &tx_extra_nonce_out)
{
    return parse_tx_extra_for_scanning(tx_prefix.extra,
        tx_prefix.vout.size(),
        main_tx_ephemeral_pubkeys_out,
        additional_tx_ephemeral_pubkeys_out,
        tx_extra_nonce_out);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
static void perform_ecdh_derivations(const epee::span<const crypto::public_key> main_tx_ephemeral_pubkeys,
    const epee::span<const crypto::public_key> additional_tx_ephemeral_pubkeys,
    const cryptonote::account_keys &acc,
    const bool is_carrot,
    std::vector<crypto::key_derivation> &main_derivations_out,
    std::vector<crypto::key_derivation> &additional_derivations_out)
{
    main_derivations_out.clear();
    additional_derivations_out.clear();
    main_derivations_out.reserve(main_tx_ephemeral_pubkeys.size());
    additional_derivations_out.reserve(additional_derivations_out.size());

    if (is_carrot)
    {
        //! @TODO: HW device
        carrot::view_incoming_key_ram_borrowed_device k_view_dev(acc.m_view_secret_key);
    
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
            acc.get_device().generate_key_derivation(main_tx_ephemeral_pubkey,
                acc.m_view_secret_key,
                tools::add_element(main_derivations_out));
        }

        for (const crypto::public_key &additional_tx_ephemeral_pubkey : additional_tx_ephemeral_pubkeys)
        {
            acc.get_device().generate_key_derivation(additional_tx_ephemeral_pubkey,
                acc.m_view_secret_key,
                tools::add_element(additional_derivations_out));
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
std::optional<enote_view_incoming_scan_info_t> try_view_incoming_scan_enote_destination(
    const cryptonote::tx_out &enote_destination,
    const carrot::lazy_amount_commitment_t &amount_commitment,
    const epee::span<const crypto::public_key> main_tx_ephemeral_pubkeys,
    const epee::span<const crypto::public_key> additional_tx_ephemeral_pubkeys,
    const cryptonote::blobdata &tx_extra_nonce,
    const cryptonote::txin_v &first_tx_input,
    const std::size_t local_output_index,
    const epee::span<const crypto::key_derivation> main_derivations,
    const std::vector<crypto::key_derivation> &additional_derivations,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map)
{
    // 1. check that:
    //    a) has main or additional ephemeral pubkey, and
    //    b) main derivations match with main ephemeral pubkeys, and
    //    c) additional derivations match with additional pubkeys, and
    //    d) output_index isn't out of range for a non-empty additional ephemeral pubkeys, and
    //    e) main ephemeral pubkeys count is not greater than two, and
    //    f) we can extract an output public key from the enote destination
    enote_view_incoming_scan_info_t res{};
    if (main_tx_ephemeral_pubkeys.empty() && additional_tx_ephemeral_pubkeys.empty())
        return std::nullopt;
    else if (main_derivations.size() != main_tx_ephemeral_pubkeys.size())
        return std::nullopt;
    else if (additional_derivations.size() != additional_tx_ephemeral_pubkeys.size())
        return std::nullopt;
    else if (!additional_derivations.empty() && local_output_index >= additional_derivations.size())
        return std::nullopt;
    else if (main_derivations.size() > 2)
        return std::nullopt;
    else if (!cryptonote::get_output_public_key(enote_destination, res.onetime_address))
        return std::nullopt;

    // 2. setup default values
    res.amount = enote_destination.amount;
    res.amount_blinding_factor = rct::I;
    res.local_output_index = local_output_index;

    boost::optional<cryptonote::subaddress_receive_info> subaddr_receive_info;

    // 3. copy long plaintext payment ID, if applicable
    if (!tx_extra_nonce.empty() && !cryptonote::get_payment_id_from_tx_extra_nonce(tx_extra_nonce, res.payment_id))
        res.payment_id = crypto::hash{};

    // 4. view-incoming scan
    const bool is_carrot = enote_destination.target.type() == typeid(cryptonote::txout_to_carrot_v1);
    const bool is_coinbase = first_tx_input.type() == typeid(cryptonote::txin_gen);
    if (is_carrot)
    {
        const crypto::public_key &enote_ephemeral_pubkey_pk = main_tx_ephemeral_pubkeys.empty()
            ? additional_tx_ephemeral_pubkeys[local_output_index] : main_tx_ephemeral_pubkeys[0];
        const auto enote_ephemeral_pubkey = carrot::raw_byte_convert<mx25519_pubkey>(enote_ephemeral_pubkey_pk);

        const auto &txout_carrot = boost::get<cryptonote::txout_to_carrot_v1>(enote_destination.target);

        //! @TODO: HW device
        const carrot::view_incoming_key_ram_borrowed_device k_view_dev(acc.m_view_secret_key);
        mx25519_pubkey s_sender_receiver_unctx;
        if (!k_view_dev.view_key_scalar_mult_x25519(enote_ephemeral_pubkey, s_sender_receiver_unctx))
            return std::nullopt;

        if (is_coinbase)
        {
            const carrot::CarrotCoinbaseEnoteV1 enote{
                .onetime_address = txout_carrot.key,
                .amount = enote_destination.amount,
                .anchor_enc = txout_carrot.encrypted_janus_anchor,
                .view_tag = txout_carrot.view_tag,
                .enote_ephemeral_pubkey = enote_ephemeral_pubkey,
                .block_index = boost::get<cryptonote::txin_gen>(first_tx_input).height
            };

            if (!carrot::try_scan_carrot_coinbase_enote(enote,
                    s_sender_receiver_unctx,
                    k_view_dev,
                    acc.m_account_address.m_spend_public_key,
                    res.sender_extension_g,
                    res.sender_extension_t))
                return std::nullopt;

            res.address_spend_pubkey = acc.m_account_address.m_spend_public_key;
        }
        else // !is_coinbase
        {
            const carrot::CarrotEnoteV1 enote{
                .onetime_address = txout_carrot.key,
                .amount_commitment = carrot::calculate_amount_commitment(amount_commitment),
                //.anchor_enc = ... doesn't matter for try_scan_carrot_enote_external_destination_only() ...
                .anchor_enc = txout_carrot.encrypted_janus_anchor,
                .view_tag = txout_carrot.view_tag,
                .enote_ephemeral_pubkey = enote_ephemeral_pubkey,
                .tx_first_key_image = boost::get<cryptonote::txin_to_key>(first_tx_input).k_image
            };

            // convert pid_enc
            std::optional<carrot::encrypted_payment_id_t> encrypted_carrot_payment_id;
            if (!tx_extra_nonce.empty())
            {
                crypto::hash8 pid_enc_8;
                if (cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(tx_extra_nonce, pid_enc_8))
                    encrypted_carrot_payment_id = carrot::raw_byte_convert<carrot::encrypted_payment_id_t>(pid_enc_8);
            }

            // try Carrot view-scan w/o amount commitment recomputation
            carrot::payment_id_t carrot_payment_id;
            if (!carrot::try_scan_carrot_enote_external_destination_only(enote,
                    encrypted_carrot_payment_id,
                    s_sender_receiver_unctx,
                    k_view_dev,
                    acc.m_account_address.m_spend_public_key,
                    res.sender_extension_g,
                    res.sender_extension_t,
                    res.address_spend_pubkey,
                    carrot_payment_id))
                return std::nullopt;

            memcpy(&res.payment_id, &carrot_payment_id, sizeof(carrot_payment_id));
        }

        // find K^j_s in subaddress map
        const auto subaddr_it = subaddress_map.find(res.address_spend_pubkey);
        if (subaddr_it == subaddress_map.cend())
            return std::nullopt;

        carrot::input_context_t input_context;
        if (is_coinbase)
        {
            // input_context = "C" || IntToBytes256(block_index)
            carrot::make_carrot_input_context_coinbase(boost::get<cryptonote::txin_gen>(first_tx_input).height,
                input_context);
        }
        else
        {
            // input_context = "R" || KI_1
            carrot::make_carrot_input_context(boost::get<cryptonote::txin_to_key>(first_tx_input).k_image,
                input_context);
        }

        //! TODO: return s^ctx_sr from scan function
        // s^ctx_sr = H_32(s_sr, D_e, input_context)
        crypto::hash s_sender_receiver;
        carrot::make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
            enote_ephemeral_pubkey,
            input_context,
            s_sender_receiver);

        // j
        subaddr_receive_info = cryptonote::subaddress_receive_info{
            .index = subaddr_it->second,
            .derivation = carrot::raw_byte_convert<crypto::key_derivation>(s_sender_receiver)
        };

        // we don't have the extra main tx pubkey bug in Carrot
        res.main_tx_pubkey_index = 0;
    }
    else // !is_carrot
    {
        const boost::optional<crypto::view_tag> view_tag = cryptonote::get_output_view_tag(enote_destination);
        for (size_t i = 0; i < std::max<size_t>(main_derivations.size(), 1); ++i)
        {
            // try view-scan, testing view tag if applicable
            subaddr_receive_info = cryptonote::is_out_to_acc_precomp(subaddress_map,
                res.onetime_address,
                (i < main_derivations.size()) ? main_derivations[i] : crypto::key_derivation{},
                (i == 0) ? additional_derivations : std::vector<crypto::key_derivation>{},
                local_output_index,
                acc.get_device(),
                view_tag);

            if (!subaddr_receive_info)
                continue;

            // derive k^g_o
            if (!acc.get_device().derivation_to_scalar(subaddr_receive_info->derivation,
                    local_output_index,
                    res.sender_extension_g))
                continue;

            // k^t_o = 0
            res.sender_extension_t = crypto::null_skey;

            // K^j_s'
            if (!acc.get_device().derive_subaddress_public_key(res.onetime_address,
                    subaddr_receive_info->derivation,
                    local_output_index,
                    res.address_spend_pubkey))
                continue;

            res.main_tx_pubkey_index = i;
        }

        // decrypt pid
        const bool should_try_decrypt_pid = !tx_extra_nonce.empty()
            && res.main_tx_pubkey_index < main_tx_ephemeral_pubkeys.size();
        if (should_try_decrypt_pid)
        {
            crypto::hash8 pid_8;
            if (cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(tx_extra_nonce, pid_8))
            {
                if (acc.get_device().decrypt_payment_id(pid_8,
                        main_tx_ephemeral_pubkeys[res.main_tx_pubkey_index],
                        acc.m_view_secret_key))
                    memcpy(&res.payment_id, &pid_8, sizeof(pid_8));
            }
        }
    }

    if (!subaddr_receive_info)
        return std::nullopt;

    res.subaddr_index = carrot::subaddress_index_extended{
        .index = { subaddr_receive_info->index.major, subaddr_receive_info->index.minor },
        .derive_type = carrot::AddressDeriveType::Auto //! @TODO: handle hybrid devices
    };

    res.derivation = subaddr_receive_info->derivation;

    return res;
}
//-------------------------------------------------------------------------------------------------------------------
std::optional<enote_view_incoming_scan_info_t> try_view_incoming_scan_enote_destination(
    const cryptonote::transaction_prefix &tx_prefix,
    const carrot::lazy_amount_commitment_t &amount_commitment,
    const std::size_t local_output_index,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map)
{
    // 1. parse tx extra
    std::vector<crypto::public_key> main_tx_ephemeral_pubkeys;
    std::vector<crypto::public_key> additional_tx_ephemeral_pubkeys;
    cryptonote::blobdata tx_extra_nonce;
    parse_tx_extra_for_scanning(tx_prefix, main_tx_ephemeral_pubkeys, additional_tx_ephemeral_pubkeys, tx_extra_nonce);

    // 2. perform ECDH derivations
    std::vector<crypto::key_derivation> main_derivations;
    std::vector<crypto::key_derivation> additional_derivations;
    perform_ecdh_derivations(epee::to_span(main_tx_ephemeral_pubkeys),
        epee::to_span(additional_tx_ephemeral_pubkeys),
        acc,
        carrot::is_carrot_transaction_v1(tx_prefix),
        main_derivations,
        additional_derivations);

    // 3. view-scan enote destination
    return try_view_incoming_scan_enote_destination(tx_prefix.vout.at(local_output_index),
        amount_commitment,
        epee::to_span(main_tx_ephemeral_pubkeys),
        epee::to_span(additional_tx_ephemeral_pubkeys),
        tx_extra_nonce,
        tx_prefix.vin.at(0),
        local_output_index,
        epee::to_span(main_derivations),
        additional_derivations,
        acc,
        subaddress_map);
}
//-------------------------------------------------------------------------------------------------------------------
bool try_decrypt_enote_amount(const crypto::public_key &onetime_address,
    const rct::key &enote_amount_commitment,
    const std::uint8_t rct_type,
    const rct::ecdhTuple &rct_ecdh_tuple,
    const crypto::key_derivation &derivation,
    const std::size_t local_output_index,
    const crypto::public_key &address_spend_pubkey,
    hw::device &hwdev,
    rct::xmr_amount &amount_out,
    rct::key &amount_blinding_factor_out)
{
    const bool is_carrot = rct_type >= carrot::carrot_v1_rct_type;
    if (is_carrot)
    {
        //! @TODO: put this into format utils
        carrot::encrypted_amount_t encrypted_amount;
        memcpy(&encrypted_amount, &rct_ecdh_tuple.amount, sizeof(encrypted_amount));

        const crypto::hash s_sender_receiver = carrot::raw_byte_convert<crypto::hash>(derivation);

        carrot::CarrotEnoteType dummy_enote_type;
        crypto::secret_key amount_blinding_factor_sk;
        if (!carrot::try_get_carrot_amount(s_sender_receiver,
                encrypted_amount,
                onetime_address,
                address_spend_pubkey,
                enote_amount_commitment,
                dummy_enote_type,
                amount_out, // a
                amount_blinding_factor_sk))
            return false;
        // z
        amount_blinding_factor_out = rct::sk2rct(amount_blinding_factor_sk);
    }
    else // !is_carrot
    {
        crypto::ec_scalar amount_key;
        if (!hwdev.derivation_to_scalar(derivation,
                local_output_index,
                amount_key))
            return false;

        const bool is_short_amount = rct_type >= rct::RCTTypeBulletproof2;
        rct::ecdhTuple decoded_ecdh_tuple = rct_ecdh_tuple;
        if (!hwdev.ecdhDecode(decoded_ecdh_tuple,
                carrot::raw_byte_convert<rct::key>(amount_key),
                is_short_amount))
            return false;

        // a
        amount_out = rct::h2d(decoded_ecdh_tuple.amount);
        // z
        amount_blinding_factor_out = decoded_ecdh_tuple.mask;

        // C' = z G + a H
        const rct::key recomputed_amount_commitment = rct::commit(amount_out,
            amount_blinding_factor_out);

        // C' ?= C
        if (!rct::equalKeys(enote_amount_commitment, recomputed_amount_commitment))
            return false;
    }

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
std::optional<enote_view_incoming_scan_info_t> try_view_incoming_scan_enote(
    const cryptonote::tx_out &enote_destination,
    const rct::rctSigBase &rct_sig,
    const epee::span<const crypto::public_key> main_tx_ephemeral_pubkeys,
    const epee::span<const crypto::public_key> additional_tx_ephemeral_pubkeys,
    const cryptonote::blobdata &tx_extra_nonce,
    const cryptonote::txin_v &first_tx_input,
    const std::size_t local_output_index,
    const epee::span<const crypto::key_derivation> main_derivations,
    const std::vector<crypto::key_derivation> &additional_derivations,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map)
{
    const bool is_rct = rct_sig.type != rct::RCTTypeNull;

    carrot::lazy_amount_commitment_t amount_commitment;
    if (is_rct) amount_commitment = rct_sig.outPk.at(local_output_index).mask;
    else amount_commitment = enote_destination.amount;

    auto res = try_view_incoming_scan_enote_destination(
        enote_destination,
        amount_commitment,
        main_tx_ephemeral_pubkeys,
        additional_tx_ephemeral_pubkeys,
        tx_extra_nonce,
        first_tx_input,
        local_output_index,
        main_derivations,
        additional_derivations,
        acc,
        subaddress_map);

    if (!res)
        return res;

    // a, z
    if (is_rct)
    {
        const bool decrypted_amount = try_decrypt_enote_amount(
            res->onetime_address,
            rct_sig.outPk.at(local_output_index).mask,
            rct_sig.type,
            rct_sig.ecdhInfo.at(local_output_index),
            res->derivation,
            res->local_output_index,
            res->address_spend_pubkey,
            acc.get_device(),
            res->amount,
            res->amount_blinding_factor);

        if (!decrypted_amount)
            res.reset();
    }
    else // !is_rct
    {
        res->amount = enote_destination.amount;
        res->amount_blinding_factor = rct::I;
    }
    
    return res;
}
//-------------------------------------------------------------------------------------------------------------------
void view_incoming_scan_transaction(
    const cryptonote::transaction &tx,
    const epee::span<const crypto::public_key> main_tx_ephemeral_pubkeys,
    const epee::span<const crypto::public_key> additional_tx_ephemeral_pubkeys,
    const cryptonote::blobdata &tx_extra_nonce,
    const epee::span<const crypto::key_derivation> main_derivations,
    const std::vector<crypto::key_derivation> &additional_derivations,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const epee::span<std::optional<enote_view_incoming_scan_info_t>> enote_scan_infos_out)
{
    const size_t n_outputs = tx.vout.size();

    CHECK_AND_ASSERT_THROW_MES(enote_scan_infos_out.size() == n_outputs,
        "view_incoming_scan_transaction: enote scan span wrong length");

    // do view-incoming scan for each output enotes
    for (size_t local_output_index = 0; local_output_index < n_outputs; ++local_output_index)
    {
        auto &enote_scan_info = enote_scan_infos_out[local_output_index];

        const cryptonote::tx_out &enote_destination = tx.vout.at(local_output_index);

        enote_scan_info = try_view_incoming_scan_enote(enote_destination,
            tx.rct_signatures,
            main_tx_ephemeral_pubkeys,
            additional_tx_ephemeral_pubkeys,
            tx_extra_nonce,
            tx.vin.at(0),
            local_output_index,
            main_derivations,
            additional_derivations,
            acc,
            subaddress_map);
    }
}
//-------------------------------------------------------------------------------------------------------------------
void view_incoming_scan_transaction(
    const cryptonote::transaction &tx,
    const epee::span<const crypto::key_derivation> custom_main_derivations,
    const std::vector<crypto::key_derivation> &custom_additional_derivations,
    const cryptonote::account_keys &acc,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddress_map,
    const epee::span<std::optional<enote_view_incoming_scan_info_t>> enote_scan_infos_out)
{
    // 1. parse tx extra
    std::vector<crypto::public_key> main_tx_ephemeral_pubkeys;
    std::vector<crypto::public_key> additional_tx_ephemeral_pubkeys;
    cryptonote::blobdata tx_extra_nonce;
    if (!parse_tx_extra_for_scanning(tx, main_tx_ephemeral_pubkeys, additional_tx_ephemeral_pubkeys, tx_extra_nonce))
        MWARNING("Transaction extra has unsupported format: " << cryptonote::get_transaction_hash(tx));

    // 2. view-incoming scan output enotes
    view_incoming_scan_transaction(tx,
        epee::to_span(main_tx_ephemeral_pubkeys),
        epee::to_span(additional_tx_ephemeral_pubkeys),
        tx_extra_nonce,
        custom_main_derivations,
        custom_additional_derivations,
        acc,
        subaddress_map,
        enote_scan_infos_out);
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
    if (!parse_tx_extra_for_scanning(tx, main_tx_ephemeral_pubkeys, additional_tx_ephemeral_pubkeys, tx_extra_nonce))
        MWARNING("Transaction extra has unsupported format: " << cryptonote::get_transaction_hash(tx));

    // 2. perform ECDH derivations
    std::vector<crypto::key_derivation> main_derivations;
    std::vector<crypto::key_derivation> additional_derivations;
    perform_ecdh_derivations(epee::to_span(main_tx_ephemeral_pubkeys),
        epee::to_span(additional_tx_ephemeral_pubkeys),
        acc,
        carrot::is_carrot_transaction_v1(tx),
        main_derivations,
        additional_derivations);

    // 3. view-incoming scan output enotes
    view_incoming_scan_transaction(tx,
        epee::to_span(main_tx_ephemeral_pubkeys),
        epee::to_span(additional_tx_ephemeral_pubkeys),
        tx_extra_nonce,
        epee::to_span(main_derivations),
        additional_derivations,
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

    const cryptonote::subaddress_index subaddr_index_cn{
        enote_scan_info.subaddr_index->index.major,
        enote_scan_info.subaddr_index->index.minor
    };

    const bool is_carrot = enote_scan_info.sender_extension_t != crypto::null_skey;
    if (is_carrot)
    {
        //! @TODO: compact helper function like generate_key_image_helper_precomp()
        //! @TODO: verify x G + y T = O to prevent downstream debugging issues

        // I = Hp(O)
        crypto::ec_point ki_generator;
        crypto::derive_key_image_generator(enote_scan_info.onetime_address, ki_generator);

        //! @TODO: HW devices

        rct::key subaddress_extension;
        if (subaddr_index_cn.is_zero())
            subaddress_extension = rct::Z;
        else // !subaddr_index_cn.is_zero()
            subaddress_extension = rct::sk2rct(
                acc.get_device().get_subaddress_secret_key(acc.m_view_secret_key, subaddr_index_cn));

        // x = k_s + k^j_subext + k_o
        rct::key x;
        sc_add(x.bytes,
            to_bytes(acc.m_spend_secret_key),
            to_bytes(enote_scan_info.sender_extension_g));
        sc_add(x.bytes, x.bytes, subaddress_extension.bytes);

        // L = x I = (k_s + k^j_subext + k_o) Hp(O)
        return rct::rct2ki(rct::scalarmultKey(rct::pt2rct(ki_generator), x));
    }
    else // !is_carrot
    {
        crypto::key_image ki;
        cryptonote::keypair ota_kp;
        if (!cryptonote::generate_key_image_helper_precomp(acc,
                enote_scan_info.onetime_address,
                enote_scan_info.derivation,
                enote_scan_info.local_output_index,
                subaddr_index_cn,
                ota_kp,
                ki,
                acc.get_device()))
            return std::nullopt;
        return ki;
    }
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace wallet
} //namespace tools
