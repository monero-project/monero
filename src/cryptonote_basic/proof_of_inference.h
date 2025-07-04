#pragma once

#include <string>
#include <cstdint>

#include "crypto/hash.h"
#include "cryptonote_basic.h"
#include "difficulty.h"

namespace cryptonote
{
  /**
   * \brief Calculate a deterministic score from a model output
   *
   * The output is hashed using `cn_fast_hash` and the first eight
   * bytes of the hash are interpreted as a little-endian 64-bit
   * value. This score can be used in experimental proof-of-inference
   * consensus algorithms.
   */
  uint64_t calculate_inference_score(const std::string &model_output);
  uint64_t calculate_block_inference_score(const block &b);
  bool check_inference_score(uint64_t score, difficulty_type difficulty);
}

