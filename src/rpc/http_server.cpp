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

#include "http_server.h"

#include <algorithm>
#include <stdexcept>
#include <boost/asio/buffer.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/version.hpp>

#include "net/abstract_tcp_server2.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

namespace rpc
{
namespace
{
namespace beast = boost::beast;
namespace beast_http = boost::beast::http;

using field_list = std::vector<std::pair<std::string, std::string>>;

constexpr const std::size_t max_header_size = 100'000; // without the empty line ending it
constexpr const std::size_t max_leading_empty_lines = 4;
constexpr const std::size_t max_idle_capacity = 64 * 1024;
constexpr const std::string_view crlf = "\r\n";
constexpr const std::size_t header_limit = max_header_size + 2 * crlf.size(); // as Beast counts it
constexpr const std::string_view continue_response = "HTTP/1.1 100 Continue\r\n\r\n";
constexpr const std::string_view allowed_methods = "GET, HEAD, POST, OPTIONS";

// `beast::string_view` is `boost::string_view` or `boost::core::string_view`, depending on the
// Boost version, and neither converts to or from `std::string_view` in all of them
std::string_view to_std(const beast::string_view s) noexcept
{
    return {s.data(), s.size()};
}

beast::string_view to_beast(const std::string_view s) noexcept
{
    return {s.data(), s.size()};
}

// According to RFC 9110 (9.1), methods are case-sensitive; Boost.Beast will
// only map exact matches to a verb
constexpr bool allowed_method(const beast_http::verb method) noexcept
{
    return method == beast_http::verb::get || method == beast_http::verb::head ||
           method == beast_http::verb::post || method == beast_http::verb::options;
}

// The target path for routing. Drops the query, and the scheme and authority of
// an absolute-form target (RFC 9112, 3.2.2). Empty when the target has no
// usable path.
std::string_view target_path(std::string_view target) noexcept
{
    if (!target.empty() && target.front() != '/')
    {
        const std::size_t scheme_end = target.find("://");
        if (scheme_end == std::string_view::npos) return {};
        const beast::string_view scheme = to_beast(target.substr(0, scheme_end));
        if (!beast::iequals(scheme, "http") && !beast::iequals(scheme, "https")) return {};

        target.remove_prefix(scheme_end + 3);
        const std::size_t path_start = target.find_first_of("/?");
        if (path_start == std::string_view::npos || target[path_start] == '?') return "/";
        target.remove_prefix(path_start);
    }
    return target.substr(0, target.find('?'));
}

// Adds nothing unless `origins` (sorted) allows the request's Origin
template<class Request>
void add_cors_fields(const std::vector<std::string>& origins, const Request& req, field_list& out)
{
    const auto field = req.find(beast_http::field::origin);
    if (field == req.end()) return;
    const std::string_view origin = to_std(field->value());
    if (origin.empty()) return;

    const bool allowed =
        std::binary_search(origins.begin(), origins.end(), std::string_view{"*"}, std::less<>{}) ||
        std::binary_search(origins.begin(), origins.end(), origin, std::less<>{});
    if (!allowed) return;

    out.emplace_back("Access-Control-Allow-Origin", std::string{origin});
    out.emplace_back("Access-Control-Expose-Headers", "www-authenticate");
    if (req.method() == beast_http::verb::options)
        out.emplace_back("Access-Control-Allow-Headers",
                         "Content-Type, Authorization, X-Requested-With");
    out.emplace_back("Access-Control-Allow-Methods", std::string{allowed_methods});
}

// Status for a request Boost.Beast failed to parse
unsigned int parse_error_status(const boost::system::error_code& error) noexcept
{
    if (error == beast_http::error::header_limit) return 431;
#if BOOST_VERSION >= 108700 // older versions throw instead, handled in `process`
    if (error == beast_http::error::header_field_name_too_large ||
        error == beast_http::error::header_field_value_too_large)
        return 431;
#endif
    return 400;
}

bool valid_field_value(const std::string_view value) noexcept
{
    return value.find_first_of(std::string_view{"\r\n\0", 3}) == std::string_view::npos;
}

std::string write_response_header(const unsigned int code, const std::size_t content_length,
                                  const std::string_view content_type, const bool keep_alive,
                                  const field_list& fields)
{
    beast_http::response<beast_http::empty_body> res{};
    res.version(11);
    res.result(code);
    res.set(beast_http::field::server, "Epee-based");
    res.content_length(content_length);
    // Dropped rather than allowed to inject header lines
    if (!content_type.empty() && valid_field_value(content_type))
        res.set(beast_http::field::content_type, to_beast(content_type));
    if (!keep_alive) res.set(beast_http::field::connection, "close");
    for (const auto& field : fields)
    {
        if (valid_field_value(field.second))
            res.insert(to_beast(field.first), to_beast(field.second));
    }

    // `split` stops the serializer after the header; `send` adds the body
    beast_http::response_serializer<beast_http::empty_body> serializer{res};
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
    if (error) throw boost::system::system_error{error};
    return out;
}

// Beast's `string_body` without reserving the Content-Length upfront; `discard`
// drops the body rather than storing it
struct request_body
{
    struct value_type
    {
        std::string data;
        bool discard = false;
    };

