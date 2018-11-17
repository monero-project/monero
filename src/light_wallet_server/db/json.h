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

#include <iosfwd>
#include <rapidjson/fwd.h>

#include "common/expect.h"
#include "light_wallet_server/db/fwd.h"

namespace lws
{
namespace db
{
namespace json
{
    struct status_
    {
        expect<void> operator()(std::ostream& dest, db::account_status src) const;
    };
    constexpr const status_ status{};

    struct account_address_
    {
        expect<void> operator()(rapidjson::Value const& src, db::account_address& dest) const;
        expect<void> operator()(std::ostream& dest, db::account_address const& src) const;
    };
    constexpr const account_address_ account_address{};

    struct account_
    {
        const bool show_sensitive;

        expect<void> operator()(std::ostream& dest, db::account const& src) const;
    };
    constexpr const account_ account{false};
    constexpr const account_ account_with_key{true};

    struct block_info_
    {
        expect<void> operator()(rapidjson::Value const& src, db::block_info& dest) const;
        expect<void> operator()(std::ostream& dest, db::block_info const& src) const;
    };
    constexpr const block_info_ block_info{};

    struct output_id_
    {
        expect<void> operator()(rapidjson::Value const& src, db::output_id& dest) const;
        expect<void> operator()(std::ostream& dest, db::output_id const& src) const;
    };
    constexpr const output_id_ output_id{};

    struct output_
    {
        expect<void> operator()(std::ostream& dest, db::output const& src) const;
    };
    constexpr const output_ output{};

    struct spend_
    {
        expect<void> operator()(rapidjson::Value const& src, db::spend& dest) const;
        expect<void> operator()(std::ostream& dest, db::spend const& src) const;
    };
    constexpr const spend_ spend{};

    struct key_image_
    {
        expect<void> operator()(rapidjson::Value const& src, db::key_image& dest) const;
        expect<void> operator()(std::ostream& dest, db::key_image const& src) const;
    };
    constexpr const key_image_ key_image{};

    struct request_info_
    {
        const bool show_sensitive;

        expect<void> operator()(std::ostream& dest, db::request_info const& src) const;
    };
    constexpr const request_info_ request_info{false};
    constexpr const request_info_ request_info_with_key{true};
} // json
} // db
} // lws
