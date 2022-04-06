// Copyright (c) 2019-2022, The Monero Project
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

#include <memory>
#include <string>
#include <system_error>
#include <zmq.h>

#include "common/expect.h"
#include "span.h"

//! If the expression is less than 0, return the current ZMQ error code.
#define MONERO_ZMQ_CHECK(...)                      \
    do                                             \
    {                                              \
        if (( __VA_ARGS__ ) < 0)                   \
            return {::net::zmq::get_error_code()}; \
    } while (0)

//! Print a message followed by the current ZMQ error message. 
#define MONERO_LOG_ZMQ_ERROR(...)                                                   \
    do                                                                          \
    {                                                                           \
        MERROR( __VA_ARGS__ << ": " << ::net::zmq::get_error_code().message()); \
    } while (0)

//! Throw an exception with a custom `msg`, current ZMQ error code, filename, and line number.
#define MONERO_ZMQ_THROW(msg)                         \
    MONERO_THROW( ::net::zmq::get_error_code(), msg )

namespace epee
{
    class byte_slice;
}

namespace net
{
namespace zmq
{
    //! \return Category for ZMQ errors.
    const std::error_category& error_category() noexcept;

    //! \return `code` (usally from zmq_errno()`) using `net::zmq::error_category()`.
    inline std::error_code make_error_code(int code) noexcept
    {
        return std::error_code{code, error_category()};
    }

    //! \return Error from `zmq_errno()` using `net::zmq::error_category()`.
    inline std::error_code get_error_code() noexcept
    {
        return make_error_code(zmq_errno());
    }

    //! Calls `zmq_term`
    class terminate
    {
        static void call(void* ptr) noexcept;
    public:
        void operator()(void* ptr) const noexcept
        {
            if (ptr)
                call(ptr);
        }
    };

    //! Calls `zmq_close`
    struct close
    {
        void operator()(void* ptr) const noexcept
        {
            if (ptr)
                zmq_close(ptr);
        }
    };

    //! Unique ZMQ context handle, calls `zmq_term` on destruction.
    using context = std::unique_ptr<void, terminate>;

    //! Unique ZMQ socket handle, calls `zmq_close` on destruction.
    using socket = std::unique_ptr<void, close>;

  /*! Retry a ZMQ function on `EINTR` errors. `F` must return an int with
      values less than 0 on error.

      \param op The ZMQ function to execute + retry
      \param args Forwarded to `op`. Must be resuable in case of retry.
      \return All errors except for `EINTR`. */
    template<typename F, typename... T>
    expect<void> retry_op(F op, T&&... args) noexcept(noexcept(op(args...)))
    {
      for (;;)
      {
        if (0 <= op(args...))
          return success();

        const int error = zmq_errno();
        if (error != EINTR)
          return make_error_code(error);
      }
    }

    /*! Read all parts of the next message on `socket`. Blocks until the entire
        next message (all parts) are read, or until `zmq_term` is called on the
        `zmq_context` associated with `socket`. If the context is terminated,
        `make_error_code(ETERM)` is returned.

        \note This will automatically retry on `EINTR`, so exiting on
            interrupts requires context termination.
        \note If non-blocking behavior is requested on `socket` or by `flags`,
            then `net::zmq::make_error_code(EAGAIN)` will be returned if this
            would block.

        \param socket Handle created with `zmq_socket`.
        \param flags See `zmq_msg_read` for possible flags.
     	\return Message payload read from `socket` or ZMQ error. */
    expect<std::string> receive(void* socket, int flags = 0);

    /*! Sends `payload` on `socket`. Blocks until the entire message is queued
        for sending, or until `zmq_term` is called on the `zmq_context`
        associated with `socket`. If the context is terminated,
        `make_error_code(ETERM)` is returned.

        \note This will automatically retry on `EINTR`, so exiting on
            interrupts requires context termination.
        \note If non-blocking behavior is requested on `socket` or by `flags`,
            then `net::zmq::make_error_code(EAGAIN)` will be returned if this
            would block.

        \param payload sent as one message on `socket`.
        \param socket Handle created with `zmq_socket`.
        \param flags See `zmq_send` for possible flags.
        \return `success()` if sent, otherwise ZMQ error. */
    expect<void> send(epee::span<const std::uint8_t> payload, void* socket, int flags = 0) noexcept;

    /*! Sends `payload` on `socket`. Blocks until the entire message is queued
        for sending, or until `zmq_term` is called on the `zmq_context`
        associated with `socket`. If the context is terminated,
        `make_error_code(ETERM)` is returned.

        \note This will automatically retry on `EINTR`, so exiting on
            interrupts requires context termination.
        \note If non-blocking behavior is requested on `socket` or by `flags`,
            then `net::zmq::make_error_code(EAGAIN)` will be returned if this
            would block.

        \param payload sent as one message on `socket`.
        \param socket Handle created with `zmq_socket`.
        \param flags See `zmq_msg_send` for possible flags.

        \post `payload.emtpy()` - ownership is transferred to zmq.
        \return `success()` if sent, otherwise ZMQ error. */
    expect<void> send(epee::byte_slice&& payload, void* socket, int flags = 0) noexcept;
} // zmq
} // net
