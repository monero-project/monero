#include <algorithm>
#include <atomic>
#include <cstddef>
#include <string>
#include <vector>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "gtest/gtest.h"
#include "rpc/http_server.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"

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
    explicit dummy_server(const std::size_t size)
        : dummy_size(size)
    {
        rpc::router routes;
        routes.add_bin("/dummy", this, &dummy_server::on_dummy);

        rpc::server_options options{};
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
} // namespace

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
