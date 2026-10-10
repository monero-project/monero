// Copyright (c) 2026, The Monero Project
//
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

#include "net/http_message.h"

#include <array>
#include <charconv>
#include <ctime>
#include <limits>
#include <system_error>

#include "time_helper.h"

namespace net
{
namespace http
{

bool parse_size(const std::string_view s, std::size_t& out) noexcept
{
    if (s.empty()) return false;
    const char* const end = s.data() + s.size();
    const auto result = std::from_chars(s.data(), end, out);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parse_content_length(std::string_view value, std::size_t& out) noexcept
{
    bool found = false;
    for (;;)
    {
        const std::size_t comma = value.find(',');
        std::size_t length = 0;
        if (!parse_size(trim(value.substr(0, comma)), length)) return false;
        if (found && length != out) return false;

        out = length;
        found = true;
        if (comma == std::string_view::npos) return true;
        value.remove_prefix(comma + 1);
    }
}

bool parse_header_fields(std::string_view fields, header_fields& out)
{
    while (!fields.empty())
    {
        const std::size_t line_end = fields.find(crlf);
        const std::string_view line = fields.substr(0, line_end);
        fields.remove_prefix(line_end == std::string_view::npos ? fields.size()
                                                                : line_end + crlf.size());

        const std::size_t colon = line.find(':');
        if (colon == std::string_view::npos) return false;
        const std::string_view name = line.substr(0, colon);
        const std::string_view value = trim(line.substr(colon + 1));

        // RFC 9112 5.1 and 5.2: no whitespace around the name, which also rules out obs-fold
        if (!is_token(name)) return false;
        // RFC 9110 5.5
        if (value.find_first_of(invalid_value_chars) != std::string_view::npos) return false;

        if (iequals(name, "Connection"))
        {
            // RFC 9110 7.6.1: a comma-separated list of options
            for (std::string_view rest = value; !rest.empty();)
            {
                const std::size_t comma = rest.find(',');
                const std::string_view option = trim(rest.substr(0, comma));
                if (iequals(option, "close")) out.connection_close = true;
                if (iequals(option, "keep-alive")) out.connection_keep_alive = true;
                rest.remove_prefix(comma == std::string_view::npos ? rest.size() : comma + 1);
            }
        }
        else if (iequals(name, "Content-Length"))
        {
            std::size_t length = 0;
            if (!parse_content_length(value, length)) return false;
            if (out.has_content_length && length != out.content_length) return false;
            out.content_length = length;
            out.has_content_length = true;
        }
        else if (iequals(name, "Transfer-Encoding"))
        {
            return false;
        }
        else if (iequals(name, "Host"))
        {
            ++out.host_count;
        }
        else if (iequals(name, "Expect"))
        {
            out.expect = value;
        }
    }
    return true;
}

bool parse_request_header(const std::string_view header, request_line& line, header_fields& fields)
{
    const std::size_t line_end = header.find(crlf);
    if (!parse_request_line(header.substr(0, line_end), line)) return false;
    if (line_end == std::string_view::npos) return true;
    return parse_header_fields(header.substr(line_end + crlf.size()), fields);
}

std::string write_response_header(const unsigned int code, const std::size_t content_length,
                                  const std::string_view content_type, const bool keep_alive,
                                  const std::string_view extra_fields)
{
    std::array<char, std::numeric_limits<unsigned int>::digits10 + 1> code_str{};
    char* const code_end =
        std::to_chars(code_str.data(), code_str.data() + code_str.size(), code).ptr;

    std::array<char, std::numeric_limits<std::size_t>::digits10 + 1> length{};
    char* const length_end =
        std::to_chars(length.data(), length.data() + length.size(), content_length).ptr;

    std::array<char, 64> date{};
    std::size_t date_size = 0;
    std::tm tm{};
    if (epee::misc_utils::get_gmt_time(std::time(nullptr), tm))
        date_size = std::strftime(date.data(), date.size(), "%a, %d %b %Y %H:%M:%S GMT", &tm);

    std::string head;
    head.reserve(256 + content_type.size() + extra_fields.size());

    head.append("HTTP/1.1 ")
        .append(code_str.data(), code_end)
        .append(" ")
        .append(status_text(code))
        .append(crlf);

    head.append("Server: Epee-based\r\nContent-Length: ")
        .append(length.data(), length_end)
        .append(crlf);

    // dropped rather than allowed to inject header lines
    const bool valid_type =
        !content_type.empty() &&
        content_type.find_first_of(invalid_value_chars) == std::string_view::npos;
    if (valid_type) head.append("Content-Type: ").append(content_type).append(crlf);

    head.append("Last-Modified: ").append(date.data(), date_size).append(crlf);
    head.append("Accept-Ranges: bytes\r\n");
    if (!keep_alive) head.append("Connection: close\r\n");
    head.append(extra_fields);

    head.append(crlf);
    return head;
}

} // namespace http
} // namespace net
