// Copyright (c) 2012-present The Bitcoin Core developers
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// Modifications for Monero: Copyright (c) 2026, The Monero Project
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
#include <boost/asio/ip/address.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

#include "gtest/gtest.h"

#include "net/asmap.h"
#include "string_tools.h"
#include "unit_tests_utils.h"

namespace
{
  std::vector<std::uint8_t> from_hex(const std::string &hex)
  {
    std::string bin;
    if (!epee::string_tools::parse_hexstr_to_binbuff(hex, bin))
      return {};
    return {bin.begin(), bin.end()};
  }

  uint32_t GetMappedAS(const std::vector<std::uint8_t> &asmap, const char *str)
  {
    const auto ip = boost::asio::ip::make_address_v6(str).to_bytes();
    return net::asmap::Interpret(epee::to_span(asmap), epee::to_span(ip));
  }
}

TEST(asmap, asmap_test_vectors)
{
    // Randomly generated encoded ASMap with 128 ranges, up to 20-bit AS numbers.
    const std::vector<std::uint8_t> ASMAP_DATA = from_hex(
        "fd38d50f7d5d665357f64bba6bfc190d6078a7e68e5d3ac032edf47f8b5755f87881bfd3633d9aa7c1fa279b3"
        "6fe26c63bbc9de44e0f04e5a382d8e1cddbe1c26653bc939d4327f287e8b4d1f8aff33176787cb0ff7cb28e3f"
        "daef0f8f47357f801c9f7ff7a99f7f9c9f99de7f3156ae00f23eb27a303bc486aa3ccc31ec19394c2f8a53ddd"
        "ea3cc56257f3b7e9b1f488be9c1137db823759aa4e071eef2e984aaf97b52d5f88d0f373dd190fe45e06efef1"
        "df7278be680a73a74c76db4dd910f1d30752c57fe2bc9f079f1a1e1b036c2a69219f11c5e11980a3fa51f4f82"
        "d36373de73b1863a8c27e36ae0e4f705be3d76ecff038a75bc0f92ba7e7f6f4080f1c47c34d095367ecf4406c"
        "1e3bbc17ba4d6f79ea3f031b876799ac268b1e0ea9babf0f9a8e5f6c55e363c6363df46afc696d7afceaf49b6"
        "e62df9e9dc27e70664cafe5c53df66dd0b8237678ada90e73f05ec60e6f6e96c3cbb1ea2f9dece115d5bdba10"
        "33e53662a7d72a29477b5beb35710591d3e23e5f0379baea62ffdee535bcdf879cbf69b88d7ea37c8015381cf"
        "63dc33d28f757a4a5e15d6a08");

    // Check this data is a valid asmap.
    ASSERT_TRUE(net::asmap::CheckStandardAsmap(epee::to_span(ASMAP_DATA)));

    // Check some randomly-generated IPv6 addresses in it (biased towards the very beginning and
    // very end of the 128-bit range).
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "0:1559:183:3728:224c:65a5:62e6:e991"), 961340);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "d0:d493:faa0:8609:e927:8b75:293c:f5a4"), 961340);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "2a0:26f:8b2c:2ee7:c7d1:3b24:4705:3f7f"), 693761);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "a77:7cd4:4be5:a449:89f2:3212:78c6:ee38"), 0);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "1336:1ad6:2f26:4fe3:d809:7321:6e0d:4615"), 672176);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "1d56:abd0:a52f:a8d5:d5a7:a610:581d:d792"), 499880);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "378e:7290:54e5:bd36:4760:971c:e9b9:570d"), 0);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "406c:820b:272a:c045:b74e:fc0a:9ef2:cecc"), 248495);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "46c2:ae07:9d08:2d56:d473:2bc7:57e3:20ac"), 248495);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "50d2:3db6:52fa:2e7:12ec:5bc4:1bd1:49f9"), 124471);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "53e1:1812:ffa:dccf:f9f2:64be:75fa:795"), 539993);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "544d:eeba:3990:35d1:ad66:f9a3:576d:8617"), 374443);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "6a53:40dc:8f1d:3ffa:efeb:3aa3:df88:b94b"), 435070);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "87aa:d1c9:9edb:91e7:aab1:9eb9:baa0:de18"), 244121);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "9f00:48fa:88e3:4b67:a6f3:e6d2:5cc1:5be2"), 862116);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "c49f:9cc6:86ad:ba08:4580:315e:dbd1:8a62"), 969411);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "dff5:8021:61d:b17d:406d:7888:fdac:4a20"), 969411);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "e888:6791:2960:d723:bcfd:47e1:2d8c:599f"), 824019);
    EXPECT_EQ(GetMappedAS(ASMAP_DATA, "ffff:d499:8c4b:4941:bc81:d5b9:b51e:85a8"), 824019);
}

TEST(asmap, decode_file)
{
  const auto asmap = net::asmap::DecodeAsmap((unit_test::data_dir / "asmap.raw").string());
  ASSERT_EQ(asmap.size(), 59);
  EXPECT_TRUE(net::asmap::CheckStandardAsmap(epee::to_span(asmap)));
}

TEST(asmap, decode_missing_file)
{
  EXPECT_TRUE(net::asmap::DecodeAsmap((unit_test::data_dir / "asmap-does-not-exist.raw").string()).empty());
}

TEST(asmap, decode_directory)
{
  EXPECT_TRUE(net::asmap::DecodeAsmap(unit_test::data_dir.string()).empty());
}

TEST(asmap, decode_device)
{
  if (!boost::filesystem::exists("/dev/zero"))
    return;
  EXPECT_TRUE(net::asmap::DecodeAsmap("/dev/zero").empty());
}

TEST(asmap, decode_truncated_file)
{
  const auto asmap = net::asmap::DecodeAsmap((unit_test::data_dir / "asmap.raw").string());
  ASSERT_FALSE(asmap.empty());
  const boost::filesystem::path path = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
  {
    std::ofstream out(path.string(), std::ios::binary);
    out.write(reinterpret_cast<const char*>(asmap.data()), asmap.size() - 1);
  }
  EXPECT_TRUE(net::asmap::DecodeAsmap(path.string()).empty());
  boost::filesystem::remove(path);
}
