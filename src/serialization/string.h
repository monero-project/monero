// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <memory>
#include "serialization.h"

template <template <bool> class Archive>
inline bool do_serialize(Archive<false>& ar, std::string& str)
{
  size_t size = 0;
  ar.serialize_varint(size);
  if (ar.remaining_bytes() < size)
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

  std::unique_ptr<std::string::value_type[]> buf(new std::string::value_type[size]);
  ar.serialize_blob(buf.get(), size);
  str.erase();
  str.append(buf.get(), size);
  return true;
}


template <template <bool> class Archive>
inline bool do_serialize(Archive<true>& ar, std::string& str)
{
  size_t size = str.size();
  ar.serialize_varint(size);
  ar.serialize_blob(const_cast<std::string::value_type*>(str.c_str()), size);
  return true;
}
