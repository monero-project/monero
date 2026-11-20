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

#include "net/http_client.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <boost/asio/buffer.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/optional/optional.hpp>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

namespace epee
{
namespace net_utils
{
namespace http
{
namespace
{
namespace beast = boost::beast;
namespace beast_http = boost::beast::http;

constexpr const std::uint32_t max_header_size = 100'000;
// Unconsumed chunk-size line, or trailer section, as the old client limited them
constexpr const std::size_t max_chunk_metadata_size = 100'000;

// `beast::string_view` differs by Boost version and doesn't always convert from `string_ref`
beast::string_view to_beast(const boost::string_ref s) noexcept
{
  return {s.data(), s.size()};
}

std::string to_string(const beast::string_view s)
{
  return {s.data(), s.size()};
}

// RFC 9110, 5.6.2
bool valid_token(const boost::string_ref s) noexcept
{
  static constexpr const char extra[] = "!#$%&'*+-.^_`|~";
  if (s.empty())
    return false;
  for (const char c : s)
  {
    const bool alnum = ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
    if (!alnum && !std::char_traits<char>::find(extra, sizeof(extra) - 1, c))
      return false;
  }
  return true;
}

bool valid_target(const boost::string_ref s) noexcept
{
  if (s.empty())
    return false;
  for (const char c : s)
  {
    const unsigned char u = static_cast<unsigned char>(c);
    if (u <= 0x20 || u == 0x7f)
      return false;
  }
  return true;
}

bool valid_field_value(const boost::string_ref s) noexcept
{
  for (const char c : s)
  {
    if (c == '\r' || c == '\n' || c == '\0')
      return false;
  }
  return true;
}

bool add_request_field(beast_http::request<beast_http::empty_body>& req,
                       const boost::string_ref name, const boost::string_ref value)
{
  if (!valid_token(name) || !valid_field_value(value))
  {
    MERROR("Invalid HTTP request field (" << name.size() << " byte name)");
    return false;
  }
  req.insert(to_beast(name), to_beast(value));
  return true;
}

// Streams the body to `i_response_handler::handle_target_data`
struct handler_body
{
  struct value_type
  {
    i_response_handler* handler = nullptr;
  };

  class reader
  {
  public:
    template<bool isRequest, class Fields>
    reader(beast_http::header<isRequest, Fields>&, value_type& body) noexcept
      : body_(body), piece_()
    {}

    void init(const boost::optional<std::uint64_t>&, boost::system::error_code& error) noexcept
    {
      error = {};
    }

    template<class ConstBufferSequence>
    std::size_t put(const ConstBufferSequence& buffers, boost::system::error_code& error)
    {
      error = {};
      std::size_t size = 0;
      for (auto it = boost::asio::buffer_sequence_begin(buffers);
           it != boost::asio::buffer_sequence_end(buffers); ++it)
      {
        const boost::asio::const_buffer buffer = *it;
        piece_.assign(static_cast<const char*>(buffer.data()), buffer.size());
        size += buffer.size();
        if (!body_.handler->handle_target_data(piece_))
        {
          MDEBUG("HTTP response body stopped by handler");
          error = boost::asio::error::operation_aborted;
          break;
        }
      }
      return size;
    }

    void finish(boost::system::error_code& error) noexcept
    {
      error = {};
    }

  private:
    value_type& body_;
    std::string piece_;
  };
};

using parser_type = beast_http::response_parser<handler_body>;

// Only "chunked" alone is supported, across every Transfer-Encoding field (RFC 9112, 6.1)
bool only_chunked(const beast_http::response_header<>& res)
{
  unsigned codings = 0;
  const auto range = res.equal_range(beast_http::field::transfer_encoding);
  for (auto it = range.first; it != range.second; ++it)
  {
    const beast::string_view value = it->value();
    std::size_t start = 0;
    for (;;)
    {
      const std::size_t end = std::min(value.find(',', start), value.size());
      boost::string_ref coding{value.data() + start, end - start};
      while (!coding.empty() && (coding.front() == ' ' || coding.front() == '\t'))
        coding.remove_prefix(1);
      while (!coding.empty() && (coding.back() == ' ' || coding.back() == '\t'))
        coding.remove_suffix(1);
      if (!coding.empty())
      {
        if (string_tools::compare_no_case(std::string{coding}, "chunked"))
          return false;
        ++codings;
      }
      if (end == value.size())
        break;
      start = end + 1;
    }
  }
  return codings == 1;
}

// RFC 9110, 15.2; no upgrade is ever requested, so 101 is final
bool is_interim(const unsigned code) noexcept
{
  return 100 <= code && code < 200 && code != 101;
}
} // anonymous

struct response_parser::impl
{
  impl(http_response_info& info, i_response_handler& handler, const bool head)
    : info(info), handler(handler), head(head), parser(), cache(), on_chunk_header(),
      metadata_size(0), metadata_seen(0), trailer(false), keep_alive(false)
  {
    on_chunk_header = [this](const std::uint64_t size, beast::string_view, boost::system::error_code&)
    {
      if (size == 0)
        trailer = true;
    };
    reset();
  }

