#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"
#include "rpc/http_server.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"

namespace
{
struct echo_request
{
    std::string value;

    BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(value)
    END_KV_SERIALIZE_MAP()
};

struct echo_response
{
    std::string value;
    std::uint64_t length = 0;

    BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(value)
    KV_SERIALIZE(length)
    END_KV_SERIALIZE_MAP()
};

// `value` of "fail" or "throw" makes a handler fail in that way
struct test_server
{
    bool on_echo(const echo_request& req, echo_response& res, const rpc::connection_context*)
    {
        if (req.value == "fail") return false;
        if (req.value == "throw") throw std::runtime_error{"test"};
        res.value = req.value;
        res.length = req.value.size();
        return true;
    }

    bool on_echo_we(const echo_request& req, echo_response& res, epee::json_rpc::error& error,
                    const rpc::connection_context* ctx)
    {
        if (req.value == "fail")
        {
            error.code = -1;
            error.message = "custom failure";
            return false;
        }
        return on_echo(req, res, ctx);
    }
};

struct result
{
    unsigned int code;
    std::string body;
};

// Drops whitespace outside of strings, so expected JSON can be written on one line
std::string compact(const std::string_view json)
{
    std::string out;
    bool in_string = false;
    for (std::size_t i = 0; i < json.size(); ++i)
    {
        const char c = json[i];
        if (in_string && c == '\\' && i + 1 < json.size())
        {
            out.append(json.substr(i, 2));
            ++i;
            continue;
        }
        if (c == '"') in_string = !in_string;
        if (in_string || (c != ' ' && c != '\r' && c != '\n')) out.push_back(c);
    }
    return out;
}

std::string to_binary(const std::string& value)
{
    echo_request req{};
    req.value = value;
    const epee::byte_slice bin = epee::serialization::store_t_to_binary(req);
    return std::string{reinterpret_cast<const char*>(bin.data()), bin.size()};
}

std::string to_binary_response(const std::string& value)
{
    echo_response res{};
    res.value = value;
    res.length = value.size();
    const epee::byte_slice bin = epee::serialization::store_t_to_binary(res);
    return std::string{reinterpret_cast<const char*>(bin.data()), bin.size()};
}

// The expected outputs were recorded from the epee URI map macros this router replaced
class router_output : public ::testing::Test
{
protected:
    router_output()
    {
        routes_.add_json("/echo", &server_, &test_server::on_echo);
        routes_.add_bin("/echo.bin", &server_, &test_server::on_echo);
        rpc::json_rpc_router& methods = routes_.json_rpc("/json_rpc");
        methods.add("echo", &server_, &test_server::on_echo);
        methods.add("echo_we", &server_, &test_server::on_echo_we);
    }

    result call(const std::string_view target, const std::string_view body)
    {
        const rpc::handler* const h = routes_.find(target);
        if (!h) return {404, {}};

        // `http_connection` answers an escaped exception with an empty 500
        try
        {
            const rpc::http_response r = (*h)(rpc::http_request{"POST", target, body}, context_);
            return {r.code,
                    std::string{reinterpret_cast<const char*>(r.body.data()), r.body.size()}};
        }
        catch (const std::exception&)
        {
            return {500, {}};
        }
    }

    void expect_json(const std::string_view target, const std::string_view body,
                     const unsigned int code, const std::string_view expected)
    {
        SCOPED_TRACE(std::string{target} + " " + std::string{body});
        const result actual = call(target, body);
        EXPECT_EQ(code, actual.code);
        EXPECT_EQ(expected, compact(actual.body));
    }

    void expect_bin(const std::string& body, const unsigned int code, const std::string& expected)
    {
        const result actual = call("/echo.bin", body);
        EXPECT_EQ(code, actual.code);
        EXPECT_EQ(expected, actual.body);
    }

    test_server server_;
    rpc::router routes_;
    rpc::connection_context context_;
};
} // namespace

TEST_F(router_output, json)
{
    expect_json("/echo", R"({"value":"hello"})", 200, R"({"length":5,"value":"hello"})");
    expect_json("/echo", "{}", 200, R"({"length":0,"value":""})");
    expect_json("/echo", R"({"other":"field"})", 200, R"({"length":0,"value":""})");
    expect_json("/echo", R"({"value":"fail"})", 500, "");
    expect_json("/echo", R"({"value":"throw"})", 500, "");
    expect_json("/echo", R"({"value":5})", 400, "");
    expect_json("/echo", "not json", 400, "");
    expect_json("/echo", "", 400, "");
}

