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
#include "carrot_core/device_ram_borrowed.h"
#include "carrot_core/output_set_finalization.h"
#include "carrot_impl/carrot_tx_builder_utils.h"
#include "carrot_impl/carrot_tx_format_utils.h"
#include "common/container_helpers.h"
#include "crypto/generators.h"
#include "ringct/rctOps.h"

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
    const std::vector<fcmp_pp::curve_trees::OutputContext> &leaves)
{
    // this check compares the paths returned by tree_cache and global_tree against each other for a
    // given set of leaves

    using namespace fcmp_pp::curve_trees;

    CHECK_AND_ASSERT_MES(tree_cache.get_n_leaf_tuples() == global_tree.get_n_leaf_tuples(), false, 
        "mismatch in number of leaf tuples");
    for (const OutputContext &leaf : leaves)
    {
        CurveTreesV1::Path path_in_cache;
        CHECK_AND_ASSERT_THROW_MES(tree_cache.get_output_path(leaf.output_pair, path_in_cache),
            "could not get path from tree cache");
        const CurveTreesV1::Path path_in_global =
            global_tree.get_path_at_leaf_idx(leaf.output_id);
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
bool construct_miner_tx_fake_reward_1out(const size_t height,
    const rct::xmr_amount reward,
    const cryptonote::account_public_address &miner_address,
    cryptonote::transaction& tx,
    const uint8_t hf_version,
    const size_t num_tx_outputs)
{
    const bool is_carrot = hf_version >= HF_VERSION_CARROT;
    if (is_carrot)
    {
        carrot::CarrotDestinationV1 miner_destination;
        make_carrot_main_address_v1(miner_address.m_spend_public_key,
            miner_address.m_view_public_key,
            miner_destination);

        std::vector<carrot::CarrotPaymentProposalV1> normal_payment_proposals;
        normal_payment_proposals.reserve(num_tx_outputs);
        for (size_t i = 0; i < num_tx_outputs; ++i)
        {
            normal_payment_proposals.push_back(carrot::CarrotPaymentProposalV1{
                .destination = miner_destination,
                .amount = reward,
                .randomness = carrot::gen_janus_anchor()
            });
        }

        std::vector<carrot::CarrotCoinbaseEnoteV1> coinbase_enotes;
        carrot::get_coinbase_output_enotes(normal_payment_proposals,
            height,
            coinbase_enotes);

        tx = carrot::store_carrot_to_coinbase_transaction_v1(coinbase_enotes);
    }
    else // !is_carrot
    {
        tx.vin.clear();
        tx.vout.clear();
        tx.extra.clear();

        cryptonote::txin_gen in;
        in.height = height;

        if (hf_version >= 4)
            tx.version = 2;
        else
            tx.version = 1;

        tx.unlock_time = height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
        tx.vin.push_back(in);

        for (size_t i = 0; i < num_tx_outputs; ++i)
        {
            cryptonote::keypair txkey = cryptonote::keypair::generate(hw::get_device("default"));
            cryptonote::add_tx_pub_key_to_extra(tx, txkey.pub);
            if (!cryptonote::sort_tx_extra(tx.extra, tx.extra))
                return false;

            crypto::key_derivation derivation;
            crypto::public_key out_eph_public_key;
            bool r = crypto::generate_key_derivation(miner_address.m_view_public_key, txkey.sec, derivation);
            CHECK_AND_ASSERT_MES(r, false,
                "while creating outs: failed to generate_key_derivation(" << miner_address.m_view_public_key << ", "
                << crypto::secret_key_explicit_print_ref{txkey.sec} << ")");

            const size_t local_output_index = 0;
            r = crypto::derive_public_key(derivation, local_output_index, miner_address.m_spend_public_key, out_eph_public_key);
            CHECK_AND_ASSERT_MES(r, false,
                "while creating outs: failed to derive_public_key(" << derivation << ", "
                << local_output_index << ", "<< miner_address.m_spend_public_key << ")");

            const bool use_view_tags = hf_version >= HF_VERSION_VIEW_TAGS;
            crypto::view_tag view_tag;
            if (use_view_tags)
                crypto::derive_view_tag(derivation, local_output_index, view_tag);

            cryptonote::tx_out out;
            cryptonote::set_tx_out(reward, out_eph_public_key, use_view_tags, view_tag, out);

            tx.vout.push_back(out);
        }

        tx.invalidate_hashes();
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_miner_tx_fake_reward_1out(const size_t height,
    const rct::xmr_amount reward,
    const cryptonote::account_public_address &miner_address,
    const uint8_t hf_version,
    const size_t num_tx_outputs)
{
    cryptonote::transaction tx;
    const bool r = construct_miner_tx_fake_reward_1out(height, reward, miner_address, tx, hf_version, num_tx_outputs);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to construct miner tx");
    return tx;
}
//----------------------------------------------------------------------------------------------------------------------
cryptonote::tx_source_entry gen_tx_source_entry_fake_members(
    const stripped_down_tx_source_entry_t &in,
    const size_t mixin,
    const uint64_t max_global_output_index)
{
    const size_t ring_size = mixin + 1;
    const bool is_rct = in.mask == rct::I;

    CHECK_AND_ASSERT_THROW_MES(in.global_output_index <= max_global_output_index,
        "real global output index too low");
    CHECK_AND_ASSERT_THROW_MES(max_global_output_index >= ring_size,
        "not enough global output indices for mixin");

    cryptonote::tx_source_entry res;

    // populate ring with fake data
    std::unordered_set<uint64_t> used_indices;
    res.outputs.reserve(mixin + 1);
    res.outputs.push_back(
        {in.global_output_index,
            { rct::pk2rct(in.onetime_address), rct::commit(in.amount, in.mask) }});
    used_indices.insert(in.global_output_index);
    while (res.outputs.size() < ring_size)
    {
        const uint64_t global_output_index = crypto::rand_range<uint64_t>(0, max_global_output_index);
        if (used_indices.count(global_output_index))
            continue;
        used_indices.insert(global_output_index);
        const rct::ctkey output_pair{rct::pkGen(),
            is_rct ? rct::pkGen() : rct::zeroCommitVartime(in.amount)};
        res.outputs.push_back({global_output_index, output_pair});
    }
    // sort by index
    std::sort(res.outputs.begin(), res.outputs.end(), [](const auto &a, const auto &b) -> bool {
        return a.first < b.first;
    });

    // real_output
    res.real_output = 0;
    while (res.outputs.at(res.real_output).second.dest != in.onetime_address)
        ++res.real_output;

    // copy from in
    res.real_out_tx_key = in.real_out_tx_key;
    res.real_out_additional_tx_keys = in.real_out_additional_tx_keys;
    res.real_output_in_tx_index = in.local_output_index;
    res.amount = in.amount;
    res.rct = is_rct;
    res.mask = in.mask;

    return res;
}
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_pre_carrot_tx_with_fake_inputs(
    const cryptonote::account_keys &sender_account_keys,
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &sender_subaddress_map,
    std::vector<stripped_down_tx_source_entry_t> &&stripped_sources,
    std::vector<cryptonote::tx_destination_entry> &destinations,
    const boost::optional<cryptonote::account_public_address> &change_addr,
    const rct::xmr_amount fee,
    const uint8_t hf_version,
    const bool sweep_unmixable_override)
{
    // derive config from hf version
    const bool rct = hf_version >= HF_VERSION_DYNAMIC_FEE && !sweep_unmixable_override;
    rct::RCTConfig rct_config;
    switch (hf_version)
    {
        case 1:
        case 2:
        case 3:
        case HF_VERSION_DYNAMIC_FEE:
        case 5:
        case HF_VERSION_MIN_MIXIN_4:
        case 7:
            rct_config = { rct::RangeProofBorromean, 0 };
            break;
        case HF_VERSION_PER_BYTE_FEE:
        case 9:
            rct_config = { rct::RangeProofPaddedBulletproof, 1 };
            break;
        case HF_VERSION_SMALLER_BP:
        case 11:
        case HF_VERSION_MIN_2_OUTPUTS:
            rct_config = { rct::RangeProofPaddedBulletproof, 2 };
            break;
        case HF_VERSION_CLSAG:
        case 14:
            rct_config = { rct::RangeProofPaddedBulletproof, 3 };
            break;
        case HF_VERSION_BULLETPROOF_PLUS:
        case 16:
            rct_config = { rct::RangeProofPaddedBulletproof, 4 };
            break;
        default:
            ASSERT_MES_AND_THROW("unrecognized hf version");
    }
    const bool use_view_tags = hf_version >= HF_VERSION_VIEW_TAGS;
    const size_t mixin = 15;
    const uint64_t max_global_output_index = 1000000;

    // count missing money and balance if necessary
    boost::multiprecision::int128_t missing_money = fee;
    for (const cryptonote::tx_destination_entry &destination : destinations)
        missing_money += destination.amount;
    for (const stripped_down_tx_source_entry_t &stripped_source : stripped_sources)
        missing_money -= stripped_source.amount;
    if (missing_money > 0)
    {
        const rct::xmr_amount missing_money64 = boost::numeric_cast<rct::xmr_amount>(missing_money);

        hw::device &hwdev = hw::get_device("default");
        cryptonote::keypair main_tx_keypair = cryptonote::keypair::generate(hwdev);

        std::vector<crypto::public_key> dummy_additional_tx_public_keys;
        std::vector<rct::key> amount_keys;
        crypto::public_key input_onetime_address;
        crypto::view_tag vt;
        const bool r = hwdev.generate_output_ephemeral_keys(rct ? 2 : 1,
            cryptonote::account_keys(),
            main_tx_keypair.pub, 
            main_tx_keypair.sec,
            cryptonote::tx_destination_entry(missing_money64, sender_account_keys.m_account_address, false),
            boost::none,
            /*output_index=*/0,
            /*need_additional_txkeys=*/false,
            /*additional_tx_keys=*/{},
            dummy_additional_tx_public_keys,
            amount_keys,
            input_onetime_address,
            use_view_tags,
            vt);
        CHECK_AND_ASSERT_THROW_MES(r, "failed to generate balancing input");

        const stripped_down_tx_source_entry_t balancing_in{
            .global_output_index = crypto::rand_range<uint64_t>(0, max_global_output_index),
            .onetime_address = input_onetime_address,
            .real_out_tx_key = main_tx_keypair.pub,
            .real_out_additional_tx_keys = {},
            .local_output_index = 0,
            .amount = missing_money64,
            .mask = rct ? rct::genCommitmentMask(amount_keys.at(0)) : rct::I
        };

        stripped_sources.push_back(balancing_in);
    }

    // populate random sources
    std::vector<cryptonote::tx_source_entry> sources;
    sources.reserve(stripped_sources.size());
    for (const auto &stripped_source : stripped_sources)
        sources.push_back(gen_tx_source_entry_fake_members(stripped_source,
            mixin,
            max_global_output_index));

    // construct tx
    cryptonote::transaction tx;
    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    fcmp_pp::ProofParams dummy_fcmp_params;
    const bool r = cryptonote::construct_tx_and_get_tx_key(
        sender_account_keys,
        sender_subaddress_map,
        sources,
        destinations,
        change_addr,
        /*extra=*/{},
        tx,
        tx_key,
        additional_tx_keys,
        dummy_fcmp_params,
        rct,
        rct_config, 
        use_view_tags);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to construct tx");
    return tx;
}
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_pre_carrot_tx_with_fake_inputs(
    std::vector<cryptonote::tx_destination_entry> &destinations,
    const rct::xmr_amount fee,
    const uint8_t hf_version,
    const bool sweep_unmixable_override)
{
    cryptonote::account_base aether_acb;
    aether_acb.generate();
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> sender_subaddress_map = {
        { aether_acb.get_keys().m_account_address.m_spend_public_key, {0, 0 } }
    };

    return construct_pre_carrot_tx_with_fake_inputs(aether_acb.get_keys(),
        sender_subaddress_map,
        /*stripped_sources=*/{},
        destinations,
        /*change_addr*/boost::none,
        fee,
        hf_version,
        sweep_unmixable_override);
}
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_carrot_pruned_transaction_fake_inputs(
    const std::vector<carrot::CarrotPaymentProposalV1> &normal_payment_proposals,
    const std::vector<carrot::CarrotPaymentProposalVerifiableSelfSendV1> &selfsend_payment_proposals,
    const cryptonote::account_keys &acc_keys)
{
    carrot::select_inputs_func_t select_inputs = [](
        const boost::multiprecision::int128_t &nominal_output_sum,
        const std::map<std::size_t, rct::xmr_amount> &fee_by_input_count,
        const std::size_t,
        const std::size_t,
        std::vector<carrot::CarrotSelectedInput> &select_inputs_out
    )
    {
        const auto in_amount = boost::numeric_cast<rct::xmr_amount>(nominal_output_sum + fee_by_input_count.at(1));
        const crypto::key_image ki = rct::rct2ki(rct::pkGen());
        select_inputs_out = {carrot::CarrotSelectedInput{.amount = in_amount, .key_image = ki}};
    };

    const carrot::view_incoming_key_ram_borrowed_device k_view_dev(acc_keys.m_view_secret_key);

    carrot::CarrotTransactionProposalV1 tx_proposal;
    carrot::make_carrot_transaction_proposal_v1_transfer(
        normal_payment_proposals,
        selfsend_payment_proposals,
        fake_fee_per_weight,
        /*extra=*/{},
        std::move(select_inputs),
        /*s_view_balance_dev=*/nullptr,
        &k_view_dev,
        acc_keys.m_account_address.m_spend_public_key,
        tx_proposal);

    cryptonote::transaction tx;
    carrot::make_pruned_transaction_from_carrot_proposal_v1(tx_proposal,
        /*s_view_balance_dev=*/nullptr,
        &k_view_dev,
        tx);

    return tx;
}
//----------------------------------------------------------------------------------------------------------------------
const cryptonote::account_public_address null_addr{
    .m_spend_public_key = crypto::get_G(),
    .m_view_public_key = crypto::get_G()
};
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
                    global_output_index, output_pair
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
                    global_output_index, output_pair
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
    std::vector<fcmp_pp::curve_trees::OutputContext> paths_to_check;
    paths_to_check.reserve(w.m_transfers.size());
    for (const tools::wallet2::transfer_details &td : w.m_transfers)
    {
        const uint64_t unlock_block_index = td.m_tx.unlock_time
            ? td.m_tx.unlock_time
            : (td.m_block_height +  CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE);
        if (unlock_block_index > this->height())
            continue;
        paths_to_check.push_back({td.m_global_output_index, td.get_output_pair()});
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
