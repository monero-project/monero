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
#include "net/http_auth.h"

#include <array>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/query/any.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/range/join.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_and_predicate.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_char_class.hpp>
#include <boost/spirit/include/qi_difference.hpp>
#include <boost/spirit/include/qi_kleene.hpp>
#include <boost/spirit/include/qi_optional.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_plus.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/include/qi_raw.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_sequence.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_symbols.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/utility/string_ref.hpp>
#include <cassert>
#include <functional>
#include <iterator>
#include <tuple>
#include <type_traits>

#include "crypto/crypto.h"
#include "md5_l.h"
#include "string_coding.h"

/* This file uses the `u8` prefix and specifies all chars by ASCII numeric
value. This is for maximum portability - C++ does not actually specify ASCII
as the encoding type for unprefixed string literals, etc. Although rare, the
effort required to support rare compiler encoding types is low.

Also be careful of qi::ascii character classes (`qi::asci::alpha`, etc.) -
non-ASCII characters will cause undefined behavior in the table lookup until
boost 1.60. The expression `&qi::ascii::char_` will fail on non-ASCII
characters without "consuming" the input character. */

namespace
{
  // string_ref is only constexpr if length is given
  template<std::size_t N>
  constexpr boost::string_ref ceref(const char (&arg)[N])
  {
    return boost::string_ref(arg, N - 1);
  }

  constexpr const auto auth_realm = ceref(u8"monero-wallet-rpc");
  constexpr const char comma = 44;
  constexpr const char equal_sign = 61;
  constexpr const char quote = 34;
  constexpr const auto sess_algo = ceref(u8"-sess");

  //// Digest Algorithms

  template<std::size_t N>
  std::array<char, N * 2> to_hex(const std::array<std::uint8_t, N>& digest) noexcept
  {
    static constexpr const char alphabet[] = u8"0123456789abcdef";
    static_assert(sizeof(alphabet) == 17, "bad alphabet size");

    // TODO upgrade (improve performance) of to hex in epee string tools
    std::array<char, N * 2> out{{}};
    auto current = out.begin();
    for (const std::uint8_t byte : digest)
    {
      *current = alphabet[byte >> 4];
      ++current;
      *current = alphabet[byte & 0x0F];
      ++current;
    }
    return out;
  }

  struct md5_
  {
    static constexpr const boost::string_ref name = ceref(u8"MD5");

    struct update
    {
      template<typename T>
      void operator()(const T& arg) const
      {
        const boost::iterator_range<const char*> data(boost::as_literal(arg));
        md5::MD5Update(
          std::addressof(ctx),
          reinterpret_cast<const std::uint8_t*>(data.begin()),
          data.size()
        );
      }
      void operator()(const std::string& arg) const
      {
        (*this)(boost::string_ref(arg));
      }

      md5::MD5_CTX& ctx;
    };

    template<typename... T>
    std::array<char, 32> operator()(const T&... args) const
    {      
      md5::MD5_CTX ctx{};
      md5::MD5Init(std::addressof(ctx));
      boost::fusion::for_each(std::tie(args...), update{ctx});

      std::array<std::uint8_t, 16> digest{{}};
      md5::MD5Final(digest.data(), std::addressof(ctx));
      return to_hex(digest);
    }
  };
  constexpr const boost::string_ref md5_::name;

  //! Digest Algorithms available for HTTP Digest Auth.
  constexpr const std::tuple<md5_> digest_algorithms{};

  //// Various String Parsing Utilities

  struct ascii_tolower_
  {
    template<typename Char>
    Char operator()(Char value) const noexcept
    {
      static_assert(std::is_integral<Char>::value, "only integral types supported");
      return (65 <= value && value <= 90) ? (value + 32) : value;
    }
  };
  constexpr const ascii_tolower_ ascii_tolower{};

  struct ascii_iequal_
  {
    template<typename Char>
    bool operator()(Char left, Char right) const noexcept
    {
      return ascii_tolower(left) == ascii_tolower(right);
    }
  };
  constexpr const ascii_iequal_ ascii_iequal{};

  //// Digest Authentication
  
  struct auth_request
  {
    using iterator = const char*;
    enum status{ kFail = 0, kStale, kPass };

