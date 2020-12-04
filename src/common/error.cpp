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
#include "error.h"

#include <string>

namespace
{
    struct category final : monero::error_category
    {
        category() noexcept
          : monero::error_category()
        {}

        virtual const char* name() const noexcept override final
        {
            return "common_category()";
        }

        virtual std::string message(int value) const override final
        {
            switch (monero::error(value))
            {
                case monero::error::none:
                    return "No error (success)";
                case monero::error::invalid_argument:
                    return make_error_code(monero::errc::invalid_argument).message();
                case monero::error::invalid_error_code:
                    return "expect<T> was given an error value of zero";
                default:
                    break;
            }
            return "Unknown basic_category() value";
        }

        virtual monero::error_condition default_error_condition(int value) const noexcept override final
        {
            // maps specific errors to generic `std::errc` cases.
            switch (monero::error(value))
            {
                case monero::error::none:
                    return monero::errc::success;
                case monero::error::invalid_argument:
                case monero::error::invalid_error_code:
                    return monero::errc::invalid_argument;
                default:
                    break;
            }
            return monero::error_condition{value, *this};
        }
    };
    //! function in anonymous namespace allows compiler to optimize `error_category::failed` call
    category const& category_instance() noexcept
    {
        static const category instance;
        return instance;
    }
}

namespace monero
{
    error_category const& default_category() noexcept
    {
        return category_instance();
    }
    error_code make_error_code(monero::error value) noexcept
    {
        return {int(value), category_instance()};
    }
}
