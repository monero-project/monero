#include <cstddef>
#include <limits>
#include <string>
#include <string_view>

#include "gtest/gtest.h"
#include "net/http_message.h"

namespace
{
constexpr bool valid_request_line(const std::string_view line) noexcept
{
    net::http::request_line out{};
    return net::http::parse_request_line(line, out);
}

static_assert(net::http::trim(" \t a b \t") == "a b");
static_assert(net::http::trim(" \t ").empty());
static_assert(net::http::trim("").empty());

static_assert(net::http::is_token("X!#$%&'*+-.^_`|~09az"));
static_assert(!net::http::is_token(""));
static_assert(!net::http::is_token(" Host"));
static_assert(!net::http::is_token("Host "));
static_assert(!net::http::is_token("Ho@st"));

static_assert(net::http::is_request_target("/json_rpc?a=b%20c"));
static_assert(!net::http::is_request_target(""));
static_assert(!net::http::is_request_target("/a\tb"));
static_assert(!net::http::is_request_target("/a\x7F"));

static_assert(net::http::status_text(200) == "OK");
static_assert(net::http::status_text(404) == "Not found");
static_assert(net::http::status_text(405) == "Method Not Allowed");
static_assert(net::http::status_text(431) == "Request Header Fields Too Large");
static_assert(net::http::status_text(999).empty());

static_assert(net::http::iequals("Content-Length", "content-LENGTH"));
static_assert(net::http::iequals("", ""));
static_assert(!net::http::iequals("Host", "Hos"));
static_assert(!net::http::iequals("[", "{"));       // only letters fold
static_assert(!net::http::iequals("\xC9", "\xE9")); // Latin-1 E-acute: not ASCII, so not folded

static_assert(net::http::target_path("/json_rpc") == "/json_rpc");
static_assert(net::http::target_path("/json_rpc?a=b?c") == "/json_rpc");
static_assert(net::http::target_path("http://example.com/json_rpc?x") == "/json_rpc");
static_assert(net::http::target_path("HTTPS://user@[::1]:18081/a/b") == "/a/b");
static_assert(net::http::target_path("http://example.com") == "/");
static_assert(net::http::target_path("http://example.com?/json_rpc") == "/");
static_assert(net::http::target_path("ftp://example.com/json_rpc").empty());
static_assert(net::http::target_path("json_rpc").empty());
static_assert(net::http::target_path("*").empty());

constexpr bool persists(const std::string_view version, const bool close, const bool keep_alive)
{
    net::http::request_line line{};
    line.version = version;
    net::http::header_fields fields{};
    fields.connection_close = close;
    fields.connection_keep_alive = keep_alive;
    return net::http::persistent(line, fields);
}
static_assert(persists("HTTP/1.1", false, false));
static_assert(!persists("HTTP/1.1", true, false));
static_assert(!persists("HTTP/1.1", true, true));
static_assert(!persists("HTTP/1.0", false, false));
static_assert(persists("HTTP/1.0", false, true));

static_assert(valid_request_line("GET / HTTP/1.1"));
static_assert(valid_request_line("POST /json_rpc HTTP/1.0"));
static_assert(!valid_request_line(""));
static_assert(!valid_request_line("GET /"));
static_assert(!valid_request_line("GET  / HTTP/1.1"));
static_assert(!valid_request_line("GET / HTTP/1.1 "));
static_assert(!valid_request_line("GET / HTTP/2.0"));
static_assert(!valid_request_line("G@T / HTTP/1.1"));
static_assert(!valid_request_line("GET /\t HTTP/1.1"));

bool parse_fields(const std::string_view fields, net::http::header_fields& out)
{
    out = net::http::header_fields{};
    return net::http::parse_header_fields(fields, out);
}
} // namespace

TEST(net_http, parse_request_line)
{
    net::http::request_line line{};
    ASSERT_TRUE(net::http::parse_request_line("POST /json_rpc HTTP/1.1", line));
    EXPECT_EQ("POST", line.method);
    EXPECT_EQ("/json_rpc", line.target);
    EXPECT_EQ("HTTP/1.1", line.version);
}

