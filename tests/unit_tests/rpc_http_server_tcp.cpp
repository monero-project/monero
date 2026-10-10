#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <limits>
#include <string>
#include <thread>
#include <vector>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#ifndef _WIN32
#include <sys/resource.h>
#endif

#include "gtest/gtest.h"
#include "int-util.h"
#include "net/abstract_tcp_server2.h"
#include "rpc/http_server.h"
#include "scope_guard.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "string_tools.h"

// Tests of `rpc::http_server` over real sockets, through `boosted_tcp_server`

namespace
{
namespace beast_http = boost::beast::http;

constexpr const std::size_t payload_size = 26 * 1024 * 1024;

struct dummy
{
    struct request
    {
        BEGIN_KV_SERIALIZE_MAP()
        END_KV_SERIALIZE_MAP()
    };

    struct response
    {
        std::string payload;

        BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payload)
        END_KV_SERIALIZE_MAP()
    };
};

struct dummy_server
{
    explicit dummy_server(const std::size_t size, rpc::server_options options = {})
        : dummy_size(size)
    {
        rpc::router routes;
        routes.add_bin("/dummy", this, &dummy_server::on_dummy);

        options.bind_ip = "127.0.0.1";
        options.ssl = epee::net_utils::ssl_support_t::e_ssl_support_disabled;
        if (!server.init(std::move(routes), std::move(options)))
            throw std::runtime_error{"failed to bind test server"};
    }

    bool on_dummy(const dummy::request&, dummy::response& res, const rpc::connection_context*)
    {
        res.payload.resize(dummy_size.load(), 'f');
        return true;
    }

    boost::asio::ip::tcp::endpoint endpoint()
    {
        return {boost::asio::ip::make_address("127.0.0.1"),
                static_cast<unsigned short>(server.get_binded_port())};
    }

    std::atomic<std::size_t> dummy_size;
    rpc::http_server server; // last, so it stops before `dummy_size` is destroyed
};

beast_http::request<beast_http::string_body> make_request()
{
    dummy::request request{};
    const epee::byte_slice body = epee::serialization::store_t_to_binary(request);

    beast_http::request<beast_http::string_body> req{beast_http::verb::get, "/dummy", 11};
    req.set(beast_http::field::host, "127.0.0.1");
    req.set(beast_http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.body().assign(reinterpret_cast<const char*>(body.data()), body.size());
    req.prepare_payload();
    return req;
}
using connection_t = epee::net_utils::connection<rpc::http_connection>;
using epee::net_utils::detail::get_per_ip_timeout_shift;
} // namespace

// These go through `get_per_ip_timeout_shift`, as `boosted_tcp_server` does, so they also check
// that it finds `server_config::get_timeout_shift` rather than falling back to the full shift
TEST(rpc_http_server_tcp, per_ip_timeout_shift)
{
    connection_t::shared_state state;
    const epee::net_utils::network_address local =
        epee::net_utils::ipv4_network_address{SWAP32LE(0x0100007f), 0};
    const epee::net_utils::network_address remote =
        epee::net_utils::ipv4_network_address{SWAP32LE(0x010200c0), 0};
    state.max_public_ip_connections = 3;
    state.max_private_ip_connections = 200;

    struct test_case
    {
        bool local;
        unsigned count;
        unsigned shift;
        unsigned expected;
    };
    const test_case cases[] = {
        {false, 1, 8, 2},  {false, 2, 8, 5},  {false, 3, 2, 2},  {true, 0, 8, 0},
        {true, 9, 8, 0},   {true, 24, 8, 0},  {true, 25, 8, 1},  {true, 50, 8, 2},
        {true, 121, 8, 4}, {true, 150, 8, 6}, {true, 175, 8, 7}, {true, 199, 8, 7},
        {true, 200, 8, 8}, {true, 201, 8, 8}, {true, 200, 0, 0}};
    for (const auto& test : cases)
    {
        SCOPED_TRACE(test.count);
        const auto& address = test.local ? local : remote;
        state.connections[rpc::connection_limit_key(address)] = test.count;
        EXPECT_EQ(test.expected, get_per_ip_timeout_shift(state, address, test.shift));
    }

    state.max_private_ip_connections = 0;
    EXPECT_EQ(0u, get_per_ip_timeout_shift(state, local, 8));
    state.max_private_ip_connections = std::numeric_limits<unsigned>::max();
    state.connections[local.host_str()] = std::numeric_limits<unsigned>::max();
    EXPECT_EQ(8u, get_per_ip_timeout_shift(state, local, 8));
}

