// Copyright (c) 2018-2024, The Monero Project

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

#include <vector>
#include "misc_log_ex.h"
#include "span.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.buffer"

//#define NET_BUFFER_LOG(x) MDEBUG(x)
#define NET_BUFFER_LOG(x) ((void)0)

namespace epee
{
namespace net_utils
{
class buffer
{
public:
  buffer(size_t reserve = 0): offset(0) { storage.reserve(reserve); }

  void append(const void *data, size_t sz);
  void erase(size_t sz) { NET_BUFFER_LOG("erasing " << sz << "/" << size()); CHECK_AND_ASSERT_THROW_MES(offset + sz <= storage.size(), "erase: sz too large"); offset += sz; if (offset == storage.size()) { storage.resize(0); offset = 0; } }
  epee::span<const uint8_t> span(size_t sz) const { CHECK_AND_ASSERT_THROW_MES(sz <= size(), "span is too large"); return epee::span<const uint8_t>(storage.data() + offset, sz); }
  // carve must keep the data in scope till next call, other API calls (such as append, erase) can invalidate the carved buffer
  epee::span<const uint8_t> carve(size_t sz) { CHECK_AND_ASSERT_THROW_MES(sz <= size(), "span is too large"); offset += sz; return epee::span<const uint8_t>(storage.data() + offset - sz, sz); }
  size_t size() const { return storage.size() - offset; }

private:
  std::vector<uint8_t> storage;
  size_t offset;
};
}
}
