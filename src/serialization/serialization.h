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

/*! \struct is_blob_type 
 *
 * \brief a descriptor for dispatching serialize
 */
template <class T>
struct is_blob_type { typedef boost::false_type type; };

/*! \struct has_free_serializer
 *
 * \brief a descriptor for dispatching serialize
 */
template <class T>
struct has_free_serializer { typedef boost::true_type type; };

/*! \struct is_basic_type
 *
 * \brief a descriptor for dispatching serialize
 */
template <class T>
struct is_basic_type { typedef boost::false_type type; };

template<typename F, typename S>
struct is_basic_type<std::pair<F,S>> { typedef boost::true_type type; };
template<>
struct is_basic_type<std::string> { typedef boost::true_type type; };

/*! \struct serializer
 *
 * \brief ... wouldn't a class be better?
 * 
 * \detailed The logic behind serializing data. Places the archive
 * data into the supplied parameter. This dispatches based on the
 * supplied \a T template parameter's traits of is_blob_type or it is
 * an integral (as defined by the is_integral trait). Depends on the
 * \a Archive parameter to have overloaded the serialize_blob(T v,
 * size_t size) and serialize_int(T v) base on which trait it
 * applied. When the class has neither types, it falls to the
 * overloaded method do_serialize(Archive ar) in T to do the work.
 */
template <class Archive, class T>
struct serializer{
  static bool serialize(Archive &ar, T &v) {
    return serialize(ar, v, typename boost::is_integral<T>::type(), typename is_blob_type<T>::type(), typename is_basic_type<T>::type());
  }
  template<typename A>
  static bool serialize(Archive &ar, T &v, boost::false_type, boost::true_type, A a) {
    ar.serialize_blob(&v, sizeof(v));
    return true;
  }
  template<typename A>
  static bool serialize(Archive &ar, T &v, boost::true_type, boost::false_type, A a) {
    ar.serialize_int(v);
    return true;
  }
  static bool serialize(Archive &ar, T &v, boost::false_type, boost::false_type, boost::false_type) {
    //serialize_custom(ar, v, typename has_free_serializer<T>::type());
    return v.do_serialize(ar);
  }
  static bool serialize(Archive &ar, T &v, boost::false_type, boost::false_type, boost::true_type) {
    //serialize_custom(ar, v, typename has_free_serializer<T>::type());
    return do_serialize(ar, v);
  }
  static void serialize_custom(Archive &ar, T &v, boost::true_type) {
  }
};

/*! \fn do_serialize(Archive &ar, T &v)
 *
 * \brief just calls the serialize function defined for ar and v...
 */
template <class Archive, class T>
inline bool do_serialize(Archive &ar, T &v)
{
  return ::serializer<Archive, T>::serialize(ar, v);
}
template <class Archive>
inline bool do_serialize(Archive &ar, bool &v)
{
  ar.serialize_blob(&v, sizeof(v));
  return true;
}

// Never used in the code base
// #ifndef __GNUC__
// #ifndef constexpr
// #define constexpr
// #endif
// #endif

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

/*! \macro FREE_SERIALIZER
 *
 * \brief adds the has_free_serializer to the type
 */
