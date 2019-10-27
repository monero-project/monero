// Copyright (c) 2014-2019, The Monero Project
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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/optional/optional.hpp>
#include <boost/thread/future.hpp>
#include <boost/utility/string_ref.hpp>
#include <chrono>
#include <utility>

#include "common/command_line.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "net_node.h"
#include "net/net_utils_base.h"
#include "net/socks.h"
#include "net/parse.h"
#include "net/tor_address.h"
#include "net/i2p_address.h"
#include "p2p/p2p_protocol_defs.h"
#include "string_tools.h"

namespace
{
    constexpr const boost::chrono::milliseconds future_poll_interval{500};
    constexpr const std::chrono::seconds socks_connect_timeout{P2P_DEFAULT_SOCKS_CONNECT_TIMEOUT};

    std::int64_t get_max_connections(const boost::iterator_range<boost::string_ref::const_iterator> value) noexcept
    {
        // -1 is default, 0 is error
        if (value.empty())
            return -1;

        std::uint32_t out = 0;
        if (epee::string_tools::get_xtype_from_string(out, std::string{value.begin(), value.end()}))
            return out;
        return 0;
    }

    template<typename T>
    epee::net_utils::network_address get_address(const boost::string_ref value)
    {
        expect<T> address = T::make(value);
        if (!address)
        {
            MERROR(
                "Failed to parse " << epee::net_utils::zone_to_string(T::get_zone()) << " address \"" << value << "\": " << address.error().message()
            );
            return {};
        }
        return {std::move(*address)};
    }

    bool start_socks(std::shared_ptr<net::socks::client> client, const boost::asio::ip::tcp::endpoint& proxy, const epee::net_utils::network_address& remote)
    {
        CHECK_AND_ASSERT_MES(client != nullptr, false, "Unexpected null client");

        bool set = false;
        switch (remote.get_type_id())
        {
        case net::tor_address::get_type_id():
            set = client->set_connect_command(remote.as<net::tor_address>());
            break;
        case net::i2p_address::get_type_id():
            set = client->set_connect_command(remote.as<net::i2p_address>());
            break;
        default:
            MERROR("Unsupported network address in socks_connect");
            return false;
        }

        const bool sent =
            set && net::socks::client::connect_and_send(std::move(client), proxy);
        CHECK_AND_ASSERT_MES(sent, false, "Unexpected failure to init socks client");
        return true;
    }
}

namespace nodetool
{
    const command_line::arg_descriptor<std::string> arg_p2p_bind_ip        = {"p2p-bind-ip", "Interface for p2p network protocol (IPv4)", "0.0.0.0"};
    const command_line::arg_descriptor<std::string> arg_p2p_bind_ipv6_address        = {"p2p-bind-ipv6-address", "Interface for p2p network protocol (IPv6)", "::"};
    const command_line::arg_descriptor<std::string, false, true, 2> arg_p2p_bind_port = {
        "p2p-bind-port"
      , "Port for p2p network protocol (IPv4)"
      , std::to_string(config::P2P_DEFAULT_PORT)
      , {{ &cryptonote::arg_testnet_on, &cryptonote::arg_stagenet_on }}
      , [](std::array<bool, 2> testnet_stagenet, bool defaulted, std::string val)->std::string {
          if (testnet_stagenet[0] && defaulted)
            return std::to_string(config::testnet::P2P_DEFAULT_PORT);
          else if (testnet_stagenet[1] && defaulted)
            return std::to_string(config::stagenet::P2P_DEFAULT_PORT);
          return val;
        }
      };
    const command_line::arg_descriptor<std::string, false, true, 2> arg_p2p_bind_port_ipv6 = {
        "p2p-bind-port-ipv6"
      , "Port for p2p network protocol (IPv6)"
      , std::to_string(config::P2P_DEFAULT_PORT)
      , {{ &cryptonote::arg_testnet_on, &cryptonote::arg_stagenet_on }}
      , [](std::array<bool, 2> testnet_stagenet, bool defaulted, std::string val)->std::string {
          if (testnet_stagenet[0] && defaulted)
            return std::to_string(config::testnet::P2P_DEFAULT_PORT);
          else if (testnet_stagenet[1] && defaulted)
            return std::to_string(config::stagenet::P2P_DEFAULT_PORT);
          return val;
        }
      };

