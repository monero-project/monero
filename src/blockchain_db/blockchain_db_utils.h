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

#include "cryptonote_basic/cryptonote_basic.h"
#include "fcmp_pp/curve_trees.h"

#include <functional>
#include <utility>
#include <vector>

namespace cryptonote
{

struct OutsByLastLockedBlockMeta
{
    fcmp_pp::curve_trees::OutsByLastLockedBlock outs_by_last_locked_block;
    std::unordered_map<uint64_t/*output_id*/, uint64_t/*last locked block_id*/> timelocked_outputs;
    uint64_t next_output_id;
};

// These functions internally rely on ringct for zeroCommitVartime. I implemented in this blockchain_db_utils file
// instead of cryptonote_basic (where it would seem the better place to put it) to avoid a circular dependency between
// ringct <> cryptonote_basic.
// Note that zeroCommitVartime is expensive.
OutsByLastLockedBlockMeta get_outs_by_last_locked_block(
    const cryptonote::transaction &miner_tx,
    const std::vector<std::reference_wrapper<const cryptonote::transaction>> &txs,
    const uint64_t first_output_id,
    const uint64_t block_idx);
}
