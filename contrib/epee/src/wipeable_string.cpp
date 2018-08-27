// Copyright (c) 2017-2018, The Monero Project
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

#include <boost/optional/optional.hpp>
#include <string.h>
#include "memwipe.h"
#include "misc_log_ex.h"
#include "wipeable_string.h"

static constexpr const char hex[] = u8"0123456789abcdef";

namespace
{
  int atolower(int c)
  {
    if (c >= 'A' && c <= 'Z')
      c |= 32;
    return c;
  }
}

namespace epee
{

wipeable_string::wipeable_string(const wipeable_string &other):
  buffer(other.buffer)
{
}

wipeable_string::wipeable_string(wipeable_string &&other)
{
  if (&other == this)
    return;
  buffer = std::move(other.buffer);
}

wipeable_string::wipeable_string(const std::string &other)
{
  grow(other.size());
  memcpy(buffer.data(), other.c_str(), size());
}

wipeable_string::wipeable_string(std::string &&other)
{
  grow(other.size());
  memcpy(buffer.data(), other.c_str(), size());
  if (!other.empty())
  {
    memwipe(&other[0], other.size()); // we're kinda left with this again aren't we
    other = std::string();
  }
}

wipeable_string::wipeable_string(const char *s)
{
  grow(strlen(s));
  memcpy(buffer.data(), s, size());
}

wipeable_string::wipeable_string(const char *s, size_t len)
{
  grow(len);
  memcpy(buffer.data(), s, len);
}

wipeable_string::~wipeable_string()
{
  wipe();
}

void wipeable_string::wipe()
{
  if (!buffer.empty())
    memwipe(buffer.data(), buffer.size() * sizeof(char));
}

void wipeable_string::grow(size_t sz, size_t reserved)
{
  if (reserved < sz)
    reserved = sz;
  if (reserved <= buffer.capacity())
  {
    if (sz < buffer.size())
      memwipe(buffer.data() + sz, buffer.size() - sz);
    buffer.resize(sz);
    return;
  }
  size_t old_sz = buffer.size();
  std::unique_ptr<char[]> tmp{new char[old_sz]};
  memcpy(tmp.get(), buffer.data(), old_sz * sizeof(char));
  if (old_sz > 0)
    memwipe(buffer.data(), old_sz * sizeof(char));
  buffer.reserve(reserved);
  buffer.resize(sz);
  memcpy(buffer.data(), tmp.get(), old_sz * sizeof(char));
  if (old_sz > 0)
    memwipe(tmp.get(), old_sz * sizeof(char));
}

void wipeable_string::push_back(char c)
{
  grow(size() + 1);
  buffer.back() = c;
}

void wipeable_string::operator+=(char c)
{
  push_back(c);
}

void wipeable_string::append(const char *ptr, size_t len)
{
  const size_t orgsz = size();
  CHECK_AND_ASSERT_THROW_MES(orgsz < std::numeric_limits<size_t>::max() - len, "Appended data too large");
  grow(orgsz + len);
  if (len > 0)
    memcpy(data() + orgsz, ptr, len);
}

void wipeable_string::operator+=(const char *s)
{
  append(s, strlen(s));
}

void wipeable_string::operator+=(const epee::wipeable_string &s)
{
  append(s.data(), s.size());
}

void wipeable_string::operator+=(const std::string &s)
{
  append(s.c_str(), s.size());
}

void wipeable_string::trim()
{
  size_t prefix = 0;
  while (prefix < size() && data()[prefix] == ' ')
    ++prefix;
  if (prefix > 0)
    memmove(buffer.data(), buffer.data() + prefix, size() - prefix);

  size_t suffix = 0;
  while (suffix < size()-prefix && data()[size() - 1 - prefix - suffix] == ' ')
    ++suffix;

  resize(size() - prefix - suffix);
}

void wipeable_string::split(std::vector<wipeable_string> &fields) const
{
  fields.clear();
  size_t len = size();
  const char *ptr = data();
  bool space = true;
  while (len--)
  {
    const char c = *ptr++;
    if (c != ' ')
    {
      if (space)
        fields.push_back({});
      fields.back().push_back(c);
    }
    space = c == ' ';
  }
}

boost::optional<epee::wipeable_string> wipeable_string::parse_hexstr() const
{
  if (size() % 2 != 0)
    return boost::none;
  boost::optional<epee::wipeable_string> res = epee::wipeable_string("");
  const size_t len = size();
  const char *d = data();
  res->grow(0, len / 2);
  for (size_t i = 0; i < len; i += 2)
  {
    char c = atolower(d[i]);
    const char *ptr0 = strchr(hex, c);
    if (!ptr0)
      return boost::none;
    c = atolower(d[i+1]);
    const char *ptr1 = strchr(hex, c);
    if (!ptr1)
      return boost::none;
    res->push_back(((ptr0-hex)<<4) | (ptr1-hex));
  }
  return res;
}

char wipeable_string::pop_back()
{
  const size_t sz = size();
  CHECK_AND_ASSERT_THROW_MES(sz > 0, "Popping from an empty string");
  const char c = buffer.back();
  resize(sz - 1);
  return c;
}

void wipeable_string::resize(size_t sz)
{
  grow(sz);
}

void wipeable_string::reserve(size_t sz)
{
  grow(size(), sz);
}

void wipeable_string::clear()
{
  resize(0);
}

wipeable_string &wipeable_string::operator=(wipeable_string &&other)
{
  if (&other != this)
    buffer = std::move(other.buffer);
  return *this;
}

wipeable_string &wipeable_string::operator=(const wipeable_string &other)
{
  if (&other != this)
    buffer = other.buffer;
  return *this;
}

}
