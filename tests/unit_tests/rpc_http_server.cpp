#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address_v6.hpp>

#include "gtest/gtest.h"
#include "net/net_utils_base.h"
#include "rpc/http_server.h"
#include "string_tools.h"

namespace
{
class test_endpoint final : public epee::net_utils::i_service_endpoint
{
public:
    bool do_send(epee::byte_slice message) override
    {
        if (fail_send) return false;
        sent.append(reinterpret_cast<const char*>(message.data()), message.size());
        return true;
    }
    bool close(const bool) override { return true; }
    bool send_done() override { return true; }
    bool call_run_once_service_io() override { return true; }
    bool request_callback() override { return true; }
    boost::asio::io_context& get_io_context() override { return io_context; }

    boost::asio::io_context io_context;
    std::string sent;
    bool fail_send = false;
};

struct captured_request
{
    std::string method, target, body;
};

struct capture
{
    std::vector<bool> results;
    std::vector<captured_request> requests;
    std::string sent;
};

std::shared_ptr<const rpc::router> make_router(std::vector<captured_request>& requests)
{
    auto routes = std::make_shared<rpc::router>();
    routes->add("/echo", [&requests](const rpc::http_request& q, const rpc::connection_context&)
    {
        requests.push_back({std::string{q.method}, std::string{q.target}, std::string{q.body}});
        rpc::http_response r{};
        r.body = epee::byte_slice{std::string{q.body}};
        r.content_type = "text/plain";
        return r;
    });
    routes->add("/throw",
                [](const rpc::http_request&, const rpc::connection_context&) -> rpc::http_response
    {
        throw std::runtime_error{"test"};
    });
    routes->add("/fail", [](const rpc::http_request&, const rpc::connection_context&)
    {
        return rpc::http_response{500};
    });
    return routes;
}

capture feed_chunks(const std::vector<std::string>& chunks,
                    const std::size_t max_content_length = std::numeric_limits<std::size_t>::max())
{
    capture out{};
    test_endpoint endpoint;
    epee::net_utils::connection_context_base context;
    rpc::server_config config;
    config.routes = make_router(out.requests);
    config.max_content_length = max_content_length;

    rpc::http_connection connection{&endpoint, config, context};
    for (const std::string& chunk : chunks)
    {
        out.results.push_back(connection.handle_recv(chunk.data(), chunk.size()));
        if (!out.results.back()) break; // transport closes the connection here
    }
    out.sent = std::move(endpoint.sent);
    return out;
}

capture feed(const std::string& request)
{
    return feed_chunks(std::vector<std::string>{request});
}

std::size_t count(const std::string_view haystack, const std::string_view needle)
{
    std::size_t n = 0;
    for (std::size_t pos = haystack.find(needle); pos != std::string_view::npos;
         pos = haystack.find(needle, pos + needle.size()))
        ++n;
    return n;
}

bool starts_with(const std::string_view s, const std::string_view prefix)
{
    return s.substr(0, prefix.size()) == prefix;
}

bool contains(const std::string_view s, const std::string_view part)
{
    return s.find(part) != std::string_view::npos;
}

// expects a single status-only response that closes the connection
void expect_rejected(const capture& c, const std::string_view status_line)
{
    ASSERT_EQ(1u, c.results.size());
    EXPECT_FALSE(c.results[0]);
    EXPECT_TRUE(c.requests.empty());
    EXPECT_TRUE(starts_with(c.sent, status_line)) << c.sent;
    EXPECT_TRUE(contains(c.sent, "\r\nConnection: close\r\n")) << c.sent;
    EXPECT_EQ(1u, count(c.sent, "HTTP/1.1 ")) << c.sent;
}

epee::net_utils::network_address make_ipv6(const char* const address)
{
    return epee::net_utils::ipv6_network_address{boost::asio::ip::make_address_v6(address), 18081};
}

const std::string get_echo = "GET /echo HTTP/1.1\r\nHost: x\r\n\r\n";
} // namespace

TEST(rpc_http_server, body_with_content_length)
{
    const auto c = feed("POST /echo HTTP/1.1\r\nContent-Length: 10\r\nHost: example.com\r\n\r\n"
                        "0123456789");

    ASSERT_EQ(1u, c.results.size());
    EXPECT_TRUE(c.results[0]);
    ASSERT_EQ(1u, c.requests.size());
    EXPECT_EQ("POST", c.requests[0].method);
    EXPECT_EQ("/echo", c.requests[0].target);
    EXPECT_EQ("0123456789", c.requests[0].body);
    EXPECT_TRUE(starts_with(c.sent, "HTTP/1.1 200 OK\r\n"));
    EXPECT_TRUE(contains(c.sent, "\r\nContent-Type: text/plain\r\n"));
    EXPECT_FALSE(contains(c.sent, "Connection: close"));
    EXPECT_EQ("\r\n\r\n0123456789", c.sent.substr(c.sent.size() - 14));
}

