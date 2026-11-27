// Copyright (c) 2026, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list
//    of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may
//    be used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "gtest/gtest.h"

#include <limits>
#include <memory>

#include "net/jsonrpc_structs.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "wallet/wallet2.h"

class wallet_ring_size_test
{
public:
    static void set_cached_height(tools::wallet2 &wallet, size_t height)
    {
        while (wallet.m_blockchain.size() < height)
            wallet.m_blockchain.push_back(crypto::null_hash);
    }

    static void set_daemon_height(tools::wallet2 &wallet, uint64_t height)
    {
        wallet.m_node_rpc_proxy.set_height(height);
    }

    static uint64_t min(tools::wallet2 &wallet) { return wallet.get_min_ring_size(); }
    static uint64_t max(tools::wallet2 &wallet) { return wallet.get_max_ring_size(); }
    static uint64_t adjust(tools::wallet2 &wallet, uint64_t mixin) { return wallet.adjust_mixin(mixin); }
};

namespace
{
    class fork_info_http_client : public net::http::client
    {
        epee::net_utils::http::http_response_info response;
        const bool suppress_later_forks;

    public:
        explicit fork_info_http_client(bool suppress_later_forks): suppress_later_forks(suppress_later_forks) {}

        bool invoke(const boost::string_ref uri, const boost::string_ref method, const boost::string_ref body,
            std::chrono::milliseconds timeout, const epee::net_utils::http::http_response_info **result,
            const epee::net_utils::http::fields_list &headers) override
        {
            EXPECT_EQ("/json_rpc", uri);
            epee::json_rpc::request<cryptonote::COMMAND_RPC_HARD_FORK_INFO::request> request;
            if (!epee::serialization::load_t_from_json(request, std::string(body.data(), body.size())))
                return false;
            EXPECT_EQ("hard_fork_info", request.method);

            epee::json_rpc::response<cryptonote::COMMAND_RPC_HARD_FORK_INFO::response,
                epee::json_rpc::dummy_error> reply;
            reply.jsonrpc = "2.0";
            reply.id = request.id;
            reply.result.status = CORE_RPC_STATUS_OK;
            reply.result.version = request.params.version;
            reply.result.earliest_height = request.params.version == HF_VERSION_MIN_MIXIN_15 ||
                (suppress_later_forks && request.params.version >= 6)
                ? std::numeric_limits<uint64_t>::max() : 1;
            response.m_response_code = 200;
            response.m_body = epee::serialization::store_t_to_json(reply);
            *result = &response;
            return true;
        }
    };

    class fork_info_http_client_factory : public epee::net_utils::http::http_client_factory
    {
        const bool suppress_later_forks;

    public:
        explicit fork_info_http_client_factory(bool suppress_later_forks): suppress_later_forks(suppress_later_forks) {}

        std::unique_ptr<epee::net_utils::http::abstract_http_client> create() override
        {
            return std::unique_ptr<epee::net_utils::http::abstract_http_client>(new fork_info_http_client(suppress_later_forks));
        }
    };
}

TEST(wallet_ring_size, cached_post_fork_height_overrides_daemon_schedule)
{
    for (const bool suppress_later_forks : {false, true})
    {
        tools::wallet2 wallet(cryptonote::MAINNET, 1, true,
            std::unique_ptr<epee::net_utils::http::http_client_factory>(new fork_info_http_client_factory(suppress_later_forks)));
        wallet_ring_size_test::set_daemon_height(wallet, 3000000);

        // Before the fork, retain the daemon-driven ring size.
        wallet_ring_size_test::set_cached_height(wallet, 2689607);
        EXPECT_EQ(suppress_later_forks ? 3 : 11, wallet_ring_size_test::min(wallet));
        EXPECT_EQ(suppress_later_forks ? 0 : 11, wallet_ring_size_test::max(wallet));
        EXPECT_EQ(suppress_later_forks ? 15 : 10, wallet_ring_size_test::adjust(wallet, 15));

        // At the next fork, false earliest_height values cannot lower either bound.
        wallet_ring_size_test::set_cached_height(wallet, 2689608);
        EXPECT_EQ(16, wallet_ring_size_test::min(wallet));
        EXPECT_EQ(16, wallet_ring_size_test::max(wallet));
        EXPECT_EQ(15, wallet_ring_size_test::adjust(wallet, 0));
        EXPECT_EQ(15, wallet_ring_size_test::adjust(wallet, 15));
    }
}
