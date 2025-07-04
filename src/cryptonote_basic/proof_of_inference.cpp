#include "proof_of_inference.h"
#include <cstring>
#include <limits>
#include <boost/multiprecision/cpp_int.hpp>
#include "cryptonote_format_utils.h"

namespace cryptonote
{
  using boost::multiprecision::uint128_t;

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

  uint64_t calculate_block_inference_score(const block &b)
  {
    const blobdata blob = get_block_hashing_blob(b);
    return calculate_inference_score(blob);
  }

  bool check_inference_score(uint64_t score, difficulty_type difficulty)
  {
    const uint128_t max64 = std::numeric_limits<uint64_t>::max();
    return uint128_t(score) * difficulty <= max64;
  }
}

