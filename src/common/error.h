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

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <type_traits>

#define MONERO_DECLARE_ERROR_ENUM(name)         \
    namespace boost {                           \
        namespace system {                      \
            template<>                          \
            struct is_error_code_enum< name >   \
                : std::true_type                \
            {};                                 \
        }                                       \
    }

namespace monero
{
    namespace errc = boost::system::errc;
    using error_category = boost::system::error_category;
    using error_code = boost::system::error_code;
    using error_condition = boost::system::error_condition;
    using system_error = boost::system::system_error;

    enum class error : int
    {
        none = 0,
        invalid_argument,  //!< A function argument is invalid
        invalid_error_code //!< Default `monero::error_code` given to `expect<T>`
    };

    error_category const& default_category() noexcept;
    error_code make_error_code(monero::error value) noexcept;
}

MONERO_DECLARE_ERROR_ENUM(monero::error)
