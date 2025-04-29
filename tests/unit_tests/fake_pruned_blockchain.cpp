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

#define IN_UNIT_TESTS

//paired header
#include "fake_pruned_blockchain.h"

//local headers
#include "common/container_helpers.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"
#include "tx_construction_helpers.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "unit_tests.fake_pruned_bc"

namespace mock
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static constexpr std::size_t selene_chunk_width = fcmp_pp::curve_trees::SELENE_CHUNK_WIDTH;
static constexpr std::size_t helios_chunk_width = fcmp_pp::curve_trees::HELIOS_CHUNK_WIDTH;
const auto curve_trees = fcmp_pp::curve_trees::curve_trees_v1(selene_chunk_width, helios_chunk_width);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template <class C>
static bool compare_curve_point(const typename C::Point &p1, const typename C::Point &p2)
{
    const crypto::ec_point p1_compressed = C().to_bytes(p1);
    const crypto::ec_point p2_compressed = C().to_bytes(p2);
    return p1_compressed == p2_compressed;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template <class C>
static bool compare_curve_layer(const std::vector<typename C::Point> &p1s,
    const std::vector<typename C::Point> &p2s)
{
    if (p1s.size() != p2s.size())
        return false;
    for (size_t i = 0; i < p1s.size(); ++i)
    {
        if (!compare_curve_point<C>(p1s.at(i), p2s.at(i)))
            return false;
    }
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template <class C>
static bool compare_curve_chunks(const std::vector<std::vector<typename C::Point>> &chunks1,
    const std::vector<std::vector<typename C::Point>> &chunks2)
{
    if (chunks1.size() != chunks2.size())
        return false;
    for (size_t i = 0; i < chunks1.size(); ++i)
    {
        if (!compare_curve_layer<C>(chunks1.at(i), chunks2.at(i)))
            return false;
    }
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool compare_output_tuple(const fcmp_pp::curve_trees::OutputTuple &tup1,
    const fcmp_pp::curve_trees::OutputTuple &tup2)
{
    return tup1.O == tup2.O && tup1.I == tup2.I && tup1.C == tup2.C;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool compare_leaf_layer(const std::vector<fcmp_pp::curve_trees::OutputTuple> &leaves1,
    const std::vector<fcmp_pp::curve_trees::OutputTuple> &leaves2)
{
    MDEBUG("compare_leaf_layer: " << leaves1.size() << " vs " << leaves2.size());
    if (leaves1.size() != leaves2.size())
        return false;
    bool r = true;
    for (size_t i = 0; i < leaves1.size(); ++i)
    {
        MDEBUG("    Leaf O: " << leaves1.at(i).O << " vs " << leaves2.at(i).O);
        if (!compare_output_tuple(leaves1.at(i), leaves2.at(i)))
            r = false;
    }
    return r;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool compare_paths_between_tree_cache_and_global_tree(
    const fcmp_pp::curve_trees::TreeCacheV1 &tree_cache,
    const CurveTreesGlobalTree &global_tree,
    const std::vector<fcmp_pp::curve_trees::OutputPair> &leaves)
{
    // this check compares the paths returned by tree_cache and global_tree against each other for a
    // given set of leaves

    using namespace fcmp_pp::curve_trees;

    CHECK_AND_ASSERT_MES(tree_cache.get_n_leaf_tuples() == global_tree.get_n_leaf_tuples(), false, 
        "mismatch in number of leaf tuples");
    for (const OutputPair &leaf : leaves)
    {
        CurveTreesV1::Path path_in_cache;
        CHECK_AND_ASSERT_THROW_MES(tree_cache.get_output_path(leaf, path_in_cache),
            "could not get path from tree cache");
        CHECK_AND_ASSERT_THROW_MES(!path_in_cache.empty(), "tree cache path is empty for given output pair");
        const std::set<std::size_t> global_tree_leaf_idxs = global_tree.get_leaf_idxs_with_output_pair(leaf);
        CHECK_AND_ASSERT_THROW_MES(!global_tree_leaf_idxs.empty(), "global tree missing given output pair");
        const CurveTreesV1::Path path_in_global =
            global_tree.get_path_at_leaf_idx(*global_tree_leaf_idxs.cbegin()); // use earliest leaf index
        CHECK_AND_ASSERT_THROW_MES(!path_in_cache.empty(), "global tree path is empty for given output pair");
        CHECK_AND_ASSERT_MES(compare_leaf_layer(path_in_cache.leaves, path_in_global.leaves), false,
            "paths' leaves are not equal");
        CHECK_AND_ASSERT_MES(compare_curve_chunks<Selene>(path_in_cache.c1_layers, path_in_global.c1_layers),
            false,
            "paths' c1 layers are not equal");
        CHECK_AND_ASSERT_MES(compare_curve_chunks<Helios>(path_in_cache.c2_layers, path_in_global.c2_layers),
            false,
            "paths' c2 layers are not equal");
    }
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static bool is_valid_output_pair_for_tree(const fcmp_pp::curve_trees::OutputPair &p)
{
    return rct::isInMainSubgroup(rct::pk2rct(p.output_pubkey)) && rct::isInMainSubgroup(p.commitment);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
fake_pruned_blockchain::fake_pruned_blockchain(const uint64_t start_block_index,
    const cryptonote::network_type nettype):
    m_start_block_index(start_block_index),
    m_nettype(nettype),
    m_num_outputs(0),
    m_global_curve_tree(*curve_trees)
{
    add_starting_block();
}
//----------------------------------------------------------------------------------------------------------------------
void fake_pruned_blockchain::add_block(const uint8_t hf_version,
    std::vector<cryptonote::transaction> &&txs,
    const cryptonote::account_public_address &miner_address,
    const size_t num_miner_tx_outputs)
{
    std::vector<crypto::hash> tx_prunable_hashes(txs.size());
    std::vector<crypto::hash> tx_hashes(txs.size());
    for (size_t i = 0; i < tx_hashes.size(); ++i)
    {
        const cryptonote::transaction &tx = txs.at(i);
        if (tx.pruned)
        {
            tx_prunable_hashes[i] = crypto::rand<crypto::hash>();
            tx_hashes[i] = cryptonote::get_pruned_transaction_hash(tx, tx_prunable_hashes.at(i));
        }
        else // !tx.pruned
        {
            tx_prunable_hashes[i] = crypto::null_hash;
            tx_hashes[i] = cryptonote::get_transaction_hash(tx);
        }
    }

    CHECK_AND_ASSERT_THROW_MES(tx_hashes.size() == txs.size(), "wrong tx_hashes size");

    cryptonote::transaction miner_tx = construct_miner_tx_fake_reward_1out(this->height(),
        miner_reward,
        miner_address,
        hf_version,
        num_miner_tx_outputs);

    cryptonote::block blk;
    blk.major_version = hf_version;
    blk.minor_version = hf_version;
    blk.timestamp = std::max<uint64_t>(this->timestamp() + 120, time(NULL));
    blk.prev_id = this->top_block_hash();
    blk.nonce = crypto::rand<uint32_t>();
    blk.miner_tx = std::move(miner_tx);
    blk.tx_hashes = std::move(tx_hashes);

    add_block(std::move(blk), std::move(txs), std::move(tx_prunable_hashes));
}
//----------------------------------------------------------------------------------------------------------------------
void fake_pruned_blockchain::add_block(cryptonote::block &&blk,
    std::vector<cryptonote::transaction> &&pruned_txs,
    std::vector<crypto::hash> &&prunable_hashes)
{
    // sanity checks and block rules
    assert_chain_count();
    CHECK_AND_ASSERT_THROW_MES(blk.major_version >= this->hf_version(), 
        "hf version too low");
    CHECK_AND_ASSERT_THROW_MES(blk.miner_tx.vin.size() == 1,
        "miner tx wrong input count");
    CHECK_AND_ASSERT_THROW_MES(blk.miner_tx.vin.front().type() == typeid(cryptonote::txin_gen),
        "miner tx wrong input type");
    CHECK_AND_ASSERT_THROW_MES(boost::get<cryptonote::txin_gen>(blk.miner_tx.vin.front()).height == this->height(),
        "miner tx wrong height");
    CHECK_AND_ASSERT_THROW_MES(blk.miner_tx.unlock_time == this->height() + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW,
        "miner tx wrong unlock time");
    CHECK_AND_ASSERT_THROW_MES(blk.tx_hashes.size() == pruned_txs.size(),
        "wrong number of txs provided");
    CHECK_AND_ASSERT_THROW_MES(blk.tx_hashes.size() == prunable_hashes.size(),
        "wrong number of prunable hashes provided");
    CHECK_AND_ASSERT_THROW_MES(m_parsed_blocks.empty() || m_parsed_blocks.back().hash == blk.prev_id,
        "wrong block prev_id");

    // calculate weight and num outputs in block
    size_t total_block_weight = 0;
    size_t num_new_block_outputs = blk.miner_tx.vout.size();
    for (const cryptonote::transaction &tx : pruned_txs)
    {
        total_block_weight += tx.pruned
                ? cryptonote::get_pruned_transaction_weight(tx)
                : cryptonote::get_transaction_weight(tx);
        num_new_block_outputs += tx.vout.size();
    }

    // make block_complete_entry
    cryptonote::block_complete_entry blk_entry;
    blk_entry.pruned = true;
    blk_entry.block = cryptonote::block_to_blob(blk);
    blk_entry.block_weight = total_block_weight;
    blk_entry.txs.reserve(pruned_txs.size());
    for (size_t i = 0; i < pruned_txs.size(); ++i)
    {
        std::stringstream ss;
        binary_archive<true> ar(ss);
        CHECK_AND_ASSERT_THROW_MES(pruned_txs.at(i).serialize_base(ar), "tx failed to serialize");
        blk_entry.txs.push_back(cryptonote::tx_blob_entry(ss.str(), prunable_hashes.at(i)));
    }

    size_t running_num_chain_outputs = m_num_outputs;

    // make parsed_block
    tools::wallet2::parsed_block par_blk;
    par_blk.hash = cryptonote::get_block_hash(blk);
    par_blk.block = std::move(blk);
    par_blk.txes = std::move(pruned_txs);
    {
        auto &tx_o_indices = tools::add_element(par_blk.o_indices.indices);
        for (size_t i = 0; i < par_blk.block.miner_tx.vout.size(); ++i)
        {
            tx_o_indices.indices.push_back(running_num_chain_outputs);
            ++running_num_chain_outputs;
        }
    }
    for (const cryptonote::transaction &tx : par_blk.txes)
    {
        CHECK_AND_ASSERT_THROW_MES(tx.unlock_time == 0,
            "I don't want to code tree logic for custom unlock times plz");
        auto &tx_o_indices = tools::add_element(par_blk.o_indices.indices);
        for (size_t i = 0; i < tx.vout.size(); ++i)
        {
            tx_o_indices.indices.push_back(running_num_chain_outputs);
            ++running_num_chain_outputs;
        }
    }
    par_blk.error = false;

    CHECK_AND_ASSERT_THROW_MES(running_num_chain_outputs - m_num_outputs == num_new_block_outputs,
        "new block output count mismatch (1)");

    const size_t next_height = this->height() + 1;
    const bool should_try_expand_fcmp_tree = next_height >= CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;
    if (should_try_expand_fcmp_tree)
    {
        // pull miner outputs from CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW blocks ago, 
        // and non-miner outputs from CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE blocks ago
        const bool should_try_pull_coinbase_outs = next_height >= CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
        static_assert(CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW >= CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE,
            "Cannot assume that we should always try pulling non-miner outputs when we can pull miner outputs");

        std::vector<fcmp_pp::curve_trees::OutputContext> new_spendable_outputs;
        if (should_try_pull_coinbase_outs)
        {
            const tools::wallet2::parsed_block parsed_block_with_unlocked_miner_tx = 
                this->get_parsed_block(next_height - CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW);
            const cryptonote::transaction &unlocked_miner_tx =
                parsed_block_with_unlocked_miner_tx.block.miner_tx;
            for (size_t local_output_index = 0; local_output_index < unlocked_miner_tx.vout.size(); ++local_output_index)
            {
                const cryptonote::tx_out &o = unlocked_miner_tx.vout.at(local_output_index);
                crypto::public_key output_pubkey;
                CHECK_AND_ASSERT_THROW_MES(cryptonote::get_output_public_key(o, output_pubkey),
                    "cannot get output pubkey");
                const rct::key amount_commitment = rct::zeroCommitVartime(o.amount);
                const fcmp_pp::curve_trees::OutputPair output_pair{output_pubkey, amount_commitment};
                if (!is_valid_output_pair_for_tree(output_pair))
                    continue;
                const uint64_t global_output_index =
                    parsed_block_with_unlocked_miner_tx.o_indices.indices.at(0).indices.at(local_output_index);
                new_spendable_outputs.push_back(fcmp_pp::curve_trees::OutputContext{
                    global_output_index, false/*torsion_checked*/, output_pair
                });
            }
        }

        const tools::wallet2::parsed_block parsed_block_with_unlocked_txs = 
            this->get_parsed_block(next_height - CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE);
        for (size_t tx_index = 0; tx_index < parsed_block_with_unlocked_txs.txes.size(); ++tx_index)
        {
            const cryptonote::transaction &unlocked_tx = parsed_block_with_unlocked_txs.txes.at(tx_index);
            for (size_t local_output_index = 0; local_output_index < unlocked_tx.vout.size(); ++local_output_index)
            {
                const cryptonote::tx_out &o = unlocked_tx.vout.at(local_output_index);
                crypto::public_key output_pubkey;
                CHECK_AND_ASSERT_THROW_MES(cryptonote::get_output_public_key(o, output_pubkey),
                    "cannot get output pubkey");
                const rct::key amount_commitment = o.amount
                    ? rct::zeroCommitVartime(o.amount) : unlocked_tx.rct_signatures.outPk.at(local_output_index).mask;
                const fcmp_pp::curve_trees::OutputPair output_pair{output_pubkey, amount_commitment};
                if (!is_valid_output_pair_for_tree(output_pair))
                    continue;
                const uint64_t global_output_index =
                    parsed_block_with_unlocked_txs.o_indices.indices.at(tx_index + 1).indices.at(local_output_index);
                new_spendable_outputs.push_back(fcmp_pp::curve_trees::OutputContext{
                    global_output_index, false/*torsion_checked*/, output_pair
                });
            }
        }

        if (!new_spendable_outputs.empty())
        {
            const bool tree_grow_success = m_global_curve_tree.grow_tree(
                m_global_curve_tree.get_n_leaf_tuples(),
                new_spendable_outputs.size(),
                new_spendable_outputs);

            CHECK_AND_ASSERT_THROW_MES(tree_grow_success,
                "fake_pruned_blockchain::add_block: error while growing tree!!!");
        }
    }

    // everything looks good! time to update fields
    m_num_outputs = running_num_chain_outputs;
    m_block_entries.emplace_back(std::move(blk_entry));
    m_parsed_blocks.emplace_back(std::move(par_blk));
    m_curve_tree_roots.emplace_back(make_fcmp_generic_object(m_global_curve_tree.get_tree_root()));
}
//----------------------------------------------------------------------------------------------------------------------
void fake_pruned_blockchain::get_blocks_data(const uint64_t start_block_index,
    const uint64_t stop_block_index,
    std::vector<cryptonote::block_complete_entry> &block_entries_out,
    std::vector<tools::wallet2::parsed_block> &parsed_blocks_out) const
{
    block_entries_out.clear();
    parsed_blocks_out.clear();

    assert_chain_count();

    if (start_block_index < m_start_block_index || stop_block_index >= this->height())
        throw std::out_of_range("get_blocks_data requested block indices");

    for (size_t block_index = start_block_index; block_index <= stop_block_index; ++block_index)
    {
        const size_t i = block_index - m_start_block_index;
        block_entries_out.push_back(m_block_entries.at(i));
        parsed_blocks_out.push_back(m_parsed_blocks.at(i));
    }
}
//----------------------------------------------------------------------------------------------------------------------
void fake_pruned_blockchain::init_wallet_for_starting_block(tools::wallet2 &w) const
{
    assert_chain_count();
    CHECK_AND_ASSERT_THROW_MES(!m_parsed_blocks.empty(), "blockchain missing starting block");

    w.set_refresh_from_block_height(m_start_block_index);

    w.m_blockchain.clear();
    for (size_t i = 0; i < m_start_block_index; ++i)
        w.m_blockchain.push_back(crypto::null_hash);
    w.m_blockchain.push_back(m_parsed_blocks.front().hash);
    w.m_blockchain.trim(m_start_block_index);

    w.m_tree_cache.clear();
    w.m_tree_cache.init(m_start_block_index,
        m_parsed_blocks.front().hash,
        /*n_leaf_tuples=*/0,
        /*last_path=*/{},
        /*locked_outputs=*/{});
}
//----------------------------------------------------------------------------------------------------------------------
uint64_t fake_pruned_blockchain::refresh_wallet(tools::wallet2 &w, const size_t start_height) const
{
    // fetch blocks data
    std::vector<cryptonote::block_complete_entry> blk_entries;
    std::vector<tools::wallet2::parsed_block> parsed_blks;
    this->get_blocks_data(start_height, this->height()-1, blk_entries, parsed_blks);

    // wallet process blocks data
    uint64_t blocks_added;
    auto output_tracker_cache = w.create_output_tracker_cache();
    w.process_parsed_blocks(start_height, blk_entries, parsed_blks, blocks_added, output_tracker_cache);

    // collect paths_to_check
    std::vector<fcmp_pp::curve_trees::OutputPair> paths_to_check;
    paths_to_check.reserve(w.m_transfers.size());
    for (const tools::wallet2::transfer_details &td : w.m_transfers)
    {
        const uint64_t unlock_block_index = td.m_tx.unlock_time
            ? td.m_tx.unlock_time
            : (td.m_block_height +  CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE);
        if (unlock_block_index > this->height())
            continue;
        paths_to_check.push_back(td.get_output_pair());
    }

    // check effective FCMP tree equality between blockchain and wallet
    const bool tree_cache_and_global_tree_synced = compare_paths_between_tree_cache_and_global_tree(
        w.m_tree_cache, m_global_curve_tree, paths_to_check);
    CHECK_AND_ASSERT_THROW_MES(tree_cache_and_global_tree_synced,
        "fake_pruned_blockchain: mismatched wallet tree cache and blockchain global tree");

    return blocks_added;
}
//----------------------------------------------------------------------------------------------------------------------
void fake_pruned_blockchain::assert_chain_count() const
{
    CHECK_AND_ASSERT_THROW_MES(m_block_entries.size() == m_parsed_blocks.size(),
        "blockchain size mismatch");
    CHECK_AND_ASSERT_THROW_MES(m_block_entries.size() == m_curve_tree_roots.size(),
        "curve tree roots size mismatch");
    CHECK_AND_ASSERT_THROW_MES(m_block_entries.empty() || m_num_outputs > 0,
        "missing outputs in starting block");
    CHECK_AND_ASSERT_THROW_MES(m_global_curve_tree.get_n_leaf_tuples() <= m_num_outputs,
        "tree has wrong number of leafs");
}
//----------------------------------------------------------------------------------------------------------------------
void fake_pruned_blockchain::add_starting_block()
{
    if (m_start_block_index == 0)
    {
        // add actual genesis block for this network type
        cryptonote::block genesis_blk;
        CHECK_AND_ASSERT_THROW_MES(cryptonote::generate_genesis_block(genesis_blk,
                get_config(m_nettype).GENESIS_TX,
                get_config(m_nettype).GENESIS_NONCE),
            "failed to generate genesis block");
        add_block(std::move(genesis_blk), {}, {});
    }
    else // m_start_block_index > 0
    {
        // make up start block
        add_block(1, {}, null_addr);
    }
}
//----------------------------------------------------------------------------------------------------------------------
} //namespace mock
