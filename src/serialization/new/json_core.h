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
#include <cfloat>
#include <cstdint>
#include <iosfwd>
#include <rapidjson/fwd.h>
#include <string>
#include <type_traits>
#include <utility>

#include "common/expect.h"
#include "span.h"

namespace json
{
    //! Converts enum to C-string representation in `json::map`.
    template<typename C>
    struct from_enum_
    {
        C converter;

        template<typename T>
        expect<const char*> operator()(T value) const noexcept
        {
            char const* const str = converter(value);
            MONERO_PRECOND(str != nullptr);
            return str;
        }
    };

    /*!
        \tparam C Callable with the signature `const char*(T)`.
        \param converter Function for enum -> const char* mapping.
        \return A function that can be used in `json::map`.
    */
    template<typename C>
    inline constexpr from_enum_<C> from_enum(C converter) noexcept
    {
        return {static_cast<C&&>(converter)};
    }


    // Formatter for boolean types.
    struct boolean_
    {
        expect<void> operator()(rapidjson::Value const&, bool&) const;
        expect<void> operator()(std::ostream&, bool) const;
    };
    constexpr const boolean_ boolean {};


    //! Formatter for unsigned integer types.
    template<typename T>
    struct unsigned_
    {
        static_assert(std::is_integral<T>(), "only integer type allowed");
        static_assert(!std::is_signed<T>(), "signed types not allowed");

        expect<void> operator()(rapidjson::Value const& src, T& dest) const noexcept;

        template<typename U>
        expect<void> operator()(rapidjson::Value const& src, U& dest) const noexcept;

        expect<void> operator()(std::ostream& dest, T src) const;

        template<typename U>
        expect<void> operator()(std::ostream& dest, U src) const;
    };
    constexpr const unsigned_<std::uint16_t> uint16 {};
    constexpr const unsigned_<std::uint32_t> uint32 {};
    constexpr const unsigned_<std::uint64_t> uint64 {};

    template<typename T>
    struct real_
    {
        static_assert(std::is_floating_point<T>(), "only float types allowed");
        static_assert(
            FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128,
            "float is not 32-bit compatible"
        );
        static_assert(
            DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024,
            "double is not 64-bit compatible"
        );

        expect<void> operator()(rapidjson::Value const& src, T& dest) const noexcept;
        expect<void> operator()(std::ostream& dest, T src) const;
    };
    constexpr const real_<float> real32 {};
    constexpr const real_<double> real64 {};


    //! Formatter for string types.
    struct string_
    {
        //! Copy a string of any size from `src` to `dest`.
        expect<void> operator()(rapidjson::Value const& src, std::string& dest) const;

        //! Copy a string of exactly `dest.size()` from `src` to `dest`.
        expect<void> operator()(rapidjson::Value const& src, epee::span<char> dest) const;

        //! Store a non-owning pointer to the string owned by `src`.
        expect<void> operator()(rapidjson::Value const& src, boost::string_ref&) const;

        expect<void> operator()(std::ostream&, boost::string_ref src) const;
    };
    constexpr const string_ string {};


    //! Formatter that converts POD types to/from hex strings.
    struct hex_string_
    {
        expect<void> operator()(rapidjson::Value const& src, epee::span<std::uint8_t> dest) const;

        template<typename T>
        expect<void> operator()(rapidjson::Value const& src, T& dest) const
        {
            return (*this)(src, epee::as_mut_byte_span(dest));
        }

        expect<void> operator()(std::ostream& dest, epee::span<const std::uint8_t> src) const;

        template<typename T>
        expect<void> operator()(std::ostream& dest, T const& src) const
        {
            return (*this)(dest, epee::as_byte_span(src));
        }
    };
    constexpr const hex_string_ hex_string {};


    /*!
        \brief Formatter for optional values (i.e. possibly null).

        For reading, the value provided must have a default constructor
        that sets the value to "null"/"nil", an inner `typedef`
        named `value_type` that specifies the "real" type being stored,
        and an `operator=` from that inner `value_type`. Lastly,
        `value_type` must be default constructible, and compatible with the
        format `F`.

        For writing, the value provided must be pointer-like; conversion to
        bool must indicate whether a value is held, and `operator*` must
        return the real value. The real value must be compatible with the
        format `F`. `nullptr` can be used to explicitly write "null" to the
        stream.
    */
    template<typename F>
    struct optional_
    {
        const F fmt;

        expect<void> operator()(rapidjson::Value const& src, rapidjson::Value& dest) const;

        //!
        template<typename T>
        expect<void> operator()(rapidjson::Value const& src, T& dest) const;

        //! Write "null" to the output stream.
        expect<void> operator()(std::ostream& dest, std::nullptr_t) const;

        template<typename T>
        expect<void> operator()(std::ostream& dest, T const& src) const;
    };

    //! \return A formatter function that can handle optional json values.
    template<typename F>
    inline constexpr optional_<F> optional(F fmt) noexcept
    {
        return {static_cast<F&&>(fmt)};
    }


