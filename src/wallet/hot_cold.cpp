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
#include "hot_cold.h"

//local headers
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/enote_utils.h"
#include "carrot_core/output_set_finalization.h"
#include "carrot_core/scan.h"
#include "carrot_core/scan_unsafe.h"
#include "carrot_impl/carrot_offchain_serialization.h"
#include "carrot_impl/format_utils.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctOps.h"
#include "scanning_tools.h"
#include "serialization/binary_archive.h"

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.hot_cold"

namespace tools
{
namespace wallet
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(exported_pre_carrot_transfer_details, bool skip_version)
    if (!skip_version)
    {
        VERSION_FIELD(1)
    }
    FIELD_F(m_pubkey)
    VARINT_FIELD_F(m_internal_output_index)
    VARINT_FIELD_F(m_global_output_index)
    FIELD_F(m_tx_pubkey)
    FIELD_F(m_flags.flags)
    VARINT_FIELD_F(m_amount)
    FIELD_F(m_additional_tx_keys)
    VARINT_FIELD_F(m_subaddr_index_major)
    VARINT_FIELD_F(m_subaddr_index_minor)
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(exported_carrot_transfer_details, bool skip_version)
    if (!skip_version)
    {
        VERSION_FIELD(2)
    }
    VARINT_FIELD_N("flags", v.flags.flags)
    if (v.flags.m_coinbase)
    {
        FIELD_F(block_index)
        v.tx_first_key_image = crypto::key_image{};
    }
    else
    {
        v.block_index = 0;
        FIELD_F(tx_first_key_image)
    }
    if (v.flags.m_has_pid)
    {
        FIELD_F(payment_id)
        v.subaddr_index = {0, 0};
    }
    else // !m_has_pid
    {
        FIELD_F(subaddr_index)
        v.payment_id = carrot::null_payment_id;
    }
    VARINT_FIELD_F(amount)
    FIELD_F(janus_anchor)
    if (v.flags.m_selfsend)
        FIELD_F(selfsend_enote_ephemeral_pubkey)
    else
        v.selfsend_enote_ephemeral_pubkey = mx25519_pubkey{{0}};
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
BEGIN_SERIALIZE_OBJECT_FN(exported_transfer_details_variant)
    const bool is_saving_carrot = typename Archive<W>::is_saving()
        && std::holds_alternative<exported_carrot_transfer_details>(v);
    VERSION_FIELD(is_saving_carrot ? 2 : 1)
    if (version < 1 || version > 2)
        return false;
    if constexpr (!typename Archive<W>::is_saving())
    {
        if (version == 1)
            v = exported_pre_carrot_transfer_details{};
        else
            v = exported_carrot_transfer_details{};
    }
    return std::visit([&ar](auto &x) -> bool { return do_serialize_object(ar, x, true); }, v);
END_SERIALIZE()
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
// Explicit serialization instantiations
template bool do_serialize_object<true, binary_archive>(binary_archive<true> &ar,
    exported_pre_carrot_transfer_details &v,
    bool skip_version);
template bool do_serialize_object<false, binary_archive>(binary_archive<false> &ar,
    exported_pre_carrot_transfer_details &v,
    bool skip_version);
template bool do_serialize_object<true, binary_archive>(binary_archive<true> &ar,
    exported_carrot_transfer_details &v,
    bool skip_version);
template bool do_serialize_object<false, binary_archive>(binary_archive<false> &ar,
    exported_carrot_transfer_details &v,
    bool skip_version);
template bool do_serialize_object<true, binary_archive>(binary_archive<true> &ar,
    exported_transfer_details_variant &v);
template bool do_serialize_object<false, binary_archive>(binary_archive<false> &ar,
    exported_transfer_details_variant &v);
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
exported_pre_carrot_transfer_details export_cold_pre_carrot_output(const wallet2_basic::transfer_details &td)
{
    exported_pre_carrot_transfer_details etd{};
    etd.m_pubkey = td.get_public_key();
    etd.m_tx_pubkey = cryptonote::get_tx_pub_key_from_extra(td.m_tx, td.m_pk_index);
    etd.m_internal_output_index = td.m_internal_output_index;
    etd.m_global_output_index = td.m_global_output_index;
    etd.m_flags.flags = 0;
    etd.m_flags.m_spent = td.m_spent;
    etd.m_flags.m_frozen = td.m_frozen;
    etd.m_flags.m_rct = td.m_rct;
    etd.m_flags.m_key_image_known = td.m_key_image_known;
    etd.m_flags.m_key_image_request = td.m_key_image_request;
    etd.m_flags.m_key_image_partial = td.m_key_image_partial;
    etd.m_amount = td.m_amount;
    etd.m_additional_tx_keys = cryptonote::get_additional_tx_pub_keys_from_extra(td.m_tx);
    etd.m_subaddr_index_major = td.m_subaddr_index.major;
    etd.m_subaddr_index_minor = td.m_subaddr_index.minor;
    return etd;
}
//-------------------------------------------------------------------------------------------------------------------
exported_carrot_transfer_details export_cold_carrot_output(const wallet2_basic::transfer_details &td,
    const carrot::cryptonote_hierarchy_address_device &addr_dev)
{
    // 1. easy flags
    exported_carrot_transfer_details etd{};
    etd.flags.flags = 0;
    etd.flags.m_spent = td.m_spent;
    etd.flags.m_key_image_known = td.m_key_image_known;
    etd.flags.m_key_image_request = td.m_key_image_request;
    //etd.flags.m_selfsend = ...
    //etd.flags.m_enote_type_change = ...
    etd.flags.m_carrot_derived_addr = 0; //! @TODO: carrot hierarchy
    etd.flags.m_internal = 0;
    etd.flags.m_coinbase = cryptonote::is_coinbase(td.m_tx);
    //etd.flags.m_has_pid = ...
    etd.flags.m_frozen = td.m_frozen;
    etd.flags.m_key_image_partial = td.m_key_image_partial;

    // 2. other easy fields
    if (etd.flags.m_coinbase)
    {
        etd.block_index = etd.flags.m_coinbase ? td.m_block_height : 0;
        etd.tx_first_key_image = crypto::key_image{};
    }
    else // non-coinbase
    {
        etd.block_index = 0;
        const cryptonote::txin_to_key *in_to_key = boost::strict_get<cryptonote::txin_to_key>(&td.m_tx.vin.at(0));
        CHECK_AND_ASSERT_THROW_MES(nullptr != in_to_key,
            "cannot export transfer details: failed to get key image from transaction");
        etd.tx_first_key_image = in_to_key->k_image;
    }
    etd.subaddr_index = {td.m_subaddr_index.major, td.m_subaddr_index.minor};
    etd.amount = td.amount();

    // 3. parse carrot from tx.extra
    std::vector<mx25519_pubkey> enote_ephemeral_pubkeys;
    std::optional<carrot::encrypted_payment_id_t> encrypted_payment_id;
    CHECK_AND_ASSERT_THROW_MES(
        carrot::try_load_carrot_extra_v1(td.m_tx.extra,
            enote_ephemeral_pubkeys,
            encrypted_payment_id),
        "cannot export transfer details: failed to parse Carrot tx extra");
    const std::size_t ephemeral_pubkey_idx = enote_ephemeral_pubkeys.size() == 1 ? 0 : td.m_internal_output_index;
    CHECK_AND_ASSERT_THROW_MES(ephemeral_pubkey_idx < enote_ephemeral_pubkeys.size(),
        "cannot export transfer details: wrong number of ephemeral pubkeys");

    // 4. s_sr = k_v D_e
    const mx25519_pubkey &enote_ephemeral_pubkey = enote_ephemeral_pubkeys.at(ephemeral_pubkey_idx);
    mx25519_pubkey s_sender_receiver_unctx;
    carrot::make_carrot_uncontextualized_shared_key_receiver(addr_dev,
        enote_ephemeral_pubkey,
        s_sender_receiver_unctx);

    // 5. input_context
    carrot::input_context_t input_context;
    CHECK_AND_ASSERT_THROW_MES(carrot::parse_carrot_input_context(td.m_tx, input_context),
        "cannot export transfer details: failed to parse input context");

    // 6. s^ctx_sr = H_32(s_sr, D_e, input_context)
    crypto::hash s_sender_receiver; //! @TODO: wipe
    carrot::make_carrot_sender_receiver_secret(s_sender_receiver_unctx.data,
        enote_ephemeral_pubkey,
        input_context,
        s_sender_receiver);

    // 7. get encrypted janus anchor: anchor_enc
    CHECK_AND_ASSERT_THROW_MES(td.m_internal_output_index < td.m_tx.vout.size(),
        "cannot export transfer details: wrong number of transaction outputs");
    const cryptonote::txout_target_v &o_target = td.m_tx.vout.at(td.m_internal_output_index).target;
    const cryptonote::txout_to_carrot_v1 *o_carrot = boost::strict_get<cryptonote::txout_to_carrot_v1>(&o_target);
    CHECK_AND_ASSERT_THROW_MES(nullptr != o_carrot, "cannot export transfer details: output isn't carrot");
    const carrot::encrypted_janus_anchor_t &encrypted_janus_anchor = o_carrot->encrypted_janus_anchor;
    const crypto::public_key &onetime_address = o_carrot->key;

    // 8. anchor = m_anchor XOR anchor_enc
    etd.janus_anchor = carrot::decrypt_carrot_anchor(encrypted_janus_anchor, s_sender_receiver, onetime_address);

    // 9. treat as selfsend iff special janus check passes
    etd.flags.m_selfsend = 0;
    etd.selfsend_enote_ephemeral_pubkey = mx25519_pubkey{};
    if (!etd.flags.m_coinbase)
    {
        const bool is_special = carrot::verify_carrot_special_janus_protection(etd.tx_first_key_image,
            enote_ephemeral_pubkey,
            onetime_address,
            addr_dev,
            etd.janus_anchor);
        if (is_special)
        {
            etd.flags.m_selfsend = is_special;
            etd.selfsend_enote_ephemeral_pubkey = enote_ephemeral_pubkey;
        }
    }

    // 10. C_a = k_a G + a H
    const rct::key amount_commitment = rct::commit(td.amount(), td.m_mask);

    // 11. K^j_s = Ko - K^o_ext = Ko - (k^o_g G + k^o_t U)
    crypto::public_key address_spend_pubkey;
    carrot::recover_address_spend_pubkey(onetime_address,
        s_sender_receiver,
        amount_commitment,
        address_spend_pubkey);
    const bool is_subaddress = address_spend_pubkey != addr_dev.get_cryptonote_account_spend_pubkey();
    //!@TODO: check nominal subaddress spend pubkey against real one derived from addr_dev

    // 12. calc k_a assuming enote_type="change": k_a' = H_n(s^ctx_sr, a, K^j_s, "change")
    crypto::secret_key change_amount_blinding_factor;
    carrot::make_carrot_amount_blinding_factor(s_sender_receiver,
        td.amount(),
        address_spend_pubkey,
        carrot::CarrotEnoteType::CHANGE,
        change_amount_blinding_factor);

    // 13. if k_a' == k_a, then it IS true that enote_type="change"
    etd.flags.m_enote_type_change = rct::sk2rct(change_amount_blinding_factor) == td.m_mask;

    // 14. pid decrypting and setting flag
    if (encrypted_payment_id && !etd.flags.m_selfsend)
    {
        // pid = m_pid XOR pid_enc
        etd.payment_id = carrot::decrypt_legacy_payment_id(*encrypted_payment_id,
            s_sender_receiver,
            onetime_address);

        // do normal janus verification and reset PID if d_e is null-bound
        CHECK_AND_ASSERT_THROW_MES(
            carrot::verify_carrot_normal_janus_protection(input_context,
                address_spend_pubkey,
                is_subaddress,
                enote_ephemeral_pubkey,
                etd.janus_anchor,
                etd.payment_id),
            "cannot export transfer details: normal janus check failed");

        etd.flags.m_has_pid = etd.payment_id != carrot::null_payment_id;
    }
    else // no encrypted payment ID in tx or is selfsend
    {
        etd.flags.m_has_pid = false;
        etd.payment_id = carrot::null_payment_id;
    }

    return etd;
}
//-------------------------------------------------------------------------------------------------------------------
exported_transfer_details_variant export_cold_output(const wallet2_basic::transfer_details &td,
    const carrot::cryptonote_hierarchy_address_device &addr_dev)
{
    exported_transfer_details_variant etd_v;
    if (carrot::is_carrot_transaction_v1(td.m_tx))
        etd_v = export_cold_carrot_output(td, addr_dev);
    else // not carrot
        etd_v = export_cold_pre_carrot_output(td);

    return etd_v;
}
//-------------------------------------------------------------------------------------------------------------------
wallet2_basic::transfer_details import_cold_pre_carrot_output(const exported_pre_carrot_transfer_details &etd,
    const cryptonote::account_keys &acc_keys)
{
    hw::device &hwdev = acc_keys.get_device();

    wallet2_basic::transfer_details td{};

    // setup td with "cheap" loaded data
    td.m_block_height = 0;
    td.m_txid = crypto::null_hash;
    td.m_global_output_index = etd.m_global_output_index;
    td.m_spent = etd.m_flags.m_spent;
    td.m_frozen = etd.m_flags.m_frozen;
    td.m_spent_height = 0;
    td.m_amount = etd.m_amount;
    td.m_rct = etd.m_flags.m_rct;
    td.m_key_image_known = etd.m_flags.m_key_image_known;
    td.m_key_image_request = etd.m_flags.m_key_image_request;
    td.m_key_image_partial = false;
    td.m_subaddr_index.major = etd.m_subaddr_index_major;
    td.m_subaddr_index.minor = etd.m_subaddr_index_minor;

    // construct a synthetic tx prefix that has the info we'll need: the output with its pubkey, the tx pubkey in extra
    td.m_tx = {};

    THROW_WALLET_EXCEPTION_IF(etd.m_internal_output_index >= 65536, error::wallet_internal_error, "internal output index seems outrageously high, rejecting");
    td.m_internal_output_index = etd.m_internal_output_index;
    cryptonote::txout_to_key tk;
    tk.key = etd.m_pubkey;
    cryptonote::tx_out out;
    out.amount = etd.m_amount;
    out.target = tk;
    td.m_tx.vout.resize(etd.m_internal_output_index);
    td.m_tx.vout.push_back(out);

    td.m_pk_index = 0;
    cryptonote::add_tx_pub_key_to_extra(td.m_tx, etd.m_tx_pubkey);
    if (!etd.m_additional_tx_keys.empty())
      cryptonote::add_additional_tx_pub_keys_to_extra(td.m_tx.extra, etd.m_additional_tx_keys);

    // the hot wallet wouldn't have known about key images (except if we already exported them)
    cryptonote::keypair in_ephemeral;
    const crypto::public_key &tx_pub_key = etd.m_tx_pubkey;
    const std::vector<crypto::public_key> &additional_tx_pub_keys = etd.m_additional_tx_keys;
    const crypto::public_key& out_key = etd.m_pubkey;
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> mini_subaddress_map{
        { hwdev.get_subaddress_spend_public_key(acc_keys, td.m_subaddr_index), td.m_subaddr_index }
    };
    const bool r = cryptonote::generate_key_image_helper(acc_keys,
        mini_subaddress_map,
        out_key,
        tx_pub_key,
        additional_tx_pub_keys,
        td.m_internal_output_index,
        in_ephemeral,
        td.m_key_image,
        hwdev);
    THROW_WALLET_EXCEPTION_IF(!r, error::wallet_internal_error, "Failed to generate key image");

    // get amount blinding factor if RingCT
    if (td.m_rct)
    {
        crypto::key_derivation kd;
        CHECK_AND_ASSERT_THROW_MES(hwdev.generate_key_derivation(tx_pub_key, acc_keys.m_view_secret_key, kd),
            "could not import transfer details: ECDH key derivation failed");
        
        crypto::secret_key amount_key;
        CHECK_AND_ASSERT_THROW_MES(hwdev.derivation_to_scalar(kd, td.m_internal_output_index, amount_key),
            "could not import transfer details: derivation to scalar failed");

        td.m_mask = rct::genCommitmentMask(rct::sk2rct(amount_key));
    }

    td.m_key_image_known = true;
    td.m_key_image_request = true;
    td.m_key_image_partial = false;
    THROW_WALLET_EXCEPTION_IF(in_ephemeral.pub != out_key,
        error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key at index ");

    return td;
}
//-------------------------------------------------------------------------------------------------------------------
wallet2_basic::transfer_details import_cold_carrot_output(const exported_carrot_transfer_details &etd,
    const cryptonote::account_keys &acc_keys)
{
    hw::device &hwdev = acc_keys.get_device();

    wallet2_basic::transfer_details td{};

    td.m_block_height = 0;
    td.m_tx.set_null();
    td.m_txid = crypto::null_hash;
    td.m_internal_output_index = 0;
    td.m_global_output_index = 0;
    td.m_spent = etd.flags.m_spent;
    td.m_frozen = etd.flags.m_frozen;
    td.m_spent_height = 0;
    td.m_amount = etd.amount;
    td.m_rct = true;
    td.m_key_image_known = etd.flags.m_key_image_known;
    td.m_key_image_request = etd.flags.m_key_image_request;
    td.m_pk_index = 0;
    td.m_subaddr_index.major = etd.subaddr_index.major;
    td.m_subaddr_index.minor = etd.subaddr_index.minor;
    td.m_key_image_partial = etd.flags.m_key_image_partial;
    td.m_multisig_k.clear();
    td.m_multisig_info.clear();
    td.m_uses.clear();

    // get receive subaddress
    CHECK_AND_ASSERT_THROW_MES(etd.flags.m_carrot_derived_addr,
        "cannot import transfer details: carrot key hierarchy addresses are not yet supported"); //! @TODO
    const cryptonote::account_public_address receive_addr = hwdev.get_subaddress(acc_keys, td.m_subaddr_index);
    const bool is_subaddress = receive_addr.m_spend_public_key != acc_keys.m_account_address.m_spend_public_key;
    const carrot::CarrotDestinationV1 destination{
        .address_spend_pubkey = receive_addr.m_spend_public_key,
        .address_view_pubkey = receive_addr.m_view_public_key,
        .is_subaddress = is_subaddress,
        .payment_id = etd.flags.m_has_pid ? etd.payment_id : carrot::null_payment_id
    };

    // Use exported_carrot_transfer_details to make payment proposals to ourselves,
    // then construct transaction outputs and set amount blinding factor
    if (etd.flags.m_coinbase)
    {
        const carrot::CarrotPaymentProposalV1 payment_proposal{
            .destination = destination,
            .amount = td.amount(),
            .randomness = etd.janus_anchor
        };

        carrot::CarrotCoinbaseEnoteV1 enote;
        carrot::get_coinbase_output_proposal_v1(payment_proposal, etd.block_index, enote);
        td.m_tx = carrot::store_carrot_to_coinbase_transaction_v1({enote}, {});
        td.m_mask = rct::I;
    }
    else // non-coinbase
    {
        carrot::RCTOutputEnoteProposal output_enote_proposal;
        std::optional<carrot::encrypted_payment_id_t> encrypted_payment_id;
        if (etd.flags.m_selfsend)
        {
            CHECK_AND_ASSERT_THROW_MES(!etd.flags.m_internal,
                "cannot import transfer details: carrot key hierarchy addresses are not yet supported"); //! @TODO");

            const carrot::CarrotEnoteType enote_type = etd.flags.m_enote_type_change
                ? carrot::CarrotEnoteType::CHANGE : carrot::CarrotEnoteType::PAYMENT;

            const carrot::CarrotPaymentProposalSelfSendV1 payment_proposal{
                .destination_address_spend_pubkey = destination.address_spend_pubkey,
                .amount = td.amount(),
                .enote_type = enote_type,
                .enote_ephemeral_pubkey = etd.selfsend_enote_ephemeral_pubkey,
                .internal_message = etd.janus_anchor
            };

            //! @TODO: HW device k_view
            const carrot::view_incoming_key_ram_borrowed_device k_view_dev(acc_keys.m_view_secret_key);

            // construct enote
            carrot::get_output_proposal_special_v1(payment_proposal,
                k_view_dev,
                etd.tx_first_key_image,
                etd.selfsend_enote_ephemeral_pubkey,
                output_enote_proposal);
        }
        else // normal non-coinbase
        {
            const carrot::CarrotPaymentProposalV1 payment_proposal{
                .destination = destination,
                .amount = td.amount(),
                .randomness = etd.janus_anchor
            };

            carrot::get_output_proposal_normal_v1(payment_proposal,
                etd.tx_first_key_image,
                output_enote_proposal,
                encrypted_payment_id.emplace());
        }

        td.m_tx = carrot::store_carrot_to_transaction_v1({output_enote_proposal.enote},
            {etd.tx_first_key_image},
            /*fee=*/0,
            encrypted_payment_id.value_or(carrot::encrypted_payment_id_t{}));
        td.m_mask = rct::sk2rct(output_enote_proposal.amount_blinding_factor);
    }

    // Now all that's left to do is calculate the key image. First do a view-incoming scan, and then derive
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> mini_subaddress_map{
        { receive_addr.m_spend_public_key, td.m_subaddr_index }
    };
    const std::optional<enote_view_incoming_scan_info_t> enote_scan_info = view_incoming_scan_enote_from_prefix(td.m_tx,
        td.amount(),
        td.m_mask,
        /*local_output_index=*/0,
        receive_addr,
        acc_keys.m_view_secret_key,
        mini_subaddress_map,
        hwdev);
    CHECK_AND_ASSERT_THROW_MES(enote_scan_info,
        "cannot import transfer details: minimal recomputed tx prefix from carrot export failed to scan");

    const std::optional<crypto::key_image> ki = try_derive_enote_key_image(*enote_scan_info, acc_keys);
    CHECK_AND_ASSERT_THROW_MES(ki,
        "cannot import transfer details: key image could not be calculated for provided Carrot enote");

    td.m_key_image = *ki;

    return td;
}
//-------------------------------------------------------------------------------------------------------------------
wallet2_basic::transfer_details import_cold_output(const exported_transfer_details_variant &etd,
    const cryptonote::account_keys &acc_keys)
{
    struct import_cold_output_visitor
    {
        wallet2_basic::transfer_details operator()(const exported_pre_carrot_transfer_details &etd) const
        {
           return import_cold_pre_carrot_output(etd, acc_keys); 
        }

        wallet2_basic::transfer_details operator()(const exported_carrot_transfer_details &etd) const
        {
           return import_cold_carrot_output(etd, acc_keys); 
        }

        const cryptonote::account_keys &acc_keys;
    };

    return std::visit(import_cold_output_visitor{acc_keys}, etd);
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace wallet
} //namespace tools
