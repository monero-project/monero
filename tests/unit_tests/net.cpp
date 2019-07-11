// Copyright (c) 2018, The Monero Project
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

#include <atomic>
#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/thread.hpp>
#include <cstring>
#include <functional>
#include <gtest/gtest.h>
#include <memory>

#include "net/error.h"
#include "net/net_utils_base.h"
#include "net/socks.h"
#include "net/socks_connect.h"
#include "net/parse.h"
#include "net/tor_address.h"
#include "p2p/net_peerlist_boost_serialization.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage.h"

namespace
{
    static constexpr const char v2_onion[] =
        "xmrto2bturnore26.onion";
    static constexpr const char v3_onion[] =
        "vww6ybal4bd7szmgncyruucpgfkqahzddi37ktceo3ah7ngmcopnpyyd.onion";
}

TEST(tor_address, constants)
{
    static_assert(!net::tor_address::is_local(), "bad is_local() response");
    static_assert(!net::tor_address::is_loopback(), "bad is_loopback() response");
    static_assert(net::tor_address::get_type_id() == epee::net_utils::address_type::tor, "bad get_type_id() response");

    EXPECT_FALSE(net::tor_address::is_local());
    EXPECT_FALSE(net::tor_address::is_loopback());
    EXPECT_EQ(epee::net_utils::address_type::tor, net::tor_address::get_type_id());
    EXPECT_EQ(epee::net_utils::address_type::tor, net::tor_address::get_type_id());
}

TEST(tor_address, invalid)
{
    EXPECT_TRUE(net::tor_address::make("").has_error());
    EXPECT_TRUE(net::tor_address::make(":").has_error());
    EXPECT_TRUE(net::tor_address::make(".onion").has_error());
    EXPECT_TRUE(net::tor_address::make(".onion:").has_error());
    EXPECT_TRUE(net::tor_address::make(v2_onion + 1).has_error());
    EXPECT_TRUE(net::tor_address::make(v3_onion + 1).has_error());
    EXPECT_TRUE(net::tor_address::make(boost::string_ref{v2_onion, sizeof(v2_onion) - 2}).has_error());
    EXPECT_TRUE(net::tor_address::make(boost::string_ref{v3_onion, sizeof(v3_onion) - 2}).has_error());
    EXPECT_TRUE(net::tor_address::make(std::string{v2_onion} + ":-").has_error());
    EXPECT_TRUE(net::tor_address::make(std::string{v2_onion} + ":900a").has_error());
    EXPECT_TRUE(net::tor_address::make(std::string{v3_onion} + ":65536").has_error());
    EXPECT_TRUE(net::tor_address::make(std::string{v3_onion} + ":-1").has_error());

    std::string onion{v3_onion};
    onion.at(10) = 1;
    EXPECT_TRUE(net::tor_address::make(onion).has_error());
}

TEST(tor_address, unblockable_types)
{
    net::tor_address tor{};

    ASSERT_NE(nullptr, tor.host_str());
    EXPECT_STREQ("<unknown tor host>", tor.host_str());
    EXPECT_STREQ("<unknown tor host>", tor.str().c_str());
    EXPECT_EQ(0u, tor.port());
    EXPECT_TRUE(tor.is_unknown());
    EXPECT_FALSE(tor.is_local());
    EXPECT_FALSE(tor.is_loopback());
    EXPECT_EQ(epee::net_utils::address_type::tor, tor.get_type_id());
    EXPECT_EQ(epee::net_utils::zone::tor, tor.get_zone());

    tor = net::tor_address::unknown();
    ASSERT_NE(nullptr, tor.host_str());
    EXPECT_STREQ("<unknown tor host>", tor.host_str());
    EXPECT_STREQ("<unknown tor host>", tor.str().c_str());
    EXPECT_EQ(0u, tor.port());
    EXPECT_TRUE(tor.is_unknown());
    EXPECT_FALSE(tor.is_local());
    EXPECT_FALSE(tor.is_loopback());
    EXPECT_EQ(epee::net_utils::address_type::tor, tor.get_type_id());
    EXPECT_EQ(epee::net_utils::zone::tor, tor.get_zone());

    EXPECT_EQ(net::tor_address{}, net::tor_address::unknown());
}

