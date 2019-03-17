// Copyright (c) 2017-2019, The Monero Project
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

#pragma once

#include <boost/optional/optional.hpp>
#include <stddef.h>
#include <vector>
#include <string>
#include "memwipe.h"
#include "fnv1.h"

namespace epee
{
  class wipeable_string
  {
  public:
    typedef char value_type;

    wipeable_string() {}
    wipeable_string(const wipeable_string &other);
    wipeable_string(wipeable_string &&other);
    wipeable_string(const std::string &other);
    wipeable_string(std::string &&other);
    wipeable_string(const char *s);
    wipeable_string(const char *s, size_t len);
    ~wipeable_string();
    void wipe();
    void push_back(char c);
    void operator+=(char c);
    void operator+=(const std::string &s);
    void operator+=(const epee::wipeable_string &s);
    void operator+=(const char *s);
    void append(const char *ptr, size_t len);
    char pop_back();
    const char *data() const noexcept { return buffer.data(); }
    char *data() noexcept { return buffer.data(); }
    size_t size() const noexcept { return buffer.size(); }
    size_t length() const noexcept { return buffer.size(); }
    bool empty() const noexcept { return buffer.empty(); }
    void trim();
    void split(std::vector<wipeable_string> &fields) const;
    boost::optional<wipeable_string> parse_hexstr() const;
    template<typename T> inline bool hex_to_pod(T &pod) const;
    template<typename T> inline bool hex_to_pod(tools::scrubbed<T> &pod) const { return hex_to_pod(unwrap(pod)); }
    void resize(size_t sz);
    void reserve(size_t sz);
    void clear();
    bool operator==(const wipeable_string &other) const noexcept { return buffer == other.buffer; }
    bool operator!=(const wipeable_string &other) const noexcept { return buffer != other.buffer; }
    wipeable_string &operator=(wipeable_string &&other);
    wipeable_string &operator=(const wipeable_string &other);

  private:
    void grow(size_t sz, size_t reserved = 0);

  private:
    std::vector<char> buffer;
  };

  template<typename T> inline bool wipeable_string::hex_to_pod(T &pod) const
  {
    static_assert(std::is_pod<T>::value, "expected pod type");
    if (size() != sizeof(T) * 2)
      return false;
    boost::optional<epee::wipeable_string> blob = parse_hexstr();
    if (!blob)
      return false;
    if (blob->size() != sizeof(T))
      return false;
    pod = *(const T*)blob->data();
    return true;
  }
}

namespace std
{
  template<> struct hash<epee::wipeable_string>
  {
    size_t operator()(const epee::wipeable_string &s) const
    {
      return epee::fnv::FNV1a(s.data(), s.size());
    }
  };
}
