// Copyright (c) 2019-2022, The Monero Project
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

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

#include "byte_slice.h"
#include "byte_stream.h"

namespace
{
  const std::size_t page_size = 4096;
}

namespace epee
{
  struct byte_slice_data
  {
    byte_slice_data() noexcept
      : ref_count(1)
    {}

    virtual ~byte_slice_data() noexcept
    {}

    std::atomic<std::size_t> ref_count;
  };

  void release_byte_slice::call(void*, void* ptr) noexcept
  {
    if (ptr)
    {
      byte_slice_data* self = static_cast<byte_slice_data*>(ptr);
      if (--(self->ref_count) == 0)
      {
        self->~byte_slice_data();
        free(self);
      }
    }
  }

  namespace
  {
    template<typename T>
    struct adapted_byte_slice final : byte_slice_data
    {
      explicit adapted_byte_slice(T&& buffer)
        : byte_slice_data(), buffer(std::move(buffer))
      {}

      virtual ~adapted_byte_slice() noexcept final override
      {}

      const T buffer;
    };

    // bytes "follow" this structure in memory slab
    struct raw_byte_slice final : byte_slice_data
    {
      raw_byte_slice() noexcept
        : byte_slice_data()
      {}

      virtual ~raw_byte_slice() noexcept final override
      {}
    };

    /* This technique is not-standard, but allows for the reference count and
       memory for the bytes (when given a list of spans) to be allocated in a
       single call. In that situation, the dynamic sized bytes are after/behind
       the raw_byte_slice class. The C runtime has to track the number of bytes
       allocated regardless, so free'ing is relatively easy. */

    template<typename T, typename... U>
    std::unique_ptr<T, release_byte_slice> allocate_slice(std::size_t extra_bytes, U&&... args)
    {
      if (std::numeric_limits<std::size_t>::max() - sizeof(T) < extra_bytes)
        throw std::bad_alloc{};

      void* const ptr = malloc(sizeof(T) + extra_bytes);
      if (ptr == nullptr)
        throw std::bad_alloc{};

      try
      {
        new (ptr) T{std::forward<U>(args)...};
      }
      catch (...)
      {
        free(ptr);
        throw;
      }
      return std::unique_ptr<T, release_byte_slice>{reinterpret_cast<T*>(ptr)};
    }
  } // anonymous

  void release_byte_buffer::operator()(std::uint8_t* buf) const noexcept
  {
    if (buf)
      std::free(buf - sizeof(raw_byte_slice));
  }

  byte_slice::byte_slice(byte_slice_data* storage, span<const std::uint8_t> portion) noexcept
    : storage_(storage), portion_(portion)
  {
    if (storage_)
      ++(storage_->ref_count);
  }

  template<typename T>
  byte_slice::byte_slice(const adapt_buffer, T&& buffer)
    : storage_(nullptr), portion_(nullptr)
  {
    if (!buffer.empty())
    {
      storage_ = allocate_slice<adapted_byte_slice<T>>(0, std::move(buffer));
      portion_ = to_byte_span(to_span(static_cast<adapted_byte_slice<T> *>(storage_.get())->buffer));
    }
  }

  byte_slice::byte_slice(std::initializer_list<span<const std::uint8_t>> sources)
    : byte_slice()
  {
    std::size_t space_needed = 0;
    for (const auto& source : sources)
      space_needed += source.size();

    if (space_needed)
    {
      auto storage = allocate_slice<raw_byte_slice>(space_needed);
      span<std::uint8_t> out{reinterpret_cast<std::uint8_t*>(storage.get() + 1), space_needed};
      portion_ = {out.data(), out.size()};

      for (const auto& source : sources)
      {
        std::memcpy(out.data(), source.data(), source.size());
        if (out.remove_prefix(source.size()) < source.size())
          throw std::bad_alloc{}; // size_t overflow on space_needed
      }
      storage_ = std::move(storage);
    }
  }

  byte_slice::byte_slice(std::string&& buffer)
    : byte_slice(adapt_buffer{}, std::move(buffer))
  {}

  byte_slice::byte_slice(std::vector<std::uint8_t>&& buffer)
    : byte_slice(adapt_buffer{}, std::move(buffer))
  {}

  byte_slice::byte_slice(byte_stream&& stream, const bool shrink)
    : storage_(nullptr), portion_(stream.data(), stream.size())
  {
    if (portion_.size())
    {
      byte_buffer buf;
      if (shrink && page_size <= stream.available())
      {
          buf = byte_buffer_resize(stream.take_buffer(), portion_.size());
          if (!buf)
            throw std::bad_alloc{};
          portion_ = {buf.get(), portion_.size()};
      }
      else // no need to shrink buffer
        buf = stream.take_buffer();

      std::uint8_t* const data = buf.release() - sizeof(raw_byte_slice);
      new (data) raw_byte_slice{};
      storage_.reset(reinterpret_cast<raw_byte_slice*>(data));
    }
    else // empty stream
      portion_ = nullptr;
  }

  byte_slice::byte_slice(byte_slice&& source) noexcept
    : storage_(std::move(source.storage_)), portion_(source.portion_)
  {
    source.portion_ = epee::span<const std::uint8_t>{};
  }

  byte_slice& byte_slice::operator=(byte_slice&& source) noexcept
  {
    storage_ = std::move(source.storage_);
    portion_ = source.portion_;
    if (source.storage_ == nullptr)
      source.portion_ = epee::span<const std::uint8_t>{};

    return *this;
  }

  std::size_t byte_slice::remove_prefix(std::size_t max_bytes) noexcept
  {
    max_bytes = portion_.remove_prefix(max_bytes);
    if (portion_.empty())
      storage_ = nullptr;
    return max_bytes;
  }

  byte_slice byte_slice::take_slice(const std::size_t max_bytes) noexcept
  {
    byte_slice out{};

    if (max_bytes)
    {
      std::uint8_t const* const ptr = data();
      out.portion_ = {ptr, portion_.remove_prefix(max_bytes)};

      if (portion_.empty())
        out.storage_ = std::move(storage_); // no atomic inc/dec
      else
        out = {storage_.get(), out.portion_};
    }
    return out;
  }

  byte_slice byte_slice::get_slice(const std::size_t begin, const std::size_t end) const
  {
    if (end < begin || portion_.size() < end)
      throw std::out_of_range{"bad slice range"};

    if (begin == end)
      return {};
    return {storage_.get(), {portion_.begin() + begin, end - begin}};
  }

  std::unique_ptr<byte_slice_data, release_byte_slice> byte_slice::take_buffer() noexcept
  {
    std::unique_ptr<byte_slice_data, release_byte_slice> out{std::move(storage_)};
    portion_ = nullptr;
    return out;
  }

  byte_buffer byte_buffer_resize(byte_buffer buf, const std::size_t length) noexcept
  {
    if (std::numeric_limits<std::size_t>::max() - sizeof(raw_byte_slice) < length)
      return nullptr;

    std::uint8_t* data = buf.get();
    if (data != nullptr)
      data -= sizeof(raw_byte_slice);

    data = static_cast<std::uint8_t*>(std::realloc(data, sizeof(raw_byte_slice) + length));
    if (data == nullptr)
      return nullptr;

    buf.release();
    buf.reset(data + sizeof(raw_byte_slice));
    return buf;
  }

  byte_buffer byte_buffer_increase(byte_buffer buf, const std::size_t current, const std::size_t more)
  {
    if (std::numeric_limits<std::size_t>::max() - current < more)
      throw std::range_error{"byte_buffer_increase size_t overflow"};
    return byte_buffer_resize(std::move(buf), current + more);
  }
} // epee