#define FREE_SERIALIZER(T)						\
  template<>								\
  struct has_free_serializer<T> {					\
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
  bool do_serialize(Archive<W> &ar) {

/*! \macro BEGIN_SERIALIZE_OBJECT
 *
 *  \brief begins the environment of the DSL
 *  \detailed for described the serialization of an object
 */
#define BEGIN_SERIALIZE_OBJECT()					\
  template <bool W, template <bool> class Archive>			\
  bool do_serialize(Archive<W> &ar) {					\
    ar.begin_object();							\
    bool r = do_serialize_object(ar);					\
    ar.end_object();							\
    return r;								\
  }									\
  template <bool W, template <bool> class Archive>			\
  bool do_serialize_object(Archive<W> &ar){

/*! \macro PREPARE_CUSTOM_VECTOR_SERIALIZATION
 */
#define PREPARE_CUSTOM_VECTOR_SERIALIZATION(size, vec)			\
  ::serialization::detail::prepare_custom_vector_serialization(size, vec, typename Archive<W>::is_saving())

/*! \macro PREPARE_CUSTOM_DEQUE_SERIALIZATION
 */
#define PREPARE_CUSTOM_DEQUE_SERIALIZATION(size, vec)			\
  ::serialization::detail::prepare_custom_deque_serialization(size, vec, typename Archive<W>::is_saving())

/*! \macro END_SERIALIZE
 * \brief self-explanatory
 */
#define END_SERIALIZE()				\
  return true;					\
  }

/*! \macro VALUE(f)
 * \brief the same as FIELD(f)
 */
#define VALUE(f)					\
  do {							\
    ar.tag(#f);						\
    bool r = ::do_serialize(ar, f);			\
    if (!r || !ar.stream().good()) return false;	\
  } while(0);

/*! \macro FIELD_N(t,f)
 *
 * \brief serializes a field \a f tagged \a t  
 */
#define FIELD_N(t, f)					\
  do {							\
    ar.tag(t);						\
    bool r = ::do_serialize(ar, f);			\
    if (!r || !ar.stream().good()) return false;	\
  } while(0);

/*! \macro FIELD(f)
 *
 * \brief tags the field with the variable name and then serializes it
 */
#define FIELD(f)					\
  do {							\
    ar.tag(#f);						\
    bool r = ::do_serialize(ar, f);			\
    if (!r || !ar.stream().good()) return false;	\
  } while(0);

/*! \macro FIELDS(f)
 *
 * \brief does not add a tag to the serialized value
 */
#define FIELDS(f)							\
  do {									\
    bool r = ::do_serialize(ar, f);					\
    if (!r || !ar.stream().good()) return false;			\
  } while(0);

/*! \macro VARINT_FIELD(f)
 *  \brief tags and serializes the varint \a f
 */
#define VARINT_FIELD(f)				\
  do {						\
    ar.tag(#f);					\
    ar.serialize_varint(f);			\
    if (!ar.stream().good()) return false;	\
  } while(0);

/*! \macro VARINT_FIELD_N(t, f)
 *
 * \brief tags (as \a t) and serializes the varint \a f
 */
#define VARINT_FIELD_N(t, f)			\
  do {						\
    ar.tag(t);					\
    ar.serialize_varint(f);			\
    if (!ar.stream().good()) return false;	\
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

    template <typename T>
    void prepare_custom_deque_serialization(size_t size, std::deque<T>& vec, const boost::mpl::bool_<true>& /*is_saving*/)
    {
    }

    template <typename T>
    void prepare_custom_deque_serialization(size_t size, std::deque<T>& vec, const boost::mpl::bool_<false>& /*is_saving*/)
    {
      vec.resize(size);
    }

    /*! \fn do_check_stream_state
     *
     * \brief self explanatory
     */
    template<class Stream>
    bool do_check_stream_state(Stream& s, boost::mpl::bool_<true>)
    {
      return s.good();
    }
    /*! \fn do_check_stream_state
     *
     * \brief self explanatory
     *
     * \detailed Also checks to make sure that the stream is not at EOF
     */
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

  /*! \fn check_stream_state
   *
   * \brief calls detail::do_check_stream_state for ar
   */
  template<class Archive>
  bool check_stream_state(Archive& ar)
  {
    return detail::do_check_stream_state(ar.stream(), typename Archive::is_saving());
  }

  /*! \fn serialize
   *
   * \brief serializes \a v into \a ar
   */
  template <class Archive, class T>
  inline bool serialize(Archive &ar, T &v)
  {
    bool r = do_serialize(ar, v);
    return r && check_stream_state(ar);
  }
}
