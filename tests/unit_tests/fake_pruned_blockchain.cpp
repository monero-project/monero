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

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "unit_tests.fake_pruned_bc"

namespace mock
{
//----------------------------------------------------------------------------------------------------------------------
bool construct_miner_tx_fake_reward_1out(const size_t height,
    const rct::xmr_amount reward,
    const cryptonote::account_public_address &miner_address,
    cryptonote::transaction& tx,
    const uint8_t hf_version)
{
    const bool is_carrot = hf_version >= HF_VERSION_CARROT;
    if (is_carrot)
    {
        carrot::CarrotDestinationV1 miner_destination;
        make_carrot_main_address_v1(miner_address.m_spend_public_key,
            miner_address.m_view_public_key,
            miner_destination);
        
        const carrot::CarrotPaymentProposalV1 normal_payment_proposal{
            .destination = miner_destination,
            .amount = reward,
            .randomness = carrot::gen_janus_anchor()
        };

        std::vector<carrot::CarrotCoinbaseEnoteV1> coinbase_enotes;
        carrot::get_coinbase_output_enotes({normal_payment_proposal},
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

        if (hf_version >= 4)
            tx.version = 2;
        else
            tx.version = 1;

        tx.unlock_time = height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
        tx.vin.push_back(in);

        tx.invalidate_hashes();
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_miner_tx_fake_reward_1out(const size_t height,
    const rct::xmr_amount reward,
    const cryptonote::account_public_address &miner_address,
    const uint8_t hf_version)
{
    cryptonote::transaction tx;
    const bool r = construct_miner_tx_fake_reward_1out(height, reward, miner_address, tx, hf_version);
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
    const std::unordered_map<crypto::public_key, cryptonote::subaddress_index> &subaddresses,
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
        subaddresses,
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
void fake_pruned_blockchain::add_block(const uint8_t hf_version,
    std::vector<cryptonote::transaction> &&txs,
    const cryptonote::account_public_address &miner_address)
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
        hf_version);

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
    assert_chain_count();
    CHECK_AND_ASSERT_THROW_MES(blk.major_version >= this->hf_version(), 
        "hf version too low");
    CHECK_AND_ASSERT_THROW_MES(blk.tx_hashes.size() == pruned_txs.size(),
        "wrong number of txs provided");
    CHECK_AND_ASSERT_THROW_MES(blk.tx_hashes.size() == prunable_hashes.size(),
        "wrong number of prunable hashes provided");

    size_t total_block_weight = 0;
    for (const cryptonote::transaction &tx : pruned_txs)
    {
        total_block_weight += tx.pruned
                ? cryptonote::get_pruned_transaction_weight(tx)
                : cryptonote::get_transaction_weight(tx);
    }

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

    tools::wallet2::parsed_block par_blk;
    par_blk.hash = cryptonote::get_block_hash(blk);
    par_blk.block = std::move(blk);
    par_blk.txes = std::move(pruned_txs);
    {
        auto &tx_o_indices = tools::add_element(par_blk.o_indices.indices);
        for (size_t n = 0; n < par_blk.block.miner_tx.vout.size(); ++n)
            tx_o_indices.indices.push_back(m_output_index++);
    }
    for (const cryptonote::transaction &tx : par_blk.txes)
    {
        auto &tx_o_indices = tools::add_element(par_blk.o_indices.indices);
        for (size_t n = 0; n < tx.vout.size(); ++n)
            tx_o_indices.indices.push_back(m_output_index++);
    }
    par_blk.error = false;

    m_block_entries.emplace_back(std::move(blk_entry));
    m_parsed_blocks.emplace_back(std::move(par_blk));
}
//----------------------------------------------------------------------------------------------------------------------
void fake_pruned_blockchain::pop_block()
{
    assert_chain_count();

    CHECK_AND_ASSERT_THROW_MES(m_block_entries.size() >= 2, "Cannot pop starting block");

    m_block_entries.pop_back();
    m_parsed_blocks.pop_back();
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
    CHECK_AND_ASSERT_THROW_MES(!m_block_entries.empty(), "blockchain missing starting block");

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
