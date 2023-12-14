#pragma once

#include <stdint.h>

namespace tools
{
  template<uint64_t a, uint64_t b>
  struct PowerOf
  {
    enum Data : uint64_t
    {
      Value = a * PowerOf<a, b - 1>::Value,
    };
  };

  template<uint64_t a>
  struct PowerOf<a, 0>
  {
    enum Data : uint64_t
    {
      Value = 1,
    };
  };
}