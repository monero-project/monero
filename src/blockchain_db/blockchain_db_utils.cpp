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
#include "ringct/rctOps.h"

//----------------------------------------------------------------------------------------------------------------------
// Helper function to group outputs by unlock block
static uint64_t set_tx_outs_by_unlock_block(const cryptonote::transaction &tx,
    const uint64_t &first_output_id,
    const uint64_t block_idx,
    const bool miner_tx,
    fcmp_pp::curve_trees::OutputsByUnlockBlock &outs_by_unlock_block_inout)
{
    const uint64_t unlock_block = cryptonote::get_unlock_block_index(tx.unlock_time, block_idx);

    for (std::size_t i = 0; i < tx.vout.size(); ++i)
    {
        const auto &out = tx.vout[i];

        crypto::public_key output_public_key;
        if (!cryptonote::get_output_public_key(out, output_public_key))
            throw std::runtime_error("Could not get an output public key from a tx output.");

        static_assert(CURRENT_TRANSACTION_VERSION == 2, "This section of code was written with 2 tx versions in mind. "
            "Revisit this section and update for the new tx version.");
        CHECK_AND_ASSERT_THROW_MES(tx.version == 1 || tx.version == 2, "encountered unexpected tx version");

        if (!miner_tx && tx.version == 2)
            CHECK_AND_ASSERT_THROW_MES(tx.rct_signatures.outPk.size() > i, "unexpected size of outPk");

        rct::key commitment = (miner_tx || tx.version < 2)
            ? rct::zeroCommit(out.amount)
            : tx.rct_signatures.outPk[i].mask;

        auto output_pair = fcmp_pp::curve_trees::OutputPair{
                .output_pubkey = std::move(output_public_key),
                .commitment    = std::move(commitment)
            };

        auto output_context = fcmp_pp::curve_trees::OutputContext{
                .output_id   = first_output_id + i,
                .output_pair = std::move(output_pair)
            };

        if (outs_by_unlock_block_inout.find(unlock_block) == outs_by_unlock_block_inout.end())
        {
            auto new_vec = std::vector<fcmp_pp::curve_trees::OutputContext>{std::move(output_context)};
            outs_by_unlock_block_inout[unlock_block] = std::move(new_vec);
        }
        else
        {
            outs_by_unlock_block_inout[unlock_block].emplace_back(std::move(output_context));
        }
    }

    return tx.vout.size();
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
namespace cryptonote
{
uint64_t get_outs_by_unlock_block(
    const cryptonote::transaction &miner_tx,
    const std::vector<std::reference_wrapper<const cryptonote::transaction>> &txs,
    const uint64_t first_output_id,
    const uint64_t block_idx,
    fcmp_pp::curve_trees::OutputsByUnlockBlock &outs_by_unlock_block_out)
{
    outs_by_unlock_block_out.clear();

    uint64_t output_id = first_output_id;

    // Get miner tx's leaf tuples
    output_id += set_tx_outs_by_unlock_block(
        miner_tx,
        output_id,
        block_idx,
        true/*miner_tx*/,
        outs_by_unlock_block_out);

    // Get all other txs' leaf tuples
    for (const auto &tx : txs)
    {
        output_id += set_tx_outs_by_unlock_block(
            tx.get(),
            output_id,
            block_idx,
            false/*miner_tx*/,
            outs_by_unlock_block_out);
    }

    return output_id;
}
//----------------------------------------------------------------------------------------------------------------------
}//namespace cryptonote
