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

#include "light_wallet_server/rates.h"
#include "serialization/new/json_input.h"
#include "serialization/new/json_output.h"

namespace
{
    template<typename S, typename T>
    expect<void> rates_format(S&& stream, T& values) noexcept
    {
        static constexpr const auto fmt = json::object(
            json::field("AUD", json::real64),
            json::field("BRL", json::real64),
            json::field("BTC", json::real64),
            json::field("CAD", json::real64),
            json::field("CHF", json::real64),
            json::field("CNY", json::real64),
            json::field("EUR", json::real64),
            json::field("GBP", json::real64),
            json::field("HKD", json::real64),
            json::field("INR", json::real64),
            json::field("JPY", json::real64),
            json::field("KRW", json::real64),
            json::field("MXN", json::real64),
            json::field("NOK", json::real64),
            json::field("NZD", json::real64),
            json::field("SEK", json::real64),
            json::field("SGD", json::real64),
            json::field("TRY", json::real64),
            json::field("USD", json::real64),
            json::field("RUB", json::real64),
            json::field("ZAR", json::real64)
        );
        return fmt(
            std::forward<S>(stream), values.AUD, values.BRL, values.BTC,
            values.CAD, values.CHF, values.CNY, values.EUR, values.GBP,
            values.HKD, values.INR, values.JPY, values.KRW, values.MXN,
            values.NOK, values.NZD, values.SEK, values.SGD, values.TRY,
            values.USD, values.RUB, values.ZAR
        );
    }
} // anonymous

namespace lws
{
namespace json
{
    expect<void> rates_::operator()(rapidjson::Value const& src, lws::rates& dest) const noexcept
    {
        return rates_format(src, dest);
    }
    expect<void> rates_::operator()(std::ostream& dest, lws::rates const& src) const
    {
        return rates_format(dest, src);
    }
} // json
} // lws