TEST(rpc_http_server_tcp, per_ip_timeout_shift_groups_public_ipv6)
{
    using boost::asio::ip::make_address_v6;
    using epee::net_utils::ipv6_network_address;
    connection_t::shared_state state;
    state.max_public_ip_connections = 200;
    state.max_private_ip_connections = 25;
    state.connections["2001:db8:abcd:1234::/64"] = 25;

    const ipv6_network_address first{make_address_v6("2001:db8:abcd:1234::1"), 1000};
    const ipv6_network_address second{make_address_v6("2001:db8:abcd:1234::2"), 2000};
    const ipv6_network_address other{make_address_v6("2001:db8:abcd:1235::1"), 1000};
    EXPECT_EQ(1u, get_per_ip_timeout_shift(state, first, 8));
    EXPECT_EQ(1u, get_per_ip_timeout_shift(state, second, 8));
    EXPECT_EQ(0u, get_per_ip_timeout_shift(state, other, 8));
    EXPECT_EQ(0u, get_per_ip_timeout_shift(state, first, 0));
    EXPECT_EQ(1u, state.connections.size());

    state.connections["2001:db8:abcd:1234::/64"] = 200;
    EXPECT_EQ(8u, get_per_ip_timeout_shift(state, second, 8));
    EXPECT_EQ(2u, get_per_ip_timeout_shift(state, second, 2));

    state.connections["::1"] = 25;
    EXPECT_EQ(8u, get_per_ip_timeout_shift(state, ipv6_network_address{make_address_v6("::1"), 0}, 8));
    state.connections["fe80::1"] = 25;
    EXPECT_EQ(8u,
              get_per_ip_timeout_shift(state, ipv6_network_address{make_address_v6("fe80::1"), 0}, 8));
    EXPECT_EQ(0u,
              get_per_ip_timeout_shift(state, ipv6_network_address{make_address_v6("fe80::2"), 0}, 8));
}

TEST(rpc_http_server_tcp, timeout_shift_respects_server_capacity)
{
    const epee::net_utils::network_address address =
        epee::net_utils::ipv4_network_address{SWAP32LE(0x0100007f), 0};
    connection_t::shared_state state;
    state.max_private_ip_connections = 100;
    state.max_connections = 200;
    state.connections[address.host_str()] = 11;

    struct test_case
    {
        std::size_t count;
        unsigned expected;
    };
    const test_case cases[] = {{11, 0},  {24, 0},  {25, 1},  {50, 2},  {99, 3},
                               {100, 4}, {124, 4}, {125, 5}, {149, 5}, {150, 6},
                               {174, 6}, {175, 7}, {199, 7}, {200, 8}, {201, 8}};
    for (const auto& test : cases)
    {
        SCOPED_TRACE(test.count);
        state.connection_count = test.count;
        for (unsigned shift = 0; shift <= 8; ++shift)
        {
            SCOPED_TRACE(shift);
            EXPECT_EQ(std::min(shift, test.expected),
                      get_per_ip_timeout_shift(state, address, shift));
        }
    }

    state.connection_count = 199;
    state.max_private_ip_connections = 0;
    EXPECT_EQ(7u, get_per_ip_timeout_shift(state, address, 8));
    state.connections.clear();
    EXPECT_EQ(7u, get_per_ip_timeout_shift(state, address, 8));
    EXPECT_TRUE(state.connections.empty());

    state.max_connections = std::numeric_limits<unsigned>::max();
    state.connection_count = state.max_connections - 1;
    EXPECT_EQ(7u, get_per_ip_timeout_shift(state, address, 8));
    state.max_connections = state.connection_count = 0;
    EXPECT_EQ(8u, get_per_ip_timeout_shift(state, address, 8));
    EXPECT_EQ(0u, get_per_ip_timeout_shift(state, address, 0));

    state.max_connections = 1000;
    state.connection_count = 125;
    state.max_private_ip_connections = 100;
    state.connections[address.host_str()] = 50;
    EXPECT_EQ(4u, get_per_ip_timeout_shift(state, address, 8));
}

