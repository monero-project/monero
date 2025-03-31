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

#pragma once

#include "cryptonote_config.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "curve_trees.h"

#include <memory>


namespace fcmp_pp
{
namespace curve_trees
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Interface to sync the tree, keeping track of known output paths
// - Wallets can use this interface to sync the tree locally, making sure they can construct fcmp++'s for received
//   outputs using the outputs' latest paths in the tree, without revealing which output is being spent to the daemon.
// - The caller first calls register_output for any known received outputs.
// - The caller then calls sync_block, which identifies and updates known output paths in the tree.
// - The caller can get an output's latest path in the tree via get_output_path.
// - If there's a reorg, the caller can use pop_block, which trims the locally synced tree and updates paths as needed.
template<typename C1, typename C2>
class TreeSync
{
public:
    TreeSync(std::shared_ptr<CurveTrees<C1, C2>> curve_trees,
        const uint64_t max_reorg_depth = ORPHANED_BLOCKS_MAX_COUNT):
            m_curve_trees{curve_trees},
            m_max_reorg_depth{max_reorg_depth}
    {};

    // Registers an output with the TreeSync object so that syncing will keep track of the output's path in the tree
    // - Returns true on successful new insertion
    // - Returns false:
    //    - if the output is already registered.
    //    - if the TreeSync object has already synced the block in which the output unlocks. The scanner would not
    //      be able to determine the output's position in the tree in this case.
    virtual bool register_output(const OutputPair &output, const uint64_t last_locked_block_idx) = 0;

    // TODO: bool cancel_output_registration

    // Sync the outputs created in the provided block and grow the tree with outputs with last locked block block_idx
    // - The block must be contiguous to the most recently synced block
    // - If any registered outputs are present in the new leaf tuples, keeps track of their paths in the tree
    // - Uses the new leaf tuples to update any existing known output paths in the tree
    // TODO: Sync from arbitrary restore height:
    // - the client needs the last chunks at each layer at the given height, n leaf tuples, and n outputs in the chain
    // - the client also needs to know the state of all locked outputs at the given height, so that the client knows how to grow the tree correctly
    // - naive: client is required to build the tree from genesis
    // - better solution: daemon separately keeps a table of all timelocked outputs and their creation height into perpetuity
    // - client requests:
    // - all timelocked outputs with unlock height > current height. Once the hf is past, all timelocked outputs in the chain can potentially be hard-coded in the client.
    // - all coinbase outputs created in all blocks starting at current height - 60
    // - all normal outputs created in all blocks starting at current height - 10
    virtual void sync_block(const uint64_t block_idx,
        const crypto::hash &block_hash,
        const crypto::hash &prev_block_hash,
        const fcmp_pp::curve_trees::OutsByLastLockedBlock &outs_by_last_locked_block) = 0;

    // Trim from the locally synced tree and update any paths as necesary
    // - Returns false if we cannot pop any more blocks (if the max reorg depth is reached, or no more blocks to pop)
    virtual bool pop_block() = 0;

    // Get a registered output's path in the tree
    // - Returns false if the output is not registered
    // - Returns true with empty path_out if the output is registered but not yet included in the tree
    virtual bool get_output_path(const OutputPair &output, typename CurveTrees<C1, C2>::Path &path_out) const = 0;

    // Overwrite the max reorg depth
    void set_max_reorg_depth(const uint64_t max_reorg_depth) { m_max_reorg_depth = max_reorg_depth; };

// Internal member variables accessible by derived class
protected:
    std::shared_ptr<CurveTrees<C1, C2>> m_curve_trees;
    uint64_t m_max_reorg_depth;
};
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace curve_trees
}//namespace fcmp_pp