TEST(rpc_http_server, split_request_line)
{
    const auto c =
        feed_chunks({"POST /echo HTTP/1.1\r\n", "Host: x\r\nContent-Length: 10\r\n\r\n0123456789"});

    ASSERT_EQ(2u, c.results.size());
    EXPECT_TRUE(c.results[0]);
    EXPECT_TRUE(c.results[1]);
    ASSERT_EQ(1u, c.requests.size());
    EXPECT_EQ("0123456789", c.requests[0].body);
}

TEST(rpc_http_server, split_header_terminator)
{
    for (const std::size_t split : {1u, 2u, 3u})
    {
        const std::size_t at = get_echo.size() - split;
        const auto c = feed_chunks({get_echo.substr(0, at), get_echo.substr(at)});

        ASSERT_EQ(2u, c.results.size()) << split;
        EXPECT_TRUE(c.results[0]) << split;
        EXPECT_TRUE(c.results[1]) << split;
        EXPECT_EQ(1u, c.requests.size()) << split;
    }
}

TEST(rpc_http_server, one_byte_at_a_time)
{
    const std::string request = "POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\ntest";
    std::vector<std::string> chunks;
    for (const char c : request)
        chunks.emplace_back(1, c);

    const auto c = feed_chunks(chunks);
    ASSERT_EQ(request.size(), c.results.size());
    for (const bool result : c.results)
        EXPECT_TRUE(result);
    ASSERT_EQ(1u, c.requests.size());
    EXPECT_EQ("test", c.requests[0].body);
}

TEST(rpc_http_server, pipelined_requests)
{
    const std::string first = "POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 3\r\n\r\none";
    const std::string second = "POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 3\r\n\r\ntwo";

    const auto together = feed(first + second);
    ASSERT_EQ(1u, together.results.size());
    EXPECT_TRUE(together.results[0]);
    ASSERT_EQ(2u, together.requests.size());
    EXPECT_EQ("one", together.requests[0].body);
    EXPECT_EQ("two", together.requests[1].body);
    EXPECT_EQ(2u, count(together.sent, "HTTP/1.1 200 OK\r\n"));

    const std::size_t half = second.size() / 2;
    const auto split = feed_chunks({first + second.substr(0, half), second.substr(half)});
    ASSERT_EQ(2u, split.results.size());
    EXPECT_TRUE(split.results[0]);
    EXPECT_TRUE(split.results[1]);
    ASSERT_EQ(2u, split.requests.size());
    EXPECT_EQ("two", split.requests[1].body);
}

TEST(rpc_http_server, rejects_malformed_requests)
{
    for (const std::string request :
         {"POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 5abc\r\n\r\n0123456789",
          "POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 0\r\nContent-Length: 4\r\n\r\ntest",
          "POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 0, 4\r\n\r\ntest",
          "POST /echo HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: "
          "chunked\r\n\r\n4\r\ntest\r\n0\r\n\r\n",
          "POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\nTransfer-Encoding: "
          "chunked\r\n\r\n"
          "test",
          "GET /echo HTTP/1.1\r\nBad Header Without Colon\r\nHost: x\r\n\r\n",
          "GET /echo HTTP/1.1\r\nHost: x\r\nBad Header Without Colon\r\n\r\n",
          "GET /echo HTTP/1.1\r\nHost: x\r\n Content-Length: 4\r\n\r\ntest",
          "GET /echo HTTP/1.1\r\nHost: x\r\nX: a\nContent-Length: 4\r\n\r\ntest",
          "GET /echo\r\nHost: x\r\n\r\n", "GET /echo HTTP/2.0\r\nHost: x\r\n\r\n",
          "\nGET /echo HTTP/1.1\r\nHost: x\r\n\r\n", "\r\n\nGET /echo HTTP/1.1\r\nHost: x\r\n\r\n",
          "\rGET /echo HTTP/1.1\r\nHost: x\r\n\r\n"})
    {
        SCOPED_TRACE(request);
        expect_rejected(feed(request), "HTTP/1.1 400 Bad request\r\n");
    }
}

