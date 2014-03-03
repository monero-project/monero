// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "json_archive.h"
#include "variant.h"

template <bool W>
struct debug_archive : public json_archive<W> {
  typedef typename json_archive<W>::stream_type stream_type;

  debug_archive(stream_type &s) : json_archive<W>(s) { }
};

template <class T>
struct serializer<debug_archive<true>, T>
{
  static void serialize(debug_archive<true> &ar, T &v)
  {
    ar.begin_object();
    ar.tag(variant_serialization_traits<debug_archive<true>, T>::get_tag());
    serializer<json_archive<true>, T>::serialize(ar, v);
    ar.end_object();
    ar.stream() << std::endl;
  }
};
