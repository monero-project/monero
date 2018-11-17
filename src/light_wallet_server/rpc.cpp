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

#include "rpc.h"

#include <boost/thread/mutex.hpp>
#include <cassert>
#include <system_error>

#include "common/error.h"
#include "light_wallet_server/error.h"
#include "net/http_client.h"

namespace lws
{
namespace rpc
{
    namespace http = epee::net_utils::http;

    namespace
    {
        constexpr const char signal_endpoint[] = "inproc://signal";
        constexpr const char abort_scan_signal[] = "SCAN";
        constexpr const char abort_process_signal[] = "PROCESS";
        constexpr const int daemon_zmq_linger = 0;

        inline std::error_code zerror(int value) noexcept
        {
            struct category final : std::error_category
            {
                virtual const char* name() const noexcept override final
                {
                    return "error::error_category()";
                }

                virtual std::string message(int value) const override final
                {
                    char const* const msg = zmq_strerror(value);
                    if (msg)
                        return msg;
                    return "zmq_strerror failure";
                }
            };
            static const category instance{};
            return std::error_code{value, instance};
        }

        struct terminate
        {
            void operator()(void* ptr) const noexcept
            {
                if (ptr)
                {
                    while (zmq_term(ptr))
                    {
                        if (zmq_errno() != EINTR)
                            break;
                    }
                }
            }
        };
        using zcontext = std::unique_ptr<void, terminate>;

        struct message
        {
            zmq_msg_t handle_;

            ~message() noexcept
            {
                zmq_msg_close(handle());
            }

            zmq_msg_t* handle() noexcept
            {
                return std::addressof(handle_);
            }
        };

        expect<void> do_wait(void* daemon, void* signal_sub, short events, std::chrono::milliseconds timeout) noexcept
        {
            if (timeout <= std::chrono::seconds{0})
                return {lws::error::daemon_timeout};

            zmq_pollitem_t items[2]{
                {daemon, 0, short(events | ZMQ_POLLERR), 0},
                {signal_sub, 0, short(ZMQ_POLLIN | ZMQ_POLLERR), 0}
            };

            for (;;)
            {
                const auto start = std::chrono::steady_clock::now();
                const int ready = zmq_poll(items, 2, timeout.count());
                const auto end = std::chrono::steady_clock::now();
                const auto spent = std::chrono::duration_cast<std::chrono::milliseconds>(start - end);
                timeout -= std::min(spent, timeout);

                if (ready == 0)
                    return {lws::error::daemon_timeout};
                if (0 < ready)
                    break;
                const int err = zmq_errno(); 
                if (err != EINTR)
                    return zerror(err);
            }
            if (items[0].revents)
                return success();

            char buf[1];
            if (zmq_recv(signal_sub, buf, 1, 0) < 0)
                return zerror(zmq_errno());

            switch (buf[0])
            {
                case 'P':
                    return {lws::error::signal_abort_process};
                case 'S':
                    return {lws::error::signal_abort_scan};
                default:
                    break;
            }
            return {lws::error::signal_unknown};
        }

        template<std::size_t N>
        expect<void> do_signal(void* signal_pub, const char (&signal)[N]) noexcept
        {
            if (zmq_send(signal_pub, signal, sizeof(signal), 0) < 0)
                return zerror(zmq_errno());
            return success();
        }

        template<std::size_t N>
        expect<void> do_subscribe(void* signal_sub, const char (&signal)[N]) noexcept
        {
            if (zmq_setsockopt(signal_sub, ZMQ_SUBSCRIBE, signal, sizeof(signal)) < 0)
                return zerror(zmq_errno());
            return success();
        }
    } // anonymous

    namespace detail
    {
        struct context
        {
            explicit context(zcontext comm, socket signal_pub, std::string daemon_addr, std::chrono::minutes interval)
              : comm(std::move(comm))
              , signal_pub(std::move(signal_pub))
              , daemon_addr(std::move(daemon_addr))
              , rates_conn()
              , cache_time()
              , cache_interval(interval)
              , cached{}
              , sync_rates()
            {
                if (std::chrono::minutes{0} < cache_interval)
                    rates_conn.set_server(crypto_compare.host, boost::none, epee::net_utils::ssl_support_t::e_ssl_support_enabled);
            }

            zcontext comm;
            socket signal_pub;
            std::string daemon_addr;
            http::http_simple_client rates_conn;
            std::chrono::steady_clock::time_point cache_time;
            const std::chrono::minutes cache_interval;
            rates cached;
            boost::mutex sync_rates;
        };
    }

    expect<cryptonote::rpc::FullMessage> client::do_receive(std::chrono::seconds timeout)
    {
        MONERO_PRECOND(ctx != nullptr);
        assert(daemon != nullptr);
        assert(signal_sub != nullptr);

        zmq_msg_t zmsg{};
        if (zmq_msg_init(std::addressof(zmsg)) < 0)
            return zerror(zmq_errno());

        // RAII closing
        message msg{std::move(zmsg)};

        while (zmq_recvmsg(daemon.get(), msg.handle(), ZMQ_DONTWAIT) < 0)
        {
            const int err = zmq_errno();
            if (err == EINTR)
                continue;
            if (err != EAGAIN)
                return {zerror(err)};

            MONERO_CHECK(do_wait(daemon.get(), signal_sub.get(), ZMQ_POLLIN, timeout));
            timeout = std::chrono::seconds{0};
        }

        char const* const json = reinterpret_cast<const char*>(zmq_msg_data(msg.handle()));
        if (json == nullptr)
            return {common_error::kInvalidArgument};

        return cryptonote::rpc::FullMessage{std::string{json, zmq_msg_size(msg.handle())}};
    }