  void reset()
  {
    parser.emplace();
    parser->header_limit(max_header_size);
    // Beast defaults to 8 MiB; block requests can exceed it
    parser->body_limit(std::numeric_limits<std::uint64_t>::max());
    parser->skip(head);
    parser->get().body().handler = std::addressof(handler);
    parser->on_chunk_header(on_chunk_header);
    metadata_size = 0;
    metadata_seen = 0;
    trailer = false;
  }

  bool read_header()
  {
    const auto& res = parser->get();
    // Beast accepts any three digits (RFC 9110, 15)
    if (res.result_int() < 100)
    {
      MERROR("Invalid HTTP status code " << res.result_int());
      return false;
    }
    info.clear();
    info.m_response_code = res.result_int();
    info.m_response_comment = to_string(res.reason());
    info.m_http_ver_hi = res.version() / 10;
    info.m_http_ver_lo = res.version() % 10;

    http_header_info& fields = info.m_header_info;
    for (const auto& field : res)
    {
      std::string name = to_string(field.name_string());
      std::string value = to_string(field.value());
      if (!string_tools::compare_no_case(name, "Connection"))
        fields.m_connection = std::move(value);
      else if (!string_tools::compare_no_case(name, "Referer"))
        fields.m_referer = std::move(value);
      else if (!string_tools::compare_no_case(name, "Content-Length"))
        fields.m_content_length = std::move(value);
      else if (!string_tools::compare_no_case(name, "Content-Type"))
        fields.m_content_type = std::move(value);
      else if (!string_tools::compare_no_case(name, "Transfer-Encoding"))
        fields.m_transfer_encoding = std::move(value);
      else if (!string_tools::compare_no_case(name, "Content-Encoding"))
        fields.m_content_encoding = std::move(value);
      else if (!string_tools::compare_no_case(name, "Host"))
        fields.m_host = std::move(value);
      else if (!string_tools::compare_no_case(name, "Cookie"))
        fields.m_cookie = std::move(value);
      else if (!string_tools::compare_no_case(name, "User-Agent"))
        fields.m_user_agent = std::move(value);
      else if (!string_tools::compare_no_case(name, "Origin"))
        fields.m_origin = std::move(value);
      else
        fields.m_etc_fields.emplace_back(std::move(name), std::move(value));
    }

    if (res.find(beast_http::field::transfer_encoding) != res.end() && !only_chunked(res))
    {
      MERROR("Unsupported HTTP Transfer-Encoding (" << fields.m_transfer_encoding.size() << " bytes)");
      return false;
    }
    return true;
  }

  status process(const boost::string_ref input, std::size_t& consumed)
  {
    while (consumed < input.size())
    {
      const bool had_header = parser->is_header_done();
      const boost::string_ref pending = input.substr(consumed);

      // Beast rescans an unfinished field from its start on every call, so the
      // header and trailer are only passed once a line ends (or at the limit)
      const bool fields = !had_header || trailer;
      const std::size_t limit = had_header ? max_chunk_metadata_size : max_header_size;
      if (fields)
      {
        const bool line_end = pending.substr(metadata_seen).find('\n') != boost::string_ref::npos;
        metadata_seen = pending.size();
        if (!line_end && metadata_size + pending.size() < limit)
          return status::need_more;
      }

      boost::system::error_code error;
      const std::size_t used = parser->put(boost::asio::buffer(pending.data(), pending.size()), error);
      consumed += used;
      if (fields)
      {
        metadata_size += used;
        metadata_seen -= used;
      }

      if (error == beast_http::error::need_more)
      {
        // Before Boost 1.87, Beast doesn't count complete fields against its header
        // limit, and older versions don't limit an unfinished chunk-size line. Body
        // bytes are always taken, so whatever is left here is metadata.
        const bool header_done = parser->is_header_done();
        const std::size_t size = (!header_done || trailer ? metadata_size : 0) + (pending.size() - used);
        if (size >= (header_done ? max_chunk_metadata_size : max_header_size))
        {
          MERROR("HTTP response " << (header_done ? "chunk metadata" : "header") << " too large");
          return status::failed;
        }
        return status::need_more;
      }
      if (error)
      {
        MERROR("Failed to parse HTTP response: " << error.message());
        return status::failed;
      }

      if (!had_header && parser->is_header_done())
      {
        metadata_size = 0;
        metadata_seen = 0;
        if (is_interim(parser->get().result_int()))
        {
          MDEBUG("Skipping interim HTTP response " << parser->get().result_int());
          reset();
          continue;
        }
        if (!read_header())
          return status::failed;
        if (!handler.on_header(info))
        {
          MDEBUG("HTTP response stopped by on_header");
          return status::failed;
        }
      }

      if (parser->is_done())
      {
        if (consumed != input.size())
        {
          MERROR("HTTP response followed by " << (input.size() - consumed) << " unexpected bytes");
          return status::failed;
        }
        keep_alive = parser->keep_alive();
        return status::done;
      }
    }
    return status::need_more;
  }

