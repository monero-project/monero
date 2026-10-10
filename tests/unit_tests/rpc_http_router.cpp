#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"
#include "misc_language.h"
#include "time_helper.h"
#include "net/http_server_handlers_map2.h"
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

struct COMMAND_ECHO
{
    using request = echo_request;
    using response = echo_response;
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

// The same handlers routed through the old epee macros, for byte-for-byte comparison
struct legacy_server : test_server
{
    BEGIN_URI_MAP2()
    MAP_URI_AUTO_JON2("/echo", on_echo, COMMAND_ECHO)
    MAP_URI_AUTO_BIN2("/echo.bin", on_echo, COMMAND_ECHO)
    BEGIN_JSON_RPC_MAP("/json_rpc")
    MAP_JON_RPC("echo", on_echo, COMMAND_ECHO)
    MAP_JON_RPC_WE("echo_we", on_echo_we, COMMAND_ECHO)
    END_JSON_RPC_MAP()
    END_URI_MAP2()
};

struct result
{
    unsigned int code;
    std::string body;
};

class router_parity : public ::testing::Test
{
protected:
    router_parity()
    {
        routes_.add_json("/echo", &server_, &test_server::on_echo);
        routes_.add_bin("/echo.bin", &server_, &test_server::on_echo);
        rpc::json_rpc_router& methods = routes_.json_rpc("/json_rpc");
        methods.add("echo", &server_, &test_server::on_echo);
        methods.add("echo_we", &server_, &test_server::on_echo_we);
    }

    result call_new(const std::string_view target, const std::string_view body)
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

    result call_old(const std::string_view target, const std::string_view body)
    {
        epee::net_utils::http::http_request_info request{};
        request.m_URI = std::string{target};
        request.m_body = std::string{body};

        epee::net_utils::http::http_response_info response{};
        response.m_response_code = 200;
        // as CHAIN_HTTP_TO_MAP2 does: an escaped exception becomes a 500 with whatever body was set
        try
        {
            if (!legacy_.handle_http_request_map(request, response, context_)) return {404, {}};
        }
        catch (const std::exception&)
        {
            response.m_response_code = 500;
        }
        return {static_cast<unsigned int>(response.m_response_code), response.m_body};
    }

    void expect_same(const std::string_view target, const std::string_view body)
    {
        SCOPED_TRACE(std::string{target} + " " + std::string{body});
        const result expected = call_old(target, body);
        const result actual = call_new(target, body);
        EXPECT_EQ(expected.code, actual.code);
        EXPECT_EQ(expected.body, actual.body);
    }

    test_server server_;
    legacy_server legacy_;
    rpc::router routes_;
    rpc::connection_context context_;
};

std::string to_binary(const std::string& value)
{
    echo_request req{};
    req.value = value;
    const epee::byte_slice bin = epee::serialization::store_t_to_binary(req);
    return std::string{reinterpret_cast<const char*>(bin.data()), bin.size()};
}
} // namespace

TEST_F(router_parity, json)
{
    for (const std::string_view body :
         {R"({"value":"hello"})", "{}", R"({"value":"fail"})", R"({"value":"throw"})", "not json",
          "", R"({"value":5})", R"({"other":"field"})"})
    {
        expect_same("/echo", body);
    }
}

TEST_F(router_parity, binary)
{
    for (const std::string& body : {to_binary("hello"), to_binary(""), to_binary("fail"),
                                    to_binary("throw"), std::string{"garbage"}, std::string{}})
    {
        expect_same("/echo.bin", body);
    }
}

TEST_F(router_parity, json_rpc)
{
    for (const std::string_view body :
         {R"({"jsonrpc":"2.0","id":7,"method":"echo","params":{"value":"hi"}})",
          R"({"jsonrpc":"2.0","id":"abc","method":"echo","params":{"value":"hi"}})",
          R"({"jsonrpc":"2.0","method":"echo","params":{"value":"hi"}})",
          R"({"jsonrpc":"2.0","id":null,"method":"echo"})",
          R"({"jsonrpc":"2.0","id":1,"method":"echo","params":{"value":"fail"}})",
          R"({"jsonrpc":"2.0","id":1,"method":"echo","params":{"value":"throw"}})",
          R"({"jsonrpc":"2.0","id":2,"method":"echo_we","params":{"value":"hi"}})",
          R"({"jsonrpc":"2.0","id":2,"method":"echo_we","params":{"value":"fail"}})",
          R"({"jsonrpc":"2.0","id":2,"method":"echo_we","params":{"value":"throw"}})",
          R"({"jsonrpc":"2.0","id":3,"method":"echo","params":{"value":5}})",
          R"({"jsonrpc":"2.0","id":3,"method":"echo","params":[1,2]})",
          R"({"jsonrpc":"2.0","id":4,"method":"nope"})", R"({"jsonrpc":"2.0","id":5})",
          R"({"jsonrpc":"2.0","id":5,"method":7})", "not json", ""})
    {
        expect_same("/json_rpc", body);
    }
}

TEST_F(router_parity, unknown_target)
{
    expect_same("/nope", "{}");
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
