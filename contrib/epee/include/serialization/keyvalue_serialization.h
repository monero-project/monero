// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#pragma once

#include <tuple>
#include <type_traits>
#include <utility>
#include "serialization/wire/field.h"
#include "serialization/wire/fwd.h"
#include "serialization/wire/wrappers.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "serialization"

namespace epee
{
  template<typename B, typename F, typename... T, std::size_t... I>
  inline void unpack_object2(const B is_read, F& format, std::tuple<T...>&& fields, std::index_sequence<I...>)
  {
    ::wire::object_fwd(is_read, format, std::move(std::get<I>(fields))...);
  }

  template<typename B, typename F, typename... T>
  inline void unpack_object(const B is_read, F& format, std::tuple<T...>&& fields)
  {
    ::epee::unpack_object2(is_read, format, std::move(fields), std::make_index_sequence<sizeof...(T)>{});
  }

  // Macro does not define "root" conversion function,
    
  /************************************************************************/
  /* Serialize map declarations                                           */
  /* Macro does not define "root" conversion functions `to_bytes` and     */
  /* `from_bytes` so that expensive template expansion can be done in a   */
  /* cpp file that uses macros from `wire/epee.h` and `wire/json.h`, etc. */
  /* This also reduces the number of includes needed in this header.      */
  /************************************************************************/
#define BEGIN_KV_SERIALIZE_MAP()                                        \
  template<class R>                                                     \
  void read_bytes(R& source)                                            \
  { ::epee::unpack_object(std::true_type{}, source, get_field_list(source, *this)); } \
                                                                        \
  template<class W>                                                     \
  void write_bytes(W& dest) const                                     \
  { ::epee::unpack_object(std::false_type{}, dest, get_field_list(dest, *this)); } \
                                                                        \
  template<class F, class T>                                            \
  static auto get_field_list(F& format, T& self)                        \
  { return std::tuple_cat(

#define KV_SERIALIZE_N(variable, val_name) \
  std::make_tuple(::wire::field(val_name, std::ref(self.variable))),

#define KV_SERIALIZE_PARENT(type) \
  type::template get_field_list(format, self),

#define KV_SERIALIZE_OPT_N(variable, val_name, default_value) \
  std::make_tuple(::wire::optional_field(val_name, ::wire::defaulted(std::ref(self.variable), default_value))),

#define KV_SERIALIZE_VAL_POD_AS_BLOB_N(variable, val_name) \
  std::make_tuple(::wire::field(val_name, ::wire::blob(std::ref(self.variable)))),

#define KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(variable, val_name) \
  std::make_tuple(::wire::optional_field(val_name, ::wire::array_as_blob(std::ref(self.variable)))),

#define KV_SERIALIZE_VAL_POD_AS_BLOB_OPT_N(varialble, val_name, default_value) \
  std;:make_tuple(::wire::optional_field(val_name, ::wire::defaulted(::wire::blob(std::ref(self.varialble)), default_value))

#define KV_SERIALIZE_ARRAY_N(variable, val_name, constraint) \
  std::make_tuple(::wire::optional_field(val_name, ::wire::array<constraint>(std::ref(self.variable)))),

#define END_KV_SERIALIZE_MAP() std::make_tuple());}

#define KV_SERIALIZE(varialble)                           KV_SERIALIZE_N(varialble, #varialble)
#define KV_SERIALIZE_VAL_POD_AS_BLOB(varialble)           KV_SERIALIZE_VAL_POD_AS_BLOB_N(varialble, #varialble)
#define KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(varialble)     KV_SERIALIZE_VAL_POD_AS_BLOB(varialble) //skip is_pod compile time check
#define KV_SERIALIZE_CONTAINER_POD_AS_BLOB(varialble)     KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(varialble, #varialble)
#define KV_SERIALIZE_OPT(variable,default_value)          KV_SERIALIZE_OPT_N(variable, #variable, default_value)
#define KV_SERIALIZE_ARRAY(variable, constraint)          KV_SERIALIZE_ARRAY_N(variable, #variable, constraint)

}