TEST(rpc_http_server, leading_empty_lines)
{
    const auto one = feed("\r\n" + get_echo);
    ASSERT_EQ(1u, one.results.size());
    EXPECT_TRUE(one.results[0]);
    EXPECT_EQ(1u, one.requests.size());

    const auto max = feed("\r\n\r\n\r\n\r\n" + get_echo);
    ASSERT_EQ(1u, max.results.size());
    EXPECT_TRUE(max.results[0]);
    EXPECT_EQ(1u, max.requests.size());

    expect_rejected(feed("\r\n\r\n\r\n\r\n\r\n" + get_echo), "HTTP/1.1 400 Bad request\r\n");

    // only empty lines so far: wait for the request
    const auto waiting = feed("\r\n\r\n");
    ASSERT_EQ(1u, waiting.results.size());
    EXPECT_TRUE(waiting.results[0]);
    EXPECT_TRUE(waiting.sent.empty());

    const auto split = feed_chunks({"\r", "\n\r", "\n", get_echo});
    ASSERT_EQ(4u, split.results.size());
    for (const bool result : split.results)
        EXPECT_TRUE(result);
    EXPECT_EQ(1u, split.requests.size());

    // the count is per request, and a stray CRLF after a body doesn't break the next request
    const auto pipelined =
        feed("POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\ntest\r\n" + get_echo +
             "\r\n\r\n\r\n\r\n" + get_echo);
    ASSERT_EQ(1u, pipelined.results.size());
    EXPECT_TRUE(pipelined.results[0]);
    EXPECT_EQ(3u, pipelined.requests.size());
}

TEST(rpc_http_server, unknown_target)
{
    const auto c = feed("GET /nope HTTP/1.1\r\nHost: x\r\n\r\n");
    ASSERT_EQ(1u, c.results.size());
    EXPECT_TRUE(c.results[0]);
    EXPECT_TRUE(starts_with(c.sent, "HTTP/1.1 404 Not found\r\n"));
    EXPECT_TRUE(contains(c.sent, "\r\nContent-Length: 0\r\n"));
    // the old server's default
    EXPECT_TRUE(contains(c.sent, "\r\nContent-Type: text/plain\r\n"));
}

TEST(rpc_http_server, routes_on_path)
{
    for (const std::string target :
         {"/echo?a=b", "http://example.com/echo", "HTTPS://example.com:18081/echo?x"})
    {
        const auto c = feed("GET " + target + " HTTP/1.1\r\nHost: x\r\n\r\n");
        ASSERT_EQ(1u, c.requests.size()) << target;
        // handlers still see the full target
        EXPECT_EQ(target, c.requests[0].target);
    }

    for (const std::string target : {"ftp://example.com/echo", "http://example.com?echo", "echo"})
    {
        const auto c = feed("GET " + target + " HTTP/1.1\r\nHost: x\r\n\r\n");
        EXPECT_TRUE(c.requests.empty()) << target;
        EXPECT_TRUE(starts_with(c.sent, "HTTP/1.1 404 Not found\r\n")) << target;
    }
}

TEST(rpc_http_server, connection_close)
{
    const auto c = feed("GET /echo HTTP/1.1\r\nHost: x\r\nConnection: Close\r\n\r\n" + get_echo);
    ASSERT_EQ(1u, c.results.size());
    EXPECT_FALSE(c.results[0]);
    EXPECT_EQ(1u, c.requests.size());
    EXPECT_EQ(1u, count(c.sent, "HTTP/1.1 "));
    EXPECT_TRUE(contains(c.sent, "\r\nConnection: close\r\n"));
}

TEST(rpc_http_server, connection_options)
{
    // RFC 9110 7.6.1: `Connection` is a list
    const auto listed =
        feed("GET /echo HTTP/1.1\r\nHost: x\r\nConnection: keep-alive, close\r\n\r\n");
    ASSERT_EQ(1u, listed.results.size());
    EXPECT_FALSE(listed.results[0]);
    EXPECT_TRUE(contains(listed.sent, "\r\nConnection: close\r\n"));

    // RFC 9112 9.3: HTTP/1.0 closes unless asked to keep alive
    const auto http10 = feed("GET /echo HTTP/1.0\r\n\r\n");
    ASSERT_EQ(1u, http10.results.size());
    EXPECT_FALSE(http10.results[0]);
    EXPECT_EQ(1u, http10.requests.size());

    const auto http10_kept = feed("GET /echo HTTP/1.0\r\nConnection: Keep-Alive\r\n\r\n");
    ASSERT_EQ(1u, http10_kept.results.size());
    EXPECT_TRUE(http10_kept.results[0]);
}

