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
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "cryptonote_config.h"
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
void sanity_check_pending_tx(const wallet2::pending_tx &ptx,
    const std::vector<wallet2_basic::transfer_details> &transfers,
    const cryptonote::network_type nettype)
{
    const auto &construct = ptx.construction_data;

    // Extra
    CHECK_AND_ASSERT_THROW_MES(construct.extra == ptx.tx.extra,
        "sanity_check_pending_tx: tx_extra mismatch");
    std::vector<cryptonote::tx_extra_field> tx_extra_fields;
    CHECK_AND_ASSERT_THROW_MES(parse_tx_extra(construct.extra, tx_extra_fields),
        "sanity_check_pending_tx: tx_extra extraction failure");
    cryptonote::tx_extra_pub_key tx_extra_pub_key;
    CHECK_AND_ASSERT_THROW_MES(find_tx_extra_field_by_type(tx_extra_fields, tx_extra_pub_key),
        "sanity_check_pending_tx: tx_extra missing tx pub key");
    CHECK_AND_ASSERT_THROW_MES(tx_extra_pub_key.pub_key == rct::rct2pk(rct::scalarmultBase(rct::sk2rct(ptx.tx_key))),
        "sanity_check_pending_tx: tx_extra unable to reproduce tx pubkey");
    if (ptx.additional_tx_keys.size() > 0)
    {
        cryptonote::tx_extra_additional_pub_keys tx_extra_additional_pub_keys;
        CHECK_AND_ASSERT_THROW_MES(find_tx_extra_field_by_type(tx_extra_fields, tx_extra_additional_pub_keys),
            "sanity_check_pending_tx: tx_extra missing extra tx pub keys");
        CHECK_AND_ASSERT_THROW_MES(tx_extra_additional_pub_keys.data.size() == ptx.additional_tx_keys.size(),
            "sanity_check_pending_tx: tx_extra extra tx pub keys size mismatch");
        for (size_t p = 0; p < ptx.additional_tx_keys.size(); ++p)
        {
            const auto &extra_pk = tx_extra_additional_pub_keys.data.at(p);
            const auto &ptx_sk = ptx.additional_tx_keys.at(p);
            CHECK_AND_ASSERT_THROW_MES(extra_pk == rct::rct2pk(rct::scalarmultBase(rct::sk2rct(ptx_sk))),
                "sanity_check_pending_tx: tx_extra unable to reproduce extra tx pubkey");
        }
    }

    // Extract tx inputs
    std::vector<crypto::key_image> ext_key_images;
    std::vector<rct::xmr_amount> ext_input_amounts;
    const bool all_are_txin_to_key = std::all_of(ptx.tx.vin.begin(), ptx.tx.vin.end(), [&](const cryptonote::txin_v& s_e) -> bool
    {
        CHECKED_GET_SPECIFIC_VARIANT(s_e, const cryptonote::txin_to_key, in, false);
        ext_key_images.push_back(in.k_image);
        ext_input_amounts.push_back(in.amount);
        return true;
    });
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
        CHECK_AND_ASSERT_THROW_MES(selected_transfer < transfers.size(),
            "sanity_check_pending_tx: invalid transfers index");

        const auto &src = construct.sources.at(i);
        const auto &transfer = transfers.at(selected_transfer);

        CHECK_AND_ASSERT_THROW_MES(src.amount == ext_input_amounts.at(i),
            "sanity_check_pending_tx: input amount mismatch");
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
    // - We assume there is no payment ID (pending_tx has no field for payment ID at this time).
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

    for (const auto &dst : construct.splitted_dsts)
    {
        // Check original dst string
        if (dst.original.size() > 0)
        {
            auto reconstructed_dst = cryptonote::tx_destination_entry(dst.amount, dst.addr, dst.is_subaddress);
            reconstructed_dst.is_integrated = dst.is_integrated;
            const auto original_str = reconstructed_dst.address(nettype, crypto::hash{});  // no payment id
            CHECK_AND_ASSERT_THROW_MES(original_str == dst.original,
                "sanity_check_pending_tx: failed reconstructing original destination address string");
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
