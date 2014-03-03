// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/* binary_archive.h
 *
 * Portable (low-endian) binary archive */
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

template <class Stream, bool IsSaving>
struct binary_archive_base
{
  typedef Stream stream_type;
  typedef binary_archive_base<Stream, IsSaving> base_type;
  typedef boost::mpl::bool_<IsSaving> is_saving;

  typedef uint8_t variant_tag_type;

  explicit binary_archive_base(stream_type &s) : stream_(s) { }

  void tag(const char *) { }
  void begin_object() { }
  void end_object() { }
  void begin_variant() { }
  void end_variant() { }
  stream_type &stream() { return stream_; }
protected:
  stream_type &stream_;
};

template <bool W>
struct binary_archive;

template <>
struct binary_archive<false> : public binary_archive_base<std::istream, false>
{
  explicit binary_archive(stream_type &s) : base_type(s) {
    stream_type::streampos pos = stream_.tellg();
    stream_.seekg(0, std::ios_base::end);
    eof_pos_ = stream_.tellg();
    stream_.seekg(pos);
  }

  template <class T>
  void serialize_int(T &v)
  {
    serialize_uint(*(typename boost::make_unsigned<T>::type *)&v);
  }

  template <class T>
  void serialize_uint(T &v, size_t width = sizeof(T))
  {
    T ret = 0;
    unsigned shift = 0;
    for (size_t i = 0; i < width; i++) {
      //std::cerr << "tell: " << stream_.tellg() << " value: " << ret << std::endl;
      char c;
      stream_.get(c);
      T b = (unsigned char)c;
      ret += (b << shift);
      shift += 8;
    }
    v = ret;
  }
  void serialize_blob(void *buf, size_t len, const char *delimiter="") { stream_.read((char *)buf, len); }

  template <class T>
  void serialize_varint(T &v)
  {
    serialize_uvarint(*(typename boost::make_unsigned<T>::type *)(&v));
  }

  template <class T>
  void serialize_uvarint(T &v)
  {
    typedef std::istreambuf_iterator<char> it;
    tools::read_varint(it(stream_), it(), v); // XXX handle failure
  }
  void begin_array(size_t &s)
  {
    serialize_varint(s);
  }
  void begin_array() { }

  void delimit_array() { }
  void end_array() { }

  void begin_string(const char *delimiter="\"") { }
  void end_string(const char *delimiter="\"") { }

  void read_variant_tag(variant_tag_type &t) {
    serialize_int(t);
  }

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

template <>
struct binary_archive<true> : public binary_archive_base<std::ostream, true>
{
  explicit binary_archive(stream_type &s) : base_type(s) { }

  template <class T>
  void serialize_int(T v)
  {
    serialize_uint(static_cast<typename boost::make_unsigned<T>::type>(v));
  }
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
  void serialize_blob(void *buf, size_t len, const char *delimiter="") { stream_.write((char *)buf, len); }

  template <class T>
  void serialize_varint(T &v)
  {
    serialize_uvarint(*(typename boost::make_unsigned<T>::type *)(&v));
  }

  template <class T>
  void serialize_uvarint(T &v)
  {
    typedef std::ostreambuf_iterator<char> it;
    tools::write_varint(it(stream_), v);
  }
  void begin_array(size_t s)
  {
    serialize_varint(s);
  }
  void begin_array() { }
  void delimit_array() { }
  void end_array() { }

  void begin_string(const char *delimiter="\"") { }
  void end_string(const char *delimiter="\"") { }

  void write_variant_tag(variant_tag_type t) {
    serialize_int(t);
  }
};

POP_WARNINGS
