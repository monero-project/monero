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
#include "crypto/crypto.h"
#include "crypto/generators.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
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
    const cryptonote::tx_destination_entry &change,
    const std::vector<cryptonote::tx_destination_entry> &dests,
    const crypto::secret_key &tx_key,
    const std::vector<crypto::secret_key> &additional_tx_keys,
    crypto::public_key &tx_pubkey_out,
    std::vector<crypto::public_key> &tx_additional_pubkeys_out)
{
    tx_pubkey_out = crypto::public_key{};
    tx_additional_pubkeys_out.clear();

    // Check if multiple keys are expected
    // Note: this is the same method used in `construct_tx_and_get_tx_key()`, so we should expect the same results.
    size_t num_stdaddresses = 0;
    size_t num_subaddresses = 0;
    cryptonote::account_public_address shared_key_base = cryptonote::account_public_address{
        .m_spend_public_key = crypto::get_G(),
        .m_view_public_key = crypto::get_G(),
    };
    cryptonote::classify_addresses(dests, change.addr, num_stdaddresses, num_subaddresses, shared_key_base);
    const bool need_additional_txkeys = num_subaddresses > 0 && (num_stdaddresses > 0 || num_subaddresses > 1);

    if (!need_additional_txkeys)
    {
        CHECK_AND_ASSERT_THROW_MES(additional_tx_keys.size() == 0,
            "recontruct_tx_pubkeys: unexpected additional tx keys for 2-out");

        tx_pubkey_out = rct::rct2pk(rct::scalarmultKey(
            rct::pk2rct(shared_key_base.m_spend_public_key),
            rct::sk2rct(tx_key)
        ));
    }
    else
    {
        CHECK_AND_ASSERT_THROW_MES(dests.size() == additional_tx_keys.size(),
            "recontruct_tx_pubkeys: tx key alignment failure");

        for (size_t i = 0; i < dests.size(); ++i)
        {
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
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
static void reconstruct_payment_id(const std::string &extracted_payment_id,
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

    // Encrypt the address's payment ID.
    CHECK_AND_ASSERT_THROW_MES(hw::core::device_default().encrypt_payment_id(to_encrypt_payment_id, view_key_pub, tx_key),
        "reconstruct_payment_id: failed encrypting payment id");

    // Check equivalence.
    CHECK_AND_ASSERT_THROW_MES(to_encrypt_payment_id == encrypted_payment_id8,
        "reconstruct_payment_id: failed encrypting payment id");
}
//-------------------------------------------------------------------------------------------------------------------
void sanity_check_pending_tx(const wallet2::pending_tx &ptx, const wallet2 &wallet)
{
    const auto &construct = ptx.construction_data;
    const cryptonote::network_type nettype = wallet.nettype();

    // Extra
    crypto::public_key reconstruct_pubkey;
    std::vector<crypto::public_key> reconstruct_additional_pubkeys;
    recontruct_tx_pubkeys(
        construct.change_dts,
        construct.splitted_dsts,
        ptx.tx_key,
        ptx.additional_tx_keys,
        reconstruct_pubkey,
        reconstruct_additional_pubkeys
    );

    CHECK_AND_ASSERT_THROW_MES(construct.extra == ptx.tx.extra,
        "sanity_check_pending_tx: tx_extra mismatch");
    std::vector<cryptonote::tx_extra_field> tx_extra_fields;
    CHECK_AND_ASSERT_THROW_MES(cryptonote::parse_tx_extra(construct.extra, tx_extra_fields),
        "sanity_check_pending_tx: tx_extra extraction failure");
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
        "sanity_check_pending_tx: ring sig index out of input set size");
    CHECK_AND_ASSERT_THROW_MES(construct.sources.size() == ptx.selected_transfers.size(),
        "sanity_check_pending_tx: selected_transfers invalid size");
    CHECK_AND_ASSERT_THROW_MES(construct.selected_transfers == ptx.selected_transfers,
        "sanity_check_pending_tx: selected_transfers inconsistent");

    boost::multiprecision::uint128_t input_amnt = 0;

    for (size_t i = 0; i < ptx.tx.vin.size(); ++i)
    {
        const auto &selected_transfer = ptx.selected_transfers.at(i);
        const auto &src = construct.sources.at(i);
        const auto &transfer = wallet.get_transfer_details(selected_transfer);

        if (src.rct)
        {
            CHECK_AND_ASSERT_THROW_MES(ext_input_amounts.at(i) == 0,
                "sanity_check_pending_tx: rct amount extracted from inputs should be zero");
        }
        else
        {
            CHECK_AND_ASSERT_THROW_MES(src.amount == ext_input_amounts.at(i),
                "sanity_check_pending_tx: non-rct input amount mismatch");
        }
        CHECK_AND_ASSERT_THROW_MES(src.amount == transfer.m_amount,
            "sanity_check_pending_tx: transfer amount mismatch");
        CHECK_AND_ASSERT_THROW_MES(src.real_output < src.outputs.size(),
            "sanity_check_pending_tx: ring sig index out of input set size");
        if (src.rct)
        {
            const rct::key commitment = rct::commit(src.amount, src.mask);
            CHECK_AND_ASSERT_THROW_MES(src.outputs[src.real_output].second.mask == commitment,
                "sanity_check_pending_tx: ring sig index out of input set size");
        }

        CHECK_AND_ASSERT_THROW_MES(!transfer.m_spent,
            "sanity_check_pending_tx: transfer - is marked as spent");
        CHECK_AND_ASSERT_THROW_MES(!transfer.m_frozen,
            "sanity_check_pending_tx: transfer - is marked as frozen");
        CHECK_AND_ASSERT_THROW_MES(transfer.m_key_image_known,
            "sanity_check_pending_tx: transfer - KI is unknown");
        CHECK_AND_ASSERT_THROW_MES(ext_key_images.at(i) == transfer.m_key_image,
            "sanity_check_pending_tx: transfer - KI mismatch");
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

    boost::multiprecision::uint128_t output_amnt = 0;
    size_t integrated_count = 0;

    for (const auto &dst : construct.splitted_dsts)
    {
        // Check original dst string
        if (dst.original.size() > 0)
        {
            const auto rebuilt_addr_str = cryptonote::get_account_address_as_str(nettype, dst.is_subaddress, dst.addr);

            std::string address;
            std::string extracted_payment_id;
            uint64_t _amount;
            std::string _tx_description;
            std::string _recipient_name;
            std::vector<std::string> _unknown_parameters;
            std::string _error;
            wallet2::parse_uri_impl(dst.original,
                nettype,
                address,
                extracted_payment_id,
                _amount,
                _tx_description,
                _recipient_name,
                _unknown_parameters,
                _error);
            CHECK_AND_ASSERT_THROW_MES(address == rebuilt_addr_str,
                "sanity_check_pending_tx: failed reconstructing original destination address string");

            if (dst.is_integrated)
            {
                ++integrated_count;
                CHECK_AND_ASSERT_THROW_MES(integrated_count == 1,
                    "sanity_check_pending_tx: more than one integrated address detected");

                reconstruct_payment_id(extracted_payment_id,
                    ptx.change_dts.addr,
                    construct.splitted_dsts,
                    ptx.tx_key,
                    tx_extra_fields);
            }
        }

        // Check reproduced dests
        const auto &it = std::find_if(splitted_dsts_repro.cbegin(), splitted_dsts_repro.cend(),
            [&dst](const auto &a) {
                return a == dst;
            }
        );
        CHECK_AND_ASSERT_THROW_MES(it != splitted_dsts_repro.cend(),
            "sanity_check_pending_tx: failed checking splitted_dsts consistency");

        // Carefully erase found copies one by one in case of duplicates.
        splitted_dsts_repro.erase(it);

        output_amnt += dst.amount;
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
