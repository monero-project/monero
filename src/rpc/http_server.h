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

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/optional/optional.hpp>

#include "byte_slice.h"
#include "cryptonote_config.h"
#include "misc_log_ex.h"
#include "net/http_auth.h"
#include "net/jsonrpc_structs.h"
#include "net/net_ssl.h"
#include "net/net_utils_base.h"
#include "span.h"
#include "storages/portable_storage.h"
#include "storages/portable_storage_template_helper.h"
#include "net/http_message.h"

namespace epee
{
namespace net_utils
{
struct i_connection_filter;
} // namespace net_utils
} // namespace epee

namespace rpc
{

struct http_request
{
    std::string_view method, target, body;
};

struct http_response
{
    unsigned int code = 200;
    epee::byte_slice body;
    std::string content_type;
};

using connection_context = epee::net_utils::connection_context_base;
using handler = std::function<http_response(const http_request&, const connection_context&)>;

// Groups IPv6 addresses by /64 so one host can't evade per-IP limits by rotating addresses
std::string connection_limit_key(const epee::net_utils::network_address& address);

namespace detail
{
// A thrown exception counts as a failed call
template<class F> bool invoke(const connection_context& c, const std::string_view name, F&& f)
{
    try
    {
        return f();
    }
    catch (const std::exception& e)
    {
        MCERROR("net.http", c << "Failed to handle " << name << ": " << e.what());
    }
    catch (...)
    {
        MCERROR("net.http", c << "Failed to handle " << name << ": unknown exception");
    }
    return false;
}

http_response json_response(std::string body);
} // namespace detail

//! Dispatches JSON-RPC 2.0 requests by `method`
class json_rpc_router
{
public:
    //! The handler fills in the error on failure
    template<class T, class Req, class Res>
    void add(std::string_view method, T* self,
             bool (T::*f)(const Req&, Res&, epee::json_rpc::error&, const connection_context*))
    {
        add_method(method, [self, f](epee::serialization::portable_storage& ps,
                                     const connection_context& c, const std::string_view name)
        {
            epee::json_rpc::request<Req> req{};
            if (!req.load(ps)) return error_body(&req.id, -32602, "Invalid params");

            epee::json_rpc::error_response fail{};
            fail.jsonrpc = "2.0";
            fail.id = req.id;
            epee::json_rpc::response<Res, epee::json_rpc::dummy_error> res{};
            res.jsonrpc = "2.0";
            res.id = req.id;

            const bool ok = detail::invoke(c, name, [&]
            {
                return (self->*f)(req.params, res.result, fail.error, &c);
            });
            return ok ? epee::serialization::store_t_to_json(res)
                      : epee::serialization::store_t_to_json(fail);
        });
    }

    //! Failure is reported as "Internal error"
    template<class T, class Req, class Res>
    void add(std::string_view method, T* self,
             bool (T::*f)(const Req&, Res&, const connection_context*))
    {
        add_method(method, [self, f](epee::serialization::portable_storage& ps,
                                     const connection_context& c, const std::string_view name)
        {
            epee::json_rpc::request<Req> req{};
            if (!req.load(ps)) return error_body(&req.id, -32602, "Invalid params");

            epee::json_rpc::response<Res, epee::json_rpc::dummy_error> res{};
            res.jsonrpc = "2.0";
            res.id = req.id;

            const bool ok = detail::invoke(c, name, [&]
            {
                return (self->*f)(req.params, res.result, &c);
            });
            return ok ? epee::serialization::store_t_to_json(res)
                      : error_body(&req.id, -32603, "Internal error");
        });
    }

    http_response handle(const http_request& q, const connection_context& c) const;

private:
    using method_handler = std::function<std::string(epee::serialization::portable_storage&,
                                                     const connection_context&, std::string_view)>;

    // Throws `std::logic_error` if `method` already has a handler
    void add_method(std::string_view method, method_handler h);

    // `id` is left at its default when null
    static std::string error_body(const epee::serialization::storage_entry* id, std::int64_t code,
                                  std::string message);

