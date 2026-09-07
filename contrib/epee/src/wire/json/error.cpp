// Copyright (c) 2020, The Monero Project
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

#include "serialization/wire/json/error.h"

#include <string>

namespace wire
{
namespace error
{
  const char* get_string(const rapidjson_e value) noexcept
  {
    switch (rapidjson::ParseErrorCode(value))
    {
    default:
      break;

    case rapidjson::kParseErrorNone:
      return "No JSON parsing errors";

      // from rapidjson
    case rapidjson::kParseErrorDocumentEmpty:
      return "JSON parser expected non-empty document";
    case rapidjson::kParseErrorDocumentRootNotSingular:
      return "JSON parser expected one value at root level";

    case rapidjson::kParseErrorValueInvalid:
      return "JSON parser found invalid value";

    case rapidjson::kParseErrorObjectMissName:
      return "JSON parser expected name for object field";
    case rapidjson::kParseErrorObjectMissColon:
      return "JSON parser expected ':' between name and value";
    case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket:
      return "JSON parser expected ',' or '}'";

    case rapidjson::kParseErrorArrayMissCommaOrSquareBracket:
      return "JSON parser expected ',' or ']'";

    case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex:
      return "JSON parser found invalid unicode escape";
    case rapidjson::kParseErrorStringUnicodeSurrogateInvalid:
      return "JSON parser found invalid unicode surrogate value";
    case rapidjson::kParseErrorStringEscapeInvalid:
      return "JSON parser found invalid escape sequence in string value";
    case rapidjson::kParseErrorStringMissQuotationMark:
      return "JSON parser expected '\"'";
    case rapidjson::kParseErrorStringInvalidEncoding:
      return "JSON parser found invalid encoding";

    case rapidjson::kParseErrorNumberTooBig:
      return "JSON parser found number value larger than double float precision";
    case rapidjson::kParseErrorNumberMissFraction:
      return "JSON parser found number missing fractional component";
    case rapidjson::kParseErrorNumberMissExponent:
      return "JSON parser found number missing exponent";

    case rapidjson::kParseErrorTermination:
      return "JSON parser was stopped";
    case rapidjson::kParseErrorUnspecificSyntaxError:
      return "JSON parser found syntax error";
    }

    return "Unknown JSON parser error";
  }

  const std::error_category& rapidjson_category() noexcept
  {
    struct category final : std::error_category
    {
      virtual const char* name() const noexcept override final
      {
        return "wire::error::rapidjson_category()";
      }

      virtual std::string message(int value) const override final
      {
        return get_string(rapidjson_e(value));
      }
    };
    static const category instance{};
    return instance;
  }
} // error
} // wire
