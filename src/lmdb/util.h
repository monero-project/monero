// Copyright (c) 2018-2024, The Monero Project

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

#include <cstddef>
#include <cstring>
#include <lmdb.h>
#include <type_traits>
#include <utility>

#include "span.h"

/*! Calculates types and offset of struct field. Use in template arguments for
    `table::get_value`, `value_iterator::get_value`,
    `value_stream::make_iterator`, or `value_stream::make_range`. */
#define MONERO_FIELD(obj, field)                                     \
    obj , decltype(std::declval<obj>().field) , offsetof(obj, field)

//! Expands to `lmdb::less` for the value `field` within `obj`.
#define MONERO_SORT_BY(obj, field)                              \
    &::lmdb::less<                                              \
        lmdb::native_type<decltype(std::declval<obj>().field)>, \
        offsetof(obj, field)                                    \
    >

//! Expands to `lmdb::compare` for the value `field` within `obj`.
#define MONERO_COMPARE(obj, field)           \
    &::lmdb::compare<                        \
        decltype(std::declval<obj>().field), \
        offsetof(obj, field)                 \
    >

namespace lmdb
{
    //! Prevent instantiation of `std::underlying_type<T>` when `T` is not enum.
    template<typename T>
    struct identity
    {
        using type = T;
    };

    /*!
        Get the native type for enums, or return `T` unchanged. Useful for
        merging generated machine code for templated functions that use enums
        with identical size-widths without relying on aggressive identical
        comdat folding (ICF) support in linker. So with enum defintion
        `enum class enum_foo : unsigned long {};` will always yield
        `assert(&func_foo<unsigned long> == &func_foo<native_type<enum_foo>>)`.
    */
    template<typename T>
    using native_type = typename std::conditional<
        std::is_enum<T>::value, std::underlying_type<T>, identity<T>
    >::type::type;

    //! \return `value` as its native type.
    template<typename T, typename U = typename std::underlying_type<T>::type>
    inline constexpr U to_native(T value) noexcept
    {
        return U(value);
    }

    //! \return `value` bytes in a LMDB `MDB_val` object.
    template<typename T>
    inline MDB_val to_val(T&& value) noexcept
    {
        // lmdb does not touch user data, so const_cast is acceptable
        static_assert(!std::is_rvalue_reference<T&&>(), "cannot use temporary value");
        void const* const temp = reinterpret_cast<void const*>(std::addressof(value));
        return MDB_val{sizeof(value), const_cast<void*>(temp)};
    }

    //! \return A span over the same chunk of memory as `value`.
    inline constexpr epee::span<const std::uint8_t> to_byte_span(MDB_val value) noexcept
    {
        return {static_cast<const std::uint8_t*>(value.mv_data), value.mv_size};
    }

    /*!
        A LMDB comparison function that uses `operator<`.

        \tparam T has a defined `operator<` .
        \tparam offset to `T` within the value.

        \return -1 if `left < right`, 1 if `right < left`, and 0 otherwise.
    */
    template<typename T, std::size_t offset = 0>
    inline int less(MDB_val const* left, MDB_val const* right) noexcept
    {
        if (!left || !right || left->mv_size < sizeof(T) + offset || right->mv_size < sizeof(T) + offset)
        {
            assert("invalid use of custom comparison" == 0);
            return -1;
        }

        T left_val;
        T right_val;
        std::memcpy(std::addressof(left_val), static_cast<char*>(left->mv_data) + offset, sizeof(T));
        std::memcpy(std::addressof(right_val), static_cast<char*>(right->mv_data) + offset, sizeof(T));
        return left_val < right_val ? -1 : bool(right_val < left_val);
    }

    /*!
        A LMDB comparison function that uses `std::memcmp`.

        \toaram T is `!epee::has_padding`
        \tparam offset to `T` within the value.

        \return The result of `std::memcmp` over the value.
    */
    template<typename T, std::size_t offset = 0>
    inline int compare(MDB_val const* left, MDB_val const* right) noexcept
    {
        static_assert(!epee::has_padding<T>(), "memcmp will not work");
        if (!left || !right || left->mv_size < sizeof(T) + offset || right->mv_size < sizeof(T) + offset)
        {
            assert("invalid use of custom comparison" == 0);
            return -1;
        }
        return std::memcmp(
            static_cast<char*>(left->mv_data) + offset,
            static_cast<char*>(right->mv_data) + offset,
            sizeof(T)
        );
    }
} // lmdb
