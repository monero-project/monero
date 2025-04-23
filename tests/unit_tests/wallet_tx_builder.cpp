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

#include "unit_tests_utils.h"
#include "gtest/gtest.h"

#include "carrot_core/config.h"
#include "common/container_helpers.h"
#include "ringct/rctOps.h"
#include "wallet/tx_builder.h"

static tools::wallet2::transfer_details gen_transfer_details()
{
    return tools::wallet2::transfer_details{
        .m_block_height = crypto::rand_idx<uint64_t>(CRYPTONOTE_MAX_BLOCK_NUMBER),
        .m_tx = {},
        .m_txid = crypto::rand<crypto::hash>(),
        .m_internal_output_index = crypto::rand_idx<uint64_t>(carrot::CARROT_MAX_TX_OUTPUTS),
        .m_global_output_index = crypto::rand_idx<uint64_t>(CRYPTONOTE_MAX_BLOCK_NUMBER * 1000ull),
        .m_spent = false,
        .m_frozen = false,
        .m_spent_height = 0,
        .m_key_image = rct::rct2pk(rct::pkGen()),
        .m_mask = rct::skGen(),
        .m_amount = crypto::rand_range<rct::xmr_amount>(0, COIN), // [0, 1] XMR i.e. [0, 1e12] pXMR
        .m_rct = true,
        .m_key_image_known = true,
        .m_key_image_request = false,
        .m_pk_index = 1,
        .m_subaddr_index = {},
        .m_key_image_partial = false,
        .m_multisig_k = {},
        .m_multisig_info = {},
        .m_uses = {},
    };
}

static bool compare_transfer_to_selected_input(const tools::wallet2::transfer_details &td,
    const carrot::CarrotSelectedInput &input)
{
    return td.m_amount == input.amount && td.m_key_image == input.key_image;
}

TEST(wallet_tx_builder, input_selection_basic)
{
    std::map<std::size_t, rct::xmr_amount> fee_by_input_count;
    for (size_t i = carrot::CARROT_MIN_TX_INPUTS; i <= carrot::CARROT_MAX_TX_INPUTS; ++i)
        fee_by_input_count[i] = 30680000 * i - i*i;

    const boost::multiprecision::int128_t nominal_output_sum = 4444444444444; // 4.444... XMR 

    // add 10 random transfers
    tools::wallet2::transfer_container transfers;
    for (size_t i = 0; i < 10; ++i)
    {
        tools::wallet2::transfer_details &td = tools::add_element(transfers);
        td = gen_transfer_details();
        td.m_block_height = transfers.size(); // small ascending block heights
    }

    // modify one so that it funds the transfer all by itself
    const size_t rand_idx = crypto::rand_idx(transfers.size());
    transfers[rand_idx].m_amount = boost::numeric_cast<rct::xmr_amount>(nominal_output_sum +
            fee_by_input_count.crbegin()->second +
            crypto::rand_range<rct::xmr_amount>(0, COIN));

    // set such that all transfers are unlocked
    const std::uint64_t top_block_index = transfers.size() + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

    // make input selector
    std::set<size_t> selected_transfer_indices;
    const carrot::select_inputs_func_t input_selector = tools::wallet::make_wallet2_single_transfer_input_selector(
        transfers,
        /*from_account=*/0,
        /*from_subaddresses=*/{},
        /*ignore_above=*/std::numeric_limits<rct::xmr_amount>::max(),
        /*ignore_below=*/0,
        top_block_index,
        /*allow_carrot_external_inputs_in_normal_transfers=*/true,
        /*allow_pre_carrot_inputs_in_normal_transfers=*/true,
        selected_transfer_indices
    );

    // select inputs
    std::vector<carrot::CarrotSelectedInput> selected_inputs;
    input_selector(nominal_output_sum,
        fee_by_input_count,
        1,                             // number of normal payment proposals
        1,                             // number of self-send payment proposals
        selected_inputs);

    ASSERT_EQ(2, selected_inputs.size()); // assert two inputs selected
    ASSERT_EQ(2, selected_transfer_indices.size());
    ASSERT_LT(*selected_transfer_indices.crbegin(), transfers.size());
    ASSERT_NE(selected_inputs.front().key_image, selected_inputs.back().key_image);

    // Assert content of selected inputs matches the content in `transfers`
    std::set<size_t> matched_transfer_indices;
    for (const carrot::CarrotSelectedInput &selected_input : selected_inputs)
    {
        for (const size_t selected_transfer_index : selected_transfer_indices)
        {
            if (compare_transfer_to_selected_input(transfers.at(selected_transfer_index), selected_input))
            {
                const auto insert_res = matched_transfer_indices.insert(selected_transfer_index);
                if (insert_res.second)
                    break;
            }
        }
    }
    ASSERT_EQ(selected_transfer_indices.size(), matched_transfer_indices.size());
}
