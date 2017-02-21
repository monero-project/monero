// Copyright (c) 2014-2017, The Monero Project
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

#include "gtest/gtest.h"
#include "net/http_auth.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_list.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_right_alignment.hpp>
#include <boost/spirit/include/karma_sequence.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_char_class.hpp>
#include <boost/spirit/include/qi_difference.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_list.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_plus.hpp>
#include <boost/spirit/include/qi_sequence.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <cstdint>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "md5_l.h"
#include "string_tools.h"

namespace {
namespace http = epee::net_utils::http;
using fields = std::unordered_map<std::string, std::string>;
using auth_responses = std::vector<fields>;

std::string quoted(std::string str)
{
  str.insert(str.begin(), '"');
  str.push_back('"');
  return str;
}

void write_fields(std::string& out, const fields& args)
{
  namespace karma = boost::spirit::karma;
  karma::generate(
    std::back_inserter(out),
    (karma::string << " = " << karma::string) % " , ", 
    args);
}

std::string write_fields(const fields& args)
{
  std::string out{};
  write_fields(out, args);
  return out;
}

http::http_request_info make_request(const fields& args)
{
  std::string out{"   DIGEST   "};
  write_fields(out, args);

  http::http_request_info request{};
  request.m_http_method_str = "NOP";
  request.m_header_info.m_etc_fields.push_back(
    std::make_pair(u8"authorization", std::move(out))
  );
  return request;
}

http::http_response_info make_response(const auth_responses& choices)
{
  http::http_response_info response{};
  for (const auto& choice : choices)
  {
    std::string out{"   DIGEST   "};
    write_fields(out, choice);

    response.m_header_info.m_etc_fields.push_back(
      std::make_pair(u8"WWW-authenticate", std::move(out))
    );
  }
  return response;
}

bool has_same_fields(const auth_responses& in)
{
  const std::vector<std::string> check{u8"nonce", u8"qop", u8"realm", u8"stale"};
  
  auto current = in.begin();
  const auto end = in.end();
  if (current == end)
    return true;

  ++current;
  for ( ; current != end; ++current )
  {
    for (const auto& field : check)
    {
      const std::string& expected = in[0].at(field);
      const std::string& actual = current->at(field);
      EXPECT_EQ(expected, actual);
      if (expected != actual)
        return false;
    }
  }
  return true;
}

bool is_unauthorized(const http::http_response_info& response)
{
  EXPECT_EQ(401, response.m_response_code);
  EXPECT_STREQ(u8"Unauthorized", response.m_response_comment.c_str());
  EXPECT_STREQ(u8"text/html", response.m_mime_tipe.c_str());
  return response.m_response_code == 401 &&
    response.m_response_comment == u8"Unauthorized" &&
    response.m_mime_tipe == u8"text/html";
}

fields parse_fields(const std::string& value)
{
  namespace qi = boost::spirit::qi;

  fields out{};
  const bool rc = qi::parse(
    value.begin(), value.end(),
    qi::lit(u8"Digest ") >> ((
        +qi::ascii::alpha >>
        qi::lit('=') >> (
          (qi::lit('"') >> +(qi::ascii::char_ - '"') >> qi::lit('"')) |
          +(qi::ascii::graph - qi::ascii::char_(u8"()<>@,;:\\\"/[]?={}"))
        )
      ) % ','
    ) >> qi::eoi,
    out
  );
  if (!rc)
    throw std::runtime_error{"Bad field given in HTTP header"};

  return out;
}

auth_responses parse_response(const http::http_response_info& response)
{
  auth_responses result{};

  const auto end = response.m_additional_fields.end();
  for (auto current = response.m_additional_fields.begin();; ++current)
  {
    current = std::find_if(current, end, [] (const std::pair<std::string, std::string>& field) {
        return boost::equals(u8"WWW-authenticate", field.first);
    });

    if (current == end)
      return result;

    result.push_back(parse_fields(current->second));
  }
  return result;
}

std::string md5_hex(const std::string& in)
{
  md5::MD5_CTX ctx{};
  md5::MD5Init(std::addressof(ctx));
  md5::MD5Update(
    std::addressof(ctx),
    reinterpret_cast<const std::uint8_t*>(in.data()),
    in.size()
  );

  std::array<std::uint8_t, 16> digest{{}};
  md5::MD5Final(digest.data(), std::addressof(ctx));
  return epee::string_tools::pod_to_hex(digest);
}

std::string get_a1(const http::login& user, const fields& src)
{
  const std::string& realm = src.at(u8"realm");
  return boost::join(
    std::vector<std::string>{user.username, realm, user.password}, u8":"
  );
}

std::string get_a1(const http::login& user, const auth_responses& responses)
{
  return get_a1(user, responses.at(0));
}

std::string get_a1_sess(const http::login& user, const std::string& cnonce, const auth_responses& responses)
{
  const std::string& nonce = responses.at(0).at(u8"nonce");
  return boost::join(
    std::vector<std::string>{md5_hex(get_a1(user, responses)), nonce, cnonce}, u8":"
  );
}

std::string get_a2(const std::string& uri)
{
  return boost::join(std::vector<std::string>{"NOP", uri}, u8":");
}

std::string get_nc(std::uint32_t count)
{
  namespace karma = boost::spirit::karma;
  std::string out;
  karma::generate(
    std::back_inserter(out),
    karma::right_align(8, '0')[karma::uint_generator<std::uint32_t, 16>{}],
    count
  );

  return out;
}
}

