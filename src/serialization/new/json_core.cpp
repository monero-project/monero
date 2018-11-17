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
#include "json_core.h"

#include <ostream>
#include <rapidjson/document.h>
#include <string>

#include "hex.h"
#include "serialization/new/json_error.h"
#include "string_tools.h"

namespace json
{
    namespace
    {
        template<typename T>
        expect<T> numeric_check(T value, T min, T max) noexcept
        {
            if (value < min)
                return {json::error::underflow};
            if (max < value)
                return {json::error::overflow};
            return value;
        }
    }

    namespace detail
    {
        expect<std::uint64_t> get_unsigned(rapidjson::Value const& src, std::uint64_t max) noexcept
        {
            if (!src.IsUint64())
                return {json::error::expected_unsigned};
            return numeric_check(src.GetUint64(), std::uint64_t(0), max);
        }

        expect<void> get_real(rapidjson::Value const& src, float& dest) noexcept
        {
            if (!src.IsNumber() || !src.IsLosslessFloat())
                return {json::error::expected_float};
            dest = src.GetFloat();
            return success();
        }
        expect<void> get_real(rapidjson::Value const& src, double& dest) noexcept
        {
            if (!src.IsNumber() || !src.IsLosslessDouble())
                return {json::error::expected_double};
            dest = src.GetDouble();
            return success();
        }

        expect<void> put_unsigned(std::ostream& dest, std::uint64_t src)
        {
            dest.width(0);
            dest.setf(std::ios::dec, std::ios::basefield);
            dest.unsetf(std::ios::showpoint | std::ios::showbase | std::ios::showpos);
            dest << src;
            return success();
        }

        expect<void> put_real(std::ostream& dest, double src, unsigned precision)
        {
            dest.width(0);
            dest.setf(std::ios::dec, (std::ios::basefield | std::ios::floatfield));
            dest.unsetf(std::ios::showpoint | std::ios::showbase | std::ios::showpos);
            dest.precision(precision);
            dest << src;
            return success();
        }
    }

    expect<void> boolean_::operator()(rapidjson::Value const& src, bool& dest) const
    {
        if (!src.IsBool())
            return {json::error::expected_bool};
        dest = src.GetBool();
        return success();
    }
    expect<void> boolean_::operator()(std::ostream& dest, bool src) const
    {
        dest << (src ? "true" : "false");
        return success();
    }
   
    expect<void> string_::operator()(rapidjson::Value const& src, std::string& dest) const
    {
        if (!src.IsString())
            return {json::error::expected_string};
        dest.assign(src.GetString(), src.GetStringLength());
        return success();
    } 
    expect<void> string_::operator()(std::ostream& dest, const boost::string_ref src) const
    {
        dest << '"';
        dest.write(src.data(), src.size());
        dest << '"';
        return success();
    }

    expect<void> hex_string_::operator()(rapidjson::Value const& src, epee::span<std::uint8_t> dest) const
    {
        if (!src.IsString())
            return {json::error::expected_string};
        if (src.GetStringLength() % 2 != 0)
            return {json::error::invalid_hex};
        if (src.GetStringLength() / 2 != dest.size())
            return {json::error::buffer_overflow};
        
        std::string temp{};
        const std::string hex{src.GetString()};
        if (!epee::string_tools::parse_hexstr_to_binbuff(hex, temp) || temp.size() != dest.size())
            return {json::error::invalid_hex};

        std::memcpy(dest.data(), temp.data(), dest.size());
        return success();
    }
    expect<void> hex_string_::operator()(std::ostream& dest, epee::span<const std::uint8_t> src) const
    {
        dest << '"';
        epee::to_hex::buffer(dest, src);
        dest << '"';
        return success();
    }
} // json