TEST(net_http, parse_size)
{
    std::size_t out = 0;
    ASSERT_TRUE(net::http::parse_size("0", out));
    EXPECT_EQ(0u, out);
    ASSERT_TRUE(net::http::parse_size("0010", out));
    EXPECT_EQ(10u, out);

    const std::string max = std::to_string(std::numeric_limits<std::size_t>::max());
    ASSERT_TRUE(net::http::parse_size(max, out));
    EXPECT_EQ(std::numeric_limits<std::size_t>::max(), out);

    for (const std::string_view bad : {"", "-1", "+1", " 1", "1 ", "0x10", "5abc", "1.0"})
    {
        EXPECT_FALSE(net::http::parse_size(bad, out)) << "accepted: " << bad;
    }
    EXPECT_FALSE(net::http::parse_size(max + "0", out));
}

TEST(net_http, parse_content_length)
{
    std::size_t out = 0;
    for (const std::string_view good : {"4", "04", " 4 ", "4, 04", "4,4,4"})
    {
        out = 0;
        EXPECT_TRUE(net::http::parse_content_length(good, out)) << "rejected: " << good;
        EXPECT_EQ(4u, out) << good;
    }

    for (const std::string_view bad : {"", "abc", "1 0", "0, 4", "4, nope", ", 4", "4,", "4,,4",
                                       "999999999999999999999999999999999999"})
    {
        EXPECT_FALSE(net::http::parse_content_length(bad, out)) << "accepted: " << bad;
    }
}

TEST(net_http, parse_header_fields)
{
    net::http::header_fields fields{};

    EXPECT_TRUE(parse_fields("", fields));
    EXPECT_FALSE(fields.has_content_length);

    EXPECT_TRUE(parse_fields("Host: example.com", fields));
    EXPECT_TRUE(parse_fields("Host:example.com", fields));
    EXPECT_TRUE(parse_fields("Content-Type:\t application/json \t", fields));
    EXPECT_TRUE(parse_fields("X!#$%&'*+-.^_`|~: value", fields));
    EXPECT_TRUE(parse_fields("Empty:", fields));

    ASSERT_TRUE(parse_fields("Host: example.com\r\nconnection: \t close \r\nX-Test: abc", fields));
    EXPECT_TRUE(fields.connection_close);
    EXPECT_FALSE(fields.connection_keep_alive);
    EXPECT_EQ(1u, fields.host_count);

    ASSERT_TRUE(parse_fields("Connection: Keep-Alive , Upgrade,close", fields));
    EXPECT_TRUE(fields.connection_close);
    EXPECT_TRUE(fields.connection_keep_alive);

    ASSERT_TRUE(parse_fields("Connection: closed, keep-alive-ish", fields));
    EXPECT_FALSE(fields.connection_close);
    EXPECT_FALSE(fields.connection_keep_alive);

    ASSERT_TRUE(parse_fields("Host: a\r\nhost: b\r\nExpect:  100-continue ", fields));
    EXPECT_EQ(2u, fields.host_count);
    EXPECT_EQ("100-continue", fields.expect);

    for (const std::string_view bad :
         {"Host : example.com", " Content-Length: 1", "\tContent-Length: 1", "Bad Header",
          "Host example.com", "GET / HTTP/1.1", ": value", "Host: example.com\r\n\r\n",
          "Host: a\rb", "Host: a\nContent-Length: 4"})
    {
        EXPECT_FALSE(parse_fields(bad, fields)) << "accepted: " << bad;
    }
    EXPECT_FALSE(parse_fields(std::string_view{"Host: a\0b", 9}, fields));
}

TEST(net_http, parse_header_fields_content_length)
{
    net::http::header_fields fields{};

    for (const std::string_view good :
         {"Content-Length: 4", "content-length: 4", "Content-Length: 4\r\nContent-Length: 04",
          "Content-Length: 4, 04", "Content-Length: 4, 04\r\nContent-Length: 4"})
    {
        ASSERT_TRUE(parse_fields(good, fields)) << "rejected: " << good;
        EXPECT_TRUE(fields.has_content_length);
        EXPECT_EQ(4u, fields.content_length);
    }

    for (const std::string_view bad :
         {"Content-Length: 0\r\nContent-Length: 4", "Content-Length: 4\r\nContent-Length: 0",
          "Content-Length: 0, 4", "Content-Length: 5abc", "Content-Length: 0x10",
          "Content-Length: +10", "Content-Length: 1 0", "Content-Length:"})
    {
        EXPECT_FALSE(parse_fields(bad, fields)) << "accepted: " << bad;
    }
}

