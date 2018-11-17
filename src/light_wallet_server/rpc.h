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
#pragma once

#include <boost/optional/optional.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <zmq.h>

#include "common/expect.h"
#include "light_wallet_server/rates.h"
#include "rpc/message.h"

namespace lws
{
namespace rpc
{
    namespace detail
    {
        struct close
        {
            void operator()(void* ptr) const noexcept
            {
                if (ptr)
                    zmq_close(ptr);
            }
        };
        using socket = std::unique_ptr<void, close>;

        struct context;
    }

    //! Abstraction for ZMQ RPC client. Only `get_rates()` thread-safe; use `clone()`.
    class client
    {
        std::shared_ptr<detail::context> ctx;
        detail::socket daemon;
        detail::socket signal_sub;

        explicit client(std::shared_ptr<detail::context> ctx)
          : ctx(std::move(ctx)), daemon(), signal_sub()
        {}

        expect<cryptonote::rpc::FullMessage> do_receive(std::chrono::seconds timeout);

    public:
        //! A client with no connection (all send/receive functions fail).
        explicit client() noexcept
          : ctx(), daemon(), signal_sub()
        {}

        static expect<client> make(std::shared_ptr<detail::context> ctx) noexcept;

        client(client&&) = default;
        client(client const&) = delete;

        ~client() noexcept;

        client& operator=(client&&) = default;
        client& operator=(client const&) = delete;

        /*!
            \note `watch_scan_signals()` status is not cloned.
            \note The copy is not cheap - it creates a new ZMQ socket.
            \return A client connected to same daemon as `this`.
        */
        expect<client> clone() const noexcept
        {
            return make(ctx);
        }

        //! \return True if `this` is valid (i.e. not default or moved from).
        explicit operator bool() const noexcept
        {
            return ctx != nullptr;
        }

        //! `wait`, `send`, and `receive` will watch for `raise_abort_scan()`.
        expect<void> watch_scan_signals() noexcept;

        //! Block until `timeout` or until `context::stop()` is invoked.
        expect<void> wait(std::chrono::seconds timeout) noexcept;

        //! \return A JSON message for RPC request `M`.
        template<typename M>
        static std::string make_message(char const* const name, M& message)
        {
            return cryptonote::rpc::FullMessage::requestMessage(name, std::addressof(message)).getJson();
        }

        /*!
            Queue `message` for sending to daemon. If the queue is full,
            wait a maximum of `timeout` seconds or until
            `context::raise_abort_scan` or `context::raise_abort_process()` is
            called.
        */
        expect<void> send(std::string const& message, std::chrono::seconds timeout) noexcept;

        //! \return RPC response `M`, waiting a max of `timeout` seconds.
        template<typename M>
        expect<M> receive(std::chrono::seconds timeout)
        {
            expect<cryptonote::rpc::FullMessage> msg = do_receive(timeout);
            if (!msg)
                return msg.error();

            M out{};
            out.fromJson(msg->getMessage());
            return out;
        }

        /*!
            \note This is the one function that IS thread-safe. Multiple
                threads can call this function with the same `this` argument.

            \return Recent exchange rates.
        */
        expect<rates> get_rates() const;
    };

    //! Owns ZMQ context, and ZMQ PUB socket for signalling child `client`s.
    class context
    {
        std::shared_ptr<detail::context> ctx;

        explicit context(std::shared_ptr<detail::context> ctx)
          : ctx(std::move(ctx))
        {}

    public:
        /*! Use `daemon_addr` for call child client objects.

            \throw std::bad_alloc if internal `shared_ptr` allocation failed.
            \throw std::system_error if any ZMQ errors occur.

            \note All errors are exceptions; no recovery can occur.

            \param daemon_addr Location of ZMQ enabled `monerod` RPC.
            \param rates_interval Frequency to retrieve exchange rates. Set
                value to `<= 0` to disable exchange rate retrieval.
        */
        static context make(std::string daemon_addr, std::chrono::minutes rates_interval);

        context(context&&) = default;
        context(context const&) = delete;

        //! Calls `raise_abort_process()`. Clients can safely destruct later.
        ~context() noexcept;

        context& operator=(context&&) = default;
        context& operator=(context const&) = delete;

        // Do not create clone method, only one of these should exist right now.

        //! \return The full address of the monerod ZMQ daemon.
        std::string const& daemon_address() const;

        //! \return Client connection. Thread-safe.
        expect<client> connect() const noexcept
        {
            return client::make(ctx);
        }

        /*!
            All block `client::send`, `client::receive`, and `client::wait`
            calls originating from `this` object AND whose `watch_scan_signal`
            method was invoked, will immediately return with
            `lws::error::kSignlAbortScan`. This is NOT signal-safe NOR
            signal-safe NOR thread-safe.
        */
        expect<void> raise_abort_scan() noexcept;

        /*!
            All blocked `client::send`, `client::receive`, and `client::wait`
            calls originating from `this` object will immediately return with
            `lws::error::kSignalAbortProcess`. This call is NOT signal-safe NOR
            thread-safe.
        */
        expect<void> raise_abort_process() noexcept;

        /*!
            Retrieve exchange rates, if enabled and past cache interval.
            Not thread-safe (this can be invoked from one thread only, but this
            is thread-safe with `client::get_rates()`). All clients will see new
            rates immediately.

            \return Rates iff they were updated.
        */
        expect<boost::optional<lws::rates>> retrieve_rates();
    };
} // rpc
} // lws
