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

/*! \file binary_archive.h
 *
 * \brief Portable (low-endian) binary archive */

#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
#include <boost/type_traits/make_unsigned.hpp>

#include "common/varint.h"
#include "warnings.h"

PUSH_WARNINGS
DISABLE_VS_WARNINGS(4244)

//TODO: fix size_t warning in x32 platform
/*! The base type for a binar archive
    isSaving---writes to stream
 */
template <class Stream, bool IsSaving>
struct binary_archive_base
{
  // never used throughout codebase
  typedef Stream stream_type;
  typedef binary_archive_base<Stream, IsSaving> base_type;
  typedef boost::mpl::bool_<IsSaving> is_saving;

  typedef uint8_t variant_tag_type;

  explicit binary_archive_base(stream_type &s) : stream_(s) { }

  /* Begin the definition of the serialization API methods
   */
  void tag(const char *) { }
  void begin_object() { }
  void end_object() { }
  void begin_variant() { }
  void end_variant() { }
  stream_type &stream() { return stream_; }

protected:
  stream_type &stream_;
};

/*! Template for deciding whether a binary archive has the isSaving
    trait
 */
template <bool W>
struct binary_archive;

/*! A binary archive without the isSaving trait
 */
template <>
struct binary_archive<false> : public binary_archive_base<std::istream, false>
{
  /*! binary_arcive constructor: gets the length of stream until EOF
   */
  explicit binary_archive(stream_type &s) : base_type(s) {
    stream_type::streampos pos = stream_.tellg();
    stream_.seekg(0, std::ios_base::end);
    eof_pos_ = stream_.tellg();
    stream_.seekg(pos);
  }
  /*! Serializes and integer
   */
  template <class T>
  void serialize_int(T &v)
  {
    serialize_uint(*(typename boost::make_unsigned<T>::type *)&v);
  }

  /*! Serializes an /unsigned/ integer
   */
  template <class T>
  void serialize_uint(T &v)
  {
    // No call used the
    // second parameter---in fact, these are never used except in
    // serialize_int
    size_t width = sizeof(T);

    // Maybe I'm just so used to plan9 C that anything not declared in
    // the beginning of a function is verboten...
    T ret = 0;
    unsigned shift = 0;
    char c;
    T b;
    
    for (size_t i = 0; i < width; i++) {
      //std::cerr << "tell: " << stream_.tellg() << " value: " << ret << std::endl;
      stream_.get(c);
      b = (unsigned char)c;
      // Shifts by a byte each time... ret is zerod... or'ing should
      // be used instead---no use adding
      ret |= (b << shift);	
      shift += 8;
    }
    v = ret;
  }

  /*! just straight up read a len bytes from it the archive.
   */
  void serialize_blob(void *buf, size_t len, const char *delimiter="") {
    stream_.read((char *)buf, len);
  }
  /*! serializes a variable length integer
   */
  template <class T>
  void serialize_varint(T &v)
  {
    serialize_uvarint(*(typename boost::make_unsigned<T>::type *)(&v));
  }
  /*! serializes a variable length unsigned integer
   */
  template <class T>
  void serialize_uvarint(T &v)
  {
    typedef std::istreambuf_iterator<char> it;
    tools::read_varint(it(stream_), it(), v); // XXX handle failure (lewd~)
  }

  // Standard serialization API functions beyond here
  
  void begin_array(size_t &s)
  {
    serialize_varint(s);
  }

  void begin_array() { }

  void delimit_array() { }
  void end_array() { }

  void begin_string(const char *delimiter="\"") { }
  void end_string(const char *delimiter="\"") { }

  // synonym to serialize_variant
  void read_variant_tag(variant_tag_type &t) {
    serialize_int(t);
  }

  /*! calculates remaining bytes until EOF
   */
  size_t remaining_bytes() {
    if (!stream_.good())
      return 0;
    //std::cerr << "tell: " << stream_.tellg() << std::endl;
    assert(stream_.tellg() <= eof_pos_);
    return eof_pos_ - stream_.tellg();
  }
protected:
  std::streamoff eof_pos_;
};

/*! binary archive with the isSaving trait set
 */
template <>
struct binary_archive<true> : public binary_archive_base<std::ostream, true>
{
  explicit binary_archive(stream_type &s) : base_type(s) { }

  /*! writes an integer to the stream
   */
  template <class T>
  void serialize_int(T v)
  {
    serialize_uint(static_cast<typename boost::make_unsigned<T>::type>(v));
  }
  /*! writes an unsigned interger to the stream
   */
  template <class T>
  void serialize_uint(T v)
  {
    for (size_t i = 0; i < sizeof(T); i++) {
      stream_.put((char)(v & 0xff));
      if (1 < sizeof(T)) {
        v >>= 8;
      }
    }
  }
  /*! writes len chars from buf to stream
   */
  void serialize_blob(void *buf, size_t len, const char *delimiter="") {
    stream_.write((char *)buf, len);
  }

  /*! writes a varint
   */
  template <class T>
  void serialize_varint(T &v)
  {
    serialize_uvarint(*(typename boost::make_unsigned<T>::type *)(&v));
  }

  /*! writes an unsigned varint
   */
  template <class T>
  void serialize_uvarint(T &v)
  {
    typedef std::ostreambuf_iterator<char> it;
    tools::write_varint(it(stream_), v);
  }

  /* standard API functions follow
   */
  void begin_array(size_t s)
  {
    serialize_varint(s);
  }
  void begin_array() { }
  void delimit_array() { }
  void end_array() { }

  void begin_string(const char *delimiter="\"") { }
  void end_string(const char *delimiter="\"") { }
  // synonym to serialize_int
  void write_variant_tag(variant_tag_type t) {
    serialize_int(t);
  }
};

POP_WARNINGS
