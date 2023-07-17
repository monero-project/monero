
#include <algorithm>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

#include "cryptonote_core/cryptonote_core.h"

#define HF_VERSION_RCT HF_VERSION_DYNAMIC_FEE

namespace cryptonote
{
namespace rpc
{
  namespace
  {
    output_distribution_data
      process_distribution(bool cumulative, std::uint64_t start_height, std::vector<std::uint64_t> distribution, std::uint64_t base)
    {
      if (!cumulative && !distribution.empty())
      {
        for (std::size_t n = distribution.size() - 1; 0 < n; --n)
          distribution[n] -= distribution[n - 1];
        distribution[0] -= base;
      }

      return {std::move(distribution), start_height, base};
    }
  }

  boost::optional<output_distribution_data>
    RpcHandler::get_output_distribution(const std::function<bool(uint64_t, uint64_t, uint64_t, uint64_t&, std::vector<uint64_t>&, uint64_t&)> &f, uint64_t amount, uint64_t from_height, uint64_t to_height, const std::function<crypto::hash(uint64_t)> &get_hash, bool cumulative, uint64_t blockchain_height)
  {
      static struct D
      {
        boost::mutex mutex;
        std::vector<std::uint64_t> cached_distribution;
        std::uint64_t cached_from, cached_to, cached_start_height, cached_base;
        crypto::hash cached_m10_hash;
        crypto::hash cached_top_hash;
        bool cached;
        D(): cached_from(0), cached_to(0), cached_start_height(0), cached_base(0), cached_m10_hash(crypto::null_hash), cached_top_hash(crypto::null_hash), cached(false) {}
      } d;
      const boost::unique_lock<boost::mutex> lock(d.mutex);

      crypto::hash top_hash = crypto::null_hash;
      if (d.cached_to < blockchain_height)
        top_hash = get_hash(d.cached_to);
      if (d.cached && amount == 0 && d.cached_from == from_height && d.cached_to == to_height && d.cached_top_hash == top_hash)
        return process_distribution(cumulative, d.cached_start_height, d.cached_distribution, d.cached_base);

      std::vector<std::uint64_t> distribution;
      std::uint64_t start_height, base;

      // see if we can extend the cache - a common case
      bool can_extend = d.cached && amount == 0 && d.cached_from == from_height && to_height > d.cached_to && top_hash == d.cached_top_hash;
      if (!can_extend)
      {
        // we kept track of the hash 10 blocks below, if it exists, so if it matches,
        // we can still pop the last 10 cached slots and try again
        if (d.cached && amount == 0 && d.cached_from == from_height && d.cached_to - d.cached_from >= 10 && to_height > d.cached_to - 10)
        {
          crypto::hash hash10 = get_hash(d.cached_to - 10);
          if (hash10 == d.cached_m10_hash)
          {
            d.cached_to -= 10;
            d.cached_top_hash = hash10;
            d.cached_m10_hash = crypto::null_hash;
            CHECK_AND_ASSERT_MES(d.cached_distribution.size() >= 10, boost::none, "Cached distribution size does not match cached bounds");
            for (int p = 0; p < 10; ++p)
              d.cached_distribution.pop_back();
            can_extend = true;
          }
        }
      }
      if (can_extend)
      {
        std::vector<std::uint64_t> new_distribution;
        if (!f(amount, d.cached_to + 1, to_height, start_height, new_distribution, base))
          return boost::none;
        distribution = d.cached_distribution;
        distribution.reserve(distribution.size() + new_distribution.size());
        for (const auto &e: new_distribution)
          distribution.push_back(e);
        start_height = d.cached_start_height;
        base = d.cached_base;
      }
      else
      {
        if (!f(amount, from_height, to_height, start_height, distribution, base))
          return boost::none;
      }

      if (to_height > 0 && to_height >= from_height)
      {
        const std::uint64_t offset = std::max(from_height, start_height);
        if (offset <= to_height && to_height - offset + 1 < distribution.size())
          distribution.resize(to_height - offset + 1);
      }

      if (amount == 0)
      {
        d.cached_from = from_height;
        d.cached_to = to_height;
        d.cached_top_hash = get_hash(d.cached_to);
        d.cached_m10_hash = d.cached_to >= 10 ? get_hash(d.cached_to - 10) : crypto::null_hash;
        d.cached_distribution = distribution;
        d.cached_start_height = start_height;
        d.cached_base = base;
        d.cached = true;
      }

      return process_distribution(cumulative, start_height, std::move(distribution), base);
  }

