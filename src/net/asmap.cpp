// Copyright (c) 2019-present The Bitcoin Core developers
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
// Adapted from Bitcoin Core src/util/asmap.cpp

#include "net/asmap.h"

#include <cassert>
#include <cstddef>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>

#include "common/util.h"
#include "misc_log_ex.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.p2p"

/*
 * ASMap (Autonomous System Map) Implementation
 *
 * Provides a compressed mapping from IP address prefixes to Autonomous System Numbers (ASNs).
 * Uses a binary trie structure encoded as bytecode instructions that are interpreted
 * at runtime to find the ASN for a given IP address.
 *
 * The format of the asmap data is a bit-packed binary format where the entire mapping
 * is treated as a continuous sequence of bits. Instructions and their arguments are
 * encoded using variable numbers of bits and concatenated together without regard for
 * byte boundaries. The bits are stored in bytes using little-endian bit ordering.
 *
 * The data structure internally represents the mapping as a binary trie where:
 * - Unassigned subnets (no ASN mapping present) map to 0
 * - Subnets mapped entirely to one ASN become leaf nodes
 * - Subnets whose lower and upper halves have different mappings branch into subtrees
 *
 * The encoding uses variable-length integers and four instruction types (RETURN, JUMP,
 * MATCH, DEFAULT) to efficiently represent the trie.
 */

