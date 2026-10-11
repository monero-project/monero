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

// the same methods as `allow_field`
constexpr const std::string_view cors_fields_end =
    "Access-Control-Allow-Methods: GET, HEAD, POST, OPTIONS\r\n";

// RFC 9110 9.1: methods are case-sensitive
constexpr bool allowed_method(const std::string_view method) noexcept
{
    return method == "GET" || method == "HEAD" || method == "POST" || method == "OPTIONS";
}

// Empty unless `origins` (sorted) allows the request's Origin
std::string cors_fields(const std::vector<std::string>& origins,
                        const net::http::request_line& line, const net::http::header_fields& fields)
{
    if (fields.origin.empty()) return {};
    const bool allowed =
        std::binary_search(origins.begin(), origins.end(), std::string_view{"*"}, std::less<>{}) ||
        std::binary_search(origins.begin(), origins.end(), fields.origin, std::less<>{});
    if (!allowed) return {};

    std::string out;
    out.append("Access-Control-Allow-Origin: ").append(fields.origin).append(net::http::crlf);
    out.append("Access-Control-Expose-Headers: www-authenticate\r\n");
    if (line.method == "OPTIONS")
        out.append(
            "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With\r\n");
    out.append(cors_fields_end);
    return out;
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

void router::replace(const std::string_view path, handler h)
{
    const auto it = routes_.find(path);
    if (it == routes_.end()) throw std::logic_error{"no route to replace: " + std::string{path}};
    it->second = std::move(h);
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
        std::string extra_fields = cors_fields(config_.access_control_origins, line, fields);
        http_response response{404};
        const handler* const h =
            config_.routes ? config_.routes->find(net::http::target_path(request.target)) : nullptr;
        if (line.method == "OPTIONS")
        {
            // answered here for every target and without auth, so CORS preflights never reach a
            // handler
            response = http_response{200};
        }
        else if (std::optional<http_response> digest = challenge(line, fields, extra_fields))
        {
            response = std::move(*digest);
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
        if (!send(std::move(response), keep_alive, head, extra_fields)) return false;

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

std::optional<http_response> http_connection::challenge(const net::http::request_line& line,
                                                        const net::http::header_fields& fields,
                                                        std::string& extra_fields)
{
    if (!config_.login) return std::nullopt;

    // `http_server_auth` reads only these
    epee::net_utils::http::http_request_info info{};
    info.m_http_method_str = std::string{line.method};
    info.m_URI = std::string{line.target};
    if (!fields.authorization.empty())
        info.m_header_info.m_etc_fields.emplace_back("Authorization",
                                                     std::string{fields.authorization});

    boost::optional<epee::net_utils::http::http_response_info> digest = auth_.get_response(info);
    if (!digest) return std::nullopt;

    for (const auto& field : digest->m_additional_fields)
        extra_fields.append(field.first).append(": ").append(field.second).append(net::http::crlf);

    http_response out{static_cast<unsigned int>(digest->m_response_code)};
    out.body = epee::byte_slice{std::move(digest->m_body)};
    out.content_type = std::move(digest->m_mime_tipe);
    return out;
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
