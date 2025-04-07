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

#pragma once

#define IN_UNIT_TESTS

#include "carrot_impl/carrot_tx_builder_types.h"
#include "curve_trees.h"
#include "wallet/wallet2.h"

namespace mock
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
bool construct_miner_tx_fake_reward_1out(const size_t height,
    const rct::xmr_amount reward,
    const cryptonote::account_public_address &miner_address,
    cryptonote::transaction& tx,
    const uint8_t hf_version,
    const size_t num_tx_outputs = 1);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_miner_tx_fake_reward_1out(const size_t height,
    const rct::xmr_amount reward,
    const cryptonote::account_public_address &miner_address,
    const uint8_t hf_version,
    const size_t num_tx_outputs = 1);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct stripped_down_tx_source_entry_t
{
    uint64_t global_output_index;
    crypto::public_key onetime_address;
    crypto::public_key real_out_tx_key;
    std::vector<crypto::public_key> real_out_additional_tx_keys;
    size_t local_output_index;
    rct::xmr_amount amount;
    rct::key mask;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::tx_source_entry gen_tx_source_entry_fake_members(
    const stripped_down_tx_source_entry_t &in,
    const size_t mixin,
    const uint64_t max_global_output_index);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_pre_carrot_tx_with_fake_inputs(
    const cryptonote::account_keys &sender_account_keys,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &sender_subaddress_map,
    std::vector<stripped_down_tx_source_entry_t> &&stripped_sources,
    std::vector<cryptonote::tx_destination_entry> &destinations,
    const boost::optional<cryptonote::account_public_address> &change_addr,
    const rct::xmr_amount fee,
    const uint8_t hf_version,
    const bool sweep_unmixable_override = false);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_pre_carrot_tx_with_fake_inputs(
    std::vector<cryptonote::tx_destination_entry> &destinations,
    const rct::xmr_amount fee,
    const uint8_t hf_version,
    const bool sweep_unmixable_override = false);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static constexpr rct::xmr_amount fake_fee_per_weight = 2023;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_carrot_pruned_transaction_fake_inputs(
    const std::vector<carrot::CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const cryptonote::account_keys &acc_keys);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
extern const cryptonote::account_public_address null_addr;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
using fcmp_generic_object_t = std::unique_ptr<void, decltype(&free)>;
static inline fcmp_generic_object_t make_fcmp_generic_object(void *p) { return fcmp_generic_object_t(p, &free); }
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
class fake_pruned_blockchain
{
public:
    static constexpr rct::xmr_amount miner_reward = 600000000000; // 0.6 XMR

    fake_pruned_blockchain(const uint64_t start_block_index = 0,
        const cryptonote::network_type nettype = cryptonote::MAINNET);

    void add_block(const uint8_t hf_version,
        std::vector<cryptonote::transaction> &&txs,
        const cryptonote::account_public_address &miner_address,
        const size_t num_miner_tx_outputs = 1);

    void add_block(cryptonote::block &&blk,
        std::vector<cryptonote::transaction> &&pruned_txs,
        std::vector<crypto::hash> &&prunable_hashes);

    void get_blocks_data(const uint64_t start_block_index,
        const uint64_t stop_block_index,
        std::vector<cryptonote::block_complete_entry> &block_entries_out,
        std::vector<tools::wallet2::parsed_block> &parsed_blocks_out) const;

    void init_wallet_for_starting_block(tools::wallet2 &w) const;

    /**
     * brief: refresh_wallet - pulls block data from chain and calls process_parsed_blocks()
     * param: w -
     * param: start_height - start height to pull block data from (inclusive)
     * return: number of blocks added by process_parsed_blocks()
     */
    uint64_t refresh_wallet(tools::wallet2 &w, const size_t start_height) const;

    uint8_t hf_version() const
    {
        return m_parsed_blocks.empty() ? 1 : m_parsed_blocks.back().block.major_version;
    }

    uint64_t start_block_index() const 
    {
        return m_start_block_index;
    }

    uint64_t height() const
    {
        return m_start_block_index + m_block_entries.size();
    }

    uint64_t timestamp() const
    {
        return m_parsed_blocks.empty() ? 0 : m_parsed_blocks.back().block.timestamp;
    }

    crypto::hash top_block_hash() const
    {
        return m_parsed_blocks.empty() ? crypto::null_hash : m_parsed_blocks.back().hash;
    }

    uint64_t num_outputs() const
    {
        return m_num_outputs;
    }

    tools::wallet2::parsed_block get_parsed_block(const uint64_t block_index) const
    {
        if (block_index >= this->height() || block_index < this->m_start_block_index)
            throw std::out_of_range("get_block requested block index");

        return m_parsed_blocks.at(block_index - this->m_start_block_index);
    }

    const uint8_t *get_fcmp_tree_root_at(const uint64_t block_index) const
    {
        if (block_index >= this->height() || block_index < this->m_start_block_index)
            throw std::out_of_range("get_fcmp_tree_root_at requested block index");

        return reinterpret_cast<uint8_t*>(m_curve_tree_roots.at(block_index - this->m_start_block_index).get());
    }

private:
    void assert_chain_count() const;

    void add_starting_block();

    uint64_t m_start_block_index;
    cryptonote::network_type m_nettype;
    uint64_t m_num_outputs;
    std::vector<cryptonote::block_complete_entry> m_block_entries;
    std::vector<tools::wallet2::parsed_block> m_parsed_blocks;
    std::vector<fcmp_generic_object_t> m_curve_tree_roots;

    CurveTreesGlobalTree m_global_curve_tree;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
} //namespace mock
