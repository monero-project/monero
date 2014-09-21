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

/*! \file serialization.h
 *
 * \brief ``Simple'' templated serialization API 
 *
 * Provides the same DSL for the creation of predefined archives (the
 * Archive part of the template), or the transformation of data into
 * an archival format
 * 
 */

#pragma once
#include <vector>
#include <string>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/integral_constant.hpp>

/*! \brief compile time database of blob types
 */
template <class T>
struct is_blob_type
{
  typedef boost::false_type type;
};

/*! \brief compile time database of types with a free serializer
 */
template <class T>
struct has_free_serializer
{
  typedef boost::true_type type;
};

// May want to refactor->could easily be done to avoid overloading and
// template abuse
/*! \brief contains the overloaded functions for the serialize
    operation
 */
template <class Archive, class T>
struct serializer
{
  /*! \brief the main call to serialize---decides which overloaded
   *  function to call via a query to is_blob_type and is_integral
   */
  static bool serialize(Archive &ar, T &v) {
    return serialize(ar, v, typename boost::is_integral<T>::type(), typename is_blob_type<T>::type());
  }
  /*! is not an integral but is in the blob_type database
   */
  static bool serialize(Archive &ar, T &v, boost::false_type, boost::true_type) {
    ar.serialize_blob(&v, sizeof(v));
    return true;
  }
  /*! is integral but not in the blob type database 
   */
  static bool serialize(Archive &ar, T &v, boost::true_type, boost::false_type) {
    ar.serialize_int(v);
    return true;
  }
  /*! is neither
   */
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

/*!
  The following define the DSL for the transformation of data into archival formats 
 */

/*! \breif adds an entry to the blob_serializer database
 */
#define BLOB_SERIALIZER(T)			\
  template<> struct is_blob_type<T> {		\
    typedef boost::true_type type;		\
  }

/*! \breif adds an entry to free_serializer database
 *
 * this shouldn't exist, has_free_serializer defaults to true...
 */
#define FREE_SERIALIZER(T)			\
  template<> struct has_free_serializer<T> {	\
    typedef boost::true_type type;		\
  }

#define VARIANT_TAG(A, T, Tg)					\
  template <bool W>						\
  struct variant_serialization_traits<A<W>, T> {		\
    static inline typename A<W>::variant_tag_type get_tag() {	\
      return Tg;						\
    }								\
  }

/*! \breif defines a do_serialize method, for types that are neither
 * blob nor integral
 */
#define BEGIN_SERIALIZE()				\
  template <bool W, template <bool> class Archive>	\
  bool do_serialize(Archive<W> &ar) {            

/*! \breif defines do_serialize when there is pre and post processing
 */
#define BEGIN_SERIALIZE_OBJECT()			\
  template <bool W, template <bool> class Archive>	\
  bool do_serialize(Archive<W> &ar) {			\
    ar.begin_object();					\
    bool r = do_serialize_object(ar);			\
    ar.end_object();					\
    return r;						\
  }							\
  template <bool W, template <bool> class Archive>	\
  bool do_serialize_object(Archive<W> &ar){
/*! \breif appears to just resize the vector
 */
#define PREPARE_CUSTOM_VECTOR_SERIALIZATION(size, vec)			\
  ::serialization::detail::prepare_custom_vector_serialization(size, vec, typename Archive<W>::is_saving())

/*! \breif ends a serialization operation
 */
#define END_SERIALIZE()				\
  return true;					\
  }
/*! \brief Literally never used throughout the entire code base
 */
#define VALUE(f)					\
  do {							\
    ar.tag(#f);						\
    bool r = ::do_serialize(ar, f);			\
    if (!r || !ar.stream().good())			\
      return false;					\
  } while(0); 
/*! \brief serializes a given field t from f
 */
#define FIELD_N(t, f)					\
  do {							\
    ar.tag(t);						\
    bool r = ::do_serialize(ar, f);			\
    if (!r || !ar.stream().good())			\
      return false;					\
  } while(0);
/*! \breif  simply serializes f
 */
#define FIELDS(f)					\
  do {							\
    bool r = ::do_serialize(ar, f);			\
    if (!r || !ar.stream().good())			\
      return false;					\
  } while(0);

/*! serializes and adds a tag
 */
#define FIELD(f)					\
  do {							\
    ar.tag(#f);						\
    bool r = ::do_serialize(ar, f);			\
    if (!r || !ar.stream().good())			\
      return false;					\
  } while(0);

/*! refer to the n and n_N distinction above */
#define VARINT_FIELD(f)				\
  do {						\
    ar.tag(#f);					\
    ar.serialize_varint(f);			\
    if (!ar.stream().good())			\
      return false;				\
  } while(0);

/*! refer to the n and n_N distinction above */
#define VARINT_FIELD_N(t, f)			\
  do {						\
    ar.tag(t);					\
    ar.serialize_varint(f);			\
    if (!ar.stream().good())			\
      return false;				\
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

