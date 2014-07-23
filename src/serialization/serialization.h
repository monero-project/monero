// Copyright (c) 2014, The Monero Project
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

/* serialization.h
 *
 * Simple templated serialization API */

#pragma once
#include <vector>
#include <string>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/integral_constant.hpp>

template <class T>
struct is_blob_type { typedef boost::false_type type; };
template <class T>
struct has_free_serializer { typedef boost::true_type type; };

template <class Archive, class T>
struct serializer
{
  static bool serialize(Archive &ar, T &v) {
    return serialize(ar, v, typename boost::is_integral<T>::type(), typename is_blob_type<T>::type());
  }
  static bool serialize(Archive &ar, T &v, boost::false_type, boost::true_type) {
    ar.serialize_blob(&v, sizeof(v));
    return true;
  }
  static bool serialize(Archive &ar, T &v, boost::true_type, boost::false_type) {
    ar.serialize_int(v);
    return true;
  }
  static bool serialize(Archive &ar, T &v, boost::false_type, boost::false_type) {
    //serialize_custom(ar, v, typename has_free_serializer<T>::type());
    return v.do_serialize(ar);
  }
  static void serialize_custom(Archive &ar, T &v, boost::true_type) {
  }
};

template <class Archive, class T>
inline bool do_serialize(Archive &ar, T &v)
{
  return ::serializer<Archive, T>::serialize(ar, v);
}

#ifndef __GNUC__
#ifndef constexpr
#define constexpr
#endif
#endif

#define BLOB_SERIALIZER(T) \
  template<> struct is_blob_type<T> { typedef boost::true_type type; }
#define FREE_SERIALIZER(T) \
  template<> struct has_free_serializer<T> { typedef boost::true_type type; }
#define VARIANT_TAG(A, T, Tg) \
  template <bool W> struct variant_serialization_traits<A<W>, T> { static inline typename A<W>::variant_tag_type get_tag() { return Tg; } }
#define BEGIN_SERIALIZE() \
  template <bool W, template <bool> class Archive> bool do_serialize(Archive<W> &ar) {
#define BEGIN_SERIALIZE_OBJECT() \
  template <bool W, template <bool> class Archive> bool do_serialize(Archive<W> &ar) { ar.begin_object(); bool r = do_serialize_object(ar); ar.end_object(); return r; } \
  template <bool W, template <bool> class Archive> bool do_serialize_object(Archive<W> &ar){
#define PREPARE_CUSTOM_VECTOR_SERIALIZATION(size, vec) ::serialization::detail::prepare_custom_vector_serialization(size, vec, typename Archive<W>::is_saving())

#define END_SERIALIZE() return true;}


#define VALUE(f) \
  do { \
    ar.tag(#f); \
    bool r = ::do_serialize(ar, f); \
    if (!r || !ar.stream().good()) return false; \
  } while(0);
#define FIELD_N(t, f) \
  do { \
    ar.tag(t); \
    bool r = ::do_serialize(ar, f); \
    if (!r || !ar.stream().good()) return false; \
  } while(0);
#define FIELDS(f) \
  do { \
    bool r = ::do_serialize(ar, f); \
    if (!r || !ar.stream().good()) return false; \
  } while(0);
#define FIELD(f) \
  do { \
    ar.tag(#f); \
    bool r = ::do_serialize(ar, f); \
    if (!r || !ar.stream().good()) return false; \
  } while(0);
#define VARINT_FIELD(f) \
  do { \
    ar.tag(#f); \
    ar.serialize_varint(f); \
    if (!ar.stream().good()) return false; \
  } while(0);
#define VARINT_FIELD_N(t, f) \
  do { \
    ar.tag(t); \
    ar.serialize_varint(f); \
    if (!ar.stream().good()) return false; \
  } while(0);

namespace serialization {
  namespace detail
  {
    template <typename T>
    void prepare_custom_vector_serialization(size_t size, std::vector<T>& vec, const boost::mpl::bool_<true>& /*is_saving*/)
    {
    }

    template <typename T>
    void prepare_custom_vector_serialization(size_t size, std::vector<T>& vec, const boost::mpl::bool_<false>& /*is_saving*/)
    {
      vec.resize(size);
    }

    template<class Stream>
    bool do_check_stream_state(Stream& s, boost::mpl::bool_<true>)
    {
      return s.good();
    }

    template<class Stream>
    bool do_check_stream_state(Stream& s, boost::mpl::bool_<false>)
    {
      bool result = false;
      if (s.good())
      {
        std::ios_base::iostate state = s.rdstate();
        result = EOF == s.peek();
        s.clear(state);
      }
      return result;
    }
  }

  template<class Archive>
  bool check_stream_state(Archive& ar)
  {
    return detail::do_check_stream_state(ar.stream(), typename Archive::is_saving());
  }

  template <class Archive, class T>
  inline bool serialize(Archive &ar, T &v)
  {
    bool r = do_serialize(ar, v);
    return r && check_stream_state(ar);
  }
}

#include "string.h"
#include "vector.h"
