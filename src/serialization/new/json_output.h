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

#include <ostream>
#include <type_traits>

#include "serialization/new/json_core.h"

namespace json
{
    namespace detail
    {
        // implementations in json_core.cpp
        expect<void> put_unsigned(std::ostream&, std::uint64_t);
        expect<void> put_real(std::ostream&, double, unsigned precision);
    }

    template<typename T>
    expect<void> unsigned_<T>::operator()(std::ostream& dest, T src) const
    {
        return detail::put_unsigned(dest, src);
    }

    template<typename T>
    template<typename U>
    expect<void> unsigned_<T>::operator()(std::ostream& dest, U src) const
    {
        static_assert(
            std::is_enum<U>(), "only enums allowed to this overload - wrong integer width?"
        );
        static_assert(
            std::is_same<T, typename std::underlying_type<U>::type>(), "enum has wrong width"
        );
        return (*this)(dest, T(src));
    }


    template<typename T>
    expect<void> real_<T>::operator()(std::ostream& dest, T src) const
    {
        return detail::put_real(dest, src, std::numeric_limits<T>::digits10);
    }


    template<typename F>
    expect<void> optional_<F>::operator()(std::ostream& dest, std::nullptr_t) const
    {
        dest << u8"null";
        return success();
    }

    template<typename F>
    template<typename T>
    expect<void> optional_<F>::operator()(std::ostream& dest, T const& src) const
    {
        if (bool(src))
            return fmt(dest, *src);
        return (*this)(dest, nullptr);
    }


    template<typename F>
    template<typename T>
    expect<void> array_<F>::operator()(std::ostream& dest, T const& src) const
    {
        dest << '[';
        bool first = true;
        for (auto const& elem : src)
        {
            if (!first)
                dest << ',';
            MONERO_CHECK(fmt(dest, elem));
            first = false;
        }
        dest << ']';
        return success();
    }


    template<typename K, typename F>
    template<typename T>
    expect<void> map_<K, F>::operator()(std::ostream& dest, T const& src) const
    {
        bool first = true;
        dest << '{';
        for (auto const& elem : src)
        {
            if (!first)
                dest << ',';

            auto name = convert(elem.first);
            if (!name)
                return name.error();

            MONERO_CHECK(json::string(dest, *name));
            dest << ':';
            MONERO_CHECK(fmt(dest, elem.second));

            first = false;
        }
        dest << '}';
        return success();
    }


    template<typename F, bool Required>
    template<typename T>
    expect<bool> field_t<F, Required>::write(std::ostream& dest, T const& src, bool first, std::true_type) const
    {
        if (!first)
            dest << ',';

        MONERO_CHECK(json::string(dest, name));
        dest << ':';
        MONERO_CHECK(fmt(dest, src));
        return false;
    }

    template<typename F, bool Required>
    template<typename T>
    expect<bool> field_t<F, Required>::write(std::ostream& dest, T const& src, bool first, std::false_type) const
    {
        if (bool(src))
            return write(dest, *src, first, std::true_type{});
        return true;
    }

    template<typename F, bool Required, typename... D>
    template<typename TH, typename... TT>
    expect<void> fields_<field_t<F, Required>, D...>::write(std::ostream& dest, bool first, TH const& src, TT const&... tail) const
    {
        const expect<bool> skipped =
            fmt.write(dest, src, first, std::integral_constant<bool, Required>{});
        if (!skipped)
            return skipped.error();

        MONERO_CHECK(fields_<D...>::write(dest, (*skipped && first), tail...));
        return success();
    }

    template<typename... D>
    template<typename... T>
    expect<void> object_<D...>::operator()(std::ostream& dest, T const&... src) const
    {
        static_assert(sizeof...(D) <= sizeof...(T), "more values than fields");
        static_assert(sizeof...(D) >= sizeof...(T), "more fields than values");
        dest << '{';
        MONERO_CHECK(fields_<D...>::write(dest, true, src...));
        dest << '}';
        return success();
    }
} // json

