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
#include "json.h"

#include <boost/optional/optional.hpp>

#include "light_wallet_server/db/data.h"
#include "light_wallet_server/db/string.h"
#include "serialization/new/json_input.h"
#include "serialization/new/json_output.h"

namespace
{
    template<typename S, typename T>
    expect<void> address_format(S&& stream, T& value)
    {
        static constexpr const auto fmt = json::object(
            json::field("spend_public", json::hex_string),
            json::field("view_public", json::hex_string)
        );
        return fmt(
            std::forward<S>(stream), value.spend_public, value.view_public
        );
    }

    template<typename S, typename T1, typename T2, typename T3>
    expect<void> account_format(S&& stream, T1& value, T2& key, T3& admin, T3& generated_locally)
    {
        static constexpr const auto fmt = json::object(
            json::field("id", json::uint32),
            json::field("address", lws::db::json::account_address),
            json::optional_field("view_key", json::hex_string),
            json::field("scan_height", json::uint64),
            json::field("start_height", json::uint64),
            json::field("access_time", json::uint32),
            json::field("creation_time", json::uint32),
            json::field("admin", json::boolean),
            json::field("generated_locally", json::boolean)
        );
        return fmt(
            std::forward<S>(stream), 
            value.id,
            value.address,
            key,
            value.scan_height,
            value.start_height,
            value.access,
            value.creation,
            admin,
            generated_locally
        );
    }

    template<typename S, typename T>
    expect<void> block_info_format(S&& stream, T& value)
    {
        static constexpr const auto fmt = json::object(
            json::field("height", json::uint64),
            json::field("hash", json::hex_string)
        );
        return fmt(std::forward<S>(stream), value.id, value.hash);
    }

    template<typename S, typename T>
    expect<void> output_id_format(S&& stream, T& value)
    {
        static constexpr const auto fmt = json::object(
            json::field("high", json::uint64),
            json::field("low", json::uint64)
        );
        return fmt(std::forward<S>(stream), value.high, value.low);
    }

    template<typename S, typename T1, typename T2>
    expect<void> spend_format(S&& stream, T1& value, T2& payment_id)
    {
        static constexpr const auto fmt = json::object(
            json::field("height", json::uint64),
            json::field("tx_hash", json::hex_string),
            json::field("key_image", json::hex_string),
            json::field("output_id", lws::db::json::output_id),
            json::field("timestamp", json::uint64),
            json::field("unlock_time", json::uint64),
            json::field("mixin_count", json::uint32),
            json::optional_field("payment_id", json::hex_string)
        );
        return fmt(
            std::forward<S>(stream),
            value.link.height,
            value.link.tx_hash,
            value.image,
            value.source,
            value.timestamp,
            value.unlock_time,
            value.mixin_count,
            payment_id
        );
    }

    template<typename S, typename T>
    expect<void> image_format(S&& stream, T& value)
    {
        static constexpr const auto fmt = json::object(
            json::field("key_image", json::hex_string),
            json::field("tx_hash", json::hex_string),
            json::field("height", json::uint64)
        );
        return fmt(
            std::forward<S>(stream), value.value, value.link.tx_hash, value.link.height
        );
    }

    template<typename S, typename T1, typename T2, typename T3>
    expect<void> request_format(S&& stream, T1& value, T2& key, T3& generated_locally)
    {
        static constexpr const auto fmt = json::object(
            json::field("address", lws::db::json::account_address),
            json::optional_field("view_key", json::hex_string),
            json::field("start_height", json::uint64),
            json::field("generated_locally", json::boolean)
        );
        return fmt(
            std::forward<S>(stream),
            value.address, key, value.start_height, generated_locally
        );
    }
} // anonymous

namespace lws
{
namespace db
{
namespace json
{
    expect<void> status_::operator()(std::ostream& dest, db::account_status src) const
    {
        char const* const value = db::status_string(src);
        MONERO_PRECOND(value != nullptr);
        return ::json::string(dest, value);
    }

    expect<void> account_address_::operator()(rapidjson::Value const& src, db::account_address& dest) const
    {
        return address_format(src, dest);
    }
    expect<void> account_address_::operator()(std::ostream& dest, db::account_address const& src) const
    {
        return address_format(dest, src);
    }

