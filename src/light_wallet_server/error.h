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

#include <system_error>
#include <type_traits>

namespace lws
{
    enum class error : int
    {
        // 0 is reserved for no error, as per expect<T>
        account_exists = 1,         //!< Tried to create an account that already exists
        account_max,                //!< Maximum number of accounts have been created
        account_not_found,          //!< Account address is not in database.
        bad_address,                //!< Invalid base58 public address
        bad_view_key,               //!< Account has address/viewkey mismatch
        bad_blockchain,             //!< Blockchain is invalid or wrong network type
        bad_client_tx,              //!< REST client submitted invalid transaction
        bad_daemon_response,        //!< RPC Response from daemon was invalid
        blockchain_reorg,           //!< Blockchain reorg after fetching/scanning block(s)
        create_queue_max,           //!< Reached maximum pending account requests
        daemon_timeout,             //!< ZMQ send/receive timeout
        duplicate_request,          //!< Account already has a request of  this type pending
        exceeded_blockchain_buffer, //!< Out buffer for blockchain is too small
        exceeded_rest_request_limit,//!< Exceeded enforced size limits for request
        exchange_rates_disabled,    //!< Exchange rates fetching is disabled
        exchange_rates_fetch,       //!< Exchange rates fetching failed
        exchange_rates_old,         //!< Exchange rates are older than cache interval
        http_server,                //!< HTTP server failure (init or run)
        not_enough_mixin,           //!< Not enough outputs to meet mixin count
        signal_abort_process,       //!< In process ZMQ PUB to abort the process was received
        signal_abort_scan,          //!< In process ZMQ PUB to abort the scan was received
        signal_unknown,             //!< An unknown in process ZMQ PUB was received
        system_clock_invalid_range, //!< System clock is out of range for storage format
        tx_relay_failed             //!< Daemon failed to relayed tx from REST client
    };

    std::error_category const& error_category() noexcept;

    inline std::error_code make_error_code(lws::error value) noexcept
    {
        return std::error_code{int(value), error_category()};
    }
}

namespace std
{
    template<>
    struct is_error_code_enum<::lws::error>
      : true_type
    {};
}
