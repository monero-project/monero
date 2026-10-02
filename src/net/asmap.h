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

#ifndef BITCOIN_UTIL_ASMAP_H
#define BITCOIN_UTIL_ASMAP_H

#include <uint256.h>
#include <util/fs.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

uint32_t Interpret(std::span<const std::byte> asmap, std::span<const std::byte> ip);

bool SanityCheckAsmap(std::span<const std::byte> asmap, int bits);
/** Check standard asmap data (128 bits for IPv6) */
bool CheckStandardAsmap(std::span<const std::byte> data);

/** Read and check asmap from provided binary file */
std::vector<std::byte> DecodeAsmap(fs::path path);
/** Calculate the asmap version, a checksum identifying the asmap being used. */
uint256 AsmapVersion(std::span<const std::byte> data);

#endif // BITCOIN_UTIL_ASMAP_H