TEST_F(router_output, json_layout)
{
    EXPECT_EQ("{\r\n  \"length\": 5,\r\n  \"value\": \"hello\"\r\n}",
              call("/echo", R"({"value":"hello"})").body);
}

TEST_F(router_output, binary)
{
    expect_bin(to_binary("hello"), 200, to_binary_response("hello"));
    expect_bin(to_binary(""), 200, to_binary_response(""));
    expect_bin(to_binary("fail"), 500, "");
    expect_bin(to_binary("throw"), 500, "");
    expect_bin("garbage", 400, "");
    expect_bin("", 400, "");
}

TEST_F(router_output, json_rpc)
{
    const auto expect = [this](const std::string_view body, const std::string_view expected)
    {
        expect_json("/json_rpc", body, 200, expected);
    };

    expect(R"({"jsonrpc":"2.0","id":7,"method":"echo","params":{"value":"hi"}})",
           R"({"id":7,"jsonrpc":"2.0","result":{"length":2,"value":"hi"}})");
    expect(R"({"jsonrpc":"2.0","id":"abc","method":"echo","params":{"value":"hi"}})",
           R"({"id":"abc","jsonrpc":"2.0","result":{"length":2,"value":"hi"}})");
    expect(R"({"jsonrpc":"2.0","method":"echo","params":{"value":"hi"}})",
           R"({"id":0,"jsonrpc":"2.0","result":{"length":2,"value":"hi"}})");
    expect(R"({"jsonrpc":"2.0","id":null,"method":"echo"})",
           R"({"id":0,"jsonrpc":"2.0","result":{"length":0,"value":""}})");
    expect(R"({"jsonrpc":"2.0","id":3,"method":"echo","params":[1,2]})",
           R"({"id":3,"jsonrpc":"2.0","result":{"length":0,"value":""}})");
    expect(R"({"jsonrpc":"2.0","id":2,"method":"echo_we","params":{"value":"hi"}})",
           R"({"id":2,"jsonrpc":"2.0","result":{"length":2,"value":"hi"}})");

    expect(R"({"jsonrpc":"2.0","id":1,"method":"echo","params":{"value":"fail"}})",
           R"({"error":{"code":-32603,"message":"Internal error"},"id":1,"jsonrpc":"2.0"})");
    expect(R"({"jsonrpc":"2.0","id":1,"method":"echo","params":{"value":"throw"}})",
           R"({"error":{"code":-32603,"message":"Internal error"},"id":1,"jsonrpc":"2.0"})");
    expect(R"({"jsonrpc":"2.0","id":2,"method":"echo_we","params":{"value":"fail"}})",
           R"({"error":{"code":-1,"message":"custom failure"},"id":2,"jsonrpc":"2.0"})");
    // a `_WE` handler that throws leaves its error unset
    expect(R"({"jsonrpc":"2.0","id":2,"method":"echo_we","params":{"value":"throw"}})",
           R"({"error":{"code":0,"message":""},"id":2,"jsonrpc":"2.0"})");
    expect(R"({"jsonrpc":"2.0","id":3,"method":"echo","params":{"value":5}})",
           R"({"error":{"code":-32602,"message":"Invalid params"},"id":3,"jsonrpc":"2.0"})");
    expect(R"({"jsonrpc":"2.0","id":4,"method":"nope"})",
           R"({"error":{"code":-32601,"message":"Method not found"},"id":4,"jsonrpc":"2.0"})");
    expect(R"({"jsonrpc":"2.0","id":5})",
           R"({"error":{"code":-32600,"message":"Invalid Request"},"id":0,"jsonrpc":"2.0"})");
    expect("not json",
           R"({"error":{"code":-32700,"message":"Parse error"},"id":0,"jsonrpc":"2.0"})");
    expect("", R"({"error":{"code":-32700,"message":"Parse error"},"id":0,"jsonrpc":"2.0"})");

    // `method` of the wrong type throws out of the router
    expect_json("/json_rpc", R"({"jsonrpc":"2.0","id":5,"method":7})", 500, "");
}

