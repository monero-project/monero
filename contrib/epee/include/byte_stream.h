// Copyright (c) 2020-2022, The Monero Project

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

#include <cassert>
#include <cstdint>
#include <cstring>

#include "byte_slice.h"
#include "span.h"

namespace epee
{
  /*! \brief A partial drop-in replacement for `std::ostream`.

      Only a few base `std::ostream` functions are implemented - enough for
      rapidjson output currently.

      Advantages over `std::stringstream` or `rapidjson::StringBuffer`:
        - The internal buffer can be taken without a copy.
        - The internal buffer can be given to `byte_slice` with zero
          allocations for reference count.
        - The internal buffer can be given to `zmq_msg_data_init` without a
          copy or extra allocation.
      an additional advantage over `std::stringstream`:
        - Construction is significantly faster - the global `std::locale`
          does not have to be acquired (global thread synchronization), and
          an extra allocation for `std::stringbuf` is not needed (which is an
          addition to the buffer inside of that object). */
  class byte_stream
  {
    byte_buffer buffer_;        //! Beginning of buffer
    std::uint8_t* next_write_;  //! Current write position
    const std::uint8_t* end_;   //! End of buffer

    //! \post `requested <= available()`
    void overflow(const std::size_t requested);

    //! Ensures that at least `requested` bytes are available.
    void check(const std::size_t requested)
    {
      const std::size_t remaining = available();
      if (remaining < requested)
        overflow(requested);
    }

  public:
    using char_type = std::uint8_t;
    using Ch = char_type;

    //! Increase internal buffer by at least `byte_stream_increase` bytes.
    byte_stream() noexcept
      : buffer_(nullptr),
        next_write_(nullptr),
        end_(nullptr)
    {}

    byte_stream(byte_stream&& rhs) noexcept;
    ~byte_stream() noexcept = default;
    byte_stream& operator=(byte_stream&& rhs) noexcept;

    const std::uint8_t* data() const noexcept { return buffer_.get(); }
    std::uint8_t* tellp() const noexcept { return next_write_; }
    std::size_t available() const noexcept { return end_ - next_write_; }
    std::size_t size() const noexcept { return next_write_ - buffer_.get(); }
    std::size_t capacity() const noexcept { return end_ - buffer_.get(); }

    //! Compatibility with rapidjson.
    void Flush() const noexcept
    {}

    /*! Reserve at least `more` bytes.
        \post `size() + more <= available()`.
        \throw std::range_error if exceeding max `size_t` value.
        \throw std::bad_alloc if allocation fails. */
    void reserve(const std::size_t more)
    {
      check(more);
    }

    //! Reset write position, but do not release internal memory. \post `size() == 0`.
    void clear() noexcept { next_write_ = buffer_.get(); }

    /*! Copy `length` bytes starting at `ptr` to end of stream.
        \throw std::range_error If exceeding max size_t value.
        \throw std::bad_alloc If allocation fails. */
    void write(const std::uint8_t* ptr, const std::size_t length)
    {
      check(length);
      std::memcpy(tellp(), ptr, length);
      next_write_ += length;
    }

    /*! Copy `length` bytes starting at `ptr` to end of stream.
        \throw std::range_error if exceeding max `size_t` value.
        \throw std::bad_alloc if allocation fails. */
    void write(const char* ptr, const std::size_t length)
    {
      write(reinterpret_cast<const std::uint8_t*>(ptr), length);
    }

    /*! Copy `source` to end of stream.
        \throw std::range_error if exceeding max `size_t` value.
        \throw std::bad_alloc if allocation fails. */
    void write(const epee::span<const std::uint8_t> source)
    {
      write(source.data(), source.size());
    }

    /*! Copy `source` to end of stream.
        \throw std::range_error if exceeding max `size_t` value.
        \throw std::bad_alloc if allocation fails. */
    void write(const epee::span<const char> source)
    {
      write(source.data(), source.size());
    }

    /*! Copy `ch` to end of stream.
        \throw std::range_error if exceeding max `size_t` value.
        \throw std::bad_alloc if allocation fails. */
    void put(const std::uint8_t ch)
    {
      check(1);
      put_unsafe(ch);
    }

    /*! Copy `ch` to end of stream. Provides rapidjson compatability.
        \throw std::range_error if exceeding max `size_t` value.
        \throw std::bad_alloc if allocation fails. */
    void Put(const std::uint8_t ch)
    {
      put(ch);
    }

    /*! Writes `ch` to end of stream without runtime capacity checks. Must use
        `reserve` before calling this function. Primarily for use with
        rapidjson, which writes characters at a time but reserves memory in
        blocks. Most applications want to use `put` or `write`. */
    void put_unsafe(const std::uint8_t ch) noexcept
    {
      assert(1 <= available());
      *(tellp()) = ch;
      ++next_write_;
    }

    /*! Write `ch` to end of stream `count` times.
        \throw std::range_error if exceeding max `size_t` value.
        \throw std::bad_alloc if allocation fails. */
    void put_n(const std::uint8_t ch, const std::size_t count)
    {
      check(count);
      std::memset(tellp(), ch, count);
      next_write_ += count;
    }

    /*! Copy `ch` to end of stream.
        \throw std::range_error if exceeding max `size_t` value.
        \throw std::bad_alloc if allocation fails. */
    void push_back(const std::uint8_t ch)
    {
      put(ch);
    }

    //! \return The internal buffer. \post `size() == capacity() == 0`.
    byte_buffer take_buffer() noexcept;
  };

  //! Compatability/optimization for rapidjson.

  inline void PutReserve(byte_stream& dest, const std::size_t length)
  {
    dest.reserve(length);
  }

  //! Compatability/optimization for rapidjson.

  inline void PutUnsafe(byte_stream& dest, const std::uint8_t ch)
  {
    dest.put_unsafe(ch);
  }

  //! Compability/optimization for rapidjson.
  inline void PutN(byte_stream& dest, const std::uint8_t ch, const std::size_t count)
  {
    dest.put_n(ch, count);
  }
} // epee

