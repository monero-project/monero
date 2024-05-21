// Copyright (c) 2022-2024, The Monero Project
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

#include <exception>
#include <system_error>
#include <type_traits>

#include "misc_log_ex.h"

//! Print default `code` message followed by optional message to debug log then throw `code`.
#define WIRE_DLOG_THROW_(code, ...)                                     \
  do                                                                    \
  {                                                                     \
    MDEBUG( get_string(code) __VA_ARGS__ );                             \
    throw ::wire::exception_t<decltype(code)>{code};                    \
  }                                                                     \
  while (0)

//! Print default `code` message followed by `msg` to debug log then throw `code`.
#define WIRE_DLOG_THROW(code, msg)                      \
  WIRE_DLOG_THROW_(code, << ": " << msg)

namespace wire
{
  namespace error
  {
    enum class schema : int
    {
      none = 0,         //!< Must be zero for `expect<..>`
      array,            //!< Expected an array value
      array_max_element,//!< Exceeded max array count
      array_min_size,   //!< Below min element wire size
      binary,           //!< Expected a binary value of variable length
      boolean,          //!< Expected a boolean value
      enumeration,      //!< Expected a value from a specific set
      fixed_binary,     //!< Expected a binary value of fixed length
      integer,          //!< Expected an integer value
      invalid_key,      //!< Key for object is invalid
      larger_integer,   //!< Expected a larger integer value
      maximum_depth,    //!< Hit maximum number of object+array tracking
      missing_key,      //!< Missing required key for object
      number,           //!< Expected a number (integer or float) value
      object,           //!< Expected object value
      smaller_integer,  //!< Expected a smaller integer value
      string,           //!< Expected string value
    };

    //! \return Error message string.
    const char* get_string(schema value) noexcept;

    //! \return Category for `schema_error`.
    const std::error_category& schema_category() noexcept;

    //! \return Error code with `value` and `schema_category()`.
    inline std::error_code make_error_code(const schema value) noexcept
    {
      return std::error_code{int(value), schema_category()};
    }
  } // error

  //! `std::exception` doesn't require dynamic memory like `std::runtime_error`
  struct exception : std::exception
  {
    exception() noexcept
      : std::exception()
    {}

    exception(const exception&) = default;
    exception& operator=(const exception&) = default;
    virtual ~exception() noexcept
    {}

    virtual std::error_code code() const noexcept = 0;
  };

  template<typename T>
  class exception_t final : public wire::exception
  {
    static_assert(std::is_enum<T>(), "only enumerated types allowed");
    T value;

  public:
    exception_t(T value) noexcept
      : value(value)
    {}

    exception_t(const exception_t&) = default;
    ~exception_t() = default;
    exception_t& operator=(const exception_t&) = default;

    const char* what() const noexcept override final
    {
      static_assert(noexcept(noexcept(get_string(value))), "get_string function must be noexcept");
      return get_string(value);
    }

    std::error_code code() const noexcept override final
    {
      static_assert(noexcept(noexcept(make_error_code(value))), "make_error_code funcion must be noexcept");
      return make_error_code(value);
    }
  };
} // wire

namespace std
{
  template<>
  struct is_error_code_enum<wire::error::schema>
    : true_type
  {};
}