  bool CoinbaseOutputDistributionCache::get_coinbase_output_distribution
  (
    const uint64_t start_height, // inclusive
    const uint64_t stop_height, // inclusive
    uint64_t& true_start_height,
    std::vector<uint64_t>& num_cb_outs_per_block,
    uint64_t& base,
    bool* only_used_cache/* = nullptr*/
  )
  {
    if (only_used_cache)
      *only_used_cache = true;

    CHECK_AND_ASSERT_MES(stop_height >= start_height, false, "stop_height < start_height");
    CHECK_AND_ASSERT_MES(stop_height < CRYPTONOTE_MAX_BLOCK_NUMBER, false,
      "Request for ridiculous height: " << stop_height);

    // Start accessing the cache
    std::lock_guard<decltype(m_mutex)> ll(m_mutex);

    rollback_for_reorgs();

    if (stop_height >= height_end()) // if missing information
    {
      if (only_used_cache)
        *only_used_cache = false;

      // Fetch new information
      uint64_t start_height = 0, base = 0;
      std::vector<uint64_t> dist_ext;
      CHECK_AND_ASSERT_THROW_MES(m_get_rct_cb_dist(height_end(), stop_height, start_height, dist_ext, base),
        "Failed to obtain RCT coinbase distribution");
      CHECK_AND_ASSERT_THROW_MES(start_height == height_end() || height_end() == 0,
        "Starting height of requested distribution is off");
      CHECK_AND_ASSERT_THROW_MES(dist_ext.size() == stop_height - height_end() + 1,
        "Mismatch in requested RCT coinbase distribution size");

      // Decumulate distribution
      for (size_t i = 0; i < dist_ext.size(); ++i)
      {
        dist_ext[i] -= base;
        base += dist_ext[i];
      }

      // Extend cache
      if (m_num_cb_outs_per_block.empty())
        m_height_begin = start_height;
      m_num_cb_outs_per_block.insert(m_num_cb_outs_per_block.end(), dist_ext.cbegin(), dist_ext.cend());
      save_current_checkpoints();
    }

    CHECK_AND_ASSERT_MES(height_end() > stop_height, false, "internal bug: extend back");

    true_start_height = std::max(start_height, m_height_begin);
    if (true_start_height <= m_height_begin)
      base = 0;
    else
      base = m_num_cb_outs_per_block.at(true_start_height - m_height_begin - 1);

    const size_t num_queried_blocks = stop_height + 1 - true_start_height;
    const auto res_begin = m_num_cb_outs_per_block.data() + (true_start_height - m_height_begin); 
    const auto res_end = res_begin + num_queried_blocks;
    num_cb_outs_per_block = std::vector<uint64_t>(res_begin, res_end);

    return true;
  }

  void CoinbaseOutputDistributionCache::rollback_for_reorgs()
  {
    bool top_9_potent_invalid = false;
    bool top_99_potent_invalid = false;
    bool all_potent_invalid = false;

    if (height_end() >= 1 && m_last_1_hash != m_get_block_id_by_height(height_end() - 1))
    {
      top_9_potent_invalid = true;
      if (height_end() >= 10 && m_last_10_hash != m_get_block_id_by_height(height_end() - 10))
      {
        top_99_potent_invalid = true;
        if (height_end() >= 100 && m_last_100_hash != m_get_block_id_by_height(height_end() - 100))
        {
          all_potent_invalid = true;
        }
      }
    }

    all_potent_invalid = all_potent_invalid
      || (top_9_potent_invalid && m_num_cb_outs_per_block.size() <= 9)
      || (top_99_potent_invalid && m_num_cb_outs_per_block.size() <= 99);

    if (all_potent_invalid)
    {
      // Reset cache
      m_height_begin = 0;
      m_num_cb_outs_per_block.clear();
    }
    else if (top_99_potent_invalid)
    {
      for (size_t i = 0; i < 99; ++i) m_num_cb_outs_per_block.pop_back();
      save_current_checkpoints();
    }
    else if (top_9_potent_invalid)
    {
      for (size_t i = 0; i < 9; ++i) m_num_cb_outs_per_block.pop_back();
      save_current_checkpoints();
    }
    else
    {
      return; // Everything is valid! Nothing to do
    }
  }

  void CoinbaseOutputDistributionCache::save_current_checkpoints()
  {
    if (height_end() >= 1) m_last_1_hash = m_get_block_id_by_height(height_end() - 1);
    if (height_end() >= 10) m_last_10_hash = m_get_block_id_by_height(height_end() - 10);
    if (height_end() >= 100) m_last_100_hash = m_get_block_id_by_height(height_end() - 100);
  }
} // rpc
} // cryptonote