    class reader
    {
    public:
        template<bool isRequest, class Fields>
        reader(beast_http::header<isRequest, Fields>&, value_type& body) noexcept
            : body_(body)
        {
        }

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
                if (!body_.discard)
                    body_.data.append(static_cast<const char*>(buffer.data()), buffer.size());
                size += buffer.size();
            }
            return size;
        }

        void finish(boost::system::error_code& error) noexcept
        {
            error = {};
        }

    private:
        value_type& body_;
    };
};
} // namespace

struct http_connection::request_state
{
    request_state()
    {
        parser.header_limit(static_cast<std::uint32_t>(header_limit));
        // `accept_header` checks the size, against a limit that includes the header
        parser.body_limit(std::numeric_limits<std::uint64_t>::max());
    }

    beast_http::request_parser<request_body> parser;
    const handler* route = nullptr;        // called unless `response` is set
    std::optional<http_response> response; // when the header alone decides it
    field_list fields;                     // sent with `response`
};


std::string connection_limit_key(const epee::net_utils::network_address& address)
{
    if (address.get_type_id() == epee::net_utils::ipv6_network_address::get_type_id())
    {
        const boost::asio::ip::address_v6 ip =
            address.as<const epee::net_utils::ipv6_network_address>().ip();
        if (epee::net_utils::should_group_ipv6_by_prefix(ip))
            return epee::net_utils::get_ipv6_subnet_address(ip, 64).to_string() + "/64";
    }
    return address.host_str();
}

http_response detail::json_response(std::string body)
{
    http_response r{};
    r.body = epee::byte_slice{std::move(body)};
    r.content_type = "application/json";
    return r;
}

void json_rpc_router::add_method(const std::string_view method, method_handler h)
{
    if (!methods_.try_emplace(std::string{method}, std::move(h)).second)
        throw std::logic_error{"duplicate JSON-RPC method: " + std::string{method}};
}

std::string json_rpc_router::error_body(const epee::serialization::storage_entry* const id,
                                        const std::int64_t code, std::string message)
{
    epee::json_rpc::error_response rsp{};
    rsp.jsonrpc = "2.0";
    if (id) rsp.id = *id;
    rsp.error.code = code;
    rsp.error.message = std::move(message);
    return epee::serialization::store_t_to_json(rsp);
}