    const command_line::arg_descriptor<uint32_t>    arg_p2p_external_port  = {"p2p-external-port", "External port for p2p network protocol (if port forwarding used with NAT)", 0};
    const command_line::arg_descriptor<bool>        arg_p2p_allow_local_ip = {"allow-local-ip", "Allow local ip add to peer list, mostly in debug purposes"};
    const command_line::arg_descriptor<std::vector<std::string> > arg_p2p_add_peer   = {"add-peer", "Manually add peer to local peerlist"};
    const command_line::arg_descriptor<std::vector<std::string> > arg_p2p_add_priority_node   = {"add-priority-node", "Specify list of peers to connect to and attempt to keep the connection open"};
    const command_line::arg_descriptor<std::vector<std::string> > arg_p2p_add_exclusive_node   = {"add-exclusive-node", "Specify list of peers to connect to only."
                                                                                                  " If this option is given the options add-priority-node and seed-node are ignored"};
    const command_line::arg_descriptor<std::vector<std::string> > arg_p2p_seed_node   = {"seed-node", "Connect to a node to retrieve peer addresses, and disconnect"};
    const command_line::arg_descriptor<std::vector<std::string> > arg_tx_proxy = {"tx-proxy", "Send local txes through proxy: <network-type>,<socks-ip:port>[,max_connections][,disable_noise] i.e. \"tor,127.0.0.1:9050,100,disable_noise\""};
    const command_line::arg_descriptor<std::vector<std::string> > arg_anonymous_inbound = {"anonymous-inbound", "<hidden-service-address>,<[bind-ip:]port>[,max_connections] i.e. \"x.onion,127.0.0.1:18083,100\""};
    const command_line::arg_descriptor<bool> arg_p2p_hide_my_port   =    {"hide-my-port", "Do not announce yourself as peerlist candidate", false, true};
    const command_line::arg_descriptor<bool> arg_no_sync = {"no-sync", "Don't synchronize the blockchain with other peers", false};

    const command_line::arg_descriptor<bool>        arg_no_igd  = {"no-igd", "Disable UPnP port mapping"};
    const command_line::arg_descriptor<std::string> arg_igd = {"igd", "UPnP port mapping (disabled, enabled, delayed)", "delayed"};
    const command_line::arg_descriptor<bool>        arg_p2p_use_ipv6  = {"p2p-use-ipv6", "Enable IPv6 for p2p", false};
    const command_line::arg_descriptor<bool>        arg_p2p_ignore_ipv4  = {"p2p-ignore-ipv4", "Ignore unsuccessful IPv4 bind for p2p", false};
    const command_line::arg_descriptor<int64_t>     arg_out_peers = {"out-peers", "set max number of out peers", -1};
    const command_line::arg_descriptor<int64_t>     arg_in_peers = {"in-peers", "set max number of in peers", -1};
    const command_line::arg_descriptor<int> arg_tos_flag = {"tos-flag", "set TOS flag", -1};

    const command_line::arg_descriptor<int64_t> arg_limit_rate_up = {"limit-rate-up", "set limit-rate-up [kB/s]", P2P_DEFAULT_LIMIT_RATE_UP};
    const command_line::arg_descriptor<int64_t> arg_limit_rate_down = {"limit-rate-down", "set limit-rate-down [kB/s]", P2P_DEFAULT_LIMIT_RATE_DOWN};
    const command_line::arg_descriptor<int64_t> arg_limit_rate = {"limit-rate", "set limit-rate [kB/s]", -1};

    const command_line::arg_descriptor<bool> arg_pad_transactions = {
      "pad-transactions", "Pad relayed transactions to help defend against traffic volume analysis", false
    };

    boost::optional<std::vector<proxy>> get_proxies(boost::program_options::variables_map const& vm)
    {
        namespace ip = boost::asio::ip;

        std::vector<proxy> proxies{};

        const std::vector<std::string> args = command_line::get_arg(vm, arg_tx_proxy);
        proxies.reserve(args.size());

        for (const boost::string_ref arg : args)
        {
            proxies.emplace_back();

            auto next = boost::algorithm::make_split_iterator(arg, boost::algorithm::first_finder(","));
            CHECK_AND_ASSERT_MES(!next.eof() && !next->empty(), boost::none, "No network type for --" << arg_tx_proxy.name);
            const boost::string_ref zone{next->begin(), next->size()};

            ++next;
            CHECK_AND_ASSERT_MES(!next.eof() && !next->empty(), boost::none, "No ipv4:port given for --" << arg_tx_proxy.name);
            const boost::string_ref proxy{next->begin(), next->size()};

            ++next;
            for (unsigned count = 0; !next.eof(); ++count, ++next)
            {
                if (2 <= count)
                {
                    MERROR("Too many ',' characters given to --" << arg_tx_proxy.name);
                    return boost::none;
                }

                if (boost::string_ref{next->begin(), next->size()} == "disable_noise")
                    proxies.back().noise = false;
                else
                {
                    proxies.back().max_connections = get_max_connections(*next);
                    if (proxies.back().max_connections == 0)
                    {
                        MERROR("Invalid max connections given to --" << arg_tx_proxy.name);
                        return boost::none;
                    }
                }
            }

            switch (epee::net_utils::zone_from_string(zone))
            {
            case epee::net_utils::zone::tor:
                proxies.back().zone = epee::net_utils::zone::tor;
                break;
            case epee::net_utils::zone::i2p:
                proxies.back().zone = epee::net_utils::zone::i2p;
                break;
            default:
                MERROR("Invalid network for --" << arg_tx_proxy.name);
                return boost::none;
            }

            std::uint32_t ip = 0;
            std::uint16_t port = 0;
            if (!epee::string_tools::parse_peer_from_string(ip, port, std::string{proxy}) || port == 0)
            {
                MERROR("Invalid ipv4:port given for --" << arg_tx_proxy.name);
                return boost::none;
            }
            proxies.back().address = ip::tcp::endpoint{ip::address_v4{boost::endian::native_to_big(ip)}, port};
        }

        return proxies;
    }

