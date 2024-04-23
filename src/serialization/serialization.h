// Copyright (c) 2014-2023, The Monero Project
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
 *  \brief Simple DSL AAPI based on
 *
 * \detailed is_blob_type and  has_free_serializer are
 * both descriptors for dispatching on to the serialize function.
 *
 * The API itself defines a domain specific language via dirty macro
 * hacks. Greenspun's tenth rule is very much in action throughout
 * this entire code base.
 */

#pragma once
#include <vector>
#include <deque>
#include <list>
#include <set>
#include <unordered_set>
#include <string>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/mpl/bool.hpp>

#include "common/va_args.h"

/*! \struct is_blob_type 
 *
 * \brief a descriptor for dispatching serialize
 */
template <class T>
struct is_blob_type { typedef boost::false_type type; };

/*! \fn do_serialize(Archive &ar, T &v)
 *
 * \brief main function for dispatching serialization for a given pair of archive and value types
 * 
 * Types marked true with is_blob_type<T> will be serialized as a blob, integral types will be
 * serialized as integers, and types who have a `member_do_serialize` method will be serialized
 * using that method. Booleans are serialized like blobs.
 */
template <class Archive, class T>
inline std::enable_if_t<is_blob_type<T>::type::value, bool> do_serialize(Archive &ar, T &v)
{
  ar.serialize_blob(&v, sizeof(v));
  return true;
}
template <class Archive, class T>
inline std::enable_if_t<boost::is_integral<T>::value, bool> do_serialize(Archive &ar, T &v)
{
  ar.serialize_int(v);
  return true;
}
template <class Archive, class T>
inline auto do_serialize(Archive &ar, T &v) -> decltype(v.member_do_serialize(ar), true)
{
  return v.member_do_serialize(ar);
}
template <class Archive>
inline bool do_serialize(Archive &ar, bool &v)
{
  ar.serialize_blob(&v, sizeof(v));
  return true;
}
template <class Archive, class T, typename... Args>
inline auto do_serialize(Archive &ar, T &v, Args&&... args)
  -> decltype(do_serialize_object(ar, v, args...), true)
{
  ar.begin_object();
  const bool r = do_serialize_object(ar, v, args...);
  ar.end_object();
  return r && ar.good();
}

/* the following add a trait to a set and define the serialization DSL*/

/*! \macro BLOB_SERIALIZER
 *
 * \brief makes the type have a blob serializer trait defined
 */
#define BLOB_SERIALIZER(T)						\
  template<>								\
  struct is_blob_type<T> {						\
    typedef boost::true_type type;					\
  }

/*! \macro VARIANT_TAG
 *
 * \brief Adds the tag \tag to the \a Archive of \a Type
 */
#define VARIANT_TAG(Archive, Type, Tag)					\
  template <bool W>							\
  struct variant_serialization_traits<Archive<W>, Type> {		\
    static inline typename Archive<W>::variant_tag_type get_tag() {	\
      return Tag;							\
    }									\
  }

/*! \macro BEGIN_SERIALIZE
 * 
 * \brief Begins the environment of the DSL
 * \detailed for describing how to
 * serialize an of an archive type
 */