http_response json_rpc_router::handle(const http_request& q, const connection_context& c) const
{
    epee::serialization::portable_storage ps;
    if (!ps.load_from_json(q.body))
        return detail::json_response(error_body(nullptr, -32700, "Parse error"));

    epee::serialization::storage_entry id = std::string{};
    ps.get_value("id", id, nullptr);

    std::string method;
    if (!ps.get_value("method", method, nullptr))
        return detail::json_response(error_body(nullptr, -32600, "Invalid Request"));

    // A `params` that isn't an object is copied just to test that it exists (`portable_storage`
    // has no public lookup)
    if (!ps.open_section("params", nullptr))
    {
        epee::serialization::storage_entry params;
        if (!ps.get_value("params", params, nullptr))
            ps.set_value("params", epee::serialization::section{}, nullptr);
    }

    const auto it = methods_.find(method);
    if (it == methods_.end())
        return detail::json_response(error_body(&id, -32601, "Method not found"));

    MCINFO("net.http", c << "Calling RPC method " << method);
    return detail::json_response(it->second(ps, c, method));
}

void router::add(const std::string_view path, handler h)
{
    if (!routes_.try_emplace(std::string{path}, std::move(h)).second)
        throw std::logic_error{"duplicate route: " + std::string{path}};
}

void router::replace(const std::string_view path, handler h)
{
    const auto it = routes_.find(path);
    if (it == routes_.end()) throw std::logic_error{"no route to replace: " + std::string{path}};
    it->second = std::move(h);
}

json_rpc_router& router::json_rpc(const std::string_view path)
{
    // Reserved first; the route never refers to a router that failed to be stored
    json_rpc_.reserve(json_rpc_.size() + 1);
    auto methods = std::make_unique<json_rpc_router>();
    json_rpc_router* const ptr = methods.get();
    add(path, [ptr](const http_request& q, const connection_context& c)
    {
        return ptr->handle(q, c);
    });
    json_rpc_.push_back(std::move(methods));
    return *ptr;
}

bool server_config::is_host_limit(const epee::net_utils::network_address& address)
{
    const std::string key = connection_limit_key(address);

    const std::lock_guard<std::mutex> guard{lock};
    if (max_connections <= connection_count) return true;

    const auto it = connections.find(key);
    if (it == connections.end()) return false;

    const bool is_private = address.is_loopback() || address.is_local();
    return (is_private ? max_private_ip_connections : max_public_ip_connections) <= it->second;
}

unsigned server_config::get_timeout_shift(const epee::net_utils::network_address& address,
                                          const unsigned shift)
{
    if (!shift) return 0;

    const std::string key = connection_limit_key(address);

    const std::lock_guard<std::mutex> guard{lock};
    if (max_connections <= connection_count) return shift;

    const bool is_private = address.is_loopback() || address.is_local();
    const std::size_t limit = is_private ? max_private_ip_connections : max_public_ip_connections;
    const auto it = connections.find(key);
    const std::uint64_t host_shift =
        (limit && it != connections.end()) ? (std::uint64_t(it->second) * 8) / limit : 0;

    // Relief shrinks as the server fills, so that idle connections release slots
    const std::uint64_t total_shift = (std::uint64_t(connection_count) * 8) / max_connections;
    return static_cast<unsigned>(std::min<std::uint64_t>(shift, std::max(host_shift, total_shift)));
}

http_connection::http_connection(epee::net_utils::i_service_endpoint* const endpoint,
                                 config_type& config, const connection_context& context)
    : endpoint_(endpoint),
      config_(config),
      context_(context),
      auth_(config.login ? epee::net_utils::http::http_server_auth{*config.login, config.rng,
                                                                   config.disable_md5}
                         : epee::net_utils::http::http_server_auth{})
{
}

http_connection::~http_connection() noexcept
{
    if (limit_key_.empty()) return;

    try
    {
        const std::lock_guard<std::mutex> guard{config_.lock};
        if (config_.connection_count) --config_.connection_count;

        const auto it = config_.connections.find(limit_key_);
        if (it != config_.connections.end())
        {
            if (it->second <= 1)
                config_.connections.erase(it);
            else
                --it->second;
        }
    }
    catch (...)
    {
    }
}

bool http_connection::after_init_connection()
{
    std::string key = connection_limit_key(context_.m_remote_address);

    const std::lock_guard<std::mutex> guard{config_.lock};
    ++config_.connections[key];
    ++config_.connection_count;
    limit_key_ = std::move(key);
    return true;
}