    std::map<std::string, method_handler, std::less<>> methods_;
};

class router
{
public:
    router() = default;
    router(router&&) = default;
    router& operator=(router&&) = default;
    router(const router&) = delete;
    router& operator=(const router&) = delete;

    // Throws `std::logic_error` if `path` already has a handler
    void add(std::string_view path, handler h);

    // Throws `std::logic_error` if `path` has no handler to replace
    void replace(std::string_view path, handler h);

    //! JSON body <-> COMMAND structs
    template<class T, class Req, class Res>
    void add_json(std::string_view path, T* self,
                  bool (T::*f)(const Req&, Res&, const connection_context*))
    {
        add(path, json_handler(self, f));
    }

    template<class T, class Req, class Res>
    void replace_json(std::string_view path, T* self,
                      bool (T::*f)(const Req&, Res&, const connection_context*))
    {
        replace(path, json_handler(self, f));
    }

    //! Portable-storage body <-> COMMAND structs
    template<class T, class Req, class Res>
    void add_bin(std::string_view path, T* self,
                 bool (T::*f)(const Req&, Res&, const connection_context*))
    {
        add(path, [self, f](const http_request& q, const connection_context& c)
        {
            Req req{};
            if (!epee::serialization::load_t_from_binary(req, epee::strspan<std::uint8_t>(q.body)))
            {
                MCERROR("net.http", "Failed to parse bin body data, body size=" << q.body.size());
                return http_response{400};
            }

            Res res{};
            if (!(self->*f)(req, res, &c)) return http_response{500};

            http_response r{};
            epee::serialization::store_t_to_binary(res, r.body, 64 * 1024);
            r.content_type = "application/octet-stream";
            return r;
        });
    }

    // The returned router stays valid for the lifetime of `this`, including across moves. Add
    // methods before `this` is handed to a running server; it is then read by every connection
    // thread without locking.
    json_rpc_router& json_rpc(std::string_view path);

    const handler* find(std::string_view path) const noexcept
    {
        const auto it = routes_.find(path);
        return it == routes_.end() ? nullptr : &it->second;
    }

private:
    template<class T, class Req, class Res>
    static handler json_handler(T* self, bool (T::*f)(const Req&, Res&, const connection_context*))
    {
        return [self, f](const http_request& q, const connection_context& c)
        {
            Req req{};
            if (!epee::serialization::load_t_from_json(req, std::string{q.body}))
            {
                MCERROR("net.http", "Failed to parse JSON request");
                return http_response{400};
            }

            Res res{};
            if (!(self->*f)(req, res, &c)) return http_response{500};
            return detail::json_response(epee::serialization::store_t_to_json(res));
        };
    }

    std::map<std::string, handler, std::less<>> routes_;
    std::vector<std::unique_ptr<json_rpc_router>> json_rpc_;
};

struct server_options
{
    std::string port = "0";
    std::string bind_ip = "0.0.0.0";
    std::string bind_ipv6 = "::";
    bool use_ipv6 = false;
    bool require_ipv4 = true;
    epee::net_utils::ssl_options_t ssl = epee::net_utils::ssl_support_t::e_ssl_support_autodetect;

    std::vector<std::string> access_control_origins; // "*" allows any
    boost::optional<epee::net_utils::http::login> login;
    std::function<void(std::size_t, std::uint8_t*)> rng; // required with `login`
    bool disable_md5 = false;

    std::size_t max_public_ip_connections = DEFAULT_RPC_MAX_CONNECTIONS_PER_PUBLIC_IP;
    std::size_t max_private_ip_connections = DEFAULT_RPC_MAX_CONNECTIONS_PER_PRIVATE_IP;
    std::size_t max_connections = DEFAULT_RPC_MAX_CONNECTIONS;
    std::size_t max_content_length = std::numeric_limits<std::size_t>::max();
    std::size_t response_soft_limit = DEFAULT_RPC_SOFT_LIMIT_SIZE;