TEST(HTTP_Server_Auth, NotRequired)
{
  http::http_server_auth auth{};
  EXPECT_FALSE(auth.get_response(http::http_request_info{}));
}

TEST(HTTP_Server_Auth, MissingAuth)
{
  http::http_server_auth auth{{"foo", "bar"}};
  EXPECT_TRUE(bool(auth.get_response(http::http_request_info{})));
  {
    http::http_request_info request{};
    request.m_header_info.m_etc_fields.push_back({"\xFF", "\xFF"});
    EXPECT_TRUE(bool(auth.get_response(request)));
  }
}

TEST(HTTP_Server_Auth, BadSyntax)
{
  http::http_server_auth auth{{"foo", "bar"}};
  EXPECT_TRUE(bool(auth.get_response(make_request({{u8"algorithm", "fo\xFF"}}))));
  EXPECT_TRUE(bool(auth.get_response(make_request({{u8"cnonce", "\"000\xFF\""}}))));
  EXPECT_TRUE(bool(auth.get_response(make_request({{u8"cnonce \xFF =", "\"000\xFF\""}}))));
  EXPECT_TRUE(bool(auth.get_response(make_request({{u8" \xFF cnonce", "\"000\xFF\""}}))));
}

TEST(HTTP_Server_Auth, MD5)
{
  http::login user{"foo", "bar"};
  http::http_server_auth auth{user};

  const auto response = auth.get_response(make_request(fields{}));
  ASSERT_TRUE(bool(response));
  EXPECT_TRUE(is_unauthorized(*response));

  const auto fields = parse_response(*response);
  ASSERT_LE(2u, fields.size());
  EXPECT_TRUE(has_same_fields(fields));

  const std::string& nonce = fields[0].at(u8"nonce");
  EXPECT_EQ(24, nonce.size());

  const std::string uri{"/some_foo_thing"};

  const std::string a1 = get_a1(user, fields);
  const std::string a2 = get_a2(uri);

  const std::string auth_code = md5_hex(
    boost::join(std::vector<std::string>{md5_hex(a1), nonce, md5_hex(a2)}, u8":")
  );

  const auto request = make_request({
    {u8"nonce", quoted(nonce)},
    {u8"realm", quoted(fields[0].at(u8"realm"))},
    {u8"response", quoted(auth_code)},
    {u8"uri", quoted(uri)},
    {u8"username", quoted(user.username)}
  });

  EXPECT_FALSE(bool(auth.get_response(request)));

  const auto response2 = auth.get_response(request);
  ASSERT_TRUE(bool(response2));
  EXPECT_TRUE(is_unauthorized(*response2));

  const auto fields2 = parse_response(*response2);
  ASSERT_LE(2u, fields2.size());
  EXPECT_TRUE(has_same_fields(fields2));

  EXPECT_NE(nonce, fields2[0].at(u8"nonce"));
  EXPECT_STREQ(u8"true", fields2[0].at(u8"stale").c_str());
}

