#pragma once

#include <stddef.h>

#if defined(__cplusplus)
#include <type_traits>

namespace crypto {

  extern "C" {
#endif

  void init_random(void);
  void generate_random_bytes(size_t n, void *result);

#if defined(__cplusplus)
  }
}
#endif