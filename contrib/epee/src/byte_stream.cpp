// Copyright (c) 2020-2024, The Monero Project

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

#include "byte_stream.h"

#include <algorithm>
#include <limits>
#include <utility>

#include <iostream>

namespace
{
  constexpr const std::size_t minimum_increase = 4096;
}

namespace epee
{
  void byte_stream::overflow(const std::size_t requested)
  {
    // Recalculating `need` bytes removes at least one instruction from every
    // inlined `put` call in header

    assert(available() < requested);
    const std::size_t need = requested - available();

    const std::size_t len = size();
    const std::size_t cap = capacity();
    const std::size_t increase = std::max(std::max(need, cap), minimum_increase);

    next_write_ = nullptr;
    end_ = nullptr;

    buffer_ = byte_buffer_increase(std::move(buffer_), cap, increase);
    if (!buffer_)
      throw std::bad_alloc{};

    next_write_ = buffer_.get() + len;
    end_ = buffer_.get() + cap + increase;
  }

  byte_stream::byte_stream(byte_stream&& rhs) noexcept
    : buffer_(std::move(rhs.buffer_)),
      next_write_(rhs.next_write_),
      end_(rhs.end_)
  {
    rhs.next_write_ = nullptr;
    rhs.end_ = nullptr;
  }

  byte_stream& byte_stream::operator=(byte_stream&& rhs) noexcept
  {
    if (this != std::addressof(rhs))
    {
      buffer_ = std::move(rhs.buffer_);
      next_write_ = rhs.next_write_;
      end_ = rhs.end_;
      rhs.next_write_ = nullptr;
      rhs.end_ = nullptr;
    }
    return *this;
  }

  byte_buffer byte_stream::take_buffer() noexcept
  {
    byte_buffer out{std::move(buffer_)};
    next_write_ = nullptr;
    end_ = nullptr;
    return out;
  }
}