bool http_connection::handle_recv(const void* const data, const std::size_t size)
{
    // Parsed in place, unless bytes from an earlier read are still waiting
    std::string_view input{static_cast<const char*>(data), size};
    const bool cached = !cache_.empty();
    if (cached)
    {
        cache_.append(input);
        input = cache_;
    }

    std::size_t consumed = 0;
    const bool keep = process(input, consumed);

    // One erase per read, rather than one per pipelined request
    if (cached)
        cache_.erase(0, consumed);
    else
        cache_.assign(input.substr(consumed));
    // Don't keep a large request's buffer while the connection idles
    if (cache_.empty() && cache_.capacity() > max_idle_capacity) std::string{}.swap(cache_);
    return keep;
}


bool http_connection::process(const std::string_view input, std::size_t& consumed)
{
    for (;;)
    {
        std::string_view pending = input.substr(consumed);
        if (!request_)
        {
            // Empty lines before the request line are ignored, and bare LF is
            // not; Beast doesn't skip them itself (RFC 9112, 2.2)
            while (pending.substr(0, crlf.size()) == crlf)
            {
                if (++empty_lines_ > max_leading_empty_lines) return reject(400);
                consumed += crlf.size();
                pending.remove_prefix(crlf.size());
            }
            if (pending.empty() || pending == "\r") return true;

            request_ = std::make_unique<request_state>();
            header_size_ = 0;
            header_seen_ = 0;
        }
        else if (pending.empty())
        {
            return true;
        }

        auto& parser = request_->parser;
        const bool had_header = parser.is_header_done();
        if (!had_header)
        {
            // Waits for a line end (or the header limit), since Beast rescans
            // an unfinished line from its start on every call
            const bool line_end = pending.find('\n', header_seen_) != std::string_view::npos;
            header_seen_ = pending.size();
            if (!line_end && header_size_ + pending.size() < header_limit) return true;
        }

        boost::system::error_code error;
        std::size_t used = 0;
        try
        {
            // Boost consumes what it's able to here; the rest is once again
            // passed until more arrives
            used = parser.put(boost::asio::buffer(pending.data(), pending.size()), error);
        }
        catch (const std::length_error& e)
        {
            // Older Boost throws, rather than failing, on a field too large to store
            MWARNING(context_ << "Failed to parse HTTP request: " << e.what());
            return reject(431);
        }
        catch (const std::exception& e)
        {
            MERROR(context_ << "Failed to parse HTTP request: " << e.what());
            return reject(500);
        }
        consumed += used;
        if (!had_header)
        {
            header_size_ += used;
            header_seen_ -= used;
        }

        if (error == beast_http::error::need_more) return true;
        if (error)
        {
            MWARNING(context_ << "Malformed HTTP request: " << error.message());
            return reject(parse_error_status(error));
        }

        if (!had_header && parser.is_header_done())
        {
            const unsigned int status = accept_header();
            if (status) return reject(status);
            route();

            // Ignored for HTTP/1.0 (RFC 9110, 10.1.1). This is sent even for a
            // body that will be dropped, as answering early leaves it unclear
            // whether or not the body is still coming
            const auto& req = parser.get();
            const bool send_continue =
                req.version() == 11 && req.find(beast_http::field::expect) != req.end() &&
                !parser.is_done() &&
                input.size() - consumed < parser.content_length().value_or(0);
            const epee::span<const std::uint8_t> continue_bytes =
                epee::strspan<std::uint8_t>(continue_response);
            if (send_continue && !endpoint_->do_send(epee::byte_slice{{continue_bytes}}))
                return false;
        }

        if (!parser.is_done()) continue;

        if (!respond()) return false;
        request_.reset();
        empty_lines_ = 0;
    }
}

