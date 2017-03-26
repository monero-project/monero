// Copyright (c) 2014-2017, The Monero Project
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

#include <array>
#ifdef _WIN32
# include <winsock.h>
#else
# include <arpa/inet.h>
#endif

#include <cstdint>
#include <gtest/gtest.h>
#include <string>

#include "string_tools.h"

TEST(StringTools, GetIpString)
{
  EXPECT_EQ(
    std::string{"0.0.0.0"}, epee::string_tools::get_ip_string_from_int32(0)
  );
  EXPECT_EQ(
    std::string{"255.0.255.0"},
    epee::string_tools::get_ip_string_from_int32(htonl(0xff00ff00))
  );
  EXPECT_EQ(
    std::string{"255.255.255.255"},
    epee::string_tools::get_ip_string_from_int32(htonl(0xffffffff))
  );
}

TEST(StringTools, GetIpInt32)
{
  std::uint32_t ip = 0;
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, ""));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "1."));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "1.1."));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "1.1.1."));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "ff.0.ff.0"));
  EXPECT_FALSE(epee::string_tools::get_ip_int32_from_string(ip, "1.1.1.256"));

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "1"));
  EXPECT_EQ(htonl(1), ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "1.1"));
  EXPECT_EQ(htonl(0x1000001), ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "1.1.1"));
  EXPECT_EQ(htonl(0x1010001), ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "0.0.0.0"));
  EXPECT_EQ(0, ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "1.1.1.1"));
  EXPECT_EQ(htonl(0x01010101), ip);

/*
  The existing epee conversion function does not work with 255.255.255.255, for
  the reasons specified in the inet_addr documentation. Consider fixing in a
  future patch. This address is not likely to be used for purposes within
  monero.
  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "255.255.255.255"));
  EXPECT_EQ(htonl(0xffffffff), ip);
*/

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "10.0377.0.0377"));
  EXPECT_EQ(htonl(0xaff00ff), ip);

  EXPECT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "0xff.10.0xff.0"));
  EXPECT_EQ(htonl(0xff0aff00), ip);
}