    expect<void> account_::operator()(std::ostream& dest, db::account const& src) const
    {

        db::view_key const* const key =
            show_sensitive ? std::addressof(src.key) : nullptr;
        const bool admin = (src.flags & lws::db::admin_account);
        const bool generated = (src.flags & lws::db::account_generated_locally);
        return account_format(dest, src, key, admin, generated);
    }

    expect<void> block_info_::operator()(rapidjson::Value const& src, db::block_info& dest) const
    {
        return block_info_format(src, dest);
    }
    expect<void> block_info_::operator()(std::ostream& dest, db::block_info const& src) const
    {
        return block_info_format(dest, src);
    }

    expect<void> output_id_::operator()(rapidjson::Value const& src, db::output_id& dest) const
    {
        return output_id_format(src, dest);
    }
    expect<void> output_id_::operator()(std::ostream& dest, db::output_id const& src) const
    {
        return output_id_format(dest, src);
    }

    expect<void> output_::operator()(std::ostream& dest, db::output const& src) const
    {
        static constexpr const auto fmt = ::json::object(
            ::json::field("id", json::output_id),
            ::json::field("block", ::json::uint64),
            ::json::field("index", ::json::uint32),
            ::json::field("amount", ::json::uint64),
            ::json::field("timestamp", ::json::uint64),
            ::json::field("tx_hash", ::json::hex_string),
            ::json::field("tx_prefix_hash", ::json::hex_string),
            ::json::field("tx_public", ::json::hex_string),
            ::json::optional_field("rct_mask", ::json::hex_string),
            ::json::optional_field("payment_id", ::json::hex_string),
            ::json::field("unlock_time", ::json::uint64),
            ::json::field("mixin_count", ::json::uint32),
            ::json::field("coinbase", ::json::boolean)
        );

        const std::pair<db::extra, std::uint8_t> unpacked =
            db::unpack(src.extra);

        const bool coinbase = (unpacked.first & lws::db::coinbase_output);
        const bool rct = (unpacked.first & lws::db::ringct_output);

        const auto rct_mask = rct ? std::addressof(src.ringct_mask) : nullptr;

        epee::span<const std::uint8_t> payment_bytes{};
        if (unpacked.second == 32)
            payment_bytes = epee::as_byte_span(src.payment_id.long_);
        else if (unpacked.second == 8)
            payment_bytes = epee::as_byte_span(src.payment_id.short_);

        const auto payment_id = payment_bytes.empty() ?
            nullptr : std::addressof(payment_bytes);

        return fmt(
            dest,
            src.spend_meta.id,
            src.link.height,
            src.spend_meta.index,
            src.spend_meta.amount,
            src.timestamp,
            src.link.tx_hash,
            src.tx_prefix_hash,
            src.spend_meta.tx_public,
            rct_mask,
            payment_id,
            src.unlock_time,
            src.spend_meta.mixin_count,
            coinbase
        );
    }

    expect<void> spend_::operator()(rapidjson::Value const& src, db::spend& dest) const
    {
        boost::optional<crypto::hash> payment_id;
        MONERO_CHECK(spend_format(src, dest, payment_id));

        if (payment_id)
        {
            dest.length = sizeof(dest.payment_id);
            dest.payment_id = std::move(*payment_id);
        }
        else
            dest.length = 0;

        return success();
    }
    expect<void> spend_::operator()(std::ostream& dest, db::spend const& src) const
    {
        crypto::hash const* const payment_id =
            (src.length == sizeof(src.payment_id) ? std::addressof(src.payment_id) : nullptr);

        return spend_format(dest, src, payment_id);
    }

    expect<void> key_image_::operator()(rapidjson::Value const& src, db::key_image& dest) const
    {
        return image_format(src, dest);
    }
    expect<void> key_image_::operator()(std::ostream& dest, db::key_image const& src) const
    {
        return image_format(dest, src);
    }

    expect<void> request_info_::operator()(std::ostream& dest, db::request_info const& src) const
    {
        db::view_key const* const key =
            show_sensitive ? std::addressof(src.key) : nullptr;
        const bool generated = (src.creation_flags & lws::db::account_generated_locally);
        return request_format(dest, src, key, generated);
    }
} // json
} // db
} // lws