TEST(rpc_http_server_tcp, initial_timeout_releases_capacity)
{
#ifndef _WIN32
    // room for both client and server sockets; the original limit is restored after
    rlimit file_limit;
    ASSERT_EQ(0, getrlimit(RLIMIT_NOFILE, &file_limit));
    const epee::scope_guard restore_file_limit([file_limit]
    {
        if (file_limit.rlim_cur < 1024)
        {
            EXPECT_EQ(0, setrlimit(RLIMIT_NOFILE, &file_limit));
        }
    });
    if (file_limit.rlim_cur < 1024)
    {
        ASSERT_GE(file_limit.rlim_max, 512u) << "This test needs at least 512 file descriptors";
        rlimit raised_limit = file_limit;
        raised_limit.rlim_cur = std::min<rlim_t>(1024, file_limit.rlim_max);
        ASSERT_EQ(0, setrlimit(RLIMIT_NOFILE, &raised_limit));
    }
#endif

    rpc::server_options options{};
    options.max_connections = 200;
    options.max_public_ip_connections = 200;
    options.max_private_ip_connections = 200;
    dummy_server server{1, std::move(options)};
    ASSERT_TRUE(server.server.run(1, false));

    boost::system::error_code error{};
    boost::asio::io_context context{};
    std::vector<boost::asio::ip::tcp::socket> streams;
    for (std::size_t i = 0; i < 204; ++i)
    {
        streams.emplace_back(context);
        streams.back().connect(server.endpoint(), error);
    }
    // connections over the limit are refused, so the count settles at the limit
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (server.server.get_connections_count() < 200 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    ASSERT_EQ(200, server.server.get_connections_count());

    // at capacity the last connection must use the counted load: 7 seconds, not 14
    std::this_thread::sleep_for(std::chrono::seconds{10});
    ASSERT_LT(server.server.get_connections_count(), 200);
    EXPECT_GE(server.server.get_connections_count(), 150);

    boost::asio::ip::tcp::socket stream{context};
    stream.connect(server.endpoint(), error);
    ASSERT_FALSE(bool(error));
    beast_http::write(stream, make_request(), error);
    ASSERT_FALSE(bool(error));

    boost::beast::flat_buffer buffer;
    beast_http::response_parser<beast_http::basic_string_body<char>> parser;
    beast_http::read(stream, buffer, parser, error);
    EXPECT_FALSE(bool(error));
    ASSERT_TRUE(parser.is_done());
    EXPECT_EQ(200u, parser.get().result_int());
    server.server.send_stop_signal();
}

TEST(rpc_http_server_tcp, rpc_incomplete_request_expires)
{
    using tcp = boost::asio::ip::tcp;

    boost::asio::io_context context;
    tcp::acceptor acceptor{context, tcp::endpoint{boost::asio::ip::make_address("127.0.0.1"), 0}};
    tcp::socket peer{context};
    peer.connect(acceptor.local_endpoint());
    tcp::socket socket{context};
    acceptor.accept(socket);
    const auto shared = std::make_shared<connection_t::shared_state>();
    const auto connection = std::make_shared<connection_t>(
        context, std::move(socket), shared, epee::net_utils::e_connection_type_RPC,
        epee::net_utils::ssl_support_t::e_ssl_support_disabled);
    std::uint32_t ip = 0;
    ASSERT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "8.8.8.8"));
    ASSERT_TRUE(connection->start(
        true, false, epee::net_utils::ipv4_network_address{ip, acceptor.local_endpoint().port()}));

    const std::string partial = "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nx";
    boost::asio::write(peer, boost::asio::buffer(partial));
    // a partially received request must keep its remote timeout
    context.run_for(std::chrono::seconds{12});
    EXPECT_EQ(connection_t::WASTED, connection->get_status());
    context.stop();
}

TEST(rpc_http_server_tcp, response_soft_limit)
{
    dummy_server server{payload_size};
    ASSERT_TRUE(server.server.run(1, false));

    boost::system::error_code error{};
    boost::asio::io_context context{};
    boost::asio::ip::tcp::socket stream{context};
    stream.connect(server.endpoint(), error);
    ASSERT_FALSE(bool(error));

    const auto req = make_request();
    // Reading a complete oversized response must allow another on the same keep-alive
    // connection, even with only one worker available.
    for (unsigned i = 0; i < 4; ++i)
    {
        beast_http::write(stream, req, error);
        ASSERT_FALSE(bool(error));

        dummy::response payload{};
        boost::beast::flat_buffer buffer;
        beast_http::response_parser<beast_http::basic_string_body<char>> parser;
        parser.body_limit(payload_size + 1024);
        beast_http::read(stream, buffer, parser, error);
        EXPECT_FALSE(bool(error));
        ASSERT_TRUE(parser.is_done());
        const auto res = parser.release();
        EXPECT_EQ(200u, res.result_int());
        EXPECT_TRUE(epee::serialization::load_t_from_binary(payload, res.body()));
        EXPECT_EQ(payload_size, std::count(payload.payload.begin(), payload.payload.end(), 'f'));
    }

    // without reading, queued responses go over the soft limit and the server closes
    while (!error)
        beast_http::write(stream, req, error);
    server.server.send_stop_signal();
}

