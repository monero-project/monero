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

#include <atomic>
#include <boost/optional/optional.hpp>
#include <cstdint>
#include <string>

#include "light_wallet_server/db/storage.h"
#include "light_wallet_server/rpc.h"

namespace lws 
{
    //! Scans all active `db::account`s. Detects if another process changes active list.
    class scanner
    {
        static std::atomic<bool> running;

        scanner() = delete;

    public:
        //! Use `client` to sync blockchain data, and \return client if successful.
        static expect<rpc::client> sync(db::storage disk, rpc::client client);

        //! Poll daemon until `stop()` is called, using `thread_count` threads.
        static void run(db::storage disk, rpc::context ctx, std::size_t thread_count);

        //! \return True if `stop()` has never been called.
        static bool is_running() noexcept { return running; }

        //! Stops all scanner instances globally.
        static void stop() noexcept { running = false; }
    };
} // lws
