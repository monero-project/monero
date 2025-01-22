// Copyright (c) 2017-2024, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#ifdef __cplusplus
#include <array>
#include <cstddef>

extern "C" {
#endif

void *memwipe(void *src, size_t n);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace tools {

  /// Scrubs data in the contained type upon destruction.
  ///
  /// Primarily useful for making sure that private keys don't stick around in
  /// memory after the objects that held them have gone out of scope.
  template <class T>
  struct scrubbed : public T {
    using type = T;

    ~scrubbed() {
      scrub();
    }

    /// Destroy the contents of the contained type.
    void scrub() {
      static_assert(std::is_pod<T>::value,
                    "T cannot be auto-scrubbed. T must be POD.");
      static_assert(std::is_trivially_destructible<T>::value,
                    "T cannot be auto-scrubbed. T must be trivially destructable.");
      memwipe(this, sizeof(T));
    }
  };

  template<typename T>
  T& unwrap(scrubbed<T>& src) { return src; }

  template<typename T>
  const T& unwrap(scrubbed<T> const& src) { return src; }

  template <class T, size_t N>
  using scrubbed_arr = scrubbed<std::array<T, N>>;
} // namespace tools

#endif // __cplusplus
