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

#include <boost/range/adaptor/transformed.hpp>
#include <cstring>
#include "byte_slice.h"
#include "byte_stream.h"
#include "int-util.h"
#include "serialization/wire/error.h"
#include "serialization/wire/read.h"
#include "serialization/wire/write.h"
#include "serialization/wire/wrapper/array.h"
#include "serialization/wire/wrapper/array_blob.h"
#include "storages/portable_storage_bin_utils.h"
#include "span.h"

namespace wire
{
  //
  // free functions for `array_` wrapper
  //

  template<typename R, typename T, typename C>
  inline void read_bytes(const R&, const array_<T, C>&)
  {
    // see constraints directly above `array_` definition
    static_assert(std::is_same<R, void>::value, "array_ must have a read constraint for memory purposes");
  }
  template<typename R, typename T, std::size_t N>
  inline void read_bytes(R& source, array_<T, max_element_count<N>>& wrapper)
  {
    using array_type = array_<T, max_element_count<N>>;
    using value_type = typename array_type::value_type;
    using constraint = typename array_type::constraint;
    static_assert(constraint::template check<value_type>(), "max reserve bytes exceeded for element");
    wire_read::array(source, wrapper.get_read_object(), min_element_size<0>{}, constraint{});
  }
  template<typename R, typename T, std::size_t N>
  inline void read_bytes(R& source, array_<T, min_element_size<N>>& wrapper)
  {
    using array_type = array_<T, min_element_size<N>>;
    using value_type = typename array_type::value_type;
    using constraint = typename array_type::constraint;
    static_assert(constraint::template check<value_type>(), "max compression ratio exceeded for element");
    wire_read::array(source, wrapper.get_read_object(), constraint{});
  }

  template<typename W, typename T, typename C>
  inline void write_bytes(W& dest, const array_<T, C>& wrapper)
  {
    wire_write::array(dest, wrapper.get_container());
  }
  template<typename W, typename T, typename C, typename D>
  inline void write_bytes(W& dest, const array_<array_<T, C>, D>& wrapper)
  {
    using inner_type = typename array_<array_<T, C>, D>::inner_array_const;
    const auto wrap = [](const auto& val) -> inner_type { return {std::ref(val)}; };
    wire_write::array(dest, boost::adaptors::transform(wrapper.get_container(), wrap));
  }


  //
  // free functions for 'array_as_blob_` wrapper
  //

#if BYTE_ORDER == LITTLE_ENDIAN
  // enable optimization if `T` has `resize` and `data()`
  template<typename T>
  auto read_as_blob(epee::span<const std::uint8_t> source, T& dest) -> decltype(dest.resize(0), dest.data())
  {
    using value_type = typename T::value_type;
    dest.resize(source.size() / sizeof(value_type));
    std::memcpy(dest.data(), source.data(), source.size());
    return dest.data();
  }

  // enable optimization if `T` has `data()`
  template<typename W, typename T>
  auto write_as_blob(W& dest, const T& source) -> decltype(source.data())
  {
    using value_type = typename T::value_type;
    dest.binary({reinterpret_cast<const std::uint8_t*>(source.data()), source.size() * sizeof(value_type)});
    return source.data();
  }
#endif // LITTLE ENDIAN

  // Parameter packs take lower precedence (above preferred if `T` has extra functions)

  template<typename T, typename... Ignored>
  void read_as_blob(epee::span<const std::uint8_t> source, T& dest, const Ignored&...)
  {
    using value_type = typename T::value_type;
    dest.clear();
    wire::reserve(dest, source.size() / sizeof(value_type));
    while (!source.empty())
    {
      dest.emplace_back();
      std::memcpy(std::addressof(dest.back()), source.data(), sizeof(value_type));
      dest.back() = CONVERT_POD(dest.back());
      source.remove_prefix(sizeof(value_type));
    }
  }

  template<typename W, typename T, typename... Ignored>
  void write_as_blob(W& dest, const T& source, const Ignored&...)
  {
    using value_type = typename T::value_type;
    epee::byte_stream bytes{};
    bytes.reserve(sizeof(value_type) * source.size());
    for (auto elem : source)
    {
      elem = CONVERT_POD(elem);
      bytes.write(reinterpret_cast<const char*>(std::addressof(elem)), sizeof(elem));
    }

    dest.binary(epee::to_span(bytes));
  }

  template<typename R, typename T>
  void read_bytes(R& source, array_as_blob_<T>& dest)
  {
    static_assert(array_as_blob_<T>::value_size() != 0, "divide by zero and no progress below");
    epee::byte_slice bytes = source.binary();
    if (bytes.size() % dest.value_size() != 0)
      WIRE_DLOG_THROW_(error::schema::fixed_binary);

    read_as_blob(epee::to_span(bytes), dest.get_container());
  }

  template<typename W, typename T>
  void write_bytes(W& dest, const array_as_blob_<T>& source)
  {
    write_as_blob(dest, source.get_container());
  }
} // wire