    /*!
        \brief Formatter for array types.

        For reading, the value provided must have the following functions:
        `max_size()`, `clear()`, `reserve(std::size_t)`, and `push_back(U)`
        where `U` is provided via the inner `typedef` named `value_type`.
        The inner type (`value_type`/`U`) must also be compatible with the
        inner format `F`. The previously mentioned functions should behave
        similar to `std::vector`. Allocations can be avoided by using
        `boost::container::static_vector` which meets all of these
        requirements, but is more likely to error on the `max_size()` call.

        For writing, the value provided must be compatible with
        `std::begin` and `std::end`, and the inner element type must be
        compatible with the inner format `F`.
    */
    template<typename F>
    struct array_
    {
        const F fmt;

        template<typename T>
        expect<void> operator()(rapidjson::Value const& src, T& dest) const;

        template<typename T>
        expect<void> operator()(std::ostream& dest, T const& src) const;
    };

    //! \return A formatter function that can handle array json values.
    template<typename F>
    inline constexpr array_<F> array(F fmt) noexcept
    {
        return {static_cast<F&&>(fmt)};
    }

    template<typename K, typename F>
    struct map_
    {
        K convert;
        F fmt;

        template<typename T>
        expect<void> operator()(std::ostream& dest, T const& src) const;
    };

    template<typename K, typename F>
    inline constexpr map_<K, F> map(K convert, F fmt) noexcept
    {
        return {static_cast<K&&>(convert), static_cast<F&&>(fmt)};
    }


    //! Reads/writes fields. Does not conform to formatter concept, only usable in an object.
    template<typename F, bool Required>
    struct field_t
    {
        char const* const name;
        const F fmt;

        template<typename T>
        expect<void> do_read(rapidjson::Value const& src, T& dest, std::true_type) const;

        template<typename T>
        expect<void> do_read(rapidjson::Value const& src, T& dest, std::false_type) const;

        template<typename T>
        expect<void> read(rapidjson::Value const& src, T& dest) const;

        template<typename T>
        expect<bool> write(std::ostream& dest, T const& src, bool first, std::true_type) const;

        template<typename T>
        expect<bool> write(std::ostream& dest, T const& src, bool first, std::false_type) const;
    };

    template<typename F>
    using field_ = field_t<F, true>;

    template<typename F>
    using optional_field_ = field_t<F, false>;


    template<typename F>
    inline constexpr field_<F> field(const char* name, F fmt) noexcept
    {
        return {name, static_cast<F&&>(fmt)};
    }

    template<typename F>
    inline constexpr optional_field_<F> optional_field(const char* name, F fmt) noexcept
    {
        return {name, static_cast<F&&>(fmt)};
    }


    //! Type that handles 0 fields (1+ case below).
    template<typename... D>
    struct fields_
    {
        static_assert(sizeof...(D) == 0, "this version is meant for zero field case");
        static expect<void> read(rapidjson::Value const&)
        {
            return success();
        }

        static expect<void> write(std::ostream const&, bool)
        {
            return success();
        }
    };

    //! Type that handles 1+ fields (zero-case above).
    template<typename F, bool Required, typename... D>
    struct fields_<field_t<F, Required>, D...> : fields_<D...>
    {
        const field_t<F, Required> fmt;

        constexpr explicit fields_(field_t<F, Required> head, D... tail) noexcept
          : fields_<D...>(static_cast<D&&>(tail)...), fmt(static_cast<field_t<F, Required>&&>(head))
        {}

        template<typename TH, typename... TT>
        expect<void> read(rapidjson::Value const& src, TH& dest, TT&... tail) const;

        template<typename TH, typename... TT>
        expect<void> write(std::ostream& dest, bool first, TH const& src, TT const&... tail) const;
    };


    //! Formatter for a json object.
    template<typename... D>
    struct object_ : fields_<D...>
    {
        bool is_strict;

        constexpr object_(bool is_strict, D... fields) noexcept
          : fields_<D...>(static_cast<D&&>(fields)...), is_strict(is_strict)
        {}

        template<typename... T>
        expect<void> operator()(rapidjson::Value const& src, T&... dest) const;

        template<typename... T>
        expect<void> operator()(std::ostream& dest, T const&... src) const;
    };

    //! A formatter function that can handle specific json objects. Allows extra fields
    template<typename... F, bool... Required>
    inline constexpr
    object_<field_t<F, Required>...> object(field_t<F, Required>... fields) noexcept
    {
        return {false, static_cast<field_t<F, Required>&&>(fields)...};
    }

    //! A formatter function that can handle specific json objects. Does not allow extra fields
    template<typename... F, bool... Required>
    inline constexpr
    object_<field_t<F, Required>...> object_strict(field_t<F, Required>... fields) noexcept
    {
        return {true, static_cast<field_t<F, Required>&&>(fields)...};
    }
} // json
