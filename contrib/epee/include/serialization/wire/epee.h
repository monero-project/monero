// Copyright (c) 2023, The Monero Project
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

#include <cstdint>
#include "byte_stream.h"
#include "serialization/wire/epee/base.h"
#include "serialization/wire/epee/error.h"
#include "serialization/wire/epee/fwd.h"
#include "serialization/wire/epee/read.h"
#include "serialization/wire/epee/write.h"
#include "serialization/wire/read.h"
#include "serialization/wire/write.h"
#include "span.h"

//! Define functions that list fields in `type` (calls de-virtualized)
#define WIRE_EPEE_DEFINE_OBJECT(type, map)                              \
  void read_bytes(::wire::epee_reader& source, type& dest)              \
  { map(source, dest); }                                                \
                                                                        \
  void write_bytes(::wire::epee_writer& dest, const type& source)       \
  { map(dest, source); }

//! Define functions that convert `type` to/from epee binary bytes
#define WIRE_EPEE_DEFINE_CONVERSION(type)                               \
  std::error_code convert_from_epee(const ::epee::span<const std::uint8_t> source, type& dest) \
  { return ::wire_read::from_bytes<::wire::epee_reader>(source, dest); } \
                                                                        \
  std::error_code convert_to_epee(::epee::byte_stream& dest, const type& source) \
  { return ::wire_write::to_bytes<::wire::epee_writer>(dest, source); }

//! Define functions that convert `type::command` and `type::response` to/from epee binary bytes
#define WIRE_EPEE_DEFINE_COMMAND(type)		\
  WIRE_EPEE_DEFINE_CONVERSION(type::request)	\
  WIRE_EPEE_DEFINE_CONVERSION(type::response)
