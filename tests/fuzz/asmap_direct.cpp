// Copyright (c) 2020-present The Bitcoin Core developers
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
// Adapted from Bitcoin Core src/test/fuzz/asmap_direct.cpp

#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

#include "include_base_utils.h"
#include "net/asmap.h"
#include "fuzzer.h"

using namespace net::asmap;

static std::vector<std::uint8_t> BitsToBytes(epee::span<const uint8_t> bits) noexcept
{
    std::vector<std::uint8_t> ret;
    uint8_t next_byte{0};
    int next_byte_bits{0};
    for (uint8_t val : bits) {
        next_byte |= (val & 1) << (next_byte_bits++);
        if (next_byte_bits == 8) {
            ret.push_back(next_byte);
            next_byte = 0;
            next_byte_bits = 0;
        }
    }
    if (next_byte_bits) ret.push_back(next_byte);

    return ret;
}

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  const epee::span<const uint8_t> buffer{buf, len};

    // Encoding: [asmap using 1 bit / byte] 0xFF [addr using 1 bit / byte]
    std::optional<size_t> sep_pos_opt;
    for (size_t pos = 0; pos < buffer.size(); ++pos) {
        uint8_t x = buffer[pos];
        if ((x & 0xFE) == 0) continue;
        if (x == 0xFF) {
            if (sep_pos_opt) return 0;
            sep_pos_opt = pos;
        } else {
            return 0;
        }
    }
    if (!sep_pos_opt) return 0; // Needs exactly 1 separator
    const size_t sep_pos{sep_pos_opt.value()};
    const size_t ip_len{buffer.size() - sep_pos - 1};
    if (ip_len > 128) return 0; // At most 128 bits in IP address

    // Checks on asmap
    auto asmap = BitsToBytes({buffer.data(), sep_pos});
    if (SanityCheckAsmap(epee::to_span(asmap), ip_len)) {
        // Verify that for valid asmaps, no prefix (except up to 7 zero padding bits) is valid.
        for (size_t prefix_len = sep_pos - 1; prefix_len > 0; --prefix_len) {
            auto prefix = BitsToBytes({buffer.data(), prefix_len});
            // We have to skip the prefixes of the same length as the original
            // asmap, since they will contain some zero padding bits in the last
            // byte.
            if (prefix.size() == asmap.size()) continue;
            assert(!SanityCheckAsmap(epee::to_span(prefix), ip_len));
        }

        // No address input should trigger assertions in interpreter
        auto addr = BitsToBytes({buffer.data() + sep_pos + 1, ip_len});
        (void)Interpret(epee::to_span(asmap), epee::to_span(addr));
    }
END_SIMPLE_FUZZER()