    static status verify(const std::string& method, const std::string& request,
      const epee::net_utils::http::http_auth::session& user)
    {
      struct parser
      {
        using field_parser = std::function<bool(const parser&, iterator&, iterator, auth_request&)>;

        explicit parser() : field_table(), skip_whitespace(), header(), token(), fields() {
          using namespace std::placeholders;
          namespace qi = boost::spirit::qi;

          struct parse_nc
          {
            bool operator()(const parser&, iterator& current, const iterator end, auth_request& result) const
            {
              return qi::parse(
                current, end,
                (qi::raw[qi::uint_parser<std::uint32_t, 16, 8, 8>{}]),
                result.nc
              );
            }
          };
          struct parse_token
          {
            bool operator()(const parser& parse, iterator& current, const iterator end,
              boost::iterator_range<iterator>& result) const
            {
              return qi::parse(current, end, parse.token, result);
            }
          };
          struct parse_string
          {
            bool operator()(const parser&, iterator& current, const iterator end,
              boost::iterator_range<iterator>& result) const
            {
              return qi::parse(
                current, end,
                (qi::lit(quote) >> qi::raw[+(u8"\\\"" | (qi::ascii::char_ - quote))] >> qi::lit(quote)),
                result
              );
            }
          };
          struct parse_response
          {
            bool operator()(const parser&, iterator& current, const iterator end, auth_request& result) const
            {
              using byte = qi::uint_parser<std::uint8_t, 16, 2, 2>;
              return qi::parse(
                current, end,
                (qi::lit(quote) >> qi::raw[+(byte{})] >> qi::lit(quote)),
                result.response
              );
            }
          };

          field_table.add
            (u8"algorithm", std::bind(parse_token{}, _1, _2, _3, std::bind(&auth_request::algorithm, _4)))
            (u8"cnonce", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_request::cnonce, _4)))
            (u8"nc", parse_nc{})
            (u8"nonce", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_request::nonce, _4)))
            (u8"qop", std::bind(parse_token{}, _1, _2, _3, std::bind(&auth_request::qop, _4)))
            (u8"realm", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_request::realm, _4)))
            (u8"response", parse_response{})
            (u8"uri", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_request::uri, _4)))
            (u8"username", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_request::username, _4)));

          skip_whitespace = *(&qi::ascii::char_ >> qi::ascii::space);
          header = skip_whitespace >> qi::ascii::no_case[u8"digest"] >> skip_whitespace;
          token =
            -qi::lit(quote) >>
            qi::raw[+(&qi::ascii::char_ >> (qi::ascii::graph - qi::ascii::char_(u8"()<>@,;:\\\"/[]?={}")))] >>
            -qi::lit(quote);
          fields = field_table >> skip_whitespace >> equal_sign >> skip_whitespace;
        }

        boost::optional<auth_request> operator()(const std::string& method, const std::string& request) const
        { 
          namespace qi = boost::spirit::qi;
           
          iterator current = request.data();
          const iterator end = current + request.size();

          if (!qi::parse(current, end, header))
          {
            return boost::none;
          }

          auth_request info(method);
          field_parser null_parser{};
          std::reference_wrapper<const field_parser> field = null_parser;

          do // require at least one field; require field after `,` character
          {
            if (!qi::parse(current, end, fields, field) || !field(*this, current, end, info))
            {
              return boost::none;
            }
            qi::parse(current, end, skip_whitespace);
          } while (qi::parse(current, end, qi::char_(comma) >> skip_whitespace));
          return boost::make_optional(current == end, info);
        }

      private:
        boost::spirit::qi::symbols<
          char, field_parser, boost::spirit::qi::tst<char, field_parser>, ascii_tolower_
        > field_table;
        boost::spirit::qi::rule<iterator> skip_whitespace;
        boost::spirit::qi::rule<iterator> header;
        boost::spirit::qi::rule<iterator, boost::iterator_range<iterator>()> token;
        boost::spirit::qi::rule<iterator, std::reference_wrapper<const field_parser>()> fields;
      }; // parser

      static const parser parse;
      return do_verify(parse(method, request), user);
    }

  private:
    explicit auth_request(const std::string& method_)
      : algorithm()
      , cnonce()
      , method(method_)
      , nc()
      , nonce()
      , qop()
      , realm()
      , response()
      , uri()
      , username() {
    }

    struct has_valid_response
    {
      template<typename Digest, typename Result>
      Result generate_old_response(Digest digest, const Result& key, const Result& auth) const
      {
        return digest(key, u8":", request.nonce, u8":", auth);
      }

      template<typename Digest, typename Result>
      Result generate_new_response(Digest digest, const Result& key, const Result& auth) const
      {
        return digest(
          key, u8":", request.nonce, u8":", request.nc, u8":", request.cnonce, u8":", request.qop, u8":", auth
        );
      }

      template<typename Digest>
      typename std::result_of<Digest()>::type generate_auth(Digest digest) const
      {
        return digest(request.method, u8":", request.uri);
      }

      template<typename Result>
      bool check(const Result& result) const
      {
        return boost::equals(request.response, result, ascii_iequal);
      }

      template<typename Digest>
      bool operator()(const Digest& digest) const
      {
        if (boost::starts_with(request.algorithm, Digest::name, ascii_iequal) ||
            (request.algorithm.empty() && std::is_same<md5_, Digest>::value))
        {
          auto key = digest(user.credentials.username, u8":", auth_realm, u8":", user.credentials.password);

          if (boost::ends_with(request.algorithm, sess_algo, ascii_iequal))
          {
            key = digest(key, u8":", request.nonce, u8":", request.cnonce);
          }

          if (request.qop.empty())
          {
            return check(generate_old_response(digest, std::move(key), generate_auth(digest)));
          }
          else if (boost::equals(ceref(u8"auth"), request.qop, ascii_iequal))
          {
            return check(generate_new_response(digest, std::move(key), generate_auth(digest)));
          }
        }
        return false;
      }

      const auth_request& request;
      const epee::net_utils::http::http_auth::session& user;
    };

    static status do_verify(const boost::optional<auth_request>& request,
      const epee::net_utils::http::http_auth::session& user)
    {
      if (request && 
          boost::equals(request->username, user.credentials.username) &&
          boost::fusion::any(digest_algorithms, has_valid_response{*request, user}))
      {
        if (boost::equals(request->nonce, user.nonce))
        {
          // RFC 2069 format does not verify nc value - allow just once
          if (user.counter == 1 || (!request->qop.empty() && request->counter() == user.counter))
          {
            return kPass;
          }
        }
        return kStale;
      }
      return kFail;
    }

    boost::optional<std::uint32_t> counter() const
    {
      namespace qi = boost::spirit::qi;
      using hex = qi::uint_parser<std::uint32_t, 16>;
      std::uint32_t value = 0;
      const bool converted = qi::parse(nc.begin(), nc.end(), hex{}, value);
      return boost::make_optional(converted, value);
    }

    boost::iterator_range<iterator> algorithm;
    boost::iterator_range<iterator> cnonce;
    boost::string_ref method;
    boost::iterator_range<iterator> nc;
    boost::iterator_range<iterator> nonce;
    boost::iterator_range<iterator> qop;
    boost::iterator_range<iterator> realm;
    boost::iterator_range<iterator> response;
    boost::iterator_range<iterator> uri;
    boost::iterator_range<iterator> username; 
  }; // auth_request

  struct add_challenge
  {
    template<typename T>
    static void add_field(std::string& str, const char* const name, const T& value)
    {
      str.push_back(comma);
      str.append(name);
      str.push_back(equal_sign);
      boost::range::copy(value, std::back_inserter(str));
    }

    template<typename T>
    using quoted_result = boost::joined_range<
      const boost::joined_range<const boost::string_ref, const T>, const boost::string_ref
    >;

    template<typename T>
    static quoted_result<T> quoted(const T& arg)
    {
      return boost::range::join(boost::range::join(ceref(u8"\""), arg), ceref(u8"\""));
    }

    template<typename Digest>
    void operator()(const Digest& digest) const
    {
      static constexpr const auto fname = ceref(u8"WWW-authenticate");
      static constexpr const auto fvalue = ceref(u8"Digest qop=\"auth\"");

      for (unsigned i = 0; i < 2; ++i)
      {
        std::string out(fvalue);

        const auto algorithm = boost::range::join(
          Digest::name, (i == 0 ? boost::string_ref{} : sess_algo)
        );
        add_field(out, u8"algorithm", algorithm);
        add_field(out, u8"realm", quoted(auth_realm));
        add_field(out, u8"nonce", quoted(nonce));
        add_field(out, u8"stale", is_stale ? ceref("true") : ceref("false"));
        
        fields.push_back(std::make_pair(std::string(fname), std::move(out)));
      }
    }

    const std::string& nonce;
    std::list<std::pair<std::string, std::string>>& fields;
    const bool is_stale;
  };

  epee::net_utils::http::http_response_info create_digest_response(
    const std::string& nonce, const bool is_stale)
  {
    epee::net_utils::http::http_response_info rc{};
    rc.m_response_code = 401;
    rc.m_response_comment = u8"Unauthorized";
    rc.m_mime_tipe = u8"text/html";
    rc.m_body = 
      u8"<html><head><title>Unauthorized Access</title></head><body><h1>401 Unauthorized</h1></body></html>";

    boost::fusion::for_each(
      digest_algorithms, add_challenge{nonce, rc.m_additional_fields, is_stale}
    );
    
    return rc;
  }
}

namespace epee
{
  namespace net_utils
  {
    namespace http
    {
      http_auth::http_auth(login credentials)
        : user(session{std::move(credentials)}) {
      }

      boost::optional<http_response_info> http_auth::process(const http_request_info& request)
      {
        assert(user);
        using field = std::pair<std::string, std::string>;

        const std::list<field>& fields = request.m_header_info.m_etc_fields;
        const auto auth = boost::find_if(fields, [] (const field& value) {
          return boost::equals(ceref(u8"authorization"), value.first, ascii_iequal);
        });

        bool is_stale = false;
        if (auth != fields.end())
        {
          ++(user->counter);
          switch (auth_request::verify(request.m_http_method_str, auth->second, *user))
          {
          case auth_request::kPass:
            return boost::none;

          case auth_request::kStale:
            is_stale = true;
            break;

          default:
          case auth_request::kFail:
            break;
          }
        }
        user->counter = 0;
        {
          std::array<std::uint8_t, 16> rand_128bit{{}};
          crypto::rand(rand_128bit.size(), rand_128bit.data());
          user->nonce = string_encoding::base64_encode(rand_128bit.data(), rand_128bit.size());
        }
        return create_digest_response(user->nonce, is_stale);
      }
    }
  }
}

