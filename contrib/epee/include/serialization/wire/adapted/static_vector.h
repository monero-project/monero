// Copyright (c) 2022, The Monero Project
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

#include <cstdint>
#include <boost/container/static_vector.hpp>
#include <boost/utility/string_ref.hpp>

#include "serialization/wire/read.h"

namespace wire
{
  // enable writing of static vector arrays
  template<typename T, std::size_t N>
  struct is_array<boost::container::static_vector<T, N>>
    : std::true_type
  {};
  
  // `static_vector`s of `char` and `uint8_t` are not arrays
  template<std::size_t N>
  struct is_array<boost::container::static_vector<char, N>>
    : std::false_type
  {};
  template<std::size_t N>
  struct is_array<boost::container::static_vector<std::uint8_t, N>>
    : std::false_type
  {};

  // `static_vector` can be used without specialized macro for every type, it provides max element count
  template<typename R, typename T, std::size_t N>
  inline void read_bytes(R& source, boost::container::static_vector<T, N>& dest)
  {
    wire_read::array(source, dest, min_element_size<0>{}, max_element_count<N>{});
  }

  /* `static_vector` never allocates, so it is useful for reading small variable
     sized strings or binary data with a known fixed max. `char` and
     `std::uint8_t` are not valid types for arrays in this design anyway
     (because its clearly less efficient in every encoding scheme).  */

  template<typename R, std::size_t N>
  inline void read_bytes(R& source, boost::container::static_vector<char, N>& dest)
  {
    dest.resize(N);
    dest.resize(source.string(epee::to_mut_span(dest), /* exact= */ false));
  }

  template<typename W, std::size_t N>
  inline void write_bytes(W& dest, const boost::container::static_vector<char, N>& source)
  {
    dest.string(boost::string_ref{source.data(), source.size()});
  }

  template<typename R, std::size_t N>
  inline void read_bytes(R& source, boost::container::static_vector<std::uint8_t, N>& dest)
  {
    dest.resize(N);
    dest.resize(source.binary(epee::to_mut_span(dest), /* exact= */ false));
  }

  template<typename W, std::size_t N>
  inline void write_bytes(W& dest, const boost::container::static_vector<std::uint8_t, N>& source)
  {
    dest.binary(epee::to_span(source));
  }
}
