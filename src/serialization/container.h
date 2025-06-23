// Copyright (c) 2014-2025, The Monero Project
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

#include <cstdint>
#include <cstddef>
#include <string>
#include <type_traits>

namespace serialization
{
  namespace detail
  {
    template<typename T>
    inline constexpr bool use_container_varint() noexcept
    {
      return std::is_integral<T>::value && std::is_unsigned<T>::value && sizeof(T) > 1;
    }

    template <typename Archive, class T>
    typename std::enable_if<!use_container_varint<T>(), bool>::type
    serialize_container_element(Archive& ar, T& e)
    {
      return do_serialize(ar, e);
    }

    template<typename Archive, typename T>
    typename std::enable_if<use_container_varint<T>(), bool>::type
    serialize_container_element(Archive& ar, T& e)
    {
      static constexpr const bool previously_varint = std::is_same<uint64_t, T>() || std::is_same<uint32_t, T>();

      if (!previously_varint && ar.varint_bug_backward_compatibility_enabled() && !typename Archive::is_saving())
        return do_serialize(ar, e);
      ar.serialize_varint(e);
      return true;
    }

    //! @brief Add an element to a container, inserting at the back if applicable.
    template <class Container>
    auto do_add(Container &c, typename Container::value_type &&e) -> decltype(c.emplace_back(e))
    { return c.emplace_back(e); }
    template <class Container>
    auto do_add(Container &c, typename Container::value_type &&e) -> decltype(c.emplace(e))
    { return c.emplace(e); }

    //! @brief Reserve space for N elements if applicable for container.
    template<typename... C>
    void do_reserve(const C&...) {}
    template<typename C>
    auto do_reserve(C &c, std::size_t N) -> decltype(c.reserve(N)) { return c.reserve(N); }

    // The value_type of STL map-like containers come in the form std::pair<const K, V>.
    // Since we can't {de}serialize const types in this lib, we must convert this to std::pair<K, V>
    template <class Container, typename = void>
    struct serializable_value_type
    { using type = typename Container::value_type; };
    template <class Container>
    struct serializable_value_type<Container, std::conditional_t<false, typename Container::mapped_type, void>>
    { using type = std::pair<typename Container::key_type, typename Container::mapped_type>; };
  }
}

template <template <bool> class Archive, typename C>
bool do_serialize_container(Archive<false> &ar, C &v)
{
  size_t cnt;
  ar.begin_array(cnt);
  if (!ar.good())
    return false;
  v.clear();

  // very basic sanity check
  if (ar.remaining_bytes() < cnt) {
    ar.set_fail();
    return false;
  }

  ::serialization::detail::do_reserve(v, cnt);

  for (size_t i = 0; i < cnt; i++) {
    if (i > 0)
      ar.delimit_array();
    typename ::serialization::detail::serializable_value_type<C>::type e;
    if (!::serialization::detail::serialize_container_element(ar, e))
      return false;
    ::serialization::detail::do_add(v, std::move(e));
    if (!ar.good())
      return false;
  }
  ar.end_array();
  return true;
}

template <template <bool> class Archive, typename C>
bool do_serialize_container(Archive<true> &ar, C &v)
{
  size_t cnt = v.size();
  ar.begin_array(cnt);
  for (auto i = v.begin(); i != v.end(); ++i)
  {
    if (!ar.good())
      return false;
    if (i != v.begin())
      ar.delimit_array();
    using serializable_value_type = typename ::serialization::detail::serializable_value_type<C>::type;
    if(!::serialization::detail::serialize_container_element(ar, (serializable_value_type&)*i))
      return false;
    if (!ar.good())
      return false;
  }
  ar.end_array();
  return true;
}