TEST(tor_address, valid)
{
    const auto address1 = net::tor_address::make(v3_onion);

    ASSERT_TRUE(address1.has_value());
    EXPECT_EQ(0u, address1->port());
    EXPECT_STREQ(v3_onion, address1->host_str());
    EXPECT_STREQ(v3_onion, address1->str().c_str());
    EXPECT_TRUE(address1->is_blockable());

    net::tor_address address2{*address1};

    EXPECT_EQ(0u, address2.port());
    EXPECT_STREQ(v3_onion, address2.host_str());
    EXPECT_STREQ(v3_onion, address2.str().c_str());
    EXPECT_TRUE(address2.is_blockable());
    EXPECT_TRUE(address2.equal(*address1));
    EXPECT_TRUE(address1->equal(address2));
    EXPECT_TRUE(address2 == *address1);
    EXPECT_TRUE(*address1 == address2);
    EXPECT_FALSE(address2 != *address1);
    EXPECT_FALSE(*address1 != address2);
    EXPECT_TRUE(address2.is_same_host(*address1));
    EXPECT_TRUE(address1->is_same_host(address2));
    EXPECT_FALSE(address2.less(*address1));
    EXPECT_FALSE(address1->less(address2));

    address2 = MONERO_UNWRAP(net::tor_address::make(std::string{v2_onion} + ":6545"));

    EXPECT_EQ(6545, address2.port());
    EXPECT_STREQ(v2_onion, address2.host_str());
    EXPECT_EQ(std::string{v2_onion} + ":6545", address2.str().c_str());
    EXPECT_TRUE(address2.is_blockable());
    EXPECT_FALSE(address2.equal(*address1));
    EXPECT_FALSE(address1->equal(address2));
    EXPECT_FALSE(address2 == *address1);
    EXPECT_FALSE(*address1 == address2);
    EXPECT_TRUE(address2 != *address1);
    EXPECT_TRUE(*address1 != address2);
    EXPECT_FALSE(address2.is_same_host(*address1));
    EXPECT_FALSE(address1->is_same_host(address2));
    EXPECT_FALSE(address2.less(*address1));
    EXPECT_TRUE(address1->less(address2));

    address2 = MONERO_UNWRAP(net::tor_address::make(std::string{v3_onion} + ":", 65535));

    EXPECT_EQ(65535, address2.port());
    EXPECT_STREQ(v3_onion, address2.host_str());
    EXPECT_EQ(std::string{v3_onion} + ":65535", address2.str().c_str());
    EXPECT_TRUE(address2.is_blockable());
    EXPECT_FALSE(address2.equal(*address1));
    EXPECT_FALSE(address1->equal(address2));
    EXPECT_FALSE(address2 == *address1);
    EXPECT_FALSE(*address1 == address2);
    EXPECT_TRUE(address2 != *address1);
    EXPECT_TRUE(*address1 != address2);
    EXPECT_TRUE(address2.is_same_host(*address1));
    EXPECT_TRUE(address1->is_same_host(address2));
    EXPECT_FALSE(address2.less(*address1));
    EXPECT_TRUE(address1->less(address2));
}

TEST(tor_address, generic_network_address)
{
    const epee::net_utils::network_address tor1{MONERO_UNWRAP(net::tor_address::make(v3_onion, 8080))};
    const epee::net_utils::network_address tor2{MONERO_UNWRAP(net::tor_address::make(v3_onion, 8080))};
    const epee::net_utils::network_address ip{epee::net_utils::ipv4_network_address{100, 200}};

    EXPECT_EQ(tor1, tor2);
    EXPECT_NE(ip, tor1);
    EXPECT_LT(ip, tor1);

    EXPECT_STREQ(v3_onion, tor1.host_str().c_str());
    EXPECT_EQ(std::string{v3_onion} + ":8080", tor1.str());
    EXPECT_EQ(epee::net_utils::address_type::tor, tor1.get_type_id());
    EXPECT_EQ(epee::net_utils::address_type::tor, tor2.get_type_id());
    EXPECT_EQ(epee::net_utils::address_type::ipv4, ip.get_type_id());
    EXPECT_EQ(epee::net_utils::zone::tor, tor1.get_zone());
    EXPECT_EQ(epee::net_utils::zone::tor, tor2.get_zone());
    EXPECT_EQ(epee::net_utils::zone::public_, ip.get_zone());
    EXPECT_TRUE(tor1.is_blockable());
    EXPECT_TRUE(tor2.is_blockable());
    EXPECT_TRUE(ip.is_blockable());
}

