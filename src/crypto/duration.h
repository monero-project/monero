// Copyright (c) 2020-2022, The Monero Project

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

#include <chrono>
#include "crypto/crypto.h"

namespace crypto
{
  //! Generate poisson distributed values in discrete `D` time units.
  template<typename D>
  struct random_poisson_duration
  {
    using result_type = D;                 //!< std::chrono::duration time unit precision
    using rep = typename result_type::rep; //!< Type used to represent duration value

    //! \param average for generated durations
    explicit random_poisson_duration(result_type average)
      : dist(average.count() < 0 ? 0 : average.count())
    {}

    //! Generate a crypto-secure random duration
    result_type operator()()
    {
      crypto::random_device rand{};
      return result_type{dist(rand)};
    }

  private:
    std::poisson_distribution<rep> dist;
  };

    /* A custom duration is used for subsecond precision because of the
       variance. If 5000 milliseconds is given, 95% of the values fall between
       4859ms-5141ms in 1ms increments (not enough time variance). Providing 1/4
       seconds would yield 95% of the values between 3s-7.25s in 1/4s
       increments. */

  //! Generate random durations with 1 second precision
  using random_poisson_seconds = random_poisson_duration<std::chrono::seconds>;
  //! Generate random duration with 1/4 second precision
  using random_poisson_subseconds =
    random_poisson_duration<std::chrono::duration<std::chrono::milliseconds::rep, std::ratio<1, 4>>>;
}
