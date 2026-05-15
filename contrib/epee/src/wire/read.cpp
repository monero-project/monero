// Copyright (c) 2021, The Monero Project
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

#include "serialization/wire/read.h"

#include <stdexcept>

void wire::reader::increment_depth()
{
  if (++depth_ == max_read_depth())
    WIRE_DLOG_THROW_(error::schema::maximum_depth);
}

void wire::reader::decrement_depth()
{
  if (!depth_)
    throw std::logic_error{"reader::decrement_depth() already at zero"};
  --depth_;
}

std::size_t wire::reader::start_array(std::size_t min_element_size)
{
  increment_depth();
  return do_start_array(min_element_size);
}

std::size_t wire::reader::start_object()
{
  increment_depth();
  return do_start_object();
}

[[noreturn]] void wire::integer::throw_exception(const std::intmax_t source, const std::intmax_t min, const std::intmax_t max)
{
  static_assert(0 <= std::numeric_limits<std::intmax_t>::max(), "expected 0 <= intmax_t::max");
  static_assert(
    std::numeric_limits<std::intmax_t>::max() <= std::numeric_limits<std::uintmax_t>::max(),
    "expected intmax_t::max <= uintmax_t::max"
  );
  if (source < 0)
    WIRE_DLOG_THROW(error::schema::larger_integer, source << " given when " << min << " is minimum permitted");
  else
    throw_exception(std::uintmax_t(source), std::uintmax_t(max));
}
[[noreturn]] void wire::integer::throw_exception(const std::uintmax_t source, const std::uintmax_t max)
{
  WIRE_DLOG_THROW(error::schema::smaller_integer, source << " given when " << max << " is maximum permitted");
}

[[noreturn]] void wire_read::throw_exception(const wire::error::schema code, const char* display, epee::span<char const* const> names)
{
  const char* name = nullptr;
  for (const char* elem : names)
  {
    if (elem != nullptr)
    {
      name = elem;
      break;
    }
  }
  WIRE_DLOG_THROW(code, display << (name ? name : ""));
}