#define BEGIN_SERIALIZE()						\
  template <bool W, template <bool> class Archive>			\
  bool member_do_serialize(Archive<W> &ar) {

/*! \macro BEGIN_SERIALIZE_FN
 *
 * \brief Begins the environment of the DSL as a free function
 *
 * Inside, instead of FIELD() and VARINT_FIELD(), use FIELD_F() and
 * VARINT_FIELD_F(). Otherwise, this macro is similar to BEGIN_SERIALIZE().
 */
#define BEGIN_SERIALIZE_FN(stype)                   \
  template <bool W, template <bool> class Archive>  \
  bool do_serialize(Archive<W> &ar, stype &v) {

/*! \macro BEGIN_SERIALIZE_OBJECT
 *
 *  \brief begins the environment of the DSL
 *  \detailed for described the serialization of an object
 */
#define BEGIN_SERIALIZE_OBJECT()					\
  template <bool W, template <bool> class Archive>			\
  bool member_do_serialize(Archive<W> &ar) {					\
    ar.begin_object();							\
    bool r = do_serialize_object(ar);					\
    ar.end_object();							\
    return r;								\
  }									\
  template <bool W, template <bool> class Archive>			\
  bool do_serialize_object(Archive<W> &ar){

/*! \macro BEGIN_SERIALIZE_OBJECT_FN
 *
 * \brief Begins the environment of the DSL as a free function in object-style
 *
 * Inside, instead of FIELD() and VARINT_FIELD(), use FIELD_F() and
 * VARINT_FIELD_F(). Otherwise, this macro is similar to
 * BEGIN_SERIALIZE_OBJECT(), as you should list only field serializations.
 */
#define BEGIN_SERIALIZE_OBJECT_FN(stype, ...)                                           \
  template <bool W, template <bool> class Archive>                                      \
  bool do_serialize_object(Archive<W> &ar, stype &v VA_ARGS_COMMAPREFIX(__VA_ARGS__)) {

/*! \macro PREPARE_CUSTOM_VECTOR_SERIALIZATION
 */
#define PREPARE_CUSTOM_VECTOR_SERIALIZATION(size, vec)			\
  ::serialization::detail::prepare_custom_vector_serialization(size, vec, typename Archive<W>::is_saving())

/*! \macro END_SERIALIZE
 * \brief self-explanatory
 */
#define END_SERIALIZE()				\
  return ar.good();				\
  }

/*! \macro FIELD_N(t,f)
 *
 * \brief serializes a field \a f tagged \a t  
 */
#define FIELD_N(t, f, ...)                                    \
  do {							\
    ar.tag(t);						\
    bool r = do_serialize(ar, f VA_ARGS_COMMAPREFIX(__VA_ARGS__)); \
    if (!r || !ar.good()) return false;			\
  } while(0);

/*! \macro FIELD(f)
 *
 * \brief tags the field with the variable name and then serializes it
 */
#define FIELD(f)					\
  do {							\
    ar.tag(#f);						\
    bool r = do_serialize(ar, f);			\
    if (!r || !ar.good()) return false;			\
  } while(0);

/*! \macro FIELD_F(f)
 *
 * \brief tags the field with the variable name and then serializes it (for use in a free function)
 */
#define FIELD_F(f, ...) FIELD_N(#f, v.f VA_ARGS_COMMAPREFIX(__VA_ARGS__))

/*! \macro FIELDS(f)
 *
 * \brief does not add a tag to the serialized value
 */
#define FIELDS(f)							\
  do {									\
    bool r = do_serialize(ar, f);					\
    if (!r || !ar.good()) return false;					\
  } while(0);

/*! \macro VARINT_FIELD(f)
 *  \brief tags and serializes the varint \a f
 */
#define VARINT_FIELD(f)				\
  do {						\
    ar.tag(#f);					\
    ar.serialize_varint(f);			\
    if (!ar.good()) return false;		\
  } while(0);

/*! \macro VARINT_FIELD_N(t, f)
 *
 * \brief tags (as \a t) and serializes the varint \a f
 */
#define VARINT_FIELD_N(t, f)			\
  do {						\
    ar.tag(t);					\
    ar.serialize_varint(f);			\
    if (!ar.good()) return false;		\
  } while(0);

/*! \macro VARINT_FIELD_F(f)
 *
 * \brief tags and serializes the varint \a f (for use in a free function)
 */
#define VARINT_FIELD_F(f) VARINT_FIELD_N(#f, v.f)

/*! \macro MAGIC_FIELD(m)
 */
#define MAGIC_FIELD(m)				\
  std::string magic = m;			\
  do {						\
    ar.tag("magic");				\
    ar.serialize_blob((void*)magic.data(), magic.size()); \
    if (!ar.good()) return false;		\
    if (magic != m) return false;		\
  } while(0);

/*! \macro VERSION_FIELD(v)
 */
#define VERSION_FIELD(v)			\
  uint32_t version = v;				\
  do {						\
    ar.tag("version");				\
    ar.serialize_varint(version);		\
    if (!ar.good()) return false;		\
  } while(0);


namespace serialization {
  /*! \namespace detail
   *
   * \brief declaration and default definition for the functions used the API
   *
   */
  namespace detail
  {
    /*! \fn prepare_custom_vector_serialization
     *
     * prepares the vector /vec for serialization
     */
    template <typename T>
    void prepare_custom_vector_serialization(size_t size, std::vector<T>& vec, const boost::mpl::bool_<true>& /*is_saving*/)
    {
    }

    template <typename T>
    void prepare_custom_vector_serialization(size_t size, std::vector<T>& vec, const boost::mpl::bool_<false>& /*is_saving*/)
    {
      vec.resize(size);
    }

    /*! \fn do_check_stream_state
     *
     * \brief self explanatory
     */
    template<class Archive>
    bool do_check_stream_state(Archive& ar, boost::mpl::bool_<true>, bool noeof)
    {
      return ar.good();
    }
    /*! \fn do_check_stream_state
     *
     * \brief self explanatory
     *
     * \detailed Also checks to make sure that the stream is not at EOF
     */
    template<class Archive>
    bool do_check_stream_state(Archive& ar, boost::mpl::bool_<false>, bool noeof)
    {
      bool result = false;
      if (ar.good())
	{
	  result = noeof || ar.eof();
	}
      return result;
    }
  }

  /*! \fn check_stream_state
   *
   * \brief calls detail::do_check_stream_state for ar
   */
  template<class Archive>
  bool check_stream_state(Archive& ar, bool noeof = false)
  {
    return detail::do_check_stream_state(ar, typename Archive::is_saving(), noeof);
  }

  /*! \fn serialize
   *
   * \brief serializes \a v into \a ar
   */
  template <class Archive, class T>
  inline bool serialize(Archive &ar, T &v)
  {
    bool r = do_serialize(ar, v);
    return r && check_stream_state(ar, false);
  }

  /*! \fn serialize
   *
   * \brief serializes \a v into \a ar
   */
  template <class Archive, class T>
  inline bool serialize_noeof(Archive &ar, T &v)
  {
    bool r = do_serialize(ar, v);
    return r && check_stream_state(ar, true);
  }
}
