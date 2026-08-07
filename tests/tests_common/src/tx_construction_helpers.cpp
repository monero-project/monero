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
#include "tests_common/tx_construction_helpers.h"

//local headers
#include "crypto/generators.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "tests"

namespace mock
{
//----------------------------------------------------------------------------------------------------------------------
cryptonote::transaction construct_miner_tx_with_reward(const size_t height,
    const rct::xmr_amount reward,
    const cryptonote::account_public_address &miner_address,
    const uint8_t hf_version)
{
    constexpr rct::xmr_amount already_generated_coins = 10000 * COIN;
    constexpr size_t max_outs = 999;

    const size_t median_weight = cryptonote::get_min_block_weight(hf_version);
    const size_t weight_limit = 2 * std::max(median_weight, cryptonote::get_min_block_weight(hf_version));
    const cryptonote::blobdata extra_nonce;

    cryptonote::transaction tx;
    size_t cumulative_tx_weight = median_weight * 9 / 10;
    size_t current_block_weight = cumulative_tx_weight;
    rct::xmr_amount fee = reward;
    bool r = cryptonote::construct_miner_tx(height, median_weight, already_generated_coins,
        current_block_weight, fee, miner_address, tx);
    CHECK_AND_ASSERT_THROW_MES(r, "Coinbase transaction failed on first try");
    while (
        !r
        || cumulative_tx_weight + cryptonote::get_transaction_weight(tx) != current_block_weight
        || cryptonote::get_outs_money_amount(tx) != reward)
    {
        // adjustments
        const uint64_t coinbase_weight = cryptonote::get_transaction_weight(tx);
        const rct::xmr_amount actual_reward = cryptonote::get_outs_money_amount(tx);
        if (!r || cumulative_tx_weight + coinbase_weight > current_block_weight)
            cumulative_tx_weight = cumulative_tx_weight * 19 / 20;
        else
            cumulative_tx_weight = current_block_weight - coinbase_weight;
        if (r)
        {
            CHECK_AND_ASSERT_THROW_MES(actual_reward >= fee, "logic error in coinbase reward adjustment");
            if (actual_reward > reward)
            {
                if (fee >= (actual_reward - reward))
                    fee -= (actual_reward - reward);
                else
                    cumulative_tx_weight = (cumulative_tx_weight + weight_limit + 1) / 2;
            }
            else
                fee += (reward - actual_reward);
        }

        // re-make with new params
        current_block_weight = cumulative_tx_weight + coinbase_weight;
        r = cryptonote::construct_miner_tx(height, median_weight, already_generated_coins,
            current_block_weight, fee, miner_address, tx, extra_nonce, max_outs, hf_version);
    }

    CHECK_AND_ASSERT_THROW_MES((tx.version >= 2) == (hf_version >= HF_VERSION_DYNAMIC_FEE), "wrong tx version");
    CHECK_AND_ASSERT_THROW_MES(!tx.vout.empty(), "tx empty");
    CHECK_AND_ASSERT_THROW_MES(cryptonote::check_output_types(tx, hf_version), "wrong tx out type");
    CHECK_AND_ASSERT_THROW_MES(tx.unlock_time == height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW, "wrong tx unlock time");
    CHECK_AND_ASSERT_THROW_MES(boost::get<cryptonote::txin_gen>(tx.vin.at(0)).height == height, "tx wrong height");

    return tx;
}
//----------------------------------------------------------------------------------------------------------------------
cryptonote::tx_source_entry gen_tx_source_entry_fake_members(
    const stripped_down_tx_source_entry_t &in,
    const size_t mixin,
    const uint64_t max_global_output_index)
{
    const size_t ring_size = mixin + 1;

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
            in.is_rct ? rct::pkGen() : rct::zeroCommitVartime(in.amount)};
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
    res.rct = in.is_rct;
    res.real_out_tx_key = in.real_out_tx_key;
    res.real_out_additional_tx_keys = in.real_out_additional_tx_keys;
    res.real_output_in_tx_index = in.local_output_index;
    res.amount = in.amount;
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
    const crypto::hash &payment_id,
    const rct::xmr_amount fee,
    const uint8_t hf_version,
    crypto::secret_key &main_tx_privkey_out,
    std::vector<crypto::secret_key> &additional_tx_privkeys_out,
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
            .is_rct = rct,
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

    // count integrated addresses and check <= 1
    size_t num_integrated = 0;
    for (const cryptonote::tx_destination_entry &dst : destinations)
        num_integrated += dst.is_integrated;
    CHECK_AND_ASSERT_THROW_MES(num_integrated <= 1, "max 1 integrated address allowed");

    // make put unencrypted payment_id in tx_extra
    std::vector<uint8_t> extra;
    if (payment_id != crypto::null_hash)
    {
        const bool is_long_payment_id = memcmp(payment_id.data + 8, crypto::null_hash.data, 32 - 8);
        if (!is_long_payment_id && !num_integrated)
            MWARNING("Short payment ID provided but no destination marked as integrated");

        cryptonote::tx_extra_nonce extra_nonce;
        if (is_long_payment_id)
        {
            cryptonote::set_payment_id_to_tx_extra_nonce(extra_nonce.nonce, payment_id);
        }
        else // !is_long_payment_id
        {
            crypto::hash8 pid_8;
            memcpy(&pid_8, &payment_id, sizeof(pid_8));
            cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce.nonce, pid_8);
        }

        CHECK_AND_ASSERT_THROW_MES(cryptonote::add_extra_nonce_to_tx_extra(extra, extra_nonce.nonce),
            "failed to add nonce to tx_extra");
    }

    const bool r = cryptonote::construct_tx_and_get_tx_key(
        sender_account_keys,
        sender_subaddress_map,
        sources,
        destinations,
        change_addr,
        extra,
        tx,
        main_tx_privkey_out,
        additional_tx_privkeys_out,
        rct,
        rct_config,
        use_view_tags);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to construct tx");
    return tx;
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
    crypto::secret_key dummy_main_tx_privkey;
    std::vector<crypto::secret_key> dummy_additional_tx_privkeys;
    return construct_pre_carrot_tx_with_fake_inputs(sender_account_keys,
        sender_subaddress_map,
        std::forward<std::vector<stripped_down_tx_source_entry_t>>(stripped_sources),
        destinations,
        change_addr,
        crypto::null_hash,
        fee,
        hf_version,
        dummy_main_tx_privkey,
        dummy_additional_tx_privkeys,
        sweep_unmixable_override);
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
const cryptonote::account_public_address null_addr{
    .m_spend_public_key = crypto::get_G(),
    .m_view_public_key = crypto::get_G()
};
//----------------------------------------------------------------------------------------------------------------------
} //namespace mock
