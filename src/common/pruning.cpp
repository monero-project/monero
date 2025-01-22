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

#include "cryptonote_config.h"
#include "misc_log_ex.h"
#include "crypto/crypto.h"
#include "pruning.h"

namespace tools
{

uint32_t make_pruning_seed(uint32_t stripe, uint32_t log_stripes)
{
  CHECK_AND_ASSERT_THROW_MES(log_stripes <= PRUNING_SEED_LOG_STRIPES_MASK, "log_stripes out of range");
  CHECK_AND_ASSERT_THROW_MES(stripe > 0 && stripe <= (1ul << log_stripes), "stripe out of range");
  return (log_stripes << PRUNING_SEED_LOG_STRIPES_SHIFT) | ((stripe - 1) << PRUNING_SEED_STRIPE_SHIFT);
}

bool has_unpruned_block(uint64_t block_height, uint64_t blockchain_height, uint32_t pruning_seed)
{
  const uint32_t stripe = get_pruning_stripe(pruning_seed);
  if (stripe == 0)
    return true;
  const uint32_t log_stripes = get_pruning_log_stripes(pruning_seed);
  uint32_t block_stripe = get_pruning_stripe(block_height, blockchain_height, log_stripes);
  return block_stripe == 0 || block_stripe == stripe;
}

uint32_t get_pruning_stripe(uint64_t block_height, uint64_t blockchain_height, uint32_t log_stripes)
{
  if (block_height + CRYPTONOTE_PRUNING_TIP_BLOCKS >= blockchain_height)
    return 0;
  return ((block_height / CRYPTONOTE_PRUNING_STRIPE_SIZE) & (uint64_t)((1ul << log_stripes) - 1)) + 1;
}

uint32_t get_pruning_seed(uint64_t block_height, uint64_t blockchain_height, uint32_t log_stripes)
{
  const uint32_t stripe = get_pruning_stripe(block_height, blockchain_height, log_stripes);
  if (stripe == 0)
    return 0;
  return make_pruning_seed(stripe, log_stripes);
}

uint64_t get_next_unpruned_block_height(uint64_t block_height, uint64_t blockchain_height, uint32_t pruning_seed)
{
  CHECK_AND_ASSERT_MES(block_height <= CRYPTONOTE_MAX_BLOCK_NUMBER+1, block_height, "block_height too large");
  CHECK_AND_ASSERT_MES(blockchain_height <= CRYPTONOTE_MAX_BLOCK_NUMBER+1, block_height, "blockchain_height too large");
  const uint32_t stripe = get_pruning_stripe(pruning_seed);
  if (stripe == 0)
    return block_height;
  if (block_height + CRYPTONOTE_PRUNING_TIP_BLOCKS >= blockchain_height)
    return block_height;
  const uint32_t seed_log_stripes = get_pruning_log_stripes(pruning_seed);
  const uint64_t log_stripes = seed_log_stripes ? seed_log_stripes : CRYPTONOTE_PRUNING_LOG_STRIPES;
  const uint64_t mask = (1ul << log_stripes) - 1;
  const uint32_t block_pruning_stripe = ((block_height / CRYPTONOTE_PRUNING_STRIPE_SIZE) & mask) + 1;
  if (block_pruning_stripe == stripe)
    return block_height;
  const uint64_t cycles = ((block_height / CRYPTONOTE_PRUNING_STRIPE_SIZE) >> log_stripes);
  const uint64_t cycle_start = cycles + ((stripe > block_pruning_stripe) ? 0 : 1);
  const uint64_t h = cycle_start * (CRYPTONOTE_PRUNING_STRIPE_SIZE << log_stripes) + (stripe - 1) * CRYPTONOTE_PRUNING_STRIPE_SIZE;
  if (h + CRYPTONOTE_PRUNING_TIP_BLOCKS > blockchain_height)
    return blockchain_height < CRYPTONOTE_PRUNING_TIP_BLOCKS ? 0 : blockchain_height - CRYPTONOTE_PRUNING_TIP_BLOCKS;
  CHECK_AND_ASSERT_MES(h >= block_height, block_height, "h < block_height, unexpected");
  return h;
}

uint64_t get_next_pruned_block_height(uint64_t block_height, uint64_t blockchain_height, uint32_t pruning_seed)
{
  const uint32_t stripe = get_pruning_stripe(pruning_seed);
  if (stripe == 0)
    return blockchain_height;
  if (block_height + CRYPTONOTE_PRUNING_TIP_BLOCKS >= blockchain_height)
    return blockchain_height;
  const uint32_t seed_log_stripes = get_pruning_log_stripes(pruning_seed);
  const uint64_t log_stripes = seed_log_stripes ? seed_log_stripes : CRYPTONOTE_PRUNING_LOG_STRIPES;
  const uint64_t mask = (1ul << log_stripes) - 1;
  const uint32_t block_pruning_seed = ((block_height / CRYPTONOTE_PRUNING_STRIPE_SIZE) & mask) + 1;
  if (block_pruning_seed != stripe)
    return block_height;
  const uint32_t next_stripe = 1 + (block_pruning_seed & mask);
  return get_next_unpruned_block_height(block_height, blockchain_height, tools::make_pruning_seed(next_stripe, log_stripes));
}

uint32_t get_random_stripe()
{
  return 1 + crypto::rand<uint8_t>() % (1ul << CRYPTONOTE_PRUNING_LOG_STRIPES);
}

}

