// Copyright (c) 2021-2024, The Monero Project
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

#include "serialization/wire/fwd.h"
#include "serialization/wire/read.h"
#include "serialization/wire/write.h"

//! Define functions that list fields in `type` (using virtual interface)
#define WIRE_DEFINE_OBJECT(type, map)                          \
  void read_bytes(::wire::reader& source, type& dest)          \
  { map(source, dest); }                                       \
                                                               \
  void write_bytes(::wire::writer& dest, const type& source)   \
  { map(dest, source); }

//! Define `from_bytes` and `to_bytes` for `this`.
#define WIRE_DEFINE_CONVERSIONS()                                       \
  template<typename R, typename T>                                      \
  std::error_code from_bytes(T&& source)                                \
  { return ::wire_read::from_bytes<R>(std::forward<T>(source), *this); } \
                                                                        \
  template<typename W, typename T>                                      \
  std::error_code to_bytes(T& dest) const                               \
  { return ::wire_write::to_bytes<W>(dest, *this); }

