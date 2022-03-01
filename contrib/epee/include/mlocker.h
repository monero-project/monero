// Copyright (c) 2018-2022, The Monero Project

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

#pragma once 

#include <map>
#include <boost/thread/mutex.hpp>

namespace epee
{
  class mlocker
  {
  public:
    mlocker(void *ptr, size_t len);
    ~mlocker();

    static size_t get_page_size();
    static size_t get_num_locked_pages();
    static size_t get_num_locked_objects();

    static void lock(void *ptr, size_t len);
    static void unlock(void *ptr, size_t len);

  private:
    static size_t page_size;
    static size_t num_locked_objects;

    static boost::mutex &mutex();
    static std::map<size_t, unsigned int> &map();
    static void lock_page(size_t page);
    static void unlock_page(size_t page);

    void *ptr;
    size_t len;
  };

  /// Locks memory while in scope
  ///
  /// Primarily useful for making sure that private keys don't get swapped out
  //  to disk
  template <class T>
  struct mlocked : public T {
    using type = T;

    mlocked(): T() { mlocker::lock(this, sizeof(T)); }
    mlocked(const T &t): T(t) { mlocker::lock(this, sizeof(T)); }
    mlocked(const mlocked<T> &mt): T(mt) { mlocker::lock(this, sizeof(T)); }
    mlocked(const T &&t): T(t) { mlocker::lock(this, sizeof(T)); }
    mlocked(const mlocked<T> &&mt): T(mt) { mlocker::lock(this, sizeof(T)); }
    mlocked<T> &operator=(const mlocked<T> &mt) { T::operator=(mt); return *this; }
    ~mlocked() { try { mlocker::unlock(this, sizeof(T)); } catch (...) { /* do not propagate */ } }
  };

  template<typename T>
  T& unwrap(mlocked<T>& src) { return src; }

  template<typename T>
  const T& unwrap(mlocked<T> const& src) { return src; }

  template <class T, size_t N>
  using mlocked_arr = mlocked<std::array<T, N>>;
}