TEST(HTTP_Server_Auth, MD5_sess)
{
  constexpr const char cnonce[] = "not a good cnonce";

  http::login user{"foo", "bar"};
  http::http_server_auth auth{user};

  const auto response = auth.get_response(make_request(fields{}));
  ASSERT_TRUE(bool(response));
  EXPECT_TRUE(is_unauthorized(*response));

  const auto fields = parse_response(*response);
  ASSERT_LE(2u, fields.size());
  EXPECT_TRUE(has_same_fields(fields));

  const std::string& nonce = fields[0].at(u8"nonce");
  EXPECT_EQ(24, nonce.size());

  const std::string uri{"/some_foo_thing"};

  const std::string a1 = get_a1_sess(user, cnonce, fields);
  const std::string a2 = get_a2(uri);

  const std::string auth_code = md5_hex(
    boost::join(std::vector<std::string>{md5_hex(a1), nonce, md5_hex(a2)}, u8":")
  );

  const auto request = make_request({
    {u8"algorithm", u8"md5-sess"},
    {u8"cnonce", quoted(cnonce)},
    {u8"nonce", quoted(nonce)},
    {u8"realm", quoted(fields[0].at(u8"realm"))},
    {u8"response", quoted(auth_code)},
    {u8"uri", quoted(uri)},
    {u8"username", quoted(user.username)}
  });

  EXPECT_FALSE(bool(auth.get_response(request)));

  const auto response2 = auth.get_response(request);
  ASSERT_TRUE(bool(response2));
  EXPECT_TRUE(is_unauthorized(*response2));

  const auto fields2 = parse_response(*response2);
  ASSERT_LE(2u, fields2.size());
  EXPECT_TRUE(has_same_fields(fields2));

  EXPECT_NE(nonce, fields2[0].at(u8"nonce"));
  EXPECT_STREQ(u8"true", fields2[0].at(u8"stale").c_str());
}

TEST(HTTP_Server_Auth, MD5_auth)
{
  constexpr const char cnonce[] = "not a nonce";
  constexpr const char qop[] = "auth";

  http::login user{"foo", "bar"};
  http::http_server_auth auth{user};

  const auto response = auth.get_response(make_request(fields{}));
  ASSERT_TRUE(bool(response));
  EXPECT_TRUE(is_unauthorized(*response));

  const auto parsed = parse_response(*response);
  ASSERT_LE(2u, parsed.size());
  EXPECT_TRUE(has_same_fields(parsed));

  const std::string& nonce = parsed[0].at(u8"nonce");
  EXPECT_EQ(24, nonce.size());

  const std::string uri{"/some_foo_thing"};

  const std::string a1 = get_a1(user, parsed);
  const std::string a2 = get_a2(uri);
  std::string nc = get_nc(1);

  const auto generate_auth = [&] {
    return md5_hex(
      boost::join(
        std::vector<std::string>{md5_hex(a1), nonce, nc, cnonce, qop, md5_hex(a2)}, u8":"
      )
    );
  };

  fields args{
    {u8"algorithm", quoted(u8"md5")},
    {u8"cnonce", quoted(cnonce)},
    {u8"nc", nc},
    {u8"nonce", quoted(nonce)},
    {u8"qop", quoted(qop)},
    {u8"realm", quoted(parsed[0].at(u8"realm"))},
    {u8"response", quoted(generate_auth())},
    {u8"uri", quoted(uri)},
    {u8"username", quoted(user.username)}
  };

  const auto request = make_request(args);
  EXPECT_FALSE(bool(auth.get_response(request)));

  for (unsigned i = 2; i < 20; ++i)
  {
    nc = get_nc(i);
    args.at(u8"nc") = nc;
    args.at(u8"response") = quoted(generate_auth());
    EXPECT_FALSE(auth.get_response(make_request(args)));
  }

  const auto replay = auth.get_response(request);
  ASSERT_TRUE(bool(replay));
  EXPECT_TRUE(is_unauthorized(*replay));

  const auto parsed_replay = parse_response(*replay);
  ASSERT_LE(2u, parsed_replay.size());
  EXPECT_TRUE(has_same_fields(parsed_replay));

  EXPECT_NE(nonce, parsed_replay[0].at(u8"nonce"));
  EXPECT_STREQ(u8"true", parsed_replay[0].at(u8"stale").c_str());
}

