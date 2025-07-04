#include "proof_of_inference.h"
#include <cstring>

namespace cryptonote
{
  uint64_t calculate_inference_score(const std::string &model_output)
  {
    // Hash the model output and treat the first 8 bytes as
    // a little-endian 64-bit score. This deterministic score
    // can be used for experimental ordering of blocks.

    crypto::hash h = crypto::cn_fast_hash(model_output.data(), model_output.size());
    uint64_t score = 0;
    static_assert(sizeof(score) <= sizeof(h.data), "hash too small");
    std::memcpy(&score, h.data, sizeof(score));
    return score;
  }
}