namespace
{
    struct test_command
    {
        net::tor_address tor;

        BEGIN_KV_SERIALIZE_MAP()
            KV_SERIALIZE(tor);
        END_KV_SERIALIZE_MAP()
    };
}

TEST(tor_address, epee_serializev_v2)
{
    std::string buffer{};
    {
        test_command command{MONERO_UNWRAP(net::tor_address::make(v2_onion, 10))};
        EXPECT_FALSE(command.tor.is_unknown());
        EXPECT_NE(net::tor_address{}, command.tor);
        EXPECT_STREQ(v2_onion, command.tor.host_str());
        EXPECT_EQ(10u, command.tor.port());

        epee::serialization::portable_storage stg{};
        EXPECT_TRUE(command.store(stg));
        EXPECT_TRUE(stg.store_to_binary(buffer));
    }

    test_command command{};
    {
        EXPECT_TRUE(command.tor.is_unknown());
        EXPECT_EQ(net::tor_address{}, command.tor);
        EXPECT_STREQ(net::tor_address::unknown_str(), command.tor.host_str());
        EXPECT_EQ(0u, command.tor.port());

        epee::serialization::portable_storage stg{};
        EXPECT_TRUE(stg.load_from_binary(buffer));
        EXPECT_TRUE(command.load(stg));
    }
    EXPECT_FALSE(command.tor.is_unknown());
    EXPECT_NE(net::tor_address{}, command.tor);
    EXPECT_STREQ(v2_onion, command.tor.host_str());
    EXPECT_EQ(10u, command.tor.port());

    // make sure that exceeding max buffer doesn't destroy tor_address::_load
    {
        epee::serialization::portable_storage stg{};
        stg.load_from_binary(buffer);

        std::string host{};
        ASSERT_TRUE(stg.get_value("host", host, stg.open_section("tor", nullptr, false)));
        EXPECT_EQ(std::strlen(v2_onion), host.size());

        host.push_back('k');
        EXPECT_TRUE(stg.set_value("host", host, stg.open_section("tor", nullptr, false)));
        EXPECT_TRUE(command.load(stg)); // poor error reporting from `KV_SERIALIZE`
    }

    EXPECT_TRUE(command.tor.is_unknown());
    EXPECT_EQ(net::tor_address{}, command.tor);
    EXPECT_STREQ(net::tor_address::unknown_str(), command.tor.host_str());
    EXPECT_EQ(0u, command.tor.port());
}

TEST(tor_address, epee_serializev_v3)
{
    std::string buffer{};
    {
        test_command command{MONERO_UNWRAP(net::tor_address::make(v3_onion, 10))};
        EXPECT_FALSE(command.tor.is_unknown());
        EXPECT_NE(net::tor_address{}, command.tor);
        EXPECT_STREQ(v3_onion, command.tor.host_str());
        EXPECT_EQ(10u, command.tor.port());

        epee::serialization::portable_storage stg{};
        EXPECT_TRUE(command.store(stg));
        EXPECT_TRUE(stg.store_to_binary(buffer));
    }

    test_command command{};
    {
        EXPECT_TRUE(command.tor.is_unknown());
        EXPECT_EQ(net::tor_address{}, command.tor);
        EXPECT_STREQ(net::tor_address::unknown_str(), command.tor.host_str());
        EXPECT_EQ(0u, command.tor.port());

        epee::serialization::portable_storage stg{};
        EXPECT_TRUE(stg.load_from_binary(buffer));
        EXPECT_TRUE(command.load(stg));
    }
    EXPECT_FALSE(command.tor.is_unknown());
    EXPECT_NE(net::tor_address{}, command.tor);
    EXPECT_STREQ(v3_onion, command.tor.host_str());
    EXPECT_EQ(10u, command.tor.port());

    // make sure that exceeding max buffer doesn't destroy tor_address::_load
    {
        epee::serialization::portable_storage stg{};
        stg.load_from_binary(buffer);

        std::string host{};
        ASSERT_TRUE(stg.get_value("host", host, stg.open_section("tor", nullptr, false)));
        EXPECT_EQ(std::strlen(v3_onion), host.size());

        host.push_back('k');
        EXPECT_TRUE(stg.set_value("host", host, stg.open_section("tor", nullptr, false)));
        EXPECT_TRUE(command.load(stg)); // poor error reporting from `KV_SERIALIZE`
    }

    EXPECT_TRUE(command.tor.is_unknown());
    EXPECT_EQ(net::tor_address{}, command.tor);
    EXPECT_STRNE(v3_onion, command.tor.host_str());
    EXPECT_EQ(0u, command.tor.port());
}

