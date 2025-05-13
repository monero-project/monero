// Copyright (c) 2024, The Monero Project
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

#include "blockchain_db_utils.h"

#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctSigs.h"

#include "profile_tools.h"

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Helper function to group outputs by last locked block idx
static uint64_t set_tx_outs_by_last_locked_block(const cryptonote::transaction &tx,
    const uint64_t &first_output_id,
    const uint64_t block_idx,
    const bool miner_tx,
    fcmp_pp::curve_trees::OutsByLastLockedBlock &outs_by_last_locked_block_inout,
    std::unordered_map<uint64_t/*output_id*/, uint64_t/*last locked block_id*/> &timelocked_outputs_inout)
{
    const uint64_t last_locked_block = cryptonote::get_last_locked_block_index(tx.unlock_time, block_idx);
    const bool has_custom_timelock = cryptonote::is_custom_timelocked(miner_tx, last_locked_block, block_idx);

    uint64_t getting_commitment_ns = 0;

    for (std::size_t i = 0; i < tx.vout.size(); ++i)
    {
        const auto &out = tx.vout[i];

        crypto::public_key output_public_key;
        if (!cryptonote::get_output_public_key(out, output_public_key))
            throw std::runtime_error("Could not get an output public key from a tx output.");
        static_assert(CURRENT_TRANSACTION_VERSION == 2, "This section of code was written with 2 tx versions in mind. "
            "Revisit this section and update for the new tx version.");
        CHECK_AND_ASSERT_THROW_MES(tx.version == 1 || tx.version == 2, "encountered unexpected tx version");

        TIME_MEASURE_NS_START(getting_commitment);

        rct::key commitment;
        CHECK_AND_ASSERT_THROW_MES(rct::getCommitment(tx, i, commitment), "failed to get tx commitment");

        TIME_MEASURE_NS_FINISH(getting_commitment);

        const fcmp_pp::curve_trees::OutputPair output_pair{
                .output_pubkey = output_public_key,
                .commitment    = commitment
            };

        const uint64_t output_id = first_output_id + i;
        const fcmp_pp::curve_trees::OutputContext output_context{
                .output_id       = output_id,
                .torsion_checked = cryptonote::tx_outs_checked_for_torsion(tx),
                .output_pair     = output_pair
            };

        if (has_custom_timelock)
        {
            timelocked_outputs_inout[output_id] = last_locked_block;
        }

        outs_by_last_locked_block_inout[last_locked_block].emplace_back(output_context);

        getting_commitment_ns += getting_commitment;
    }

    LOG_PRINT_L3("getting_commitment_ms: " << getting_commitment_ns / 1000);

    return tx.vout.size();
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
namespace cryptonote
{
OutsByLastLockedBlockMeta get_outs_by_last_locked_block(
    const cryptonote::transaction &miner_tx,
    const std::vector<std::reference_wrapper<const cryptonote::transaction>> &txs,
    const uint64_t first_output_id,
    const uint64_t block_idx)
{
    OutsByLastLockedBlockMeta outs_by_last_locked_block_meta_out;
    outs_by_last_locked_block_meta_out.next_output_id = first_output_id;

    // Get miner tx's leaf tuples
    outs_by_last_locked_block_meta_out.next_output_id += set_tx_outs_by_last_locked_block(
        miner_tx,
        outs_by_last_locked_block_meta_out.next_output_id,
        block_idx,
        true/*miner_tx*/,
        outs_by_last_locked_block_meta_out.outs_by_last_locked_block,
        outs_by_last_locked_block_meta_out.timelocked_outputs);

    // Get all other txs' leaf tuples
    for (const auto &tx : txs)
    {
        outs_by_last_locked_block_meta_out.next_output_id += set_tx_outs_by_last_locked_block(
            tx.get(),
            outs_by_last_locked_block_meta_out.next_output_id,
            block_idx,
            false/*miner_tx*/,
            outs_by_last_locked_block_meta_out.outs_by_last_locked_block,
            outs_by_last_locked_block_meta_out.timelocked_outputs);
    }

    return outs_by_last_locked_block_meta_out;
}
//----------------------------------------------------------------------------------------------------------------------
}//namespace cryptonote