    boost::optional<std::vector<anonymous_inbound>> get_anonymous_inbounds(boost::program_options::variables_map const& vm)
    {
        std::vector<anonymous_inbound> inbounds{};

        const std::vector<std::string> args = command_line::get_arg(vm, arg_anonymous_inbound);
        inbounds.reserve(args.size());

        for (const boost::string_ref arg : args)
        {
            inbounds.emplace_back();

            auto next = boost::algorithm::make_split_iterator(arg, boost::algorithm::first_finder(","));
            CHECK_AND_ASSERT_MES(!next.eof() && !next->empty(), boost::none, "No inbound address for --" << arg_anonymous_inbound.name);
            const boost::string_ref address{next->begin(), next->size()};

            ++next;
            CHECK_AND_ASSERT_MES(!next.eof() && !next->empty(), boost::none, "No local ipv4:port given for --" << arg_anonymous_inbound.name);
            const boost::string_ref bind{next->begin(), next->size()};

            const std::size_t colon = bind.find_first_of(':');
            CHECK_AND_ASSERT_MES(colon < bind.size(), boost::none, "No local port given for --" << arg_anonymous_inbound.name);

            ++next;
            if (!next.eof())
            {
                inbounds.back().max_connections = get_max_connections(*next);
                if (inbounds.back().max_connections == 0)
                {
                    MERROR("Invalid max connections given to --" << arg_tx_proxy.name);
                    return boost::none;
                }
            }

            expect<epee::net_utils::network_address> our_address = net::get_network_address(address, 0);
            switch (our_address ? our_address->get_type_id() : epee::net_utils::address_type::invalid)
            {
            case net::tor_address::get_type_id():
                inbounds.back().our_address = std::move(*our_address);
                inbounds.back().default_remote = net::tor_address::unknown();
                break;
            case net::i2p_address::get_type_id():
                inbounds.back().our_address = std::move(*our_address);
                inbounds.back().default_remote = net::i2p_address::unknown();
                break;
            default:
                MERROR("Invalid inbound address (" << address << ") for --" << arg_anonymous_inbound.name << ": " << (our_address ? "invalid type" : our_address.error().message()));
                return boost::none;
            }

            // get_address returns default constructed address on error
            if (inbounds.back().our_address == epee::net_utils::network_address{})
                return boost::none;

            std::uint32_t ip = 0;
            std::uint16_t port = 0;
            if (!epee::string_tools::parse_peer_from_string(ip, port, std::string{bind}))
            {
                MERROR("Invalid ipv4:port given for --" << arg_anonymous_inbound.name);
                return boost::none;
            }
            inbounds.back().local_ip = std::string{bind.substr(0, colon)};
            inbounds.back().local_port = std::string{bind.substr(colon + 1)};
        }

        return inbounds;
    }

    bool is_filtered_command(const epee::net_utils::network_address& address, int command)
    {
        switch (command)
        {
        case nodetool::COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::ID:
        case nodetool::COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::ID:
        case cryptonote::NOTIFY_NEW_TRANSACTIONS::ID:
            return false;
        default:
            break;
        }

        if (address.get_zone() == epee::net_utils::zone::public_)
            return false;

        MWARNING("Filtered command (#" << command << ") to/from " << address.str());
        return true;
    }

    boost::optional<boost::asio::ip::tcp::socket>
    socks_connect_internal(const std::atomic<bool>& stop_signal, boost::asio::io_service& service, const boost::asio::ip::tcp::endpoint& proxy, const epee::net_utils::network_address& remote)
    {
        using socket_type = net::socks::client::stream_type::socket;
        using client_result = std::pair<boost::system::error_code, socket_type>;

        struct notify
        {
            boost::promise<client_result> socks_promise;

            void operator()(boost::system::error_code error, socket_type&& sock)
            {
                socks_promise.set_value(std::make_pair(error, std::move(sock)));
            }
        };

        boost::unique_future<client_result> socks_result{};
        {
            boost::promise<client_result> socks_promise{};
            socks_result = socks_promise.get_future();

            auto client = net::socks::make_connect_client(
                boost::asio::ip::tcp::socket{service}, net::socks::version::v4a, notify{std::move(socks_promise)}
             );
            if (!start_socks(std::move(client), proxy, remote))
                return boost::none;
        }

        const auto start = std::chrono::steady_clock::now();
        while (socks_result.wait_for(future_poll_interval) == boost::future_status::timeout)
        {
            if (socks_connect_timeout < std::chrono::steady_clock::now() - start)
            {
                MERROR("Timeout on socks connect (" << proxy << " to " << remote.str() << ")");
                return boost::none;
            }

            if (stop_signal)
                return boost::none;
        }

        try
        {
            auto result = socks_result.get();
            if (!result.first)
                return {std::move(result.second)};

            MERROR("Failed to make socks connection to " << remote.str() << " (via " << proxy << "): " << result.first.message());
        }
        catch (boost::broken_promise const&)
        {}

        return boost::none;
    }
}
