
#include <algorithm>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

#include "cryptonote_core/cryptonote_core.h"

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
    RpcHandler::get_output_distribution(const std::function<bool(uint64_t, uint64_t, uint64_t, uint64_t&, std::vector<uint64_t>&, uint64_t&)> &f, uint64_t amount, uint64_t from_height, uint64_t to_height, bool cumulative)
  {
      static struct D
      {
        boost::mutex mutex;
        std::vector<std::uint64_t> cached_distribution;
        std::uint64_t cached_from, cached_to, cached_start_height, cached_base;
        bool cached;
        D(): cached_from(0), cached_to(0), cached_start_height(0), cached_base(0), cached(false) {}
      } d;
      const boost::unique_lock<boost::mutex> lock(d.mutex);

      if (d.cached && amount == 0 && d.cached_from == from_height && d.cached_to == to_height)
        return process_distribution(cumulative, d.cached_start_height, d.cached_distribution, d.cached_base);

      std::vector<std::uint64_t> distribution;
      std::uint64_t start_height, base;
      if (!f(amount, from_height, to_height, start_height, distribution, base))
        return boost::none;

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
        d.cached_distribution = distribution;
        d.cached_start_height = start_height;
        d.cached_base = base;
        d.cached = true;
      }

      return process_distribution(cumulative, start_height, std::move(distribution), base);
  }
} // rpc
} // cryptonote
