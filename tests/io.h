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

#include <cstddef>
#include <ios>
#include <iostream>
#include <type_traits>
#include <vector>

inline bool hexdecode(const char *from, std::size_t length, void *to) {
  std::size_t i;
  for (i = 0; i < length; i++) {
    int v = 0;
    if (from[2 * i] >= '0' && from[2 * i] <= '9') {
      v = from[2 * i] - '0';
    } else if (from[2 * i] >= 'a' && from[2 * i] <= 'f') {
      v = from[2 * i] - 'a' + 10;
    } else {
      return false;
    }
    v <<= 4;
    if (from[2 * i + 1] >= '0' && from[2 * i + 1] <= '9') {
      v |= from[2 * i + 1] - '0';
    } else if (from[2 * i + 1] >= 'a' && from[2 * i + 1] <= 'f') {
      v |= from[2 * i + 1] - 'a' + 10;
    } else {
      return false;
    }
    *(reinterpret_cast<unsigned char *>(to) + i) = v;
  }
  return true;
}

inline void get(std::istream &input, bool &res) {
  std::string sres;
  input >> sres;
  if (sres == "false") {
    res = false;
  } else if (sres == "true") {
    res = true;
  } else {
    input.setstate(std::ios_base::failbit);
  }
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, void>::type
get(std::istream &input, T &res) {
  input >> res;
}

inline void getvar(std::istream &input, std::size_t length, void *res) {
  std::string sres;
  input >> sres;
  if (sres.length() != 2 * length || !hexdecode(sres.data(), length, res)) {
    input.setstate(std::ios_base::failbit);
  }
}

template<typename T>
typename std::enable_if<std::is_standard_layout<T>::value && !std::is_scalar<T>::value, void>::type
get(std::istream &input, T &res) {
  getvar(input, sizeof(T), &res);
}

inline void get(std::istream &input, std::vector<char> &res) {
  std::string sres;
  input >> sres;
  if (sres == "x") {
    res.clear();
  } else if (sres.length() % 2 != 0) {
    input.setstate(std::ios_base::failbit);
  } else {
    std::size_t length = sres.length() / 2;
    res.resize(length);
    if (!hexdecode(sres.data(), length, res.data())) {
      input.setstate(std::ios_base::failbit);
    }
  }
}

#if !defined(_MSC_VER) || _MSC_VER >= 1800

template<typename T, typename... TT>
typename std::enable_if<(sizeof...(TT) > 0), void>::type
get(std::istream &input, T &res, TT &... resres) {
  get(input, res);
  get(input, resres...);
}

#else
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#define NESTED_GET(z, n, data) get(input, BOOST_PP_CAT(res, n));
#define GET(z, n, data) \
template<BOOST_PP_ENUM_PARAMS(n, typename T)> \
void get(std::istream &input, BOOST_PP_ENUM_BINARY_PARAMS(n, T, &res)) { \
  BOOST_PP_REPEAT(n, NESTED_GET, ~) \
}
BOOST_PP_REPEAT_FROM_TO(2, 5, GET, ~)

#endif
