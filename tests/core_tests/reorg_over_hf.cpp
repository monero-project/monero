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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "reorg_over_hf.h"

using namespace cryptonote;

#define CALLBACK_SIG (cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events) -> bool

#define V14_V15_NMIX 10 // accepted by both v14 and v15
#define MAKE_BP_OR_BPP_TX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, HEAD, is_bpp) \
    cryptonote::transaction TX_NAME;                                           \
    construct_tx_to_key(VEC_EVENTS, TX_NAME, HEAD, FROM, TO, AMOUNT,           \
        TESTS_DEFAULT_FEE, V14_V15_NMIX, /*rct=*/true,                         \
        /*range_proof_type=*/rct::RangeProofPaddedBulletproof,                 \
        /*bp_version=*/(is_bpp) ? 4 : 3);                                      \
    VEC_EVENTS.push_back(TX_NAME);

#define REGISTER_STATIC_CALLBACK(fname) register_callback(#fname, &fname)
#define DO_STATIC_CALLBACK(events, fname) DO_CALLBACK(events, #fname)

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
namespace
{
static_assert(MAX_HF_VERSION == 16,
    "Add analogous test cases for the CLSAG => FCMP++ transition");
static const event_replay_settings settings{
    .hard_forks = {{
        {1, 1},
        {2, 2},
        {3, 3},
        {4, 4},
        {5, 5},
        {6, 6},
        {7, 7},
        {8, 8},
        {9, 9},
        {10, 10},
        {11, 20},
        {12, 30},
        {13, 40},
        {14, 60},
        {15, 80},
    }}
};
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
auto cb_verify_core_prefork_ready CALLBACK_SIG
{
    const size_t n_forks = settings.hard_forks->size();
    if (n_forks < 2)
        return false;
    const auto &second_last_hf = settings.hard_forks->at(n_forks - 2);
    const auto &last_hf = settings.hard_forks->at(n_forks - 1);

    return c.get_blockchain_storage().get_current_hard_fork_version() == second_last_hf.first
        && c.get_current_blockchain_height() == last_hf.second - 1;
}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
auto cb_verify_core_fork_ready CALLBACK_SIG
{
    const size_t n_forks = settings.hard_forks->size();
    if (n_forks < 2)
        return false;
    const auto &last_hf = settings.hard_forks->at(n_forks - 1);

    return c.get_blockchain_storage().get_current_hard_fork_version() == last_hf.first
        && c.get_current_blockchain_height() == last_hf.second;
}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
bool setup(std::vector<test_event_entry> &events,
    test_generator &generator,
    block &blk_0r,
    cryptonote::account_base &miner_account)
{
    // Miner account and genesis block
    miner_account.generate();
    constexpr uint64_t ts_start = 1338224400;
    generator.construct_block(blk_0r, miner_account, ts_start);
    events.push_back(blk_0r);

    // Set HFs
    events.push_back(settings);

    // Check settings / invariants
    CHECK_AND_ASSERT_MES(settings.hard_forks->size() >= 2, false, "Bad settings: num HFs");
    const v_hardforks_t::value_type &last_hf = settings.hard_forks->at(settings.hard_forks->size() - 1);
    CHECK_AND_ASSERT_MES(last_hf.first == HF_VERSION_BULLETPROOF_PLUS, false, "Bad settings: HF version");
    CHECK_AND_ASSERT_MES(last_hf.second > CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE, false, "Bad settings: HF height");
    CHECK_AND_ASSERT_MES(10 == CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE, false, "Unexpected tx defult unlock time");

    // Mine to 1 block before the BP+ fork
    size_t height = 1;
    for (size_t fork_index = 1; fork_index < settings.hard_forks->size(); ++fork_index)
    {
        const auto &hf = settings.hard_forks->at(fork_index);
        const auto &prev_hf = settings.hard_forks->at(fork_index - 1);
        assert(prev_hf.first < hf.first);
        assert(prev_hf.second < hf.second);

        const bool is_last = fork_index + 1 == settings.hard_forks->size();
        const size_t count = (hf.second - height) - is_last;
        for (size_t i = 0; i < count; ++i)
        {
            generator.construct_block(blk_0r, blk_0r, miner_account,
                {}, prev_hf.first);
            events.push_back(blk_0r);
        }
        height += count;
    }
    CHECK_AND_ASSERT_MES(HF_VERSION_BULLETPROOF_PLUS - 1 == blk_0r.major_version,
        false, "reorg_over_hf: setup: made wrong block type");
    CHECK_AND_ASSERT_MES(last_hf.second - 2 == get_block_height(blk_0r),
        false, "reorg_over_hf: setup: made wrong block height");
    DO_STATIC_CALLBACK(events, cb_verify_core_prefork_ready);

    return true;
}
} //anonymous namespace
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
gen_reorg_over_hf_1::gen_reorg_over_hf_1()
{
    REGISTER_STATIC_CALLBACK(cb_verify_core_prefork_ready);
    REGISTER_STATIC_CALLBACK(cb_verify_core_fork_ready);
}
//--------------------------------------------------------------------------------------------------
bool gen_reorg_over_hf_1::generate(std::vector<test_event_entry>& events) const
{
    // Setup
    test_generator generator;
    block blk_0r;
    cryptonote::account_base miner_account;
    setup(events, generator, blk_0r, miner_account);
    MAKE_ACCOUNT(events, recipient_account_1);

    // Add last pre-BP+ block, next block is BP+
    REWIND_BLOCKS_N(events, blk_bp, blk_0r, miner_account, 1)
    DO_STATIC_CALLBACK(events, cb_verify_core_fork_ready)

    // Make first BP+ block containing 1 BP+ tx
    const auto &last_hf = settings.hard_forks->back();
    MAKE_BP_OR_BPP_TX(events, tx_00, miner_account, recipient_account_1, MK_COINS(5), blk_0r, true);
    MAKE_NEXT_BLOCK_TX_LIST_HF(events, blk_left_1, blk_bp, miner_account, {tx_00}, last_hf.first);

    // Make a competing chain for first BP+ block containg same tx
    MAKE_NEXT_BLOCK_TX_LIST_HF(events, blk_right_1, blk_bp, miner_account, {tx_00}, last_hf.first);
    MAKE_NEXT_BLOCK(events, blk_right_2, blk_right_1, miner_account)

    return true;
}
//--------------------------------------------------------------------------------------------------
gen_reorg_over_hf_2::gen_reorg_over_hf_2()
{
    REGISTER_STATIC_CALLBACK(cb_verify_core_prefork_ready);
    REGISTER_STATIC_CALLBACK(cb_verify_core_fork_ready);
    REGISTER_CALLBACK("set_expect_block_success", gen_reorg_over_hf_2::cb_set_expect_block_success);
    REGISTER_CALLBACK("set_expect_block_failure", gen_reorg_over_hf_2::cb_set_expect_block_failure);
    REGISTER_CALLBACK("set_expect_block_inconclusive", gen_reorg_over_hf_2::cb_set_expect_block_inconclusive);
    REGISTER_CALLBACK("set_expect_main_block", gen_reorg_over_hf_2::cb_set_expect_main_block);
    REGISTER_CALLBACK("set_expect_alt_block", gen_reorg_over_hf_2::cb_set_expect_alt_block);
}
//--------------------------------------------------------------------------------------------------
bool gen_reorg_over_hf_2::generate(std::vector<test_event_entry>& events) const
{
    // Setup
    test_generator generator;
    block blk_0r;
    cryptonote::account_base miner_account;
    setup(events, generator, blk_0r, miner_account);
    MAKE_ACCOUNT(events, recipient_account_1);

    // Get source info for BP+ tx before we add the block, so if it gets
    // rejected, we know that it was due to NIC rule validation, not unlock
    // height
    std::vector<cryptonote::tx_source_entry> sources;
    std::vector<cryptonote::tx_destination_entry> destinations;
    fill_tx_sources_and_destinations(events, blk_0r, miner_account, recipient_account_1, MK_COINS(15), TESTS_DEFAULT_FEE,
        V14_V15_NMIX, sources, destinations);

    // Add last pre-BP+ block, next block is BP+
    REWIND_BLOCKS_N(events, blk_right_bp, blk_0r, miner_account, 1)
    DO_STATIC_CALLBACK(events, cb_verify_core_fork_ready)

    // Construct BP+ transaction for first BP+ block
    cryptonote::transaction bpp_tx;
    bool r = construct_tx_rct(miner_account.get_keys(),
        sources, destinations, {}, {}, bpp_tx,
        /*rct=*/true, rct::RangeProofPaddedBulletproof,
        /*bp_version=*/4);
    CHECK_AND_ASSERT_MES(r, false, "Failed to construct BP tx");
    CHECK_AND_ASSERT_MES(2 == bpp_tx.version, false, "Constructed transaction isn't RingCT");
    CHECK_AND_ASSERT_MES(bpp_tx.rct_signatures.type == rct::RCTTypeBulletproofPlus,
        false, "Constructed transaction isn't CLSAG+BP");
    events.push_back(bpp_tx);

    // Make first BP+ block containing 1 BP+ tx
    const auto &last_hf = settings.hard_forks->back();
    MAKE_NEXT_BLOCK_TX_LIST_HF(events, blk_right_bpp, blk_right_bp, miner_account, {bpp_tx}, last_hf.first);

    // Make a competing chain with BP+ tx in block below BP+ fork
    // This should fail validation in total, bad bad blocks may still be added to the DB
    DO_CALLBACK(events, "set_expect_block_inconclusive");
    DO_CALLBACK(events, "set_expect_alt_block");
    MAKE_NEXT_BLOCK_TX_LIST(events, blk_wrong_bp, blk_0r, miner_account, {bpp_tx});
    MAKE_NEXT_BLOCK_HF(events, blk_wrong_bpp, blk_wrong_bp, miner_account, last_hf.first);
    DO_CALLBACK(events, "set_expect_block_failure");
    MAKE_NEXT_BLOCK(events, blk_wrong_bpp_2, blk_wrong_bpp, miner_account);
    MAKE_NEXT_BLOCK(events, blk_wrong_bpp_3, blk_wrong_bpp_2, miner_account);

    // Go back to adding blocks to main, correct chain: successful rollback should have occurred
    DO_CALLBACK(events, "set_expect_block_success");
    DO_CALLBACK(events, "set_expect_main_block");
    MAKE_NEXT_BLOCK(events, blk_right_bpp_2, blk_right_bpp, miner_account);

    return true;
}
//--------------------------------------------------------------------------------------------------
bool gen_reorg_over_hf_2::check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& /*blk*/)
{
    //! @note: we count orphaning as a failure here

    CHECK_AND_ASSERT_MES(!(bvc.m_marked_as_orphaned && bvc.m_added_to_main_chain),
        false, "Strange block result: both orphaned and added to main chain?");

    if (m_expect_block_failure.has_value())
    {
        const bool failed_or_orphaned = bvc.m_verifivation_failed || bvc.m_marked_as_orphaned;
        if (*m_expect_block_failure)
        {
            CHECK_AND_ASSERT_MES(failed_or_orphaned, false,
                "Expected block verification failure, but got success instead");
        }
        else
        {
            CHECK_AND_ASSERT_MES(!failed_or_orphaned, false,
                "Expected block verification success, but got failure instead");
        }
    }

    if (m_expect_alt_block)
    {
        CHECK_AND_ASSERT_MES(!bvc.m_added_to_main_chain, false,
            "Expected block *not* to be added to the main chain, but it was");
    }
    else
    {
        CHECK_AND_ASSERT_MES(bvc.m_added_to_main_chain, false,
            "Expected block to be added to the main chain, but it wasn't");
    }

    return true;
}
//--------------------------------------------------------------------------------------------------
bool gen_reorg_over_hf_2::cb_set_expect_block_success(cryptonote::core&, size_t,
    const std::vector<test_event_entry>&)
{
    m_expect_block_failure = false;
    return true;
}
//--------------------------------------------------------------------------------------------------
bool gen_reorg_over_hf_2::cb_set_expect_block_failure(cryptonote::core&, size_t,
    const std::vector<test_event_entry>&)
{
    m_expect_block_failure = true;
    return true;
}
//--------------------------------------------------------------------------------------------------
bool gen_reorg_over_hf_2::cb_set_expect_block_inconclusive(cryptonote::core&,
    size_t, const std::vector<test_event_entry> &)
{
    m_expect_block_failure.reset();
    return true;
}
//--------------------------------------------------------------------------------------------------
bool gen_reorg_over_hf_2::cb_set_expect_main_block(cryptonote::core&, size_t,
    const std::vector<test_event_entry>&)
{
    m_expect_alt_block = false;
    return true;
}
//--------------------------------------------------------------------------------------------------
bool gen_reorg_over_hf_2::cb_set_expect_alt_block(cryptonote::core& c,
    size_t, const std::vector<test_event_entry>&)
{
    m_expect_alt_block = true;
    return true;
}
//--------------------------------------------------------------------------------------------------
