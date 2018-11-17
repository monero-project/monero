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

#include <limits>
#include <rapidjson/document.h>

#include "serialization/new/json_core.h"
#include "serialization/new/json_error.h"

namespace json
{
    namespace detail
    {
        template<typename T>
        using limit = std::numeric_limits<T>;

        // definition in json_core.cpp
        expect<std::uint64_t> get_unsigned(rapidjson::Value const& src, std::uint64_t max) noexcept;

        template<typename U, typename T>
        constexpr U get_max() noexcept
        {
            // Don't use `std::max`, use promotion rules with comparison
            return limit<T>::max() <= limit<U>::max() ? limit<T>::max() : limit<U>::max();
        }

        // definitions in json_core.cpp
        expect<void> get_real(rapidjson::Value const& src, float& dest) noexcept;
        expect<void> get_real(rapidjson::Value const& src, double& dest) noexcept;
    }

    template<typename T>
    expect<void> unsigned_<T>::operator()(rapidjson::Value const& src, T& dest) const noexcept
    {
        const expect<std::uint64_t> result =
            detail::get_unsigned(src, detail::get_max<std::uint64_t, T>());
        if (!result)
            return result.error();
        dest = T(*result);
        return success();
    }

    template<typename T>
    template<typename U>
    expect<void> unsigned_<T>::operator()(rapidjson::Value const& src, U& dest) const noexcept
    {
        using integer_type = typename std::underlying_type<U>::type;
        static_assert(
            std::is_enum<U>(), "only enums allowed to this overload - wrong integer width?"
        );
        static_assert(std::is_same<T, integer_type>(), "enum has wrong width");
        integer_type temp;
        MONERO_CHECK((*this)(src, temp));
        dest = U(temp);
        return success();
    }


    template<typename T>
    expect<void> real_<T>::operator()(rapidjson::Value const& src, T& dest) const noexcept
    {
        return detail::get_real(src, dest);
    }


    template<typename F>
    expect<void> optional_<F>::operator()(rapidjson::Value const& src, rapidjson::Value& dest) const
    {
        if (src.IsNull())
        {
            dest = rapidjson::Value{};
            return success();
        }
        return fmt(src, dest);
    }

    template<typename F>
    template<typename T>
    expect<void> optional_<F>::operator()(rapidjson::Value const& src, T& dest) const
    {
        if (src.IsNull())
        {
            dest = T{};
        }
        else
        {
            typename T::value_type inner{};
            BOOST_CHECK(fmt(src, inner));
            dest = std::move(inner);
        }
        return success();
    }


    template<typename F>
    template<typename T>
    expect<void> array_<F>::operator()(rapidjson::Value const& src, T& dest) const
    {
        if (!src.IsArray())
            return {json::error::expected_array};
        if (dest.max_size() < src.Size())
            return {json::error::buffer_overflow};

        dest.clear();
        dest.reserve(src.Size());

        for (auto const& elem : src.GetArray())
        {
            typename T::value_type inner{};
            fmt(elem, inner);
            dest.push_back(std::move(inner));
        }
        return success();
    }


    template<typename F, bool  Required>
    template<typename T>
    expect<void> field_t<F, Required>::do_read(rapidjson::Value const& src, T& dest, std::true_type) const
    {
        const auto elem = src.FindMember(name);
        if (elem == src.MemberEnd())
            return {json::error::missing_field};
        return fmt(elem->value, dest);
    }

    template<typename F, bool Required>
    template<typename T>
    expect<void> field_t<F, Required>::do_read(rapidjson::Value const& src, T& dest, std::false_type) const
    {
        const auto elem = src.FindMember(name);
        if (elem == src.MemberEnd())
        {
            dest = T{};
            return success();
        }
        typename T::value_type inner{};
        MONERO_CHECK(fmt(elem->value, inner));
        dest = std::move(inner);
        return success();
    }

    template<typename F, bool Required>
    template<typename T>
    expect<void> field_t<F, Required>::read(rapidjson::Value const& src, T& dest) const
    {
        return do_read(src, dest, std::integral_constant<bool, Required>{});
    }

    template<typename F, bool Required, typename... D>
    template<typename TH, typename... TT>
    expect<void> fields_<field_t<F, Required>, D...>::read(rapidjson::Value const& src, TH& dest, TT&... tail) const
    {
        MONERO_CHECK(fmt.read(src, dest));
        return fields_<D...>::read(src, tail...);
    }

    template<typename... D>
    template<typename... T>
    expect<void> object_<D...>::operator()(rapidjson::Value const& src, T&... dest) const
    {
        static_assert(sizeof...(D) <= sizeof...(T), "additional fields");
        static_assert(sizeof...(D) >= sizeof...(T), "missing fields");
        if (!src.IsObject())
            return {json::error::expected_object};
        if (is_strict && sizeof...(T) < src.MemberCount())
            return {json::error::unexpected_field};
        return fields_<D...>::read(src, dest...);
    }
} // json