TEST(tor_address, epee_serialize_unknown)
{
    std::string buffer{};
    {
        test_command command{net::tor_address::unknown()};
        EXPECT_TRUE(command.tor.is_unknown());
        EXPECT_EQ(net::tor_address{}, command.tor);
        EXPECT_STREQ(net::tor_address::unknown_str(), command.tor.host_str());
        EXPECT_EQ(0u, command.tor.port());

        epee::serialization::portable_storage stg{};
        EXPECT_TRUE(command.store(stg));
        EXPECT_TRUE(stg.store_to_binary(buffer));
    }

    test_command command{};
    {
        EXPECT_TRUE(command.tor.is_unknown());
        EXPECT_EQ(net::tor_address{}, command.tor);
        EXPECT_STRNE(v3_onion, command.tor.host_str());
        EXPECT_EQ(0u, command.tor.port());

        epee::serialization::portable_storage stg{};
        EXPECT_TRUE(stg.load_from_binary(buffer));
        EXPECT_TRUE(command.load(stg));
    }
    EXPECT_TRUE(command.tor.is_unknown());
    EXPECT_EQ(net::tor_address{}, command.tor);
    EXPECT_STREQ(net::tor_address::unknown_str(), command.tor.host_str());
    EXPECT_EQ(0u, command.tor.port());

    // make sure that exceeding max buffer doesn't destroy tor_address::_load
    {
        epee::serialization::portable_storage stg{};
        stg.load_from_binary(buffer);

        std::string host{};
        ASSERT_TRUE(stg.get_value("host", host, stg.open_section("tor", nullptr, false)));
        EXPECT_EQ(std::strlen(net::tor_address::unknown_str()), host.size());

        host.push_back('k');
        EXPECT_TRUE(stg.set_value("host", host, stg.open_section("tor", nullptr, false)));
        EXPECT_TRUE(command.load(stg)); // poor error reporting from `KV_SERIALIZE`
    }

    EXPECT_TRUE(command.tor.is_unknown());
    EXPECT_EQ(net::tor_address{}, command.tor);
    EXPECT_STRNE(v3_onion, command.tor.host_str());
    EXPECT_EQ(0u, command.tor.port());
}

TEST(tor_address, boost_serialize_v2)
{
    std::string buffer{};
    {
        const net::tor_address tor = MONERO_UNWRAP(net::tor_address::make(v2_onion, 10));
        EXPECT_FALSE(tor.is_unknown());
        EXPECT_NE(net::tor_address{}, tor);
        EXPECT_STREQ(v2_onion, tor.host_str());
        EXPECT_EQ(10u, tor.port());

        std::ostringstream stream{};
        {
            boost::archive::portable_binary_oarchive archive{stream};
            archive << tor;
        }
        buffer = stream.str();
    }

    net::tor_address tor{};
    {
        EXPECT_TRUE(tor.is_unknown());
        EXPECT_EQ(net::tor_address{}, tor);
        EXPECT_STREQ(net::tor_address::unknown_str(), tor.host_str());
        EXPECT_EQ(0u, tor.port());

        std::istringstream stream{buffer};
        boost::archive::portable_binary_iarchive archive{stream};
        archive >> tor;
    }
    EXPECT_FALSE(tor.is_unknown());
    EXPECT_NE(net::tor_address{}, tor);
    EXPECT_STREQ(v2_onion, tor.host_str());
    EXPECT_EQ(10u, tor.port());
}