TEST(HTTP_Server_Auth, MD5_sess_auth)
{
  constexpr const char cnonce[] = "not a nonce";
  constexpr const char qop[] = "auth";

  http::login user{"foo", "bar"};
  http::http_server_auth auth{user};

  const auto response = auth.get_response(make_request(fields{}));
  ASSERT_TRUE(bool(response));
  EXPECT_TRUE(is_unauthorized(*response));

  const auto parsed = parse_response(*response);
  ASSERT_LE(2u, parsed.size());
  EXPECT_TRUE(has_same_fields(parsed));

  const std::string& nonce = parsed[0].at(u8"nonce");
  EXPECT_EQ(24, nonce.size());

  const std::string uri{"/some_foo_thing"};

  const std::string a1 = get_a1_sess(user, cnonce, parsed);
  const std::string a2 = get_a2(uri);
  std::string nc = get_nc(1);

  const auto generate_auth = [&] {
    return md5_hex(
      boost::join(
        std::vector<std::string>{md5_hex(a1), nonce, nc, cnonce, qop, md5_hex(a2)}, u8":"
      )
    );
  };

  fields args{
    {u8"algorithm", u8"md5-sess"},
    {u8"cnonce", quoted(cnonce)},
    {u8"nc", nc},
    {u8"nonce", quoted(nonce)},
    {u8"qop", qop},
    {u8"realm", quoted(parsed[0].at(u8"realm"))},
    {u8"response", quoted(generate_auth())},
    {u8"uri", quoted(uri)},
    {u8"username", quoted(user.username)}
  };

  const auto request = make_request(args);
  EXPECT_FALSE(bool(auth.get_response(request)));

  for (unsigned i = 2; i < 20; ++i)
  {
    nc = get_nc(i);
    args.at(u8"nc") = nc;
    args.at(u8"response") = quoted(generate_auth());
    EXPECT_FALSE(auth.get_response(make_request(args)));
  }

  const auto replay = auth.get_response(request);
  ASSERT_TRUE(bool(replay));
  EXPECT_TRUE(is_unauthorized(*replay));

  const auto parsed_replay = parse_response(*replay);
  ASSERT_LE(2u, parsed_replay.size());
  EXPECT_TRUE(has_same_fields(parsed_replay));

  EXPECT_NE(nonce, parsed_replay[0].at(u8"nonce"));
  EXPECT_STREQ(u8"true", parsed_replay[0].at(u8"stale").c_str());
}


TEST(HTTP_Auth, DogFood)
{
  const auto add_auth_field = [] (http::http_request_info& request, http::http_client_auth& client)
  {
    auto field = client.get_auth_field(request.m_http_method_str, request.m_URI);
    EXPECT_TRUE(bool(field));
    if (!field)
      return false;
    request.m_header_info.m_etc_fields.push_back(std::move(*field));
    return true;
  };

  const http::login user{"some_user", "ultimate password"};

  http::http_server_auth server{user};
  http::http_client_auth client{user};

  http::http_request_info request{};
  request.m_http_method_str = "GET";
  request.m_URI = "/FOO";

  auto response = server.get_response(request);
  ASSERT_TRUE(bool(response));
  EXPECT_TRUE(is_unauthorized(*response));
  EXPECT_TRUE(response->m_header_info.m_etc_fields.empty());
  response->m_header_info.m_etc_fields = response->m_additional_fields;

  EXPECT_EQ(http::http_client_auth::kSuccess, client.handle_401(*response));
  EXPECT_TRUE(add_auth_field(request, client));
  EXPECT_FALSE(bool(server.get_response(request)));

  for (unsigned i = 0; i < 1000; ++i)
  {
    request.m_http_method_str += std::to_string(i);
    request.m_header_info.m_etc_fields.clear();
    EXPECT_TRUE(add_auth_field(request, client));
    EXPECT_FALSE(bool(server.get_response(request)));
  }

  // resetting counter should be rejected by server
  request.m_header_info.m_etc_fields.clear();
  client = http::http_client_auth{user};
  EXPECT_EQ(http::http_client_auth::kSuccess, client.handle_401(*response));
  EXPECT_TRUE(add_auth_field(request, client));

  auto response2 = server.get_response(request);
  ASSERT_TRUE(bool(response2));
  EXPECT_TRUE(is_unauthorized(*response2));
  EXPECT_TRUE(response2->m_header_info.m_etc_fields.empty());
  response2->m_header_info.m_etc_fields = response2->m_additional_fields;

  const auth_responses parsed1 = parse_response(*response);
  const auth_responses parsed2 = parse_response(*response2);
  ASSERT_LE(1u, parsed1.size());
  ASSERT_LE(1u, parsed2.size());
  EXPECT_NE(parsed1[0].at(u8"nonce"), parsed2[0].at(u8"nonce"));

  // with stale=true client should reset
  request.m_header_info.m_etc_fields.clear();
  EXPECT_EQ(http::http_client_auth::kSuccess, client.handle_401(*response2));
  EXPECT_TRUE(add_auth_field(request, client));
  EXPECT_FALSE(bool(server.get_response(request)));

  // client should give up if stale=false
  EXPECT_EQ(http::http_client_auth::kBadPassword, client.handle_401(*response));
}

