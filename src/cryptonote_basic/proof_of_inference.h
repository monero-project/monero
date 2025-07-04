#pragma once

#include <string>
#include <cstdint>

#include "crypto/hash.h"

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
}