TEST(rpc_http_server, host_required)
{
    expect_rejected(feed("GET /echo HTTP/1.1\r\n\r\n"), "HTTP/1.1 400 Bad request\r\n");
    expect_rejected(feed("GET /echo HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n"),
                    "HTTP/1.1 400 Bad request\r\n");
    expect_rejected(feed("GET /echo HTTP/1.0\r\nHost: a\r\nHost: b\r\n\r\n"),
                    "HTTP/1.1 400 Bad request\r\n");

    // HTTP/1.0 doesn't require it
    const auto http10 = feed("GET /echo HTTP/1.0\r\n\r\n");
    EXPECT_EQ(1u, http10.requests.size());
}

TEST(rpc_http_server, head)
{
    const auto c =
        feed("HEAD /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\ntest" + get_echo);
    ASSERT_EQ(1u, c.results.size());
    EXPECT_TRUE(c.results[0]);
    ASSERT_EQ(2u, c.requests.size());
    EXPECT_EQ("HEAD", c.requests[0].method);

    // the body's length is reported but the body itself isn't sent, so the next response
    // follows straight after the first header block
    const std::size_t second = c.sent.find("HTTP/1.1 200 OK\r\n", 1);
    ASSERT_NE(std::string::npos, second);
    const std::string_view first{c.sent.data(), second};
    EXPECT_TRUE(contains(first, "\r\nContent-Length: 4\r\n"));
    EXPECT_EQ("\r\n\r\n", first.substr(first.size() - 4));
}

TEST(rpc_http_server, options)
{
    const auto c = feed("OPTIONS /echo HTTP/1.1\r\nHost: x\r\n\r\n");
    ASSERT_EQ(1u, c.results.size());
    EXPECT_TRUE(c.results[0]);
    EXPECT_TRUE(c.requests.empty());
    EXPECT_TRUE(starts_with(c.sent, "HTTP/1.1 200 OK\r\n"));
}

TEST(rpc_http_server, method_not_allowed)
{
    for (const std::string method : {"PUT", "DELETE", "TRACE", "FOO", "get", "Post"})
    {
        SCOPED_TRACE(method);
        const auto c = feed(method + " /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\ntest");
        expect_rejected(c, "HTTP/1.1 405 Method Not Allowed\r\n");
        EXPECT_TRUE(contains(c.sent, "\r\nAllow: GET, HEAD, POST, OPTIONS\r\n"));
    }
}

TEST(rpc_http_server, expect_continue)
{
    const std::string header =
        "POST /echo HTTP/1.1\r\nHost: x\r\nExpect: 100-continue\r\nContent-Length: 4\r\n\r\n";

    // body not sent yet: the client is told to go ahead
    const auto waiting = feed_chunks({header, "test"});
    ASSERT_EQ(2u, waiting.results.size());
    EXPECT_TRUE(waiting.results[0]);
    EXPECT_TRUE(waiting.results[1]);
    ASSERT_EQ(1u, waiting.requests.size());
    EXPECT_TRUE(starts_with(waiting.sent, "HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\n"));

    // body already here: no interim response
    const auto sent_together = feed(header + "test");
    EXPECT_TRUE(starts_with(sent_together.sent, "HTTP/1.1 200 OK\r\n"));

    // ignored for HTTP/1.0
    const auto http10 = feed_chunks(
        {"POST /echo HTTP/1.0\r\nExpect: 100-continue\r\nContent-Length: 4\r\n\r\n", "test"});
    EXPECT_TRUE(starts_with(http10.sent, "HTTP/1.1 200 OK\r\n"));

    expect_rejected(feed("POST /echo HTTP/1.1\r\nHost: x\r\nExpect: something\r\n\r\n"),
                    "HTTP/1.1 417 Expectation Failed\r\n");
}

TEST(rpc_http_server, handler_exception)
{
    const auto c = feed("GET /throw HTTP/1.1\r\nHost: x\r\n\r\n");
    ASSERT_EQ(1u, c.results.size());
    EXPECT_FALSE(c.results[0]);
    EXPECT_TRUE(starts_with(c.sent, "HTTP/1.1 500 Internal Server Error\r\n"));
    EXPECT_TRUE(contains(c.sent, "\r\nConnection: close\r\n"));
}

TEST(rpc_http_server, internal_error_closes)
{
    // as the old server did, any 500 closes, not only exceptions
    const auto c = feed("GET /fail HTTP/1.1\r\nHost: x\r\n\r\n" + get_echo);
    ASSERT_EQ(1u, c.results.size());
    EXPECT_FALSE(c.results[0]);
    EXPECT_TRUE(c.requests.empty());
    EXPECT_EQ(1u, count(c.sent, "HTTP/1.1 "));
    EXPECT_TRUE(contains(c.sent, "\r\nConnection: close\r\n"));
}