TEST(tor_address, boost_serialize_v3)
{
    std::string buffer{};
    {
        const net::tor_address tor = MONERO_UNWRAP(net::tor_address::make(v3_onion, 10));
        EXPECT_FALSE(tor.is_unknown());
        EXPECT_NE(net::tor_address{}, tor);
        EXPECT_STREQ(v3_onion, tor.host_str());
        EXPECT_EQ(10u, tor.port());

        std::ostringstream stream{};
        {
            boost::archive::portable_binary_oarchive archive{stream};
            archive << tor;
        }
        buffer = stream.str();
    }

    net::tor_address tor{};
    {
        EXPECT_TRUE(tor.is_unknown());
        EXPECT_EQ(net::tor_address{}, tor);
        EXPECT_STREQ(net::tor_address::unknown_str(), tor.host_str());
        EXPECT_EQ(0u, tor.port());

        std::istringstream stream{buffer};
        boost::archive::portable_binary_iarchive archive{stream};
        archive >> tor;
    }
    EXPECT_FALSE(tor.is_unknown());
    EXPECT_NE(net::tor_address{}, tor);
    EXPECT_STREQ(v3_onion, tor.host_str());
    EXPECT_EQ(10u, tor.port());
}

TEST(tor_address, boost_serialize_unknown)
{
    std::string buffer{};
    {
        const net::tor_address tor{};
        EXPECT_TRUE(tor.is_unknown());
        EXPECT_EQ(net::tor_address::unknown(), tor);
        EXPECT_STREQ(net::tor_address::unknown_str(), tor.host_str());
        EXPECT_EQ(0u, tor.port());

        std::ostringstream stream{};
        {
            boost::archive::portable_binary_oarchive archive{stream};
            archive << tor;
        }
        buffer = stream.str();
    }

    net::tor_address tor{};
    {
        EXPECT_TRUE(tor.is_unknown());
        EXPECT_EQ(net::tor_address{}, tor);
        EXPECT_STREQ(net::tor_address::unknown_str(), tor.host_str());
        EXPECT_EQ(0u, tor.port());

        std::istringstream stream{buffer};
        boost::archive::portable_binary_iarchive archive{stream};
        archive >> tor;
    }
    EXPECT_TRUE(tor.is_unknown());
    EXPECT_EQ(net::tor_address::unknown(), tor);
    EXPECT_STREQ(net::tor_address::unknown_str(), tor.host_str());
    EXPECT_EQ(0u, tor.port());
}

TEST(get_network_address, onion)
{
    expect<epee::net_utils::network_address> address =
        net::get_network_address("onion", 0);
    EXPECT_EQ(net::error::unsupported_address, address);

    address = net::get_network_address(".onion", 0);
    EXPECT_EQ(net::error::invalid_tor_address, address);

    address = net::get_network_address(v3_onion, 1000);
    ASSERT_TRUE(bool(address));
    EXPECT_EQ(epee::net_utils::address_type::tor, address->get_type_id());
    EXPECT_STREQ(v3_onion, address->host_str().c_str());
    EXPECT_EQ(std::string{v3_onion} + ":1000", address->str());

    address = net::get_network_address(std::string{v3_onion} + ":2000", 1000);
    ASSERT_TRUE(bool(address));
    EXPECT_EQ(epee::net_utils::address_type::tor, address->get_type_id());
    EXPECT_STREQ(v3_onion, address->host_str().c_str());
    EXPECT_EQ(std::string{v3_onion} + ":2000", address->str());

    address = net::get_network_address(std::string{v3_onion} + ":65536", 1000);
    EXPECT_EQ(net::error::invalid_port, address);
}


TEST(get_network_address, ipv4)
{
    expect<epee::net_utils::network_address> address =
        net::get_network_address("0.0.0.", 0);
    EXPECT_EQ(net::error::unsupported_address, address);

    address = net::get_network_address("0.0.0.257", 0);
    EXPECT_EQ(net::error::unsupported_address, address);

    address = net::get_network_address("0.0.0.254", 1000);
    ASSERT_TRUE(bool(address));
    EXPECT_EQ(epee::net_utils::address_type::ipv4, address->get_type_id());
    EXPECT_STREQ("0.0.0.254", address->host_str().c_str());
    EXPECT_STREQ("0.0.0.254:1000", address->str().c_str());

    address = net::get_network_address("23.0.0.254:2000", 1000);
    ASSERT_TRUE(bool(address));
    EXPECT_EQ(epee::net_utils::address_type::ipv4, address->get_type_id());
    EXPECT_STREQ("23.0.0.254", address->host_str().c_str());
    EXPECT_STREQ("23.0.0.254:2000", address->str().c_str());
}

