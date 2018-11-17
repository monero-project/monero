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

#include <boost/utility/string_ref.hpp>
#include <string>

#include "common/expect.h"
#include "light_wallet_server/db/fwd.h"

namespace lws
{
namespace  db
{
    //! Callable for converting `lws::db::account_status` to/from string.
    struct status_string_
    {
        //! \return `status` as static string or `nullptr` invalid value.
        const char* operator()(account_status status) const noexcept;
        //! \return `status` as enum.
        expect<account_status> operator()(boost::string_ref status) const noexcept;
    };
    constexpr const status_string_ status_string{};

    //! Callable for converting `lws::db::request` to/from string.
    struct request_string_
    {
        //! \return `req` as static string or `nullptr` on invalid value.
        const char* operator()(request req) const noexcept;
        //! \return `req` as enum.
        expect<request> operator()(boost::string_ref req) const noexcept;
    };
    constexpr const request_string_ request_string{};

    //! Callable for converting `account_address` to/from monero base58 public address.
    struct address_string_
    {
        /*!
            \return `address` as a monero base58 public address, using
                `lws::config::network` for the tag.
        */
        std::string operator()(account_address const& address) const;
        /*!
            \return `address`, as base58 public address, using
                `lws::config::network` for the tag.
        */
        expect<account_address> operator()(boost::string_ref address) const noexcept;
    };
    constexpr const address_string_ address_string{};
} // db
} // lws