TEST_F(router_output, unknown_target)
{
    expect_json("/nope", "{}", 404, "");
}

TEST(rpc_router, content_types)
{
    test_server server;
    rpc::router routes;
    routes.add_json("/echo", &server, &test_server::on_echo);
    routes.add_bin("/echo.bin", &server, &test_server::on_echo);
    routes.json_rpc("/json_rpc").add("echo", &server, &test_server::on_echo);
    rpc::connection_context context;

    const auto call = [&](const std::string_view target, const std::string_view body)
    {
        return (*routes.find(target))(rpc::http_request{"POST", target, body}, context);
    };

    EXPECT_EQ("application/json", call("/echo", "{}").content_type);
    EXPECT_EQ("application/octet-stream", call("/echo.bin", to_binary("x")).content_type);
    EXPECT_EQ("application/json", call("/json_rpc", "not json").content_type);
    EXPECT_TRUE(call("/echo", "not json").content_type.empty());
}

TEST(rpc_router, json_rpc_survives_move)
{
    test_server server;
    rpc::router original;
    rpc::json_rpc_router& methods = original.json_rpc("/json_rpc");

    rpc::router moved{std::move(original)};
    methods.add("echo", &server, &test_server::on_echo);

    rpc::connection_context context;
    const std::string body = R"({"jsonrpc":"2.0","id":1,"method":"echo","params":{"value":"x"}})";
    const rpc::http_response r =
        (*moved.find("/json_rpc"))(rpc::http_request{"POST", "/json_rpc", body}, context);
    const std::string out{reinterpret_cast<const char*>(r.body.data()), r.body.size()};
    EXPECT_NE(std::string::npos, out.find(R"("x")")) << out;
}

TEST(rpc_router, duplicates_throw)
{
    test_server server;
    rpc::router routes;
    routes.add_json("/echo", &server, &test_server::on_echo);
    EXPECT_THROW(routes.add_bin("/echo", &server, &test_server::on_echo), std::logic_error);

    rpc::json_rpc_router& methods = routes.json_rpc("/json_rpc");
    EXPECT_THROW(routes.json_rpc("/json_rpc"), std::logic_error);
    EXPECT_THROW(routes.json_rpc("/echo"), std::logic_error);

    methods.add("echo", &server, &test_server::on_echo);
    EXPECT_THROW(methods.add("echo", &server, &test_server::on_echo_we), std::logic_error);
}

TEST(rpc_router, non_std_exception_is_a_failed_call)
{
    struct throwing_server
    {
        bool on_throw(const echo_request&, echo_response&, const rpc::connection_context*)
        {
            throw 1;
        }
    };

    throwing_server server;
    rpc::router routes;
    routes.add_json("/throw", &server, &throwing_server::on_throw);
    routes.json_rpc("/json_rpc").add("throw", &server, &throwing_server::on_throw);
    rpc::connection_context context;

    // left for `http_connection` to answer with a 500
    EXPECT_THROW((*routes.find("/throw"))(rpc::http_request{"POST", "/throw", "{}"}, context), int);

    const std::string body = R"({"jsonrpc":"2.0","id":1,"method":"throw"})";
    const rpc::http_response r =
        (*routes.find("/json_rpc"))(rpc::http_request{"POST", "/json_rpc", body}, context);
    const std::string out{reinterpret_cast<const char*>(r.body.data()), r.body.size()};
    EXPECT_NE(std::string::npos, out.find("-32603")) << out;
}

TEST(rpc_router, replace)
{
    test_server server;
    rpc::router routes;
    EXPECT_THROW(routes.replace_json("/echo", &server, &test_server::on_echo), std::logic_error);

    routes.add_bin("/echo", &server, &test_server::on_echo);
    routes.replace_json("/echo", &server, &test_server::on_echo);

    rpc::connection_context context;
    const rpc::http_response r =
        (*routes.find("/echo"))(rpc::http_request{"POST", "/echo", R"({"value":"x"})"}, context);
    EXPECT_EQ(200u, r.code);
    EXPECT_EQ("application/json", r.content_type);
}