namespace
{
    using stream_type = boost::asio::ip::tcp;

    struct io_thread
    {
        boost::asio::io_service io_service;
        boost::asio::io_service::work work;
        stream_type::socket server;
        stream_type::acceptor acceptor;
        boost::thread io;
        std::atomic<bool> connected;

        io_thread()
          : io_service(),
            work(io_service),
            server(io_service),
            acceptor(io_service),
            io([this] () { try { this->io_service.run(); } catch (const std::exception& e) { MERROR(e.what()); }}),
            connected(false)
        {
            acceptor.open(boost::asio::ip::tcp::v4());
            acceptor.bind(stream_type::endpoint{boost::asio::ip::address_v4::loopback(), 0});
            acceptor.listen();
            acceptor.async_accept(server, [this] (boost::system::error_code error) {
                this->connected = true;
                if (error)
                    throw boost::system::system_error{error};
            });
        }

        ~io_thread() noexcept
        {
            io_service.stop();
            if (io.joinable())
                io.join();
        }
    };

    struct checked_client
    {
        std::atomic<bool>* called_;
        bool expected_;

        void operator()(boost::system::error_code error, net::socks::client::stream_type::socket&&) const
        {
            EXPECT_EQ(expected_, bool(error)) << "Socks server: " << error.message();
            ASSERT_TRUE(called_ != nullptr);
            (*called_) = true;
        }
    };
}

TEST(socks_client, unsupported_command)
{
    boost::asio::io_service io_service{};
    stream_type::socket client{io_service};

    auto test_client = net::socks::make_connect_client(
        std::move(client), net::socks::version::v4, std::bind( [] {} )
    );
    ASSERT_TRUE(bool(test_client));
    EXPECT_TRUE(test_client->buffer().empty());

    EXPECT_FALSE(test_client->set_connect_command("example.com", 8080));
    EXPECT_TRUE(test_client->buffer().empty());

    EXPECT_FALSE(test_client->set_resolve_command("example.com"));
    EXPECT_TRUE(test_client->buffer().empty());
}

TEST(socks_client, no_command)
{
    boost::asio::io_service io_service{};
    stream_type::socket client{io_service};

    auto test_client = net::socks::make_connect_client(
        std::move(client), net::socks::version::v4a, std::bind( [] {} )
    );
    ASSERT_TRUE(bool(test_client));
    EXPECT_FALSE(net::socks::client::send(std::move(test_client)));
}

TEST(socks_client, connect_command)
{
    io_thread io{};
    stream_type::socket client{io.io_service};

    std::atomic<bool> called{false};
    auto test_client = net::socks::make_connect_client(
        std::move(client), net::socks::version::v4a, checked_client{std::addressof(called), false}
    );
    ASSERT_TRUE(bool(test_client));

    ASSERT_TRUE(test_client->set_connect_command("example.com", 8080));
    EXPECT_FALSE(test_client->buffer().empty());
    ASSERT_TRUE(net::socks::client::connect_and_send(std::move(test_client), io.acceptor.local_endpoint()));
    while (!io.connected)
        ASSERT_FALSE(called);

    const std::uint8_t expected_bytes[] = {
        4, 1, 0x1f, 0x90, 0x00, 0x00, 0x00, 0x01, 0x00,
        'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm', 0x00
    };

    std::uint8_t actual_bytes[sizeof(expected_bytes)];
    boost::asio::read(io.server, boost::asio::buffer(actual_bytes));
    EXPECT_TRUE(std::memcmp(expected_bytes, actual_bytes, sizeof(actual_bytes)) == 0);

    const std::uint8_t reply_bytes[] = {0, 90, 0, 0, 0, 0, 0, 0};
    boost::asio::write(io.server, boost::asio::buffer(reply_bytes));

    // yikes!
    while (!called);
}

