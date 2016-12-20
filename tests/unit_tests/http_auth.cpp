// Copyright (c) 2014-2016, The Monero Project
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
using fields = std::unordered_map<std::string, std::string>;
using auth_responses = std::vector<fields>;

std::string quoted(std::string str)
{
  str.insert(str.begin(), '"');
  str.push_back('"');
  return str;
}

epee::net_utils::http::http_request_info make_request(const fields& args)
{
  namespace karma = boost::spirit::karma;

  std::string out{"   DIGEST   "};
  karma::generate(
    std::back_inserter(out),
    (karma::string << " = " << karma::string) % " , ", 
    args);

  epee::net_utils::http::http_request_info request{};
  request.m_http_method_str = "NOP";
  request.m_header_info.m_etc_fields.push_back(
    std::make_pair(u8"authorization", std::move(out))
  );
  return request;
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

bool is_unauthorized(const epee::net_utils::http::http_response_info& response)
{
  EXPECT_EQ(401, response.m_response_code);
  EXPECT_STREQ(u8"Unauthorized", response.m_response_comment.c_str());
  EXPECT_STREQ(u8"text/html", response.m_mime_tipe.c_str());
  return response.m_response_code == 401 &&
    response.m_response_comment == u8"Unauthorized" &&
    response.m_mime_tipe == u8"text/html";
}


auth_responses parse_response(const epee::net_utils::http::http_response_info& response)
{
  namespace qi = boost::spirit::qi;

  auth_responses result{};

  const auto end = response.m_additional_fields.end();
  for (auto current = response.m_additional_fields.begin(); current != end; ++current)
  {
    current = std::find_if(current, end, [] (const std::pair<std::string, std::string>& field) {
        return boost::iequals(u8"www-authenticate", field.first);
    });

    if (current == end)
      return result;

    std::unordered_map<std::string, std::string> fields{};
    const bool rc = qi::parse(
      current->second.begin(), current->second.end(),
      qi::lit(u8"Digest ") >> ((
          +qi::ascii::alpha >>
          qi::lit('=') >> (
            (qi::lit('"') >> +(qi::ascii::char_ - '"') >> qi::lit('"')) | 
            +(qi::ascii::graph - qi::ascii::char_(u8"()<>@,;:\\\"/[]?={}"))
          )
        ) % ','
      ) >> qi::eoi,
      fields
    );

    if (rc)
      result.push_back(std::move(fields));
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

std::string get_a1(const epee::net_utils::http::http_auth::login& user, const auth_responses& responses)
{
  const std::string& realm = responses.at(0).at(u8"realm");
  return boost::join(
    std::vector<std::string>{user.username, realm, user.password}, u8":"
  );
}

std::string get_a1_sess(const epee::net_utils::http::http_auth::login& user, const std::string& cnonce, const auth_responses& responses)
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

TEST(HTTP_Auth, NotRequired)
{
  epee::net_utils::http::http_auth auth{};
  EXPECT_FALSE(auth.get_response(epee::net_utils::http::http_request_info{}));
}

TEST(HTTP_Auth, MissingAuth)
{
  epee::net_utils::http::http_auth auth{{"foo", "bar"}};
  EXPECT_TRUE(bool(auth.get_response(epee::net_utils::http::http_request_info{})));
  {
    epee::net_utils::http::http_request_info request{};
    request.m_header_info.m_etc_fields.push_back({"\xFF", "\xFF"});
    EXPECT_TRUE(bool(auth.get_response(request)));
  }
}

TEST(HTTP_Auth, BadSyntax)
{
  epee::net_utils::http::http_auth auth{{"foo", "bar"}};
  EXPECT_TRUE(bool(auth.get_response(make_request({{u8"algorithm", "fo\xFF"}}))));
  EXPECT_TRUE(bool(auth.get_response(make_request({{u8"cnonce", "\"000\xFF\""}}))));
  EXPECT_TRUE(bool(auth.get_response(make_request({{u8"cnonce \xFF =", "\"000\xFF\""}}))));
  EXPECT_TRUE(bool(auth.get_response(make_request({{u8" \xFF cnonce", "\"000\xFF\""}}))));
}

TEST(HTTP_Auth, MD5)
{
  epee::net_utils::http::http_auth::login user{"foo", "bar"};
  epee::net_utils::http::http_auth auth{user};

  const auto response = auth.get_response(make_request({}));
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

TEST(HTTP_Auth, MD5_sess)
{
  constexpr const char cnonce[] = "not a good cnonce";

  epee::net_utils::http::http_auth::login user{"foo", "bar"};
  epee::net_utils::http::http_auth auth{user};

  const auto response = auth.get_response(make_request({}));
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

TEST(HTTP_Auth, MD5_auth)
{
  constexpr const char cnonce[] = "not a nonce";
  constexpr const char qop[] = "auth";

  epee::net_utils::http::http_auth::login user{"foo", "bar"};
  epee::net_utils::http::http_auth auth{user};

  const auto response = auth.get_response(make_request({}));
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

TEST(HTTP_Auth, MD5_sess_auth)
{
  constexpr const char cnonce[] = "not a nonce";
  constexpr const char qop[] = "auth";

  epee::net_utils::http::http_auth::login user{"foo", "bar"};
  epee::net_utils::http::http_auth auth{user};

  const auto response = auth.get_response(make_request({}));
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
