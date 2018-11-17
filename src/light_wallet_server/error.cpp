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
#include "error.h"

#include <string>

namespace lws
{
    struct category final : std::error_category
    {
        virtual const char* name() const noexcept override final
        {
            return "lws::error_category()";
        }

        virtual std::string message(int value) const override final
        {
            switch (lws::error(value))
            {
                case error::account_exists:
                    return "Account with specified address already exists";
                case error::account_max:
                    return "Account limit has been reached";
                case error::account_not_found:
                    return "No account with the specified address exists";
                case error::bad_address:
                    return "Invalid base58 public address - wrong --network ?";
                case error::bad_view_key:
                    return "Address/viewkey mismatch";
                case error::bad_blockchain:
                    return "Unable to sync blockchain - wrong --network ?";
                case error::bad_client_tx:
                    return "Received invalid transaction from REST client";
                case error::bad_daemon_response:
                    return "Response from monerod daemon was bad/unexpected";
                case error::blockchain_reorg:
                    return "A blockchain reorg has been detected";
                case error::create_queue_max:
                    return "Exceeded maxmimum number of pending account requests";
                case error::daemon_timeout:
                    return "Connection failed with daemon";
                case error::duplicate_request:
                    return "A request of this type for this address has already been made";
                case error::exceeded_blockchain_buffer:
                    return "Exceeded internal buffer for blockchain hashes";
                case error::exceeded_rest_request_limit:
                    return "Request from client via REST exceeded enforced limits";
                case error::exchange_rates_disabled:
                    return "Exchange rates feature is disabled";
                case error::exchange_rates_fetch:
                    return "Unspecified error when retrieving exchange rates";
                case error::http_server:
                    return "HTTP server failed";
                case error::exchange_rates_old:
                    return "Exchange rates are older than cache interval";
                case error::not_enough_mixin:
                    return "Not enough outputs to meet requested mixin count";
                case error::signal_abort_process:
                    return "An in-process message was received to abort the process";
                case error::signal_abort_scan:
                    return "An in-process message was received to abort account scanning";
                case error::signal_unknown:
                    return "An unknown in-process message was received";
                case error::system_clock_invalid_range:
                    return "System clock is out of range for account storage format";
                case error::tx_relay_failed:
                    return "The daemon failed to relay transaction from REST client";
                default:
                    break;
            }
            return "Unknown lws::error_category() value";
        }

        virtual std::error_condition default_error_condition(int value) const noexcept override final
        {
            switch (lws::error(value))
            {
                case error::bad_address:
                case error::bad_view_key:
                    return std::errc::bad_address;
                case error::daemon_timeout:
                    return std::errc::timed_out;
                case error::exceeded_blockchain_buffer:
                    return std::errc::no_buffer_space;
                case error::signal_abort_process:
                case error::signal_abort_scan:
                case error::signal_unknown:
                    return std::errc::interrupted;
                case error::system_clock_invalid_range:
                    return std::errc::result_out_of_range;
                default:
                    break; // map to unmatchable category
            }
            return std::error_condition{value, *this};
        }
    };

    std::error_category const& error_category() noexcept
    {
        static const category instance{};
        return instance;
    }
} // lws