TEST(socks_client, connect_command_failed)
{
    io_thread io{};
    stream_type::socket client{io.io_service};

    std::atomic<bool> called{false};
    auto test_client = net::socks::make_connect_client(
        std::move(client), net::socks::version::v4, checked_client{std::addressof(called), true}
    );
    ASSERT_TRUE(bool(test_client));

    ASSERT_TRUE(
        test_client->set_connect_command(
            epee::net_utils::ipv4_network_address{boost::endian::native_to_big(std::uint32_t(5000)), 3000}
        )
    );
    EXPECT_FALSE(test_client->buffer().empty());
    ASSERT_TRUE(net::socks::client::connect_and_send(std::move(test_client), io.acceptor.local_endpoint()));
    while (!io.connected)
        ASSERT_FALSE(called);

    const std::uint8_t expected_bytes[] = {
        4, 1, 0x0b, 0xb8, 0x00, 0x00, 0x13, 0x88, 0x00
    };

    std::uint8_t actual_bytes[sizeof(expected_bytes)];
    boost::asio::read(io.server, boost::asio::buffer(actual_bytes));
    EXPECT_TRUE(std::memcmp(expected_bytes, actual_bytes, sizeof(actual_bytes)) == 0);

    const std::uint8_t reply_bytes[] = {0, 91, 0, 0, 0, 0, 0, 0};
    boost::asio::write(io.server, boost::asio::buffer(reply_bytes));

    // yikes!
    while (!called);
}

TEST(socks_client, resolve_command)
{
    static std::uint8_t reply_bytes[] = {0, 90, 0, 0, 0xff, 0, 0xad, 0};

    struct resolve_client : net::socks::client
    {
        std::atomic<unsigned> called_;
        bool expected_;

        resolve_client(stream_type::socket&& proxy)
          : net::socks::client(std::move(proxy), net::socks::version::v4a_tor)
          , called_(0)
          , expected_(false)
        {};

        virtual void done(boost::system::error_code error, std::shared_ptr<client> self) override
        {
            EXPECT_EQ(this, self.get());
            EXPECT_EQ(expected_, bool(error)) << "Resolve failure: " << error.message();

            if (!error)
            {
                ASSERT_EQ(sizeof(reply_bytes), buffer().size());
                EXPECT_EQ(0u, std::memcmp(buffer().data(), reply_bytes, sizeof(reply_bytes)));
            }

            ++called_;
        }
    };

    io_thread io{};
    stream_type::socket client{io.io_service};

    auto test_client = std::make_shared<resolve_client>(std::move(client));
    ASSERT_TRUE(bool(test_client));

    ASSERT_TRUE(test_client->set_resolve_command("example.com"));
    EXPECT_FALSE(test_client->buffer().empty());
    ASSERT_TRUE(net::socks::client::connect_and_send(test_client, io.acceptor.local_endpoint()));
    while (!io.connected)
        ASSERT_EQ(0u, test_client->called_);

    const std::uint8_t expected_bytes[] = {
        4, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
        'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm', 0x00
    };

    std::uint8_t actual_bytes[sizeof(expected_bytes)];
    boost::asio::read(io.server, boost::asio::buffer(actual_bytes));
    EXPECT_TRUE(std::memcmp(expected_bytes, actual_bytes, sizeof(actual_bytes)) == 0);

    boost::asio::write(io.server, boost::asio::buffer(reply_bytes));

    // yikes!
    while (test_client->called_ == 0);

    test_client->expected_ = true;
    ASSERT_TRUE(test_client->set_resolve_command("example.com"));
    EXPECT_FALSE(test_client->buffer().empty());
    ASSERT_TRUE(net::socks::client::send(test_client));

    boost::asio::read(io.server, boost::asio::buffer(actual_bytes));
    EXPECT_TRUE(std::memcmp(expected_bytes, actual_bytes, sizeof(actual_bytes)) == 0);

    reply_bytes[1] = 91;
    boost::asio::write(io.server, boost::asio::buffer(reply_bytes));

    // yikes!
    while (test_client->called_ == 1);
}