unsigned int http_connection::accept_header() const
{
    const auto& parser = request_->parser;
    const auto& req = parser.get();

    // Exactly one Host on HTTP/1.1, and never more than one (RFC 9112, 3.2)
    const std::size_t hosts = req.count(beast_http::field::host);
    if (hosts > 1 || (req.version() == 11 && hosts == 0)) return 400;

    if (!allowed_method(req.method())) return 405;

    // As with the old server, no transfer codings: every body is delimited by Content-Length
    if (req.find(beast_http::field::transfer_encoding) != req.end()) return 400;

    const std::uint64_t content_length = parser.content_length().value_or(0);
    if (config_.max_content_length < header_size_ ||
        config_.max_content_length - header_size_ < content_length)
    {
        MWARNING(context_ << "HTTP request too large");
        return 413;
    }

    // Only 100-continue is supported
    const auto expect = req.find(beast_http::field::expect);
    if (expect != req.end() && !beast::iequals(expect->value(), "100-continue")) return 417;
    return 0;
}

void http_connection::route()
{
    auto& req = request_->parser.get();
    if (req.method() == beast_http::verb::options)
    {
        // Answered here for every target and without auth, so that CORS
        // preflights won't reach a handler
        request_->response = http_response{200};
    }
    else if (std::optional<http_response> digest = challenge(request_->fields))
    {
        request_->response = std::move(digest);
    }
    else
    {
        request_->route =
            config_.routes ? config_.routes->find(target_path(to_std(req.target()))) : nullptr;
        if (!request_->route) request_->response = http_response{404};
    }

    // Only handlers need the body; otherwise, it's read but not stored
    req.body().discard = bool(request_->response);
}

bool http_connection::respond()
{
    const auto& parser = request_->parser;
    auto& req = request_->parser.get();
    const http_request request{to_std(req.method_string()), to_std(req.target()),
                               std::move(req.body().data)};

    MINFO("HTTP [" << context_.m_remote_address.host_str() << "] " << request.method << " "
                   << request.target);

    bool keep_alive = parser.keep_alive();
    const bool head = req.method() == beast_http::verb::head;
    field_list fields;
    add_cors_fields(config_.access_control_origins, req, fields);
    for (auto& field : request_->fields)
        fields.push_back(std::move(field));

    http_response response{};
    if (request_->response)
    {
        response = std::move(*request_->response);
    }
    else
    {
        try
        {
            response = (*request_->route)(request, context_);
        }
        catch (const std::exception& e)
        {
            MERROR(context_ << "Failed to handle " << request.target << ": " << e.what());
            response = http_response{500};
        }
        catch (...)
        {
            MERROR(context_ << "Failed to handle " << request.target << ": unknown exception");
            response = http_response{500};
        }
    }

    // Every internal error closes the connection
    if (response.code == 500) keep_alive = false;
    return send(std::move(response), keep_alive, head, fields) && keep_alive;
}

std::optional<http_response> http_connection::challenge(field_list& fields)
{
    if (!config_.login) return std::nullopt;

    const auto& req = request_->parser.get();

    // `http_server_auth` reads only these
    epee::net_utils::http::http_request_info info{};
    info.m_http_method_str = std::string{to_std(req.method_string())};
    info.m_URI = std::string{to_std(req.target())};
    const auto authorization = req.find(beast_http::field::authorization);
    if (authorization != req.end())
        info.m_header_info.m_etc_fields.emplace_back("Authorization",
                                                     std::string{to_std(authorization->value())});

    boost::optional<epee::net_utils::http::http_response_info> digest = auth_.get_response(info);
    if (!digest) return std::nullopt;

    for (auto& field : digest->m_additional_fields)
        fields.emplace_back(std::move(field.first), std::move(field.second));

    http_response out{static_cast<unsigned int>(digest->m_response_code)};
    out.body = epee::byte_slice{std::move(digest->m_body)};
    out.content_type = std::move(digest->m_mime_tipe);
    return out;
}

bool http_connection::reject(const unsigned int code) const
{
    // A 405 must list the allowed methods (RFC 9110, 15.5.6)
    field_list fields;
    if (code == 405) fields.emplace_back("Allow", std::string{allowed_methods});
    (void)send(http_response{code}, false, false, fields); // closed either way
    return false;
}

