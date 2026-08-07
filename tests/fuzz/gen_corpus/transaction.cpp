// Copyright (c) 2026, The Monero Project
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

#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "gen_corpus.h"
#include "misc_log_ex.h"
#include "tests_common/tx_construction_helpers.h"

#define SET_SEED_CORPUS_TO_TX(tx) do {                                                       \
    const cryptonote::blobdata tx_blob = cryptonote::tx_to_blob(tx);                         \
    SET_SEED_CORPUS(reinterpret_cast<const unsigned char*>(tx_blob.data()), tx_blob.size()); \
} while (0)                                                                                  \

namespace
{
cryptonote::account_public_address get_core_donation_address()
{
    const std::string address =
        "888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H";
    cryptonote::address_parse_info parsed_address;
    bool r = cryptonote::get_account_address_from_str(parsed_address, cryptonote::MAINNET, address);
    CHECK_AND_ASSERT_THROW_MES(r, "Failed to parse GF address");
    return parsed_address.address;
}

cryptonote::transaction gen_miner_tx(const std::uint8_t hf_version)
{
    const cryptonote::account_public_address miner_address = get_core_donation_address();

    constexpr size_t height = 999999;
    constexpr rct::xmr_amount reward = 2 * COIN;

    return mock::construct_miner_tx_with_reward(height, reward, miner_address, hf_version);
}

cryptonote::transaction gen_non_miner_tx(const std::uint8_t hf_version, const bool sweep_unmixable_override = false)
{
    std::vector<cryptonote::tx_destination_entry> destinations{
        {cryptonote::tx_destination_entry(COIN, get_core_donation_address(), false)}};
    return mock::construct_pre_carrot_tx_with_fake_inputs(destinations,
        COIN / 100, hf_version, sweep_unmixable_override);
}
} //anonymous namespace

SEED_CORPUS(transaction, coinbase_v1)
{
    const cryptonote::transaction coinbase_v1_tx = gen_miner_tx(/*hf_version=*/1);
    SET_SEED_CORPUS_TO_TX(coinbase_v1_tx);
}

SEED_CORPUS(transaction, v1)
{
    const cryptonote::transaction v1_tx = gen_non_miner_tx(1);
    SET_SEED_CORPUS_TO_TX(v1_tx);
}

SEED_CORPUS(transaction, v1_viewtagged)
{
    const cryptonote::transaction v1_tx = gen_non_miner_tx(HF_VERSION_VIEW_TAGS, true);
    SET_SEED_CORPUS_TO_TX(v1_tx);
}

SEED_CORPUS(transaction, coinbase_v2)
{
    const cryptonote::transaction coinbase_v2_tx = gen_miner_tx(HF_VERSION_DYNAMIC_FEE);
    SET_SEED_CORPUS_TO_TX(coinbase_v2_tx);
}

SEED_CORPUS(transaction, coinbase_v2_vt)
{
    const cryptonote::transaction coinbase_v2_vt_tx = gen_miner_tx(HF_VERSION_VIEW_TAGS);
    SET_SEED_CORPUS_TO_TX(coinbase_v2_vt_tx);
}

SEED_CORPUS(transaction, v2_mlsag_full)
{
    const cryptonote::transaction v2_mlsag_tx = gen_non_miner_tx(HF_VERSION_DYNAMIC_FEE);
    SET_SEED_CORPUS_TO_TX(v2_mlsag_tx);
}

/// @TODO: v2_mlsag_simple

SEED_CORPUS(transaction, v2_bp)
{
    const cryptonote::transaction v2_bp_tx = gen_non_miner_tx(HF_VERSION_PER_BYTE_FEE);
    SET_SEED_CORPUS_TO_TX(v2_bp_tx);
}

SEED_CORPUS(transaction, v2_bp_short)
{
    const cryptonote::transaction v2_bp_short_tx = gen_non_miner_tx(HF_VERSION_SMALLER_BP);
    SET_SEED_CORPUS_TO_TX(v2_bp_short_tx);
}

SEED_CORPUS(transaction, v2_clsag)
{
    const cryptonote::transaction v2_clsag_tx = gen_non_miner_tx(HF_VERSION_CLSAG);
    SET_SEED_CORPUS_TO_TX(v2_clsag_tx);
}

SEED_CORPUS(transaction, v2_bpp)
{
    const cryptonote::transaction v2_bpp_tx = gen_non_miner_tx(HF_VERSION_BULLETPROOF_PLUS);
    SET_SEED_CORPUS_TO_TX(v2_bpp_tx);
}

static_assert(MAX_HF_VERSION == 16, "Add FCMP++ corpora below");
