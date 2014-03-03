// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/* json_archive.h
 *
 * JSON archive */

#pragma once

#include "serialization.h"
#include <cassert>
#include <iostream>
#include <iomanip>

template <class Stream, bool IsSaving>
struct json_archive_base
{
  typedef Stream stream_type;
  typedef json_archive_base<Stream, IsSaving> base_type;
  typedef boost::mpl::bool_<IsSaving> is_saving;

  typedef const char *variant_tag_type;

  json_archive_base(stream_type &s, bool indent = false) : stream_(s), indent_(indent), object_begin(false), depth_(0) { }

  void tag(const char *tag) {
    if (!object_begin)
      stream_ << ", ";
    make_indent();
    stream_ << '"' << tag << "\": ";
    object_begin = false;
  }

  void begin_object()
  {
    stream_ << "{";
    ++depth_;
    object_begin = true;
  }

  void end_object()
  {
    --depth_;
    make_indent();
    stream_ << "}";
  }

  void begin_variant() { begin_object(); }
  void end_variant() { end_object(); }
  Stream &stream() { return stream_; }

protected:
  void make_indent()
  {
    if (indent_)
    {
      stream_ << '\n' << std::string(2 * depth_, ' ');
    }
  }

protected:
  stream_type &stream_;
  bool indent_;
  bool object_begin;
  size_t depth_;
};

template <bool W>
struct json_archive;

template <>
struct json_archive<true> : public json_archive_base<std::ostream, true>
{
  json_archive(stream_type &s, bool indent = false) : base_type(s, indent) { }

  template<typename T>
  static auto promote_to_printable_integer_type(T v) -> decltype(+v)
  {
    // Unary operator '+' performs integral promotion on type T [expr.unary.op].
    // If T is signed or unsigned char, it's promoted to int and printed as number.
    return +v;
  }

  template <class T>
  void serialize_int(T v)
  {
    stream_ << std::dec << promote_to_printable_integer_type(v);
  }

  void serialize_blob(void *buf, size_t len, const char *delimiter="\"") {
    begin_string(delimiter);
    for (size_t i = 0; i < len; i++) {
      unsigned char c = ((unsigned char *)buf)[i];
      stream_ << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    end_string(delimiter);
  }

  template <class T>
  void serialize_varint(T &v)
  {
    stream_ << std::dec << promote_to_printable_integer_type(v);
  }

  void begin_string(const char *delimiter="\"")
  {
    stream_ << delimiter;
  }

  void end_string(const char *delimiter="\"")
  {
    stream_ << delimiter;
  }

  void begin_array(size_t s=0)
  {
    inner_array_size_ = s;
    ++depth_;
    stream_ << "[ ";
  }

  void delimit_array()
  {
    stream_ << ", ";
  }

  void end_array()
  {
    --depth_;
    if (0 < inner_array_size_)
    {
      make_indent();
    }
    stream_ << "]";
  }

  void write_variant_tag(const char *t)
  {
    tag(t);
  }

private:
  size_t inner_array_size_;
};
