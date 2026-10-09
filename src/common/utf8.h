// Copyright (c) 2019-2024, The Monero Project
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

#include <cctype>
#include <cwchar>
#include <stdexcept>

#include <utf8proc.h>

namespace tools
{
  template<typename T, typename Transform>
  inline T utf8canonical(const T &s, Transform t = [](wint_t c)->wint_t { return c; })
  {
    T sc = "";
    const utf8proc_uint8_t *ptr = reinterpret_cast<const utf8proc_uint8_t*>(s.data());
    utf8proc_ssize_t avail = s.size();
    utf8proc_uint8_t wbuf[4];
    while (avail > 0)
    {
      utf8proc_int32_t cp = 0;
      utf8proc_ssize_t consumed = utf8proc_iterate(ptr, avail, &cp);
      if (consumed <= 0)
        throw std::runtime_error("Invalid UTF-8");
      ptr += consumed;
      avail -= consumed;

      cp = (utf8proc_int32_t)t((wint_t)cp);
      if (!utf8proc_codepoint_valid(cp))
        throw std::runtime_error("Invalid code point UTF-8 transformation");

      utf8proc_ssize_t written = utf8proc_encode_char(cp, wbuf);
      if (written <= 0)
        throw std::runtime_error("Invalid code point UTF-8 transformation");
      sc.append(reinterpret_cast<const char*>(wbuf), written);
    }
    return sc;
  }
}
