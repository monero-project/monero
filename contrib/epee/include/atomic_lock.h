// Copyright (c) 2014-2018, The Monero Project
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

#include <type_traits>
#include <atomic>



namespace epee
{
  template <class T>
  class atomic_lock
  {
    public:
      // the type of the underlying storage for
      // the enum that we are switching between
      using storage_type = typename std::underlying_type<T>::type;

      atomic_lock() = default;
      atomic_lock(T value) : m_value{ static_cast<storage_type>(value) } {}

      /**
       * Check whether we currently hold a specific lock
       * @param     value   The value to compare to
       * @return    Do we currently hold the given value
       */
      bool is(T value) const noexcept {
        return m_value.load() == static_cast<storage_type>(value);
      }

      /**
       * Try to lock to a given state. The lock will
       * only be acquired if the lock was in the
       * given starting point to begin with
       * @param     from    The expected original state
       * @param     to      The new state to lock to
       * @return    Did we successfully acquire the new lock state
       */
      bool try_lock(T from, T to) noexcept
      {
        // acquire the current value
        auto current = m_value.load(std::memory_order_relaxed);

        // if it's not what is expected, we cannot advance
        if (current != static_cast<storage_type>(from))
          return false;

        // change the value to the desired value, but only if we match the expectation
        return m_value.compare_exchange_strong(current, static_cast<storage_type>(to), std::memory_order_release, std::memory_order_relaxed);
      }

      /**
       * Unconditionally lock to a given state.
       * @param     to      The state to lock to
       */
      void lock(T to) noexcept
      {
        // just store the given value
        m_value = static_cast<storage_type>(to);
      }
    private:
      std::atomic<storage_type> m_value{};
  };
}