  status put(const boost::string_ref data)
  {
    boost::string_ref input = data;
    const bool cached = !cache.empty();
    if (cached)
    {
      cache.append(data.data(), data.size());
      input = cache;
    }

    std::size_t consumed = 0;
    const status result = process(input, consumed);
    if (result != status::need_more)
      std::string{}.swap(cache);
    else if (cached)
      cache.erase(0, consumed);
    else
      cache.assign(input.data() + consumed, input.size() - consumed);
    return result;
  }

  status put_eof()
  {
    // Beast accepts this as an empty response
    if (!parser->got_some())
    {
      MERROR("Connection closed before an HTTP response");
      return status::failed;
    }

    boost::system::error_code error;
    parser->put_eof(error);
    if (error)
    {
      MERROR("Connection closed before the end of the HTTP response: " << error.message());
      return status::failed;
    }
    keep_alive = false;
    return status::done;
  }

  http_response_info& info;
  i_response_handler& handler;
  const bool head;
  boost::optional<parser_type> parser;
  std::string cache;
  std::function<void(std::uint64_t, beast::string_view, boost::system::error_code&)> on_chunk_header;
  std::size_t metadata_size; //!< consumed bytes of the header, or of the trailer
  std::size_t metadata_seen; //!< unconsumed bytes already searched for a line end
  bool trailer;
  bool keep_alive;
};

response_parser::response_parser(http_response_info& info, i_response_handler& handler, const bool head)
  : impl_(new impl{info, handler, head})
{}

response_parser::~response_parser() noexcept = default;

response_parser::status response_parser::put(const boost::string_ref data)
{
  return impl_->put(data);
}

response_parser::status response_parser::put_eof()
{
  return impl_->put_eof();
}

bool response_parser::keep_alive() const noexcept
{
  return impl_->keep_alive;
}

std::string make_request_header(const boost::string_ref method, const boost::string_ref uri,
                                const boost::string_ref host, const std::size_t content_length,
                                const fields_list& fields,
                                const boost::optional<std::pair<std::string, std::string>>& auth)
{
  if (!valid_token(method) || !valid_target(uri) || !valid_field_value(host))
  {
    MERROR("Invalid HTTP request line or host");
    return {};
  }

  // Beast throws on a field too large to store, such as a Digest response built
  // from a large challenge
  try
  {
    beast_http::request<beast_http::empty_body> req{};
    req.version(11);
    req.method_string(to_beast(method));
    req.target(to_beast(uri));
    req.set(beast_http::field::host, to_beast(host));
    req.content_length(content_length);
    for (const auto& field : fields)
    {
      if (!add_request_field(req, field.first, field.second))
        return {};
    }
    if (auth && !add_request_field(req, auth->first, auth->second))
      return {};

    beast_http::request_serializer<beast_http::empty_body> serializer{req};
    serializer.split(true);

    std::string out;
    boost::system::error_code error;
    while (!error && !serializer.is_header_done())
    {
      serializer.next(error, [&out, &serializer](boost::system::error_code& ec, const auto& buffers)
      {
        ec = {};
        const std::size_t size = boost::asio::buffer_size(buffers);
        const std::size_t start = out.size();
        out.resize(start + size);
        boost::asio::buffer_copy(boost::asio::buffer(&out[start], size), buffers);
        serializer.consume(size);
      });
    }
    if (error)
    {
      MERROR("Failed to write HTTP request header: " << error.message());
      return {};
    }
    return out;
  }
  catch (const std::exception& e)
  {
    MERROR("Failed to write HTTP request header: " << e.what());
    return {};
  }
}
} // http
} // net_utils
} // epee