TEST(rpc_http_server_tcp, response_soft_limit_below_header)
{
    // Each response is still let through whole, as the old server did
    rpc::server_options options{};
    options.response_soft_limit = 0;
    constexpr const std::size_t size = 1024 * 1024;
    dummy_server server{size, std::move(options)};
    ASSERT_TRUE(server.server.run(1, false));

    boost::system::error_code error{};
    boost::asio::io_context context{};
    boost::asio::ip::tcp::socket stream{context};
    stream.connect(server.endpoint(), error);
    ASSERT_FALSE(bool(error));

    const auto req = make_request();
    for (unsigned i = 0; i < 2; ++i)
    {
        beast_http::write(stream, req, error);
        ASSERT_FALSE(bool(error));

        dummy::response payload{};
        boost::beast::flat_buffer buffer;
        beast_http::response_parser<beast_http::basic_string_body<char>> parser;
        parser.body_limit(size + 1024);
        beast_http::read(stream, buffer, parser, error);
        ASSERT_FALSE(bool(error)) << error.message();
        ASSERT_TRUE(parser.is_done());
        const auto res = parser.release();
        EXPECT_EQ(200u, res.result_int());
        EXPECT_TRUE(epee::serialization::load_t_from_binary(payload, res.body()));
        EXPECT_EQ(size, std::count(payload.payload.begin(), payload.payload.end(), 'f'));
    }
    server.server.send_stop_signal();
}

TEST(rpc_http_server_tcp, private_ip_limit)
{
    dummy_server server{1};
    ASSERT_TRUE(server.server.run(1, false));

    boost::system::error_code error{};
    boost::asio::io_context context{};
    const auto req = make_request();

    std::vector<boost::asio::ip::tcp::socket> streams{};
    for (std::size_t i = 0; i < DEFAULT_RPC_MAX_CONNECTIONS_PER_PRIVATE_IP; ++i)
    {
        streams.emplace_back(context);
        streams.back().connect(server.endpoint(), error);
        beast_http::write(streams.back(), req, error);
        EXPECT_FALSE(bool(error));

        boost::beast::flat_buffer buffer;
        beast_http::response_parser<beast_http::basic_string_body<char>> parser;
        beast_http::read(streams.back(), buffer, parser, error);
        EXPECT_FALSE(bool(error));
        EXPECT_TRUE(parser.is_done());
    }

    boost::asio::ip::tcp::socket stream{context};
    stream.connect(server.endpoint(), error);
    bool failed = bool(error);
    beast_http::write(stream, req, error);
    failed |= bool(error);
    {
        boost::beast::flat_buffer buffer;
        beast_http::response_parser<beast_http::basic_string_body<char>> parser;
        // make sure the server ran its accept code
        beast_http::read(stream, buffer, parser, error);
    }
    failed |= bool(error);
    EXPECT_TRUE(failed);
    server.server.send_stop_signal();
}

TEST(rpc_http_server_tcp, read_then_close)
{
    dummy_server server{200'000};
    ASSERT_TRUE(server.server.run(2, false)); // need at least 2 threads to trigger issues

    auto req = make_request();
    req.set(beast_http::field::connection, "close");

    bool failed_read = false;
    bool closed_all_connections = true;
    for (std::size_t j = 0; j < 1000; ++j)
    {
        boost::system::error_code error{};
        boost::asio::io_context context{};
        boost::asio::ip::tcp::socket stream{context};
        stream.connect(server.endpoint(), error);
        EXPECT_FALSE(bool(error));

        beast_http::write(stream, req, error);
        EXPECT_FALSE(bool(error));

        dummy::response payload{};
        boost::beast::flat_buffer buffer;
        beast_http::response_parser<beast_http::basic_string_body<char>> parser;
        parser.body_limit(server.dummy_size + 1024);
        beast_http::read(stream, buffer, parser, error);

        // keep going after a failed read, to check the server copes
        failed_read |= bool(error) || !parser.is_done();
        if (failed_read) continue;

        const auto res = parser.release();
        failed_read |= res.result_int() != 200u ||
                       !epee::serialization::load_t_from_binary(payload, res.body()) ||
                       server.dummy_size != std::size_t(std::count(payload.payload.begin(),
                                                                   payload.payload.end(), 'f'));

        // the server closes after sending the whole response
        char buf[1];
        stream.read_some(boost::asio::buffer(buf), error);
        closed_all_connections &= error == boost::asio::error::eof;
    }

    EXPECT_FALSE(failed_read);
    EXPECT_TRUE(closed_all_connections);
    server.server.send_stop_signal();
}