TEST(HTTP_Client_Auth, Unavailable)
{
  http::http_client_auth auth{};
  EXPECT_EQ(http::http_client_auth::kBadPassword, auth.handle_401(http::http_response_info{}));
  EXPECT_FALSE(bool(auth.get_auth_field("GET", "/file")));
}

TEST(HTTP_Client_Auth, MissingAuthenticate)
{
  http::http_client_auth auth{{"foo", "bar"}};
  EXPECT_EQ(http::http_client_auth::kParseFailure, auth.handle_401(http::http_response_info{}));
  EXPECT_FALSE(bool(auth.get_auth_field("POST", "/\xFFname")));
  {
    http::http_response_info response{};
    response.m_additional_fields.push_back({"\xFF", "\xFF"});
    EXPECT_EQ(http::http_client_auth::kParseFailure, auth.handle_401(response));
  }
  EXPECT_FALSE(bool(auth.get_auth_field("DELETE", "/file/does/not/exist")));
}

TEST(HTTP_Client_Auth, BadSyntax)
{
  http::http_client_auth auth{{"foo", "bar"}};
  EXPECT_EQ(http::http_client_auth::kParseFailure, auth.handle_401(make_response({{{u8"realm", "fo\xFF"}}})));
  EXPECT_EQ(http::http_client_auth::kParseFailure, auth.handle_401(make_response({{{u8"domain", "fo\xFF"}}})));
  EXPECT_EQ(http::http_client_auth::kParseFailure, auth.handle_401(make_response({{{u8"nonce", "fo\xFF"}}})));
  EXPECT_EQ(http::http_client_auth::kParseFailure, auth.handle_401(make_response({{{u8"nonce \xFF =", "fo\xFF"}}})));
  EXPECT_EQ(http::http_client_auth::kParseFailure, auth.handle_401(make_response({{{u8" \xFF nonce", "fo\xFF"}}})));
}

TEST(HTTP_Client_Auth, MD5)
{
  constexpr char method[] = "NOP";
  constexpr char nonce[] = "some crazy nonce";
  constexpr char realm[] = "the only realm";
  constexpr char uri[] = "/some_file";

  const http::login user{"foo", "bar"};
  http::http_client_auth auth{user};

  auto response = make_response({
    {
      {u8"domain", quoted("ignored")},
      {u8"nonce", quoted(nonce)},
      {u8"REALM", quoted(realm)}
    },
    {
      {u8"algorithm", "null"},
      {u8"domain", quoted("ignored")},
      {u8"nonce", quoted(std::string{"e"} + nonce)},
      {u8"realm", quoted(std::string{"e"} + realm)}
    },
  });

  EXPECT_EQ(http::http_client_auth::kSuccess, auth.handle_401(response));
  const auto auth_field = auth.get_auth_field(method, uri);
  ASSERT_TRUE(bool(auth_field));

  const auto parsed = parse_fields(auth_field->second);
  EXPECT_STREQ(u8"Authorization", auth_field->first.c_str());
  EXPECT_EQ(parsed.end(), parsed.find(u8"opaque"));
  EXPECT_EQ(parsed.end(), parsed.find(u8"qop"));
  EXPECT_EQ(parsed.end(), parsed.find(u8"nc"));
  EXPECT_STREQ(u8"MD5", parsed.at(u8"algorithm").c_str());
  EXPECT_STREQ(nonce, parsed.at(u8"nonce").c_str());
  EXPECT_STREQ(uri, parsed.at(u8"uri").c_str());
  EXPECT_EQ(user.username, parsed.at(u8"username"));
  EXPECT_STREQ(realm, parsed.at(u8"realm").c_str());

  const std::string a1 = get_a1(user, parsed);
  const std::string a2 = get_a2(uri);
  const std::string auth_code = md5_hex(
    boost::join(std::vector<std::string>{md5_hex(a1), nonce, md5_hex(a2)}, u8":")
  );
  EXPECT_TRUE(boost::iequals(auth_code, parsed.at(u8"response")));
  {
    const auto auth_field_dup = auth.get_auth_field(method, uri);
    ASSERT_TRUE(bool(auth_field_dup));
    EXPECT_EQ(*auth_field, *auth_field_dup);
  }


  EXPECT_EQ(http::http_client_auth::kBadPassword, auth.handle_401(response));
  response.m_header_info.m_etc_fields.front().second.append(u8"," + write_fields({{u8"stale", u8"TRUE"}}));
  EXPECT_EQ(http::http_client_auth::kSuccess, auth.handle_401(response));
}

