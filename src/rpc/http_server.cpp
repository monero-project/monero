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

#include <stdexcept>

#include "net/abstract_tcp_server2.h"
#include "net/http_message.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

namespace rpc
{
namespace
{
constexpr const std::size_t max_header_size = 100'000;
constexpr const std::size_t max_leading_empty_lines = 4;
constexpr const std::size_t max_idle_capacity = 64 * 1024;
constexpr const std::string_view allow_field = "Allow: GET, HEAD, POST, OPTIONS\r\n";
constexpr const std::string_view continue_response = "HTTP/1.1 100 Continue\r\n\r\n";

// RFC 9110 9.1: methods are case-sensitive
constexpr bool allowed_method(const std::string_view method) noexcept
{
    return method == "GET" || method == "HEAD" || method == "POST" || method == "OPTIONS";
}
} // namespace

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
    // `load_from_json` only accepts `std::string`
    epee::serialization::portable_storage ps;
    if (!ps.load_from_json(std::string{q.body}))
        return detail::json_response(error_body(nullptr, -32700, "Parse error"));

    epee::serialization::storage_entry id = std::string{};
    ps.get_value("id", id, nullptr);

    std::string method;
    if (!ps.get_value("method", method, nullptr))
        return detail::json_response(error_body(nullptr, -32600, "Invalid Request"));

    // copies `params` only to test that it exists; `portable_storage` has no public lookup
    epee::serialization::storage_entry params = epee::serialization::section{};
    if (!ps.get_value("params", params, nullptr))
        ps.set_value("params", epee::serialization::section{}, nullptr);

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

json_rpc_router& router::json_rpc(const std::string_view path)
{
    // reserved first so the route never refers to a router that failed to be stored
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

http_connection::http_connection(epee::net_utils::i_service_endpoint* const endpoint,
                                 config_type& config, const connection_context& context) noexcept
    : endpoint_(endpoint),
      config_(config),
      context_(context)
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
    cache_.append(static_cast<const char*>(data), size);

    std::size_t consumed = 0;
    const bool keep = process(consumed);

    // one erase per read, rather than one per pipelined request
    cache_.erase(0, consumed);
    // don't keep a large request's buffer while the connection idles
    if (cache_.empty() && cache_.capacity() > max_idle_capacity) std::string{}.swap(cache_);
    return keep;
}

bool http_connection::process(std::size_t& consumed)
{
    // `cache_` is not modified until `handle_recv` erases `consumed`, so views into it stay valid
    for (;;)
    {
        std::string_view pending = std::string_view{cache_}.substr(consumed);
        net::http::request_line line{};
        net::http::header_fields fields{};
        bool parsed = false;

        if (!request_size_)
        {
            // RFC 9112 2.2: empty lines before the request line are ignored; bare LF is not
            if (!scanned_)
            {
                while (pending.substr(0, net::http::crlf.size()) == net::http::crlf)
                {
                    if (++empty_lines_ > max_leading_empty_lines) return reject(400);
                    consumed += net::http::crlf.size();
                    pending.remove_prefix(net::http::crlf.size());
                }
                if (pending.empty() || pending == "\r") return true;
            }

            // resume search, allowing for a terminator split across reads
            const std::size_t start = scanned_ < net::http::header_end.size()
                                          ? 0
                                          : scanned_ - (net::http::header_end.size() - 1);
            const std::size_t end = pending.find(net::http::header_end, start);
            if (end == std::string_view::npos)
            {
                scanned_ = pending.size();
                // part of a split terminator may follow a header of the maximum size
                const std::size_t limit = max_header_size + net::http::header_end.size() - 1;
                return pending.size() <= limit || reject(431);
            }

            const unsigned int status = accept_header(pending.substr(0, end), line, fields);
            if (status) return reject(status);
            parsed = true;
            scanned_ = end;
            request_size_ = end + net::http::header_end.size() + fields.content_length;

            // RFC 9110 10.1.1: ignored for HTTP/1.0
            const bool send_continue = !fields.expect.empty() && line.version == "HTTP/1.1" &&
                                       pending.size() < request_size_;
            if (send_continue &&
                !endpoint_->do_send(epee::byte_slice{std::string{continue_response}}))
                return false;
        }

        if (pending.size() < request_size_) return true;

        // a header parsed on an earlier read pointed into the buffer as it was then, and
        // `handle_recv` has since appended to (and possibly moved) it
        if (!parsed && !net::http::parse_request_header(pending.substr(0, scanned_), line, fields))
            return false;
        const http_request request{
            line.method, line.target,
            pending.substr(scanned_ + net::http::header_end.size(), fields.content_length)};

        MINFO("HTTP [" << context_.m_remote_address.host_str() << "] " << line.method << " "
                       << line.target);

        bool keep_alive = net::http::persistent(line, fields);
        const bool head = line.method == "HEAD";
        http_response response{404};
        const handler* const h =
            config_.routes ? config_.routes->find(net::http::target_path(request.target)) : nullptr;
        if (line.method == "OPTIONS")
        {
            // answered here for every target, so CORS preflights never reach a handler
            response = http_response{200};
        }
        else if (h)
        {
            try
            {
                response = (*h)(request, context_);
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

        // every internal error closes the connection
        if (response.code == 500) keep_alive = false;
        if (!send(std::move(response), keep_alive, head)) return false;

        consumed += request_size_;
        scanned_ = 0;
        request_size_ = 0;
        empty_lines_ = 0;
        if (!keep_alive) return false;
        if (consumed == cache_.size()) return true;
    }
}

unsigned int http_connection::accept_header(const std::string_view header,
                                            net::http::request_line& line,
                                            net::http::header_fields& fields) const
{
    if (header.size() > max_header_size) return 431;

    if (!net::http::parse_request_header(header, line, fields))
    {
        MWARNING(context_ << "Malformed HTTP request header");
        return 400;
    }

    // RFC 9112 3.2: exactly one Host on HTTP/1.1, and never more than one
    if (fields.host_count > 1 || (line.version == "HTTP/1.1" && fields.host_count == 0)) return 400;

    if (!allowed_method(line.method)) return 405;

    const std::size_t header_size = header.size() + net::http::header_end.size();
    if (std::numeric_limits<std::size_t>::max() - header_size < fields.content_length ||
        config_.max_content_length < header_size + fields.content_length)
    {
        MWARNING(context_ << "HTTP request too large");
        return 413;
    }

    // only 100-continue is supported
    if (!fields.expect.empty() && !net::http::iequals(fields.expect, "100-continue")) return 417;
    return 0;
}

bool http_connection::reject(const unsigned int code) const
{
    // RFC 9110 15.5.6: a 405 must list the allowed methods
    send(http_response{code}, false, false, code == 405 ? allow_field : std::string_view{});
    return false;
}

bool http_connection::send(http_response response, const bool keep_alive, const bool head,
                           const std::string_view extra_fields) const
{
    // for responses without a type of their own: errors, 404 and OPTIONS
    const std::string_view content_type =
        response.content_type.empty() ? std::string_view{"text/plain"} : response.content_type;
    std::string header = net::http::write_response_header(response.code, response.body.size(),
                                                          content_type, keep_alive, extra_fields);
    if (!endpoint_->do_send(epee::byte_slice{std::move(header)})) return false;
    return head || response.body.empty() || endpoint_->do_send(std::move(response.body));
}

} // namespace rpc

// Checks `http_connection` against the interface `boosted_tcp_server` requires
template class epee::net_utils::boosted_tcp_server<rpc::http_connection>;
