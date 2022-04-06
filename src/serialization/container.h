// Copyright (c) 2014-2022, The Monero Project
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
      return ::do_serialize(ar, e);
    }

    template<typename Archive, typename T>
    typename std::enable_if<use_container_varint<T>(), bool>::type
    serialize_container_element(Archive& ar, T& e)
    {
      static constexpr const bool previously_varint = std::is_same<uint64_t, T>() || std::is_same<uint32_t, T>();

      if (!previously_varint && ar.varint_bug_backward_compatibility_enabled() && !typename Archive::is_saving())
        return ::do_serialize(ar, e);
      ar.serialize_varint(e);
      return true;
    }

    template <typename C>
    void do_reserve(C &c, size_t N) {}
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
    typename C::value_type e;
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
    if(!::serialization::detail::serialize_container_element(ar, (typename C::value_type&)*i))
      return false;
    if (!ar.good())
      return false;
  }
  ar.end_array();
  return true;
}
