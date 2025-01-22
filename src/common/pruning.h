// Copyright (c) 2018-2024, The Monero Project

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

#include <stdint.h>

namespace tools
{
  static constexpr uint32_t PRUNING_SEED_LOG_STRIPES_SHIFT = 7;
  static constexpr uint32_t PRUNING_SEED_LOG_STRIPES_MASK = 0x7;
  static constexpr uint32_t PRUNING_SEED_STRIPE_SHIFT = 0;
  static constexpr uint32_t PRUNING_SEED_STRIPE_MASK = 0x7f;

  constexpr inline uint32_t get_pruning_log_stripes(uint32_t pruning_seed) { return (pruning_seed >> PRUNING_SEED_LOG_STRIPES_SHIFT) & PRUNING_SEED_LOG_STRIPES_MASK; }
  inline uint32_t get_pruning_stripe(uint32_t pruning_seed) { if (pruning_seed == 0) return 0; return 1 + ((pruning_seed >> PRUNING_SEED_STRIPE_SHIFT) & PRUNING_SEED_STRIPE_MASK); }

  uint32_t make_pruning_seed(uint32_t stripe, uint32_t log_stripes);

  bool has_unpruned_block(uint64_t block_height, uint64_t blockchain_height, uint32_t pruning_seed);
  uint32_t get_pruning_stripe(uint64_t block_height, uint64_t blockchain_height, uint32_t log_stripes);
  uint32_t get_pruning_seed(uint64_t block_height, uint64_t blockchain_height, uint32_t log_stripes);
  uint64_t get_next_unpruned_block_height(uint64_t block_height, uint64_t blockchain_height, uint32_t pruning_seed);
  uint64_t get_next_pruned_block_height(uint64_t block_height, uint64_t blockchain_height, uint32_t pruning_seed);
  uint32_t get_random_stripe();
}

