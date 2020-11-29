#pragma once

#include <stdint.h>

namespace tools
{
  template<uint64_t a, uint64_t b>
  struct PowerOf
  {
    enum Data : uint64_t
    {
      // a^b = a * a^(b-1)
      Value = a * PowerOf<a, b - 1>::Value,
    };
  };

  template<uint64_t a>
  struct PowerOf<a, 0>
  {
    enum Data : uint64_t
    {
      // a^0 = 1
      Value = 1,
    };
  };
}