namespace net
{
namespace asmap
{

namespace {

// Indicates decoding errors or invalid data
constexpr uint32_t INVALID = 0xFFFFFFFF;

/**
 * Extract a single bit from byte array using little-endian bit ordering (LSB first).
 * Used for ASMap data.
 */
inline bool ConsumeBitLE(size_t& bitpos, epee::span<const std::uint8_t> bytes) noexcept
{
    const bool bit = (bytes[bitpos / 8] >> (bitpos % 8)) & 1;
    ++bitpos;
    return bit;
}

/**
 * Extract a single bit from byte array using big-endian bit ordering (MSB first).
 * Used for IP addresses to match network byte order conventions.
 */
inline bool ConsumeBitBE(uint8_t& bitpos, epee::span<const std::uint8_t> bytes) noexcept
{
    const bool bit = (bytes[bitpos / 8] >> (7 - (bitpos % 8))) & 1;
    ++bitpos;
    return bit;
}

/**
 * Variable-length integer decoder using a custom encoding scheme.
 *
 * The encoding is easiest to describe using an example. Let's say minval=100 and
 * bit_sizes=[4,2,2,3]. In that case:
 * - x in [100..115]: encoded as [0] + [4-bit BE encoding of (x-100)]
 * - x in [116..119]: encoded as [1,0] + [2-bit BE encoding of (x-116)]
 * - x in [120..123]: encoded as [1,1,0] + [2-bit BE encoding of (x-120)]
 * - x in [124..131]: encoded as [1,1,1] + [3-bit BE encoding of (x-124)]
 *
 * In general, every number is encoded as:
 * - First, k "1"-bits, where k is the class the number falls in
 * - Then, a "0"-bit, unless k is the highest class
 * - Lastly, bit_sizes[k] bits encoding in big endian the position within that class
 */
uint32_t DecodeBits(size_t& bitpos, const epee::span<const std::uint8_t> data, uint8_t minval, const epee::span<const uint8_t> bit_sizes)
{
    uint32_t val = minval;  // Start with minimum encodable value
    bool bit;
    for (auto bit_sizes_it = bit_sizes.begin(); bit_sizes_it != bit_sizes.end(); ++bit_sizes_it) {
        // Read continuation bit to determine if we're in this class
        if (bit_sizes_it + 1 != bit_sizes.end()) {  // Unless we're in the last class
            if (bitpos >= data.size() * 8) break;
            bit = ConsumeBitLE(bitpos, data);
        } else {
            bit = 0;  // Last class has no continuation bit
        }
        if (bit) {
            // If the value will not fit in this class, subtract its range from val,
            // emit a "1" bit and continue with the next class
            val += (1 << *bit_sizes_it);  // Add size of this class
        } else {
            // Decode the position within this class in big endian
            for (int b = 0; b < *bit_sizes_it; b++) {
                if (bitpos >= data.size() * 8) return INVALID; // Reached EOF in mantissa
                bit = ConsumeBitLE(bitpos, data);
                val += bit << (*bit_sizes_it - 1 - b); // Big-endian within the class
            }
            return val;
        }
    }
    return INVALID; // Reached EOF in exponent
}

/**
 * Instruction Set
 *
 * The instruction set is designed to efficiently encode a binary trie
 * that maps IP prefixes to ASNs. Each instruction type serves a specific
 * role in trie traversal and evaluation.
 */
enum class Instruction : uint32_t
{
    // A return instruction, encoded as [0], returns a constant ASN.
    // It is followed by an integer using the ASN encoding.
    RETURN = 0,
    // A jump instruction, encoded as [1,0], inspects the next unused bit in the input
    // and either continues execution (if 0), or skips a specified number of bits (if 1).
    // It is followed by an integer using jump encoding.
    JUMP = 1,
    // A match instruction, encoded as [1,1,0], inspects 1 or more of the next unused bits
    // in the input. If they all match, execution continues. If not, the default ASN is returned
    // (or 0 if unset). The match value encodes both the pattern and its length.
    MATCH = 2,
    // A default instruction, encoded as [1,1,1], sets the default variable to its argument,
    // and continues execution. It is followed by an integer in ASN encoding.
    DEFAULT = 3,
};

// Instruction type encoding: RETURN=[0], JUMP=[1,0], MATCH=[1,1,0], DEFAULT=[1,1,1]
constexpr uint8_t TYPE_BIT_SIZES[]{0, 0, 1};
Instruction DecodeType(size_t& bitpos, const epee::span<const std::uint8_t> data)
{
    return Instruction(DecodeBits(bitpos, data, 0, TYPE_BIT_SIZES));
}

// ASN encoding: Can encode ASNs from 1 to ~16.7 million.
// Uses variable-length encoding optimized for real-world ASN distribution.
// ASN 0 is reserved and used if there isn't a match.
constexpr uint8_t ASN_BIT_SIZES[]{15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
uint32_t DecodeASN(size_t& bitpos, const epee::span<const std::uint8_t> data)
{
    return DecodeBits(bitpos, data, 1, ASN_BIT_SIZES);
}

// MATCH argument: Values in [2, 511]. The highest set bit determines the match length
// n ∈ [1,8]; the lower n-1 bits are the pattern to compare.
constexpr uint8_t MATCH_BIT_SIZES[]{1, 2, 3, 4, 5, 6, 7, 8};
uint32_t DecodeMatch(size_t& bitpos, const epee::span<const std::uint8_t> data)
{
    return DecodeBits(bitpos, data, 2, MATCH_BIT_SIZES);
}

// JUMP offset: Minimum value 17. Variable-length coded and may be large
// for skipping big subtrees.
constexpr uint8_t JUMP_BIT_SIZES[]{5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
uint32_t DecodeJump(size_t& bitpos, const epee::span<const std::uint8_t> data)
{
    return DecodeBits(bitpos, data, 17, JUMP_BIT_SIZES);
}

/** Number of bits needed to represent x (std::bit_width is C++20) */
inline int BitWidth(uint32_t x) noexcept
{
    int ret = 0;
    while (x) {
        x >>= 1;
        ++ret;
    }
    return ret;
}

} // anonymous namespace

/**
 * Execute the ASMap bytecode to find the ASN for an IP
 *
 * This function interprets the asmap bytecode and uses bits from the IP
 * address to navigate through the encoded trie structure, ultimately
 * returning an ASN value.
 */
uint32_t Interpret(const epee::span<const std::uint8_t> asmap, const epee::span<const std::uint8_t> ip)
{
    size_t pos{0};
    const size_t endpos{asmap.size() * 8};
    uint8_t ip_bit{0};
    const uint8_t ip_bits_end = ip.size() * 8;
    uint32_t default_asn = 0;
    while (pos < endpos) {
        Instruction opcode = DecodeType(pos, asmap);
        if (opcode == Instruction::RETURN) {
            // Found leaf node - return the ASN
            uint32_t asn = DecodeASN(pos, asmap);
            if (asn == INVALID) break; // ASN straddles EOF
            return asn;
        } else if (opcode == Instruction::JUMP) {
            // Binary branch: if IP bit is 1, jump forward; else continue
            uint32_t jump = DecodeJump(pos, asmap);
            if (jump == INVALID) break; // Jump offset straddles EOF
            if (ip_bit == ip_bits_end) break; // No input bits left
            if (int64_t{jump} >= static_cast<int64_t>(endpos - pos)) break; // Jumping past EOF
            if (ConsumeBitBE(ip_bit, ip)) {  // Check next IP bit (big-endian)
                pos += jump;  // Bit = 1: skip to right subtree
            }
            // Bit = 0: fall through to left subtree
        } else if (opcode == Instruction::MATCH) {
            // Compare multiple IP bits against a pattern
            // The match value encodes both length and pattern:
            // - highest set bit position determines length (bit_width - 1)
            // - lower bits contain the pattern to compare
            uint32_t match = DecodeMatch(pos, asmap);
            if (match == INVALID) break; // Match bits straddle EOF
            int matchlen = BitWidth(match) - 1;  // An n-bit value matches n-1 input bits
            if ((ip_bits_end - ip_bit) < matchlen) break; // Not enough input bits
            for (int bit = 0; bit < matchlen; bit++) {
                if (ConsumeBitBE(ip_bit, ip) != ((match >> (matchlen - 1 - bit)) & 1)) {
                    return default_asn;  // Pattern mismatch - use default
                }
            }
            // Pattern matched - continue execution
        } else if (opcode == Instruction::DEFAULT) {
            // Update the default ASN for subsequent MATCH failures
            default_asn = DecodeASN(pos, asmap);
            if (default_asn == INVALID) break; // ASN straddles EOF
        } else {
            break; // Instruction straddles EOF
        }
    }
    // Reached EOF without RETURN, or aborted (see any of the breaks above)
    // - should have been caught by SanityCheckAsmap below
    assert(false);
    return 0; // 0 is not a valid ASN
}

/**
 * Validates ASMap structure by simulating all possible execution paths.
 * Ensures well-formed bytecode, valid jumps, and proper termination.
 */
bool SanityCheckAsmap(const epee::span<const std::uint8_t> asmap, int bits)
{
    size_t pos{0};
    const size_t endpos{asmap.size() * 8};
    std::vector<std::pair<uint32_t, int>> jumps; // All future positions we may jump to (bit offset in asmap -> bits to consume left)
    jumps.reserve(bits);
    Instruction prevopcode = Instruction::JUMP;
    bool had_incomplete_match = false;  // Track <8 bit matches for efficiency check

    while (pos != endpos) {
        // There was a jump into the middle of the previous instruction
        if (!jumps.empty() && pos >= jumps.back().first) return false;

        Instruction opcode = DecodeType(pos, asmap);
        if (opcode == Instruction::RETURN) {
            // There should not be any RETURN immediately after a DEFAULT (could be combined into just RETURN)
            if (prevopcode == Instruction::DEFAULT) return false;
            uint32_t asn = DecodeASN(pos, asmap);
            if (asn == INVALID) return false; // ASN straddles EOF
            if (jumps.empty()) {
                // Nothing to execute anymore
                if (endpos - pos > 7) return false; // Excessive padding
                while (pos != endpos) {
                    if (ConsumeBitLE(pos, asmap)) return false; // Nonzero padding bit
                }
                return true; // Sanely reached EOF
            } else {
                // Continue by pretending we jumped to the next instruction
                if (pos != jumps.back().first) return false; // Unreachable code
                bits = jumps.back().second; // Restore the number of bits we would have had left after this jump
                jumps.pop_back();
                prevopcode = Instruction::JUMP;
            }
        } else if (opcode == Instruction::JUMP) {
            uint32_t jump = DecodeJump(pos, asmap);
            if (jump == INVALID) return false; // Jump offset straddles EOF
            if (int64_t{jump} > static_cast<int64_t>(endpos - pos)) return false; // Jump out of range
            if (bits == 0) return false; // Consuming bits past the end of the input
            --bits;
            uint32_t jump_offset = pos + jump;
            if (!jumps.empty() && jump_offset >= jumps.back().first) return false; // Intersecting jumps
            jumps.emplace_back(jump_offset, bits);  // Queue jump target for validation
            prevopcode = Instruction::JUMP;
        } else if (opcode == Instruction::MATCH) {
            uint32_t match = DecodeMatch(pos, asmap);
            if (match == INVALID) return false; // Match bits straddle EOF
            int matchlen = BitWidth(match) - 1;
            if (prevopcode != Instruction::MATCH) had_incomplete_match = false;
            // Within a sequence of matches only at most one should be incomplete
            if (matchlen < 8 && had_incomplete_match) return false;
            had_incomplete_match = (matchlen < 8);
            if (bits < matchlen) return false; // Consuming bits past the end of the input
            bits -= matchlen;
            prevopcode = Instruction::MATCH;
        } else if (opcode == Instruction::DEFAULT) {
            // There should not be two successive DEFAULTs (they could be combined into one)
            if (prevopcode == Instruction::DEFAULT) return false;
            uint32_t asn = DecodeASN(pos, asmap);
            if (asn == INVALID) return false; // ASN straddles EOF
            prevopcode = Instruction::DEFAULT;
        } else {
            return false; // Instruction straddles EOF
        }
    }
    return false; // Reached EOF without RETURN instruction
}

/**
 * Provides a safe interface for validating ASMap data before use.
 * Returns true if the data is valid for 128 bits long inputs.
 */
bool CheckStandardAsmap(const epee::span<const std::uint8_t> data)
{
    if (!SanityCheckAsmap(data, 128)) {
        MWARNING("Sanity check of asmap data failed");
        return false;
    }
    return true;
}

/**
 * Loads an ASMap file from disk and validates it.
 */
std::vector<std::uint8_t> DecodeAsmap(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        MWARNING("Can't open asmap file " << path);
        return {};
    }
    boost::system::error_code ec;
    const boost::uintmax_t length = boost::filesystem::file_size(path, ec);
    if (ec) { // e.g. path is a directory
        MWARNING("Failed to read asmap file " << path);
        return {};
    }
    MINFO("Opened asmap file " << path << " (" << length << " bytes) from disk");

    // Read entire file into memory
    std::vector<std::uint8_t> buffer(length);
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    if (static_cast<boost::uintmax_t>(file.gcount()) != length) {
        MWARNING("Failed to read asmap file " << path);
        return {};
    }

    if (!CheckStandardAsmap(epee::to_span(buffer))) {
        MWARNING("Sanity check of asmap file " << path << " failed");
        return {};
    }

    return buffer;
}

/**
 * Computes SHA256 hash of ASMap data for versioning and consistency checks.
 */
crypto::hash AsmapVersion(const epee::span<const std::uint8_t> data)
{
    crypto::hash version = crypto::null_hash;
    if (data.empty()) return version;

    if (!tools::sha256sum(data.data(), data.size(), version)) return crypto::null_hash;
    return version;
}

} // asmap
} // net
