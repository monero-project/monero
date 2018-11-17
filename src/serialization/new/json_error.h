// Copyright (c) 2018, The Monero Project
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

#include <system_error>
#include <type_traits>

namespace json
{
    enum class error : int
    {
        // 0 is reserved for no error, as per expect<T>
        buffer_overflow = 1,//!< Fixed-sized buffer for storing data is too small
        expected_array,     //!< Expected a JSON array
        expected_bool,      //!< Expected a boolean value
        expected_double,    //!< Expected a double value
        expected_float,     //!< Expected a float value
        expected_object,    //!< Expected a JOSN object
        expected_string,    //!< Expected a string value
        expected_unsigned,  //!< Expected an unsigned integer value
        invalid_hex,        //!< Invalid hex-string
        missing_field,      //!< Missing field in JSON object
        overflow,           //!< Conversion to fixed-width integer overflowed
        parse_failure,      //!< Failed to parse JSON
        unexpected_field,   //!< An extra, unexpected, field is present
        underflow           //!< Conversion to fixed-width integer underflowed
    };

    std::error_category const& error_category() noexcept;

    inline std::error_code make_error_code(json::error value) noexcept
    {
        return std::error_code{int(value), error_category()};
    }
}

namespace std
{
    template<>
    struct is_error_code_enum<::json::error>
      : true_type
    {};
}
