// Copyright (c) 2014-2024, The Monero Project
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
#include <tuple>
#include "serialization.h"

namespace serialization
{
  namespace detail
  {
    template <typename Archive, class T>
    bool serialize_tuple_element(Archive& ar, T& e)
    {
      return do_serialize(ar, e);
    }

    template <typename Archive>
    bool serialize_tuple_element(Archive& ar, uint64_t& e)
    {
      ar.serialize_varint(e);
      return true;
    }
  }
}

template <size_t I, bool BackwardsCompat, bool W, template <bool> class Archive, typename... Ts>
bool do_serialize_tuple_nth(Archive<W>& ar, std::tuple<Ts...>& v)
{
  static constexpr const size_t tuple_size = std::tuple_size<std::tuple<Ts...>>();
  static_assert(I <= tuple_size, "bad call");

  if constexpr (I == 0)
  {
    // If BackwardsCompat is true, we serialize the size of 3-tuples and 4-tuples
    if constexpr (BackwardsCompat && (tuple_size == 3 || tuple_size == 4))
    {
      size_t cnt = tuple_size;
      ar.begin_array(cnt);
      if (cnt != tuple_size)
        return false;
    }
    else
    {
      ar.begin_array();
    }
  }
  else if constexpr (I < tuple_size)
  {
    ar.delimit_array();
  }

  if constexpr (I == tuple_size)
  {
    ar.end_array();
    return ar.good();
  }
  else
  {
    if (!::serialization::detail::serialize_tuple_element(ar, std::get<I>(v))
        || !ar.good())
      return false;

    return do_serialize_tuple_nth<I + 1, BackwardsCompat>(ar, v);
  }
}

template <bool BackwardsCompat, bool W, template <bool> class Archive, typename... Ts>
bool do_serialize_tuple(Archive<W>& ar, std::tuple<Ts...>& v)
{
  return do_serialize_tuple_nth<0, BackwardsCompat>(ar, v);
}

template <bool W, template <bool> class Archive, typename... Ts>
bool do_serialize(Archive<W>& ar, std::tuple<Ts...>& v)
{
  return do_serialize_tuple<true>(ar, v);
}

#define TUPLE_COMPACT_FIELDS(v)                          \
  do {                                                   \
    if (!do_serialize_tuple<false>(ar, v) || !ar.good()) \
      return false;                                      \
  } while (0);

#define TUPLE_COMPACT_FIELD_N(t, v) \
  do {                              \
    ar.tag(t);                      \
    TUPLE_COMPACT_FIELDS(v);        \
  } while (0);

#define TUPLE_COMPACT_FIELD(f) TUPLE_COMPACT_FIELD_N(#f, f)

#define TUPLE_COMPACT_FIELD_F(f) TUPLE_COMPACT_FIELD_N(#f, v.f)
