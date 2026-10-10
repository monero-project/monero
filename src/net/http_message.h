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

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace net
{
namespace http
{

constexpr const std::string_view crlf = "\r\n";
constexpr const std::string_view header_end = "\r\n\r\n";
constexpr const std::string_view whitespace = " \t";
constexpr const std::string_view token_symbols = "!#$%&'*+-.^_`|~";
constexpr const std::string_view invalid_value_chars{"\r\n\0", 3};

struct request_line
{
    std::string_view method, target, version;
};

struct header_fields
{
    std::string_view expect;
    std::size_t host_count = 0;
    std::size_t content_length = 0;
    bool has_content_length = false;
    bool connection_close = false;      // `Connection` lists `close`
    bool connection_keep_alive = false; // `Connection` lists `keep-alive`
};

constexpr std::string_view trim(const std::string_view s) noexcept
{
    const std::size_t first = s.find_first_not_of(whitespace);
    if (first == std::string_view::npos) return {};
    return s.substr(first, s.find_last_not_of(whitespace) - first + 1);
}

// ASCII-only, so matching can't change with the global locale
constexpr bool iequals(const std::string_view a, const std::string_view b) noexcept
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        const char x = ('A' <= a[i] && a[i] <= 'Z') ? char(a[i] - 'A' + 'a') : a[i];
        const char y = ('A' <= b[i] && b[i] <= 'Z') ? char(b[i] - 'A' + 'a') : b[i];
        if (x != y) return false;
    }
    return true;
}

// RFC 9110 5.6.2
constexpr bool is_token(const std::string_view s) noexcept
{
    if (s.empty()) return false;
    for (const char c : s)
    {
        const bool alnum =
            ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
        if (!alnum && token_symbols.find(c) == std::string_view::npos) return false;
    }
    return true;
}

// Rejects whitespace, control characters and DEL; percent-encoding is left to the handler
constexpr bool is_request_target(const std::string_view s) noexcept
{
    if (s.empty()) return false;
    for (const char c : s)
    {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u <= 0x20 || u == 0x7F) return false;
    }
    return true;
}

constexpr std::string_view status_text(const unsigned int code) noexcept
{
    switch (code)
    {
    case 100:
        return "Continue";
    case 200:
        return "OK";
    case 400:
        return "Bad request";
    case 401:
        return "Unauthorized";
    case 403:
        return "Forbidden";
    case 404:
        return "Not found";
    case 405:
        return "Method Not Allowed";
    case 413:
        return "Content Too Large";
    case 417:
        return "Expectation Failed";
    case 431:
        return "Request Header Fields Too Large";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 503:
        return "Service Unavailable";
    default:
        break;
    }
    return {};
}

constexpr bool parse_request_line(const std::string_view line, request_line& out) noexcept
{
    const std::size_t method_end = line.find(' ');
    if (method_end == std::string_view::npos) return false;
    const std::size_t target_end = line.find(' ', method_end + 1);
    if (target_end == std::string_view::npos) return false;

    out.method = line.substr(0, method_end);
    out.target = line.substr(method_end + 1, target_end - method_end - 1);
    out.version = line.substr(target_end + 1);
    return is_token(out.method) && is_request_target(out.target) &&
           (out.version == "HTTP/1.1" || out.version == "HTTP/1.0");
}

// The path used for routing: drops the query, and the scheme and authority of an absolute-form
// target (RFC 9112 3.2.2). Empty when the target has no usable path.
constexpr std::string_view target_path(std::string_view target) noexcept
{
    if (!target.empty() && target.front() != '/')
    {
        const std::size_t scheme_end = target.find("://");
        if (scheme_end == std::string_view::npos) return {};
        const std::string_view scheme = target.substr(0, scheme_end);
        if (!iequals(scheme, "http") && !iequals(scheme, "https")) return {};

        target.remove_prefix(scheme_end + 3);
        const std::size_t path_start = target.find_first_of("/?");
        if (path_start == std::string_view::npos || target[path_start] == '?') return "/";
        target.remove_prefix(path_start);
    }
    return target.substr(0, target.find('?'));
}

// RFC 9112 9.3: HTTP/1.1 persists unless `close` is listed; HTTP/1.0 only if `keep-alive` is
constexpr bool persistent(const request_line& line, const header_fields& fields) noexcept
{
    if (fields.connection_close) return false;
    return line.version == "HTTP/1.1" || fields.connection_keep_alive;
}

bool parse_size(std::string_view s, std::size_t& out) noexcept;

// RFC 9110 8.6: a list of identical values is accepted as that value
bool parse_content_length(std::string_view value, std::size_t& out) noexcept;

bool parse_header_fields(std::string_view fields, header_fields& out);

// `header` is everything before the terminating empty line
bool parse_request_header(std::string_view header, request_line& line, header_fields& fields);

// `extra_fields` is written as is, so must be complete CRLF-terminated header lines
std::string write_response_header(unsigned int code, std::size_t content_length,
                                  std::string_view content_type, bool keep_alive,
                                  std::string_view extra_fields = {});

} // namespace http
} // namespace net