    expect<client> client::make(std::shared_ptr<detail::context> ctx) noexcept
    {
        MONERO_PRECOND(ctx != nullptr);

        const int linger = daemon_zmq_linger;
        client out{std::move(ctx)};

        out.daemon.reset(zmq_socket(out.ctx->comm.get(), ZMQ_REQ));
        if (out.daemon.get() == nullptr)
            return zerror(zmq_errno());
        if (zmq_connect(out.daemon.get(), out.ctx->daemon_addr.c_str()) < 0)
            return zerror(zmq_errno());
        if (zmq_setsockopt(out.daemon.get(), ZMQ_LINGER, &linger, sizeof(linger)))
            return zerror(zmq_errno());

        out.signal_sub.reset(zmq_socket(out.ctx->comm.get(), ZMQ_SUB));
        if (out.signal_sub.get() == nullptr)
            return zerror(zmq_errno());
        if (zmq_connect(out.signal_sub.get(), signal_endpoint) < 0)
            return zerror(zmq_errno());

        MONERO_CHECK(do_subscribe(out.signal_sub.get(), abort_process_signal));
        return {std::move(out)};
    }

    client::~client() noexcept
    {}

    expect<void> client::watch_scan_signals() noexcept
    {
        MONERO_PRECOND(ctx != nullptr);
        assert(signal_sub != nullptr);
        return do_subscribe(signal_sub.get(), abort_scan_signal);
    }

    expect<void> client::wait(std::chrono::seconds timeout) noexcept
    {
        MONERO_PRECOND(ctx != nullptr);
        assert(daemon != nullptr);
        assert(signal_sub != nullptr);
        return do_wait(daemon.get(), signal_sub.get(), 0, timeout);
    }

    expect<void> client::send(std::string const& message, std::chrono::seconds timeout) noexcept
    {
        MONERO_PRECOND(ctx != nullptr);
        assert(daemon != nullptr);
        assert(signal_sub != nullptr);

        while (zmq_send(daemon.get(), message.data(), message.size(), ZMQ_DONTWAIT) < 0)
        {
            const int err = zmq_errno();
            if (err == EINTR)
                continue;
            if (err != EAGAIN)
                return zerror(err);

            MONERO_CHECK(do_wait(daemon.get(), signal_sub.get(), ZMQ_POLLOUT, timeout));
            timeout = std::chrono::seconds{0};
        }
        return success();
    }

    expect<rates> client::get_rates() const
    {
        MONERO_PRECOND(ctx != nullptr);
        if (ctx->cache_interval <= std::chrono::minutes{0})
            return {lws::error::exchange_rates_disabled};

        const auto now  = std::chrono::steady_clock::now();
        const boost::unique_lock<boost::mutex> lock{ctx->sync_rates};
        if (now - ctx->cache_time >= ctx->cache_interval + std::chrono::seconds{30})
            return {lws::error::exchange_rates_old};
        return ctx->cached;
    }

    context context::make(std::string daemon_addr, std::chrono::minutes rates_interval)
    {
        zcontext comm{zmq_init(1)};
        if (comm == nullptr)
            MONERO_THROW(zerror(zmq_errno()), "zmq_init");

        detail::socket pub{zmq_socket(comm.get(), ZMQ_PUB)};
        if (pub == nullptr)
            MONERO_THROW(zerror(zmq_errno()), "zmq_socket");
        if (zmq_bind(pub.get(), signal_endpoint) < 0)
            MONERO_THROW(zerror(zmq_errno()), "zmq_bind");

        return context{
            std::make_shared<detail::context>(
                std::move(comm), std::move(pub), std::move(daemon_addr), rates_interval
            )
        };
    }

    context::~context() noexcept
    {
        if (ctx)
            raise_abort_process();
    }

    std::string const& context::daemon_address() const
    {
        if (ctx == nullptr)
            MONERO_THROW(common_error::kInvalidArgument, "Invalid lws::rpc::context");
        return ctx->daemon_addr;
    }

    expect<void> context::raise_abort_scan() noexcept
    {
        MONERO_PRECOND(ctx != nullptr);
        assert(ctx->signal_pub != nullptr);
        return do_signal(ctx->signal_pub.get(), abort_scan_signal);
    }

    expect<void> context::raise_abort_process() noexcept
    {
        MONERO_PRECOND(ctx != nullptr);
        assert(ctx->signal_pub != nullptr);
        return do_signal(ctx->signal_pub.get(), abort_process_signal);
    }

    expect<boost::optional<lws::rates>> context::retrieve_rates()
    {
        MONERO_PRECOND(ctx != nullptr);

        if (ctx->cache_interval <= std::chrono::minutes{0})
            return boost::make_optional(false, ctx->cached);

        const auto now = std::chrono::steady_clock::now();
        if (now - ctx->cache_time < ctx->cache_interval)
            return boost::make_optional(false, ctx->cached);

        expect<rates> fresh{lws::error::exchange_rates_fetch};

        const http::http_response_info* info = nullptr;
        const bool retrieved =
            ctx->rates_conn.invoke_get(crypto_compare.path, std::chrono::seconds{20}, std::string{}, std::addressof(info)) &&
            info != nullptr &&
            info->m_response_code == 200;

        if (retrieved)
            fresh = crypto_compare(info->m_body);

        const boost::unique_lock<boost::mutex> lock{ctx->sync_rates};
        ctx->cache_time = now;
        if (fresh)
        {
            ctx->cached = *fresh;
            return boost::make_optional(*fresh);
        }
        return fresh.error();
    }
} // rpc
} // lws