TEST(socks_connector, host)
{
    io_thread io{};
    boost::asio::steady_timer timeout{io.io_service};
    timeout.expires_from_now(std::chrono::seconds{5});

    boost::unique_future<boost::asio::ip::tcp::socket> sock =
        net::socks::connector{io.acceptor.local_endpoint()}("example.com", "8080", timeout);

    while (!io.connected)
        ASSERT_FALSE(sock.is_ready());
    const std::uint8_t expected_bytes[] = {
        4, 1, 0x1f, 0x90, 0x00, 0x00, 0x00, 0x01, 0x00,
        'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm', 0x00
    };

    std::uint8_t actual_bytes[sizeof(expected_bytes)];
    boost::asio::read(io.server, boost::asio::buffer(actual_bytes));
    EXPECT_TRUE(std::memcmp(expected_bytes, actual_bytes, sizeof(actual_bytes)) == 0);

    const std::uint8_t reply_bytes[] = {0, 90, 0, 0, 0, 0, 0, 0};
    boost::asio::write(io.server, boost::asio::buffer(reply_bytes));

    ASSERT_EQ(boost::future_status::ready, sock.wait_for(boost::chrono::seconds{3}));
    EXPECT_TRUE(sock.get().is_open());
}

TEST(socks_connector, ipv4)
{
    io_thread io{};
    boost::asio::steady_timer timeout{io.io_service};
    timeout.expires_from_now(std::chrono::seconds{5});

    boost::unique_future<boost::asio::ip::tcp::socket> sock =
        net::socks::connector{io.acceptor.local_endpoint()}("250.88.125.99", "8080", timeout);

    while (!io.connected)
        ASSERT_FALSE(sock.is_ready());
    const std::uint8_t expected_bytes[] = {
        4, 1, 0x1f, 0x90, 0xfa, 0x58, 0x7d, 0x63, 0x00
    };

    std::uint8_t actual_bytes[sizeof(expected_bytes)];
    boost::asio::read(io.server, boost::asio::buffer(actual_bytes));
    EXPECT_TRUE(std::memcmp(expected_bytes, actual_bytes, sizeof(actual_bytes)) == 0);

    const std::uint8_t reply_bytes[] = {0, 90, 0, 0, 0, 0, 0, 0};
    boost::asio::write(io.server, boost::asio::buffer(reply_bytes));

    ASSERT_EQ(boost::future_status::ready, sock.wait_for(boost::chrono::seconds{3}));
    EXPECT_TRUE(sock.get().is_open());
}

TEST(socks_connector, error)
{
    io_thread io{};
    boost::asio::steady_timer timeout{io.io_service};
    timeout.expires_from_now(std::chrono::seconds{5});

    boost::unique_future<boost::asio::ip::tcp::socket> sock =
        net::socks::connector{io.acceptor.local_endpoint()}("250.88.125.99", "8080", timeout);

    while (!io.connected)
        ASSERT_FALSE(sock.is_ready());
    const std::uint8_t expected_bytes[] = {
        4, 1, 0x1f, 0x90, 0xfa, 0x58, 0x7d, 0x63, 0x00
    };

    std::uint8_t actual_bytes[sizeof(expected_bytes)];
    boost::asio::read(io.server, boost::asio::buffer(actual_bytes));
    EXPECT_TRUE(std::memcmp(expected_bytes, actual_bytes, sizeof(actual_bytes)) == 0);

    const std::uint8_t reply_bytes[] = {0, 91, 0, 0, 0, 0, 0, 0};
    boost::asio::write(io.server, boost::asio::buffer(reply_bytes));

    ASSERT_EQ(boost::future_status::ready, sock.wait_for(boost::chrono::seconds{3}));
    EXPECT_THROW(sock.get().is_open(), boost::system::system_error);
}

TEST(socks_connector, timeout)
{
    io_thread io{};
    boost::asio::steady_timer timeout{io.io_service};
    timeout.expires_from_now(std::chrono::milliseconds{10});

    boost::unique_future<boost::asio::ip::tcp::socket> sock =
        net::socks::connector{io.acceptor.local_endpoint()}("250.88.125.99", "8080", timeout);

    ASSERT_EQ(boost::future_status::ready, sock.wait_for(boost::chrono::seconds{3}));
    EXPECT_THROW(sock.get().is_open(), boost::system::system_error);
}

