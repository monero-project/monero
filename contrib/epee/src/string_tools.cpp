// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "string_tools.h"

#include <ctype.h>

#ifdef _WIN32
# include <winsock2.h>
#else
# include <arpa/inet.h>
# include <netinet/in.h>
#endif

namespace epee
{
namespace string_tools
{
  std::string get_ip_string_from_int32(uint32_t ip)
  {
    in_addr adr;
    adr.s_addr = ip;
    const char* pbuf = inet_ntoa(adr);
    if(pbuf)
      return pbuf;
    else
      return "[failed]";
  }
  //----------------------------------------------------------------------------
  bool get_ip_int32_from_string(uint32_t& ip, const std::string& ip_str)
  {
    ip = inet_addr(ip_str.c_str());
    if(INADDR_NONE == ip)
      return false;

    return true;
  }
  //----------------------------------------------------------------------------
  bool validate_hex(uint64_t length, const std::string& str)
  {
    if (str.size() != length)
      return false;
    for (char c: str)
      if (!isxdigit(c))
        return false;
    return true;
  }
  //----------------------------------------------------------------------------
  epee::string_tools::memdump_t dump(const uint8_t *ptr, size_t bytes, const std::string &prefix)
  {
    memdump_t v;
    v.reserve((bytes + 15) / 16);
    std::string full_prefix;
    if (!prefix.empty())
      full_prefix = prefix + "  ";
    size_t offset = 0;
    static const char chars[] = "0123456789abcdef";
    while (bytes > 0 || v.empty())
    {
      char offs[9], hex[50], ascii[17];
      memset(hex, 32, sizeof(hex));
      memset(ascii, 0, sizeof(ascii));
      const size_t row = std::min<size_t>(16, bytes);
      for (size_t i = 0; i < row; ++i)
      {
        hex[i * 3 + (i >= 8)] = chars[ptr[i] >> 4];
        hex[i * 3 + (i >= 8) + 1] = chars[ptr[i] & 0xf];
        ascii[i] = isprint(ptr[i]) ? ptr[i] : '.';
      }
      snprintf(offs, 9, "%08zx", offset);
      hex[49] = 0;
      v.push_back(full_prefix + std::string(offs) + "  " + hex + "  " + ascii);
      bytes -= row;
      ptr += row;
      offset += row;
    }
    return v;
  }
}
}