TEST(HTTP_Client_Auth, MD5_auth)
{
  constexpr char cnonce[] = "";
  constexpr char method[] = "NOP";
  constexpr char nonce[] = "some crazy nonce";
  constexpr char opaque[] = "this is the opaque";
  constexpr char qop[] = u8"ignore,auth,ignore";
  constexpr char realm[] = "the only realm";
  constexpr char uri[] = "/some_file";

  const http::login user{"foo", "bar"};
  http::http_client_auth auth{user};

  auto response = make_response({
    {
      {u8"algorithm", u8"MD5"},
      {u8"domain", quoted("ignored")},
      {u8"nonce", quoted(std::string{"e"} + nonce)},
      {u8"realm", quoted(std::string{"e"} + realm)},
      {u8"qop", quoted("some,thing,to,ignore")}
    },
    {
      {u8"algorIthm", quoted(u8"md5")},
      {u8"domain", quoted("ignored")},
      {u8"noNce", quoted(nonce)},
      {u8"opaque", quoted(opaque)},
      {u8"realm", quoted(realm)},
      {u8"QoP", quoted(qop)}
    }
  });

  EXPECT_EQ(http::http_client_auth::kSuccess, auth.handle_401(response));

  for (unsigned i = 1; i < 1000; ++i)
  {
    const std::string nc = get_nc(i);

    const auto auth_field = auth.get_auth_field(method, uri);
    ASSERT_TRUE(bool(auth_field));

    const auto parsed = parse_fields(auth_field->second);
    EXPECT_STREQ(u8"Authorization", auth_field->first.c_str());
    EXPECT_STREQ(u8"MD5", parsed.at(u8"algorithm").c_str());
    EXPECT_STREQ(nonce, parsed.at(u8"nonce").c_str());
    EXPECT_STREQ(opaque, parsed.at(u8"opaque").c_str());
    EXPECT_STREQ(u8"auth", parsed.at(u8"qop").c_str());
    EXPECT_STREQ(uri, parsed.at(u8"uri").c_str());
    EXPECT_EQ(user.username, parsed.at(u8"username"));
    EXPECT_STREQ(realm, parsed.at(u8"realm").c_str());
    EXPECT_EQ(nc, parsed.at(u8"nc"));

    const std::string a1 = get_a1(user, parsed);
    const std::string a2 = get_a2(uri);
    const std::string auth_code = md5_hex(
      boost::join(std::vector<std::string>{md5_hex(a1), nonce, nc, cnonce, u8"auth", md5_hex(a2)}, u8":")
    );
    EXPECT_TRUE(boost::iequals(auth_code, parsed.at(u8"response")));
  }

  EXPECT_EQ(http::http_client_auth::kBadPassword, auth.handle_401(response));
  response.m_header_info.m_etc_fields.back().second.append(u8"," + write_fields({{u8"stale", u8"trUe"}}));
  EXPECT_EQ(http::http_client_auth::kSuccess, auth.handle_401(response));
}


TEST(HTTP, Add_Field)
{
  std::string str{"leading text"};
  epee::net_utils::http::add_field(str, "foo", "bar");
  epee::net_utils::http::add_field(str, std::string("bar"), std::string("foo"));
  epee::net_utils::http::add_field(str, {"moarbars", "moarfoo"});

  EXPECT_STREQ("leading textfoo: bar\r\nbar: foo\r\nmoarbars: moarfoo\r\n", str.c_str());
}