TEST(rpc_http_server, header_too_large)
{
    const std::string start = "GET /echo HTTP/1.1\r\nHost: x\r\nX: ";
    expect_rejected(feed_chunks({start + std::string(100'000, 'a'), "\r\n\r\n"}),
                    "HTTP/1.1 431 Request Header Fields Too Large\r\n");
    expect_rejected(feed(start + std::string(100'000, 'a') + "\r\n\r\n"),
                    "HTTP/1.1 431 Request Header Fields Too Large\r\n");
}

TEST(rpc_http_server, header_limit_with_split_terminator)
{
    std::string header = "GET /echo HTTP/1.1\r\nHost: x\r\nX: ";
    header.append(100'000 - header.size(), 'a');

    // a maximum-size header is accepted however its terminator is split
    for (const std::size_t split : {0u, 1u, 2u, 3u})
    {
        const std::string terminator = "\r\n\r\n";
        const auto c =
            feed_chunks({header + terminator.substr(0, split), terminator.substr(split)});
        EXPECT_EQ(1u, c.requests.size()) << split;
    }
}

TEST(rpc_http_server, content_too_large)
{
    const std::string request =
        "POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 10\r\n\r\n0123456789";
    expect_rejected(feed_chunks({request}, request.size() - 1),
                    "HTTP/1.1 413 Content Too Large\r\n");

    const auto accepted = feed_chunks({request}, request.size());
    ASSERT_EQ(1u, accepted.results.size());
    EXPECT_TRUE(accepted.results[0]);
    EXPECT_EQ(1u, accepted.requests.size());
}

TEST(rpc_http_server, send_failure_closes)
{
    test_endpoint endpoint;
    endpoint.fail_send = true;
    epee::net_utils::connection_context_base context;
    rpc::server_config config;

    rpc::http_connection connection{&endpoint, config, context};
    const std::string request = get_echo + get_echo;
    EXPECT_FALSE(connection.handle_recv(request.data(), request.size()));
}

TEST(rpc_http_server, connection_counts)
{
    test_endpoint endpoint;
    epee::net_utils::connection_context_base context{boost::uuids::uuid{}, make_ipv6("2001:db8::1"),
                                                     true, false};
    rpc::server_config config;
    const std::string key = rpc::connection_limit_key(context.m_remote_address);

    {
        rpc::http_connection first{&endpoint, config, context};
        ASSERT_TRUE(first.after_init_connection());
        {
            rpc::http_connection second{&endpoint, config, context};
            ASSERT_TRUE(second.after_init_connection());
            EXPECT_EQ(2u, config.connection_count);
            EXPECT_EQ(2u, config.connections[key]);
        }
        EXPECT_EQ(1u, config.connection_count);
        EXPECT_EQ(1u, config.connections[key]);

        rpc::http_connection never_initialized{&endpoint, config, context};
    }
    EXPECT_EQ(0u, config.connection_count);
    EXPECT_TRUE(config.connections.empty());
}

TEST(rpc_http_server, connection_limit_key)
{
    const auto a = make_ipv6("2001:db8:abcd:1234:1111:2222:3333:4444");
    const auto b = make_ipv6("2001:db8:abcd:1234:ffff:eeee:dddd:cccc");
    const auto c = make_ipv6("2001:db8:abcd:1235::1");
    EXPECT_EQ("2001:db8:abcd:1234::/64", rpc::connection_limit_key(a));
    EXPECT_EQ(rpc::connection_limit_key(a), rpc::connection_limit_key(b));
    EXPECT_NE(rpc::connection_limit_key(a), rpc::connection_limit_key(c));

    for (const char* const ungrouped :
         {"::1", "fe80::1", "fc00::1", "fd00::1", "::", "ff00::1", "fec0::1"})
    {
        const auto address = make_ipv6(ungrouped);
        EXPECT_EQ(address.host_str(), rpc::connection_limit_key(address)) << ungrouped;
    }

    std::uint32_t ip = 0;
    ASSERT_TRUE(epee::string_tools::get_ip_int32_from_string(ip, "203.0.113.1"));
    const epee::net_utils::network_address ipv4{epee::net_utils::ipv4_network_address{ip, 18081}};
    EXPECT_EQ(ipv4.host_str(), rpc::connection_limit_key(ipv4));
}