    epee::net_utils::i_connection_filter* filter = nullptr; // must outlive the server
};

struct server_config
{
    // set before the server starts, then only read
    std::shared_ptr<const router> routes;
    std::vector<std::string> access_control_origins; // sorted
    boost::optional<epee::net_utils::http::login> login;
    std::function<void(std::size_t, std::uint8_t*)> rng;
    bool disable_md5 = false;
    std::size_t max_content_length = std::numeric_limits<std::size_t>::max();
    std::size_t max_public_ip_connections = std::numeric_limits<std::size_t>::max();
    std::size_t max_private_ip_connections = std::numeric_limits<std::size_t>::max();
    std::size_t max_connections = std::numeric_limits<std::size_t>::max();

    std::mutex lock;
    std::unordered_map<std::string, std::size_t> connections; // per-IP key -> count
    std::size_t connection_count = 0;

    // True if a new connection from `address` would go over a connection limit
    bool is_host_limit(const epee::net_utils::network_address& address);

    template<typename T> static bool after_init_connection(const std::shared_ptr<T>& self)
    {
        return self->m_protocol_handler.after_init_connection();
    }
};

//! Protocol handler for epee::net_utils::boosted_tcp_server; one per connection
class http_connection
{
public:
    using connection_context = rpc::connection_context;
    using config_type = server_config;

    http_connection(epee::net_utils::i_service_endpoint* endpoint, config_type& config,
                    const connection_context& context);
    ~http_connection() noexcept;

    http_connection(const http_connection&) = delete;
    http_connection& operator=(const http_connection&) = delete;

    bool after_init_connection();
    bool handle_recv(const void* data, std::size_t size);
    bool release_protocol() noexcept { return true; }
    void handle_qued_callback() noexcept {}

private:
    // Handles complete requests in `cache_`, advancing `consumed` past them
    bool process(std::size_t& consumed);

    // Parses and checks a complete request header (everything before the empty line). Returns 0
    // to accept the request, otherwise the status to reject it with.
    unsigned int accept_header(std::string_view header, net::http::request_line& line,
                               net::http::header_fields& fields) const;

    // A digest challenge if `login` is set and the request doesn't pass it. Its header lines are
    // appended to `extra_fields`.
    std::optional<http_response> challenge(const net::http::request_line& line,
                                           const net::http::header_fields& fields,
                                           std::string& extra_fields);

    // Sends a status-only response and closes; always returns false
    bool reject(unsigned int code) const;

    // `head` keeps the body's Content-Length but leaves the body out (RFC 9110 9.3.2)
    bool send(http_response response, bool keep_alive, bool head = false,
              std::string_view extra_fields = {}) const;

    epee::net_utils::i_service_endpoint* const endpoint_;
    config_type& config_;
    const connection_context& context_;
    epee::net_utils::http::http_server_auth auth_; // per connection, as the nonce is
    std::string cache_;
    std::string limit_key_;        // empty until `after_init_connection()` succeeds
    std::size_t scanned_ = 0;      // bytes of `cache_` already searched for end of header
    std::size_t request_size_ = 0; // header + body size, 0 until header parsed
    std::size_t empty_lines_ = 0;  // skipped before the current request line
};

//! Listens for and serves HTTP requests on its own threads
class http_server
{
public:
    http_server();
    ~http_server() noexcept;

    http_server(const http_server&) = delete;
    http_server& operator=(const http_server&) = delete;

    // Throws `std::invalid_argument` on inconsistent `options`
    bool init(router routes, server_options options);
    bool run(std::size_t threads, bool wait = true);
    void send_stop_signal();
    bool timed_wait_server_stop(std::uint64_t ms);

    int get_binded_port() const;
    long get_connections_count() const;
    boost::asio::ssl::context& get_ssl_context() noexcept;

    // `f` runs on a server thread every `period` until it returns false
    void add_idle_handler(std::function<bool()> f, std::chrono::milliseconds period);

private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace rpc