TEST(net_http, parse_header_fields_transfer_encoding)
{
    net::http::header_fields fields{};
    for (const std::string_view bad :
         {"Transfer-Encoding:", "Transfer-Encoding: identity", "Transfer-Encoding: chunked",
          "transfer-encoding: gzip, chunked", "Content-Length: 4\r\nTransfer-Encoding: chunked",
          "Transfer-Encoding: chunked\r\nContent-Length: 4"})
    {
        EXPECT_FALSE(parse_fields(bad, fields)) << "accepted: " << bad;
    }
}

TEST(net_http, parse_request_header)
{
    net::http::request_line line{};
    net::http::header_fields fields{};

    ASSERT_TRUE(net::http::parse_request_header("GET / HTTP/1.1", line, fields));
    EXPECT_EQ("/", line.target);
    EXPECT_FALSE(fields.has_content_length);

    ASSERT_TRUE(net::http::parse_request_header(
        "POST /json_rpc HTTP/1.1\r\nHost: example.com\r\nContent-Length: 10", line, fields));
    EXPECT_EQ("POST", line.method);
    EXPECT_EQ("/json_rpc", line.target);
    EXPECT_EQ(10u, fields.content_length);

    fields = net::http::header_fields{};
    EXPECT_FALSE(net::http::parse_request_header("GET / HTTP/1.1\r\nBad Header", line, fields));
    EXPECT_FALSE(net::http::parse_request_header("GET /\r\nHost: example.com", line, fields));
    EXPECT_FALSE(net::http::parse_request_header("\r\nGET / HTTP/1.1", line, fields));
}

TEST(net_http, write_response_header)
{
    const std::string ok = net::http::write_response_header(200, 5, "text/plain", true);
    EXPECT_EQ(0u, ok.find("HTTP/1.1 200 OK\r\n"));
    EXPECT_NE(std::string::npos, ok.find("\r\nContent-Length: 5\r\n"));
    EXPECT_NE(std::string::npos, ok.find("\r\nContent-Type: text/plain\r\n"));
    EXPECT_EQ(std::string::npos, ok.find("Connection:"));
    ASSERT_GE(ok.size(), net::http::header_end.size());
    EXPECT_EQ(net::http::header_end, std::string_view{ok}.substr(ok.size() - 4));

    // the fields we write must pass our own parser
    const std::size_t first_line_end = ok.find(net::http::crlf);
    const std::string_view written = std::string_view{ok}.substr(
        first_line_end + 2, ok.size() - first_line_end - 2 - net::http::header_end.size());
    net::http::header_fields fields{};
    ASSERT_TRUE(parse_fields(written, fields));
    EXPECT_EQ(5u, fields.content_length);

    const std::string closing = net::http::write_response_header(404, 0, {}, false);
    EXPECT_EQ(0u, closing.find("HTTP/1.1 404 Not found\r\n"));
    EXPECT_NE(std::string::npos, closing.find("\r\nContent-Length: 0\r\n"));
    EXPECT_EQ(std::string::npos, closing.find("Content-Type:"));
    EXPECT_NE(std::string::npos, closing.find("\r\nConnection: close\r\n"));

    const std::string extra =
        net::http::write_response_header(405, 0, {}, false, "Allow: GET, POST\r\n");
    EXPECT_NE(std::string::npos, extra.find("\r\nAllow: GET, POST\r\n\r\n"));

    // a content type that would inject header lines is dropped
    for (const std::string_view bad : {std::string_view{"text/plain\r\nX-Injected: 1"},
                                       std::string_view{"a\nb"}, std::string_view{"a\0b", 3}})
    {
        const std::string head = net::http::write_response_header(200, 0, bad, true);
        EXPECT_EQ(std::string::npos, head.find("Content-Type:"));
        EXPECT_EQ(std::string::npos, head.find("X-Injected"));
    }
}