bool http_connection::send(http_response response, const bool keep_alive, const bool head,
                           const field_list& fields) const
{
    // For responses without a type of their own (errors, 404 and OPTIONS)
    const std::string_view content_type =
        response.content_type.empty() ? std::string_view{"text/plain"} : response.content_type;
    std::string header =
        write_response_header(response.code, response.body.size(), content_type, keep_alive, fields);
    if (head || response.body.empty())
        return endpoint_->do_send(epee::byte_slice{std::move(header)});

    // One `do_send`, as the transport's soft limit only lets through a single send at a time
    return endpoint_->do_send(
        epee::byte_slice{{epee::strspan<std::uint8_t>(header), epee::to_span(response.body)}});
}

struct http_server::impl final : epee::net_utils::i_connection_limit
{
    impl()
        : net(epee::net_utils::e_connection_type_RPC)
    {
    }

    bool is_host_limit(const epee::net_utils::network_address& address) override
    {
        return net.get_config_object().is_host_limit(address);
    }

    epee::net_utils::boosted_tcp_server<http_connection> net;
};

http_server::http_server()
    : impl_(std::make_unique<impl>())
{
}

http_server::~http_server() noexcept = default;

bool http_server::init(router routes, server_options options)
{
    if (options.max_connections < options.max_public_ip_connections)
        throw std::invalid_argument{
            "Max public IP connections cannot be more than max connections"};
    if (options.max_connections < options.max_private_ip_connections)
        throw std::invalid_argument{
            "Max private IP connections cannot be more than max connections"};
    if (options.login && !options.rng) throw std::invalid_argument{"RPC login requires an RNG"};

    // `std::less<>` is what `cors_fields` searches with
    std::sort(options.access_control_origins.begin(), options.access_control_origins.end(),
              std::less<>{});

    server_config& config = impl_->net.get_config_object();
    config.routes = std::make_shared<const router>(std::move(routes));
    config.access_control_origins = std::move(options.access_control_origins);
    config.login = std::move(options.login);
    config.rng = std::move(options.rng);
    config.disable_md5 = options.disable_md5;
    config.max_content_length = options.max_content_length;
    config.max_public_ip_connections = options.max_public_ip_connections;
    config.max_private_ip_connections = options.max_private_ip_connections;
    config.max_connections = options.max_connections;

    impl_->net.set_threads_prefix("RPC");
    if (options.filter) impl_->net.set_connection_filter(options.filter);
    impl_->net.set_response_soft_limit(options.response_soft_limit);
    impl_->net.set_connection_limit(impl_.get());

    MGINFO("Binding on " << options.bind_ip << " (IPv4):" << options.port);
    if (options.use_ipv6) MGINFO("Binding on " << options.bind_ipv6 << " (IPv6):" << options.port);
    if (!impl_->net.init_server(options.port, options.bind_ip, options.port, options.bind_ipv6,
                                options.use_ipv6, options.require_ipv4, std::move(options.ssl)))
    {
        MERROR("Failed to bind server");
        return false;
    }
    return true;
}

bool http_server::run(const std::size_t threads, const bool wait)
{
    MINFO("Run net_service loop (" << threads << " threads)...");
    if (!impl_->net.run_server(threads, wait))
    {
        MERROR("Failed to run net TCP server!");
        return false;
    }
    if (wait) MINFO("net_service loop stopped.");
    return true;
}

void http_server::send_stop_signal()
{
    impl_->net.send_stop_signal();
}

bool http_server::timed_wait_server_stop(const std::uint64_t ms)
{
    return impl_->net.timed_wait_server_stop(ms);
}

int http_server::get_binded_port() const
{
    return impl_->net.get_binded_port();
}

long http_server::get_connections_count() const
{
    return impl_->net.get_connections_count();
}

boost::asio::ssl::context& http_server::get_ssl_context() noexcept
{
    return impl_->net.get_ssl_context();
}

void http_server::add_idle_handler(std::function<bool()> f, const std::chrono::milliseconds period)
{
    impl_->net.add_idle_handler(std::move(f), period);
}

} // namespace rpc
