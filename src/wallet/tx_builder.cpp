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

//paired header
#include "tx_builder.h"

//local headers
#include "misc_log_ex.h"
#include "common/apply_permutation.h"
#include "crypto/crypto.h"
#include "crypto/generators.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/subaddress_index.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "cryptonote_config.h"
#include "device/device_default.hpp"
#include "ringct/rctOps.h"
#include "ringct/rctTypes.h"
#include "wallet2.h"
#include "wallet2_basic/wallet2_types.h"

//third party headers
#include <boost/algorithm/string/join.hpp>
#include <boost/multiprecision/cpp_int.hpp>

//standard headers
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.tx_builder"

namespace tools
{
namespace wallet
{
//-------------------------------------------------------------------------------------------------------------------
static void recontruct_tx_pubkeys(
    const crypto::secret_key &k_view,
    const cryptonote::tx_destination_entry &change,
    const std::vector<cryptonote::tx_destination_entry> &dests,
    const crypto::secret_key &tx_key,
    const std::vector<crypto::secret_key> &additional_tx_keys,
    crypto::public_key &tx_pubkey_out,
    std::vector<crypto::public_key> &tx_additional_pubkeys_out,
    std::vector<crypto::key_derivation> &derivations_out)
{
    tx_pubkey_out = crypto::public_key{};
    tx_additional_pubkeys_out.clear();
    derivations_out.clear();
    derivations_out.reserve(dests.size());

    // Check if multiple keys are expected
    // Note: this is the same method used in `construct_tx_and_get_tx_key()`, so we should expect the same results.
    size_t num_stdaddresses = 0;
    size_t num_subaddresses = 0;
    cryptonote::account_public_address shared_keys_base = cryptonote::account_public_address{
        .m_spend_public_key = crypto::get_G(),
        .m_view_public_key = crypto::get_G(),
    };
    cryptonote::classify_addresses(dests, change.addr, num_stdaddresses, num_subaddresses, shared_keys_base);
    const bool need_additional_txkeys = num_subaddresses > 0 && (num_stdaddresses > 0 || num_subaddresses > 1);

    if (!need_additional_txkeys)
    {
        CHECK_AND_ASSERT_THROW_MES(additional_tx_keys.size() == 0,
            "recontruct_tx_pubkeys: unexpected additional tx keys for 2-out");

        // Collect tx pubkey
        tx_pubkey_out = rct::rct2pk(rct::scalarmultKey(
            rct::pk2rct(shared_keys_base.m_spend_public_key),
            rct::sk2rct(tx_key)
        ));

        // Collect derivations
        for (const auto &dest : dests)
        {
            if (dest.addr == change.addr)
            {
                CHECK_AND_ASSERT_THROW_MES(crypto::generate_key_derivation(tx_pubkey_out,
                    k_view,
                    derivations_out.emplace_back()),
                    "recontruct_tx_pubkeys: failed key derivation for change out in 2-out");
            }
            else
            {
                CHECK_AND_ASSERT_THROW_MES(crypto::generate_key_derivation(dest.addr.m_view_public_key,
                    tx_key,
                    derivations_out.emplace_back()),
                    "recontruct_tx_pubkeys: failed key derivation for non-change out in 2-out");
            }
        }
    }
    else
    {
        CHECK_AND_ASSERT_THROW_MES(dests.size() == additional_tx_keys.size(),
            "recontruct_tx_pubkeys: tx key alignment failure");

        for (size_t i = 0; i < dests.size(); ++i)
        {
            // Collect tx pubkey
            if (dests.at(i).is_subaddress)
            {
                tx_additional_pubkeys_out.push_back(rct::rct2pk(rct::scalarmultKey(
                    rct::pk2rct(dests.at(i).addr.m_spend_public_key),
                    rct::sk2rct(additional_tx_keys.at(i))
                )));
            }
            else
            {
                tx_additional_pubkeys_out.push_back(
                    rct::rct2pk(rct::scalarmultBase(rct::sk2rct(additional_tx_keys.at(i))))
                );
            }

            // Collect derivation
            CHECK_AND_ASSERT_THROW_MES(crypto::generate_key_derivation(dests.at(i).addr.m_view_public_key,
                tx_key,
                derivations_out.emplace_back()),
                "recontruct_tx_pubkeys: failed key derivation for out in >2-out");
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
static void reconstruct_payment_id(const std::string &extracted_payment_id,
    const std::optional<crypto::hash8> &parsed_to_check,
    // const std::optional<crypto::hash8> &pre_decrypted_to_check,
    const cryptonote::account_public_address &change_addr,
    const std::vector<cryptonote::tx_destination_entry> &dests,
    const crypto::secret_key &tx_key,
    const std::vector<cryptonote::tx_extra_field> &tx_extra_fields)
{
    // Get the destination that will be able to read the payment id in the final tx.
    // Returns non-null if there is exactly one destination (may or may not be the change addr).
    const crypto::public_key view_key_pub = cryptonote::get_destination_view_key_pub(dests, change_addr);
    CHECK_AND_ASSERT_THROW_MES(view_key_pub != crypto::null_pkey,
        "reconstruct_payment_id: encrypted payment IDs must only be in txs with one destination");

    // Extract the expected encrypted payment id.
    cryptonote::tx_extra_nonce extra_nonce;
    crypto::hash8 encrypted_payment_id8 = crypto::null_hash8;
    CHECK_AND_ASSERT_THROW_MES(cryptonote::find_tx_extra_field_by_type(tx_extra_fields, extra_nonce),
        "reconstruct_payment_id: expected payment id is missing");
    CHECK_AND_ASSERT_THROW_MES(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(
            extra_nonce.nonce,
            encrypted_payment_id8
        ),
        "reconstruct_payment_id: expected payment id is missing");
 
    // Parse the payment ID string extracted from the address.
    crypto::hash8 to_encrypt_payment_id;
    CHECK_AND_ASSERT_THROW_MES(tools::wallet2::parse_short_payment_id(extracted_payment_id, to_encrypt_payment_id),
        "reconstruct_payment_id: failed parsing short payment id from integrated address");
    if (parsed_to_check)
    {
        CHECK_AND_ASSERT_THROW_MES(*parsed_to_check == to_encrypt_payment_id,
            "reconstruct_payment_id: did not extract the same parsed payment id");
    }
    // if (pre_decrypted_to_check)
    // {
    //     CHECK_AND_ASSERT_THROW_MES(*pre_decrypted_to_check == to_encrypt_payment_id,
    //         "reconstruct_payment_id: did not extract the same pre-decrypted payment id");
    // }

    // Encrypt the address's payment ID.
    CHECK_AND_ASSERT_THROW_MES(hw::core::device_default().encrypt_payment_id(to_encrypt_payment_id, view_key_pub, tx_key),
        "reconstruct_payment_id: failed encrypting payment id");

    // Check equivalence.
    CHECK_AND_ASSERT_THROW_MES(to_encrypt_payment_id == encrypted_payment_id8,
        "reconstruct_payment_id: failed encrypting payment id");
}
//-------------------------------------------------------------------------------------------------------------------
void sanity_check_pending_tx(const wallet2::pending_tx &ptx,
    const cryptonote::network_type nettype,
    const cryptonote::account_keys &account_keys,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddresses,
    const std::vector<wallet2_basic::transfer_details> &transfers,
    const bool redacted)
{
    const auto &construct = ptx.construction_data;

    // Extra
    std::vector<cryptonote::tx_extra_field> tx_extra_fields;
    CHECK_AND_ASSERT_THROW_MES(cryptonote::parse_tx_extra(ptx.tx.extra, tx_extra_fields),
        "sanity_check_pending_tx: ptx.tx.extra extraction failure");
    // NOTE: Due to upstream chaos, we are unable to reliably validate `construct.extra`.
    // if (!cleartext_payment_id)
    // {
    //     CHECK_AND_ASSERT_THROW_MES(construct.extra == ptx.tx.extra,
    //         "sanity_check_pending_tx: tx_extra mismatch");
    // }
    // else
    // {
    //     std::vector<uint8_t> extra_clone_unencrypted = construct.extra;
    //     std::vector<uint8_t> extra_clone_encrypted = ptx.tx.extra;
    //     cryptonote::remove_field_from_tx_extra(extra_clone_unencrypted, typeid(cryptonote::tx_extra_nonce));
    //     cryptonote::remove_field_from_tx_extra(extra_clone_encrypted, typeid(cryptonote::tx_extra_nonce));
    //     CHECK_AND_ASSERT_THROW_MES(extra_clone_unencrypted == extra_clone_encrypted,
    //         "sanity_check_pending_tx: tx_extra mismatch w/ payment id removal");

    //     // The decrypted payment ID will be checked manually later.
    // }

    std::vector<crypto::key_derivation> reconstruct_derivations;
    if (!redacted)
    {
        crypto::public_key reconstruct_pubkey;
        std::vector<crypto::public_key> reconstruct_additional_pubkeys;

        recontruct_tx_pubkeys(
            account_keys.m_view_secret_key,
            construct.change_dts,
            construct.splitted_dsts,
            ptx.tx_key,
            ptx.additional_tx_keys,
            reconstruct_pubkey,
            reconstruct_additional_pubkeys,
            reconstruct_derivations
        );

        CHECK_AND_ASSERT_THROW_MES(reconstruct_derivations.size() == construct.splitted_dsts.size(),
            "sanity_check_pending_tx: failed checking derivations size [local bug]");

        cryptonote::tx_extra_pub_key tx_extra_pub_key;
        CHECK_AND_ASSERT_THROW_MES(cryptonote::find_tx_extra_field_by_type(tx_extra_fields, tx_extra_pub_key),
            "sanity_check_pending_tx: tx_extra missing tx pub key");
        if (construct.dests.size() == 2 || reconstruct_additional_pubkeys.size() == 0)
        {
            CHECK_AND_ASSERT_THROW_MES(tx_extra_pub_key.pub_key == reconstruct_pubkey,
                "sanity_check_pending_tx: tx_extra unable to reproduce tx pubkey");
        }
        else
        {
            cryptonote::tx_extra_additional_pub_keys tx_extra_additional_pub_keys;
            CHECK_AND_ASSERT_THROW_MES(cryptonote::find_tx_extra_field_by_type(tx_extra_fields, tx_extra_additional_pub_keys),
                "sanity_check_pending_tx: tx_extra missing extra tx pub keys");
            CHECK_AND_ASSERT_THROW_MES(tx_extra_additional_pub_keys.data.size() == reconstruct_additional_pubkeys.size(),
                "sanity_check_pending_tx: tx_extra extra tx pub keys size mismatch");
            for (size_t p = 0; p < reconstruct_additional_pubkeys.size(); ++p)
            {
                const auto &extra_pk = tx_extra_additional_pub_keys.data.at(p);
                const auto &re_pk = reconstruct_additional_pubkeys.at(p);
                CHECK_AND_ASSERT_THROW_MES(extra_pk == re_pk,
                    "sanity_check_pending_tx: tx_extra unable to reproduce extra tx pubkey");
            }
        }
    }

    // Extract tx inputs
    std::vector<crypto::key_image> ext_key_images;
    std::vector<rct::xmr_amount> ext_input_amounts;
    const bool all_are_txin_to_key = std::all_of(ptx.tx.vin.begin(), ptx.tx.vin.end(),
        [&](const cryptonote::txin_v& s_e) -> bool
        {
            CHECKED_GET_SPECIFIC_VARIANT(s_e, const cryptonote::txin_to_key, in, false);
            ext_key_images.push_back(in.k_image);
            ext_input_amounts.push_back(in.amount);
            return true;
        }
    );
    std::string ext_key_images_str;
    for (const auto &ki : ext_key_images)
    {
        ext_key_images_str += boost::to_string(ki) + " ";
    }
    CHECK_AND_ASSERT_THROW_MES(all_are_txin_to_key,
        "sanity_check_pending_tx: all inputs are not txin_to_key");
    CHECK_AND_ASSERT_THROW_MES(ptx.key_images == ext_key_images_str,
        "sanity_check_pending_tx: failed reconstructing key_images field");

    // Inputs
    // - We don't check the validity of tx_source_entry::real_out_additional_tx_keys.
    CHECK_AND_ASSERT_THROW_MES(construct.sources.size() == ptx.tx.vin.size(),
        "sanity_check_pending_tx: source/vin size mismatch");
    // NOTE: due to upstream inconsistencies, we are unable to validate mixRing reliably
    // if (recovered_from_serialized)
    // {
    //     CHECK_AND_ASSERT_THROW_MES(ptx.tx.rct_signatures.mixRing.size() == 0,
    //         "sanity_check_pending_tx: ptx is recovered from serialized but mixRing.size() != 0");
    // }
    // else
    // {
    //     CHECK_AND_ASSERT_THROW_MES(construct.sources.size() == ptx.tx.rct_signatures.mixRing.size(),
    //         "sanity_check_pending_tx: source(" << construct.sources.size() << ")/mixring("
    //         << ptx.tx.rct_signatures.mixRing.size() << ") size mismatch");
    // }
    CHECK_AND_ASSERT_THROW_MES(construct.sources.size() == ptx.selected_transfers.size(),
        "sanity_check_pending_tx: selected_transfers invalid size");
    CHECK_AND_ASSERT_THROW_MES(construct.selected_transfers == ptx.selected_transfers,
        "sanity_check_pending_tx: selected_transfers inconsistent");
    // - For SOME REASON, construction data sources are not always in the same order as inputs, so we need to fix that
    auto sources_ordered = construct.sources;
    std::vector<size_t> ins_order;
    for (const size_t selected_transfer : ptx.selected_transfers)
    {
        CHECK_AND_ASSERT_THROW_MES(selected_transfer < transfers.size(),
            "sanity_check_pending_tx: invalid transfers index");
        const auto &transfer = transfers.at(selected_transfer);

        for (size_t i = 0; i < sources_ordered.size(); ++i)
        {
            const auto &src = sources_ordered.at(i);
            CHECK_AND_ASSERT_THROW_MES(src.real_output < src.outputs.size(),
                "sanity_check_pending_tx: ring sig index " << src.real_output << " out of input set size "
                << src.outputs.size());
            if (src.outputs[src.real_output].second.dest == rct::pk2rct(transfer.get_public_key()))
                ins_order.push_back(i);
        }
    }
    CHECK_AND_ASSERT_THROW_MES(ins_order.size() == sources_ordered.size(),
        "sanity_check_pending_tx: onetime addr mismatch between sources and tx");
    tools::apply_permutation(ins_order, sources_ordered);

    boost::multiprecision::uint128_t input_amnt = 0;

    for (size_t i = 0; i < ptx.tx.vin.size(); ++i)
    {
        const auto &selected_transfer = ptx.selected_transfers.at(i);
        CHECK_AND_ASSERT_THROW_MES(selected_transfer < transfers.size(),
            "sanity_check_pending_tx: invalid transfers index");

        const auto &src = sources_ordered.at(i);
        const auto &transfer = transfers.at(selected_transfer);

        CHECK_AND_ASSERT_THROW_MES(src.rct,
            "sanity_check_pending_tx: tx is not marked 'rct'");
        CHECK_AND_ASSERT_THROW_MES(ext_input_amounts.at(i) == 0,
            "sanity_check_pending_tx: rct amount extracted from inputs should be zero");
        CHECK_AND_ASSERT_THROW_MES(src.amount == transfer.m_amount,
            "sanity_check_pending_tx: transfer amount mismatch");
        CHECK_AND_ASSERT_THROW_MES(src.real_output < src.outputs.size(),
            "sanity_check_pending_tx: ring sig index out of input set size");
        const rct::key commitment = rct::commit(src.amount, src.mask);
        CHECK_AND_ASSERT_THROW_MES(src.outputs[src.real_output].second.mask == commitment,
            "sanity_check_pending_tx: failed reproducing real input's amount commitment");

        // NOTE: due to upstream inconsistencies, we are unable to validate mixRing reliably.
        // if (!recovered_from_serialized)
        // {
        //     const auto &input_ring = ptx.tx.rct_signatures.mixRing.at(i);
        //     CHECK_AND_ASSERT_THROW_MES(input_ring.size() == src.outputs.size(),
        //         "sanity_check_pending_tx: input ring size mismatch");
        //     for (size_t r = 0; r < input_ring.size(); ++r)
        //     {
        //         CHECK_AND_ASSERT_THROW_MES(input_ring.at(r) == src.outputs.at(r).second,
        //             "sanity_check_pending_tx: input ring member mismatch");
        //     }
        // }

        CHECK_AND_ASSERT_THROW_MES(!transfer.m_spent,
            "sanity_check_pending_tx: transfer - is marked as spent");
        CHECK_AND_ASSERT_THROW_MES(!transfer.m_frozen,
            "sanity_check_pending_tx: transfer - is marked as frozen");
        // NOTE: due to upstream ambiguity we are unable to reliably check that transfer key images line up with
        // specified inputs. For example, cold wallets may have the spend key but don't track key images (at least
        // one test fails here because of that).
        // if (!maybe_view_wallet)
        // {
        //     CHECK_AND_ASSERT_THROW_MES(transfer.m_key_image_known,
        //         "sanity_check_pending_tx: transfer - KI is unknown");
        //     CHECK_AND_ASSERT_THROW_MES(ext_key_images.at(i) == transfer.m_key_image,
        //         "sanity_check_pending_tx: transfer - KI mismatch");
        // }
        CHECK_AND_ASSERT_THROW_MES(src.rct == transfer.m_rct,
            "sanity_check_pending_tx: transfer - 'is rct' mismatch");
        CHECK_AND_ASSERT_THROW_MES(src.mask == transfer.m_mask,
            "sanity_check_pending_tx: transfer - mask mismatch");
        CHECK_AND_ASSERT_THROW_MES(src.real_output_in_tx_index == transfer.m_internal_output_index,
            "sanity_check_pending_tx: transfer - input's output-set index mismatch");
        CHECK_AND_ASSERT_THROW_MES(src.outputs[src.real_output].second.dest == rct::pk2rct(transfer.get_public_key()),
            "sanity_check_pending_tx: transfer - onetime addr mismatch");

        input_amnt += src.amount;
    }

    CHECK_AND_ASSERT_THROW_MES(input_amnt <= UINT64_MAX,
        "sanity_check_pending_tx: input amount > 2^64 - 1");

    // Extract outputs
    std::vector<cryptonote::txout_to_tagged_key> ext_outputs;
    std::vector<rct::xmr_amount> ext_output_encoded_amnts;
    const bool all_are_txout_to_tagged_key = std::all_of(ptx.tx.vout.begin(), ptx.tx.vout.end(),
        [&](const cryptonote::tx_out& s_e) -> bool
        {
            CHECKED_GET_SPECIFIC_VARIANT(s_e.target, const cryptonote::txout_to_tagged_key, out, false);
            ext_output_encoded_amnts.push_back(s_e.amount);
            ext_outputs.push_back(out);
            return true;
        }
    );
    CHECK_AND_ASSERT_THROW_MES(all_are_txout_to_tagged_key,
        "sanity_check_pending_tx: all outputs should have view tags");
    CHECK_AND_ASSERT_THROW_MES(construct.splitted_dsts.size() == ptx.tx.vout.size(),
        "sanity_check_pending_tx: destination vecs are inconsistent");

    // Check change address
    std::optional<cryptonote::subaddress_index> repro_index;
    repro_index = cryptonote::sanity_check_change_address(ptx.change_dts.addr, subaddresses, account_keys);
    CHECK_AND_ASSERT_THROW_MES(repro_index,
        "sanity_check_pending_tx: failed reproducing the change address");

    // Destination consistency
    CHECK_AND_ASSERT_THROW_MES(construct.change_dts == ptx.change_dts,
        "sanity_check_pending_tx: change_dts inconsistent");
    CHECK_AND_ASSERT_THROW_MES(construct.dests == ptx.dests,
        "sanity_check_pending_tx: destination vecs are inconsistent");
    std::vector<cryptonote::tx_destination_entry> splitted_dsts_repro = ptx.dests;
    if (ptx.change_dts.amount > 0 || ptx.dests.size() == 1)
    {
        splitted_dsts_repro.push_back(ptx.change_dts);
    }
    CHECK_AND_ASSERT_THROW_MES(construct.splitted_dsts.size() == splitted_dsts_repro.size(),
        "sanity_check_pending_tx: failed checking splitted_dsts size");
    CHECK_AND_ASSERT_THROW_MES(ptx.tx.rct_signatures.ecdhInfo.size() == splitted_dsts_repro.size(),
        "sanity_check_pending_tx: ecdhInfo size mismatch");
    CHECK_AND_ASSERT_THROW_MES(ptx.tx.rct_signatures.outPk.size() == splitted_dsts_repro.size(),
        "sanity_check_pending_tx: outPk size mismatch");

    boost::multiprecision::uint128_t output_amnt = 0;
    size_t integrated_count = 0;

    for (size_t i = 0; i < construct.splitted_dsts.size(); ++i)
    {
        const auto &dest = construct.splitted_dsts.at(i);

        // Check original dst string
        if (dest.original.size() > 0)
        {
            std::string address;
            std::string extracted_payment_id;
            uint64_t _amount;
            std::string _tx_description;
            std::string _recipient_name;
            std::vector<std::string> _unknown_parameters;
            std::string _error;
            wallet2::parse_uri_impl(dest.original,
                nettype,
                address,
                extracted_payment_id,
                _amount,
                _tx_description,
                _recipient_name,
                _unknown_parameters,
                _error);

            // The `address` string may be an explicit address OR a URI that we can't access or assess here.
            cryptonote::address_parse_info parse_info;
            const bool parsed_info = cryptonote::get_account_address_from_str(parse_info, nettype, address);
            if (parsed_info)
            {
                CHECK_AND_ASSERT_THROW_MES(parse_info.is_subaddress == dest.is_subaddress,
                    "sanity_check_pending_tx: destination addr string mismatch - is_subaddress");
                CHECK_AND_ASSERT_THROW_MES(parse_info.has_payment_id == dest.is_integrated,
                    "sanity_check_pending_tx: destination addr string mismatch - is_integrated");
                CHECK_AND_ASSERT_THROW_MES(parse_info.address == dest.addr,
                    "sanity_check_pending_tx: destination addr string mismatch - address");
            }

            if (dest.is_integrated)
            {
                ++integrated_count;
                CHECK_AND_ASSERT_THROW_MES(integrated_count == 1,
                    "sanity_check_pending_tx: more than one integrated address detected");

                // NOTE: Due to upstream chaos, we are unable to reliably validate the construction data's payment ID.
                // `tx_construction_data::extra` is *sometimes* pre-decrypted, so we handle it here by extracting and
                // comparing directly with our decrypted version.
                // std::optional<crypto::hash8> decrypted_payment_id8 = std::nullopt;
                // if (cleartext_payment_id)
                // {
                //     std::vector<cryptonote::tx_extra_field> construct_tx_extra_fields;
                //     CHECK_AND_ASSERT_THROW_MES(cryptonote::parse_tx_extra(construct.extra, construct_tx_extra_fields),
                //         "sanity_check_pending_tx: construct.extra extraction failure");

                //     cryptonote::tx_extra_nonce extra_nonce;
                //     CHECK_AND_ASSERT_THROW_MES(cryptonote::find_tx_extra_field_by_type(construct_tx_extra_fields,
                //         extra_nonce),
                //         "reconstruct_payment_id: expected decrypted payment id is missing");
                //     CHECK_AND_ASSERT_THROW_MES(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(
                //             extra_nonce.nonce,
                //             *decrypted_payment_id8
                //         ),
                //         "reconstruct_payment_id: expected decrypted payment id is missing");
                // }

                reconstruct_payment_id(extracted_payment_id,
                    parsed_info ? std::optional<crypto::hash8>(parse_info.payment_id) : std::nullopt,
                    // decrypted_payment_id8,
                    ptx.change_dts.addr,
                    construct.splitted_dsts,
                    ptx.tx_key,
                    tx_extra_fields);
            }
        }

        // Check reproduced dests
        const auto &it = std::find_if(splitted_dsts_repro.cbegin(), splitted_dsts_repro.cend(),
            [&dest](const auto &a) {
                return a == dest;
            }
        );
        CHECK_AND_ASSERT_THROW_MES(it != splitted_dsts_repro.cend(),
            "sanity_check_pending_tx: failed checking splitted_dsts consistency");

        // Carefully erase found copies one by one in case of duplicates.
        splitted_dsts_repro.erase(it);

        if (!redacted)
        {
            // Reproduce tx output
            const auto &derivation = reconstruct_derivations.at(i);
            crypto::ec_scalar derivation_scalar;
            crypto::derivation_to_scalar(derivation, i, derivation_scalar);

            // - Ko
            crypto::public_key repro_onetime_addr;
            CHECK_AND_ASSERT_THROW_MES(crypto::derive_public_key(derivation,
                i,
                dest.addr.m_spend_public_key,
                repro_onetime_addr),
                "sanity_check_pending_tx: failed constructing onetime address");
            CHECK_AND_ASSERT_THROW_MES(repro_onetime_addr == ext_outputs.at(i).key,
                "sanity_check_pending_tx: failed reproducing onetime address");
            // - view tag
            crypto::view_tag repro_view_tag;
            crypto::derive_view_tag(derivation, i, repro_view_tag);
            CHECK_AND_ASSERT_THROW_MES(repro_view_tag == ext_outputs.at(i).view_tag,
                "sanity_check_pending_tx: failed reproducing view tag");
            // - encoded amount
            rct::ecdhTuple amnt_data{};
            memcpy(amnt_data.amount.bytes, &dest.amount, sizeof(dest.amount));
            rct::ecdhDecode(amnt_data, (const rct::key&)(derivation_scalar), true); //v2, decode gives mask
            CHECK_AND_ASSERT_THROW_MES(ptx.tx.rct_signatures.ecdhInfo.at(i).amount == amnt_data.amount,
                "sanity_check_pending_tx: failed reproducing encoded amount");
            // - C
            const rct::key repro_C = rct::commit(dest.amount, amnt_data.mask);
            CHECK_AND_ASSERT_THROW_MES(ptx.tx.rct_signatures.outPk.at(i).mask == repro_C,
                "sanity_check_pending_tx: failed reproducing amount commitment");
        }

        output_amnt += dest.amount;
    }
    CHECK_AND_ASSERT_THROW_MES(splitted_dsts_repro.size() == 0,
        "sanity_check_pending_tx: failed checking splitted_dsts consistency");

    // Balance check
    output_amnt += (ptx.dust_added_to_fee ? 0 : ptx.dust) + ptx.fee;

    if (ptx.dust_added_to_fee)
    {
        CHECK_AND_ASSERT_THROW_MES(ptx.dust <= ptx.fee,
            "sanity_check_pending_tx: invalid dust amount when dust added to fee");
    }
    CHECK_AND_ASSERT_THROW_MES(output_amnt <= UINT64_MAX,
            "sanity_check_pending_tx: output amount > 2^64 - 1");
    CHECK_AND_ASSERT_THROW_MES(output_amnt == input_amnt,
            "sanity_check_pending_tx: output amount > 2^64 - 1");
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace wallet
} //namespace tools
