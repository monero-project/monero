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
#include "net/http_auth.h"

#include <array>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/iteration/iter_fold.hpp>
#include <boost/fusion/algorithm/query/any.hpp>
#include <boost/fusion/iterator/distance.hpp>
#include <boost/fusion/iterator/value_of.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/range/join.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_and_predicate.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_char_class.hpp>
#include <boost/spirit/include/qi_difference.hpp>
#include <boost/spirit/include/qi_kleene.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_plus.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/include/qi_not_predicate.hpp>
#include <boost/spirit/include/qi_raw.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_sequence.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_symbols.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <cassert>
#include <iterator>
#include <limits>
#include <tuple>
#include <type_traits>

#include "crypto/crypto.h"
#include "hex.h"
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
  namespace http = epee::net_utils::http;

  // string_ref is only constexpr if length is given
  template<std::size_t N>
  constexpr boost::string_ref ceref(const char (&arg)[N])
  {
    return boost::string_ref(arg, N - 1);
  }

  constexpr const auto client_auth_field = ceref(u8"Authorization");
  constexpr const auto server_auth_field = ceref(u8"WWW-authenticate");
  constexpr const auto auth_realm = ceref(u8"monero-rpc");
  constexpr const char comma = 44;
  constexpr const char equal_sign = 61;
  constexpr const char quote = 34;
  constexpr const char zero = 48;
  constexpr const auto sess_algo = ceref(u8"-sess");

  constexpr const unsigned client_reserve_size = 512; //!< std::string::reserve size for clients

  //// Digest Algorithms

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
      return epee::to_hex::array(digest);
    }
  };
  constexpr const boost::string_ref md5_::name;

  //! Digest Algorithms available for HTTP Digest Auth. Sort better algos to the left
  constexpr const std::tuple<md5_> digest_algorithms{};

  //// Various String Utilities

  struct ascii_tolower_
  {
    template<typename Char>
    constexpr Char operator()(Char value) const noexcept
    {
      static_assert(std::is_integral<Char>::value, "only integral types supported");
      return (65 <= value && value <= 90) ? (value + 32) : value;
    }
  };
  constexpr const ascii_tolower_ ascii_tolower{};

  struct ascii_iequal_
  {
    template<typename Char>
    constexpr bool operator()(Char left, Char right) const noexcept
    {
      return ascii_tolower(left) == ascii_tolower(right);
    }
  };
  constexpr const ascii_iequal_ ascii_iequal{};

  struct http_list_separator_
  {
    template<typename Char>
    bool operator()(Char value) const noexcept
    {
      static_assert(std::is_integral<Char>::value, "only integral types supported");
      return boost::spirit::char_encoding::ascii::isascii_(value) &&
        (value == comma || boost::spirit::char_encoding::ascii::isspace(value));
    }
  };
  constexpr const http_list_separator_ http_list_separator{};

  std::string to_string(boost::iterator_range<const char*> source)
  {
    return {source.begin(), source.size()};
  }

  template<typename T>
  void add_first_field(std::string& str, const char* const name, const T& value)
  {
    str.append(name);
    str.push_back(equal_sign);
    boost::copy(value, std::back_inserter(str));
  }

  template<typename T>
  void add_field(std::string& str, const char* const name, const T& value)
  {
    str.push_back(comma);
    add_first_field(str, name, value);
  }

  template<typename T>
  using quoted_result = boost::joined_range<
    const boost::joined_range<const boost::string_ref, const T>, const boost::string_ref
  >;

  template<typename T>
  quoted_result<T> quoted(const T& arg)
  {
    return boost::range::join(boost::range::join(ceref(u8"\""), arg), ceref(u8"\""));
  }

  //// Digest Authentication

  template<typename Digest>
  typename std::result_of<Digest()>::type generate_a1(
    Digest digest, const http::login& creds, const boost::string_ref realm)
  {
    return digest(creds.username, u8":", realm, u8":", creds.password);
  }

  template<typename Digest>
  typename std::result_of<Digest()>::type generate_a1(
    Digest digest, const http::http_client_auth::session& user)
  {
    return generate_a1(std::move(digest), user.credentials, user.server.realm);
  }

  template<typename T>
  void init_client_value(std::string& str,
    const boost::string_ref algorithm, const http::http_client_auth::session& user,
    const boost::string_ref uri, const T& response)
  {
    str.append(u8"Digest ");
    add_first_field(str, u8"algorithm", algorithm);
    add_field(str, u8"nonce", quoted(user.server.nonce));
    add_field(str, u8"realm", quoted(user.server.realm));
    add_field(str, u8"response", quoted(response));
    add_field(str, u8"uri", quoted(uri));
    add_field(str, u8"username", quoted(user.credentials.username));
    if (!user.server.opaque.empty())
      add_field(str, u8"opaque", quoted(user.server.opaque));
  }

  //! Implements superseded algorithm specified in RFC 2069
  template<typename Digest>
  struct old_algorithm
  {
    explicit old_algorithm(Digest digest_) : digest(std::move(digest_)) {}

    std::string operator()(const http::http_client_auth::session& user,
      const boost::string_ref method, const boost::string_ref uri) const
    {
      const auto response = digest(
        generate_a1(digest, user), u8":", user.server.nonce, u8":", digest(method, u8":", uri)
      );
      std::string out{};
      out.reserve(client_reserve_size);
      init_client_value(out, Digest::name, user, uri, response);
      return out;
    }
  private:
    Digest digest;
  };

  //! Implements the `qop=auth` algorithm specified in RFC 2617
  template<typename Digest>
  struct auth_algorithm
  {
    explicit auth_algorithm(Digest digest_) : digest(std::move(digest_)) {}

    std::string operator()(const http::http_client_auth::session& user,
      const boost::string_ref method, const boost::string_ref uri) const
    {
      namespace karma = boost::spirit::karma;
      using counter_type = decltype(user.counter);
      static_assert(
        std::numeric_limits<counter_type>::radix == 2, "unexpected radix for counter"
      );
      static_assert(
        std::numeric_limits<counter_type>::digits <= 32,
        "number of digits will cause underflow on padding below"
      );

      std::string out{};
      out.reserve(client_reserve_size);

      karma::generate(std::back_inserter(out), karma::hex(user.counter));
      out.insert(out.begin(), 8 - out.size(), zero); // zero left pad
      if (out.size() != 8)
        return {};

      std::array<char, 8> nc{{}};
      boost::copy(out, nc.data());
      const auto response = digest(
        generate_a1(digest, user), u8":", user.server.nonce, u8":", nc, u8"::auth:", digest(method, u8":", uri)
      );
      out.clear();
      init_client_value(out, Digest::name, user, uri, response);
      add_field(out, u8"qop", ceref(u8"auth"));
      add_field(out, u8"nc", nc);
      return out;
    }

  private:
    Digest digest;
  };

  //! Processes client "Authorization" and server "WWW-authenticate" HTTP fields
  struct auth_message
  {
    using iterator = const char*;
    enum status{ kFail = 0, kStale, kPass };

    //! \return Status of the `response` field from the client
    static status verify(const boost::string_ref method, const boost::string_ref request,
      const http::http_server_auth::session& user)
    {
      const auto parsed = parse(request);
      if (parsed &&
          boost::equals(parsed->username, user.credentials.username) &&
          boost::fusion::any(digest_algorithms, has_valid_response{*parsed, user, method}))
      {
        if (boost::equals(parsed->nonce, user.nonce))
        {
          // RFC 2069 format does not verify nc value - allow just once
          if (user.counter == 1 || (!parsed->qop.empty() && parsed->counter() == user.counter))
          {
            return kPass;
          }
        }
        return kStale;
      }
      return kFail;
    }

    //! \return Information needed to generate client authentication `response`s.
    static http::http_client_auth::session::keys extract(
      const http::http_response_info& response, const bool is_first)
    {
      using field = std::pair<std::string, std::string>;

      server_parameters best{};

      const std::list<field>& fields = response.m_header_info.m_etc_fields;
      auto current = fields.begin();
      const auto end = fields.end();
      while (true)
      {
        current = std::find_if(current, end, [] (const field& value) {
          return boost::equals(server_auth_field, value.first, ascii_iequal);
        });
        if (current == end)
          break;

        const auto parsed = parse(current->second);
        if (parsed)
        {
          server_parameters local_best = parsed->algorithm.empty() ?
            server_parameters{*parsed, boost::fusion::find<md5_>(digest_algorithms)} :
            boost::fusion::iter_fold(digest_algorithms, server_parameters{}, matches_algorithm{*parsed});

          if (local_best.index < best.index)
            best = std::move(local_best);
        }
        ++current;
      }
      if (is_first || boost::equals(best.stale, ceref(u8"true"), ascii_iequal))
        return best.take();
      return {}; // authentication failed with bad user/pass
    }

  private:
    explicit auth_message()
      : algorithm()
      , cnonce()
      , nc()
      , nonce()
      , qop()
      , realm()
      , response()
      , stale()
      , uri()
      , username() {
    }

    static boost::optional<auth_message> parse(const boost::string_ref request)
    {
      struct parser
      {
        using field_parser = std::function<bool(const parser&, iterator&, iterator, auth_message&)>;

        explicit parser() : field_table(), skip_whitespace(), header(), quoted_string(), token(), fields() {
          using namespace std::placeholders;
          namespace qi = boost::spirit::qi;

          struct parse_nc
          {
            bool operator()(const parser&, iterator& current, const iterator end, auth_message& result) const
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
            bool operator()(const parser& parse, iterator& current, const iterator end,
              boost::iterator_range<iterator>& result) const
            {
              return qi::parse(current, end, parse.quoted_string, result);
            }
            bool operator()(const parser& parse, iterator& current, const iterator end) const
            {
              return qi::parse(current, end, parse.quoted_string);
            }
          };
          struct parse_response
          {
            bool operator()(const parser&, iterator& current, const iterator end, auth_message& result) const
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
            (u8"algorithm", std::bind(parse_token{}, _1, _2, _3, std::bind(&auth_message::algorithm, _4)))
            (u8"cnonce", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_message::cnonce, _4)))
            (u8"domain", std::bind(parse_string{}, _1, _2, _3)) // ignore field
            (u8"nc", parse_nc{})
            (u8"nonce", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_message::nonce, _4)))
            (u8"opaque", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_message::opaque, _4)))
            (u8"qop", std::bind(parse_token{}, _1, _2, _3, std::bind(&auth_message::qop, _4)))
            (u8"realm", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_message::realm, _4)))
            (u8"response", parse_response{})
            (u8"stale", std::bind(parse_token{}, _1, _2, _3, std::bind(&auth_message::stale, _4)))
            (u8"uri", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_message::uri, _4)))
            (u8"username", std::bind(parse_string{}, _1, _2, _3, std::bind(&auth_message::username, _4)));

          skip_whitespace = *(&qi::ascii::char_ >> qi::ascii::space);
          header = skip_whitespace >> qi::ascii::no_case[u8"digest"] >> skip_whitespace;
          quoted_string = (qi::lit(quote) >> qi::raw[+(u8"\\\"" | (qi::ascii::char_ - quote))] >> qi::lit(quote));
          token =
            (!qi::lit(quote) >> qi::raw[+(&qi::ascii::char_ >> (qi::ascii::graph - qi::ascii::char_(u8"()<>@,;:\\\"/[]?={}")))]) |
            quoted_string;
          fields = field_table >> skip_whitespace >> equal_sign >> skip_whitespace;
        }

        boost::optional<auth_message> operator()(const boost::string_ref request) const
        { 
          namespace qi = boost::spirit::qi;
           
          iterator current = request.begin();
          const iterator end = request.end();

          if (!qi::parse(current, end, header))
          {
            return boost::none;
          }

          auth_message info{};
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
        boost::spirit::qi::rule<iterator, boost::iterator_range<iterator>()> quoted_string;
        boost::spirit::qi::rule<iterator, boost::iterator_range<iterator>()> token;
        boost::spirit::qi::rule<iterator, std::reference_wrapper<const field_parser>()> fields;
      }; // parser

      static const parser parse_;
      return parse_(request);
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
          auto key = generate_a1(digest, user.credentials, auth_realm);
          if (boost::ends_with(request.algorithm, sess_algo, ascii_iequal))
          {
            key = digest(key, u8":", request.nonce, u8":", request.cnonce);
          }

          auto auth = digest(method, u8":", request.uri);
          if (request.qop.empty())
          {
            return check(generate_old_response(std::move(digest), std::move(key), std::move(auth)));
          }
          else if (boost::equals(ceref(u8"auth"), request.qop, ascii_iequal))
          {
            return check(generate_new_response(std::move(digest), std::move(key), std::move(auth)));
          }
        }
        return false;
      }

      const auth_message& request;
      const http::http_server_auth::session& user;
      const boost::string_ref method;
    };

    boost::optional<std::uint32_t> counter() const
    {
      namespace qi = boost::spirit::qi;
      using hex = qi::uint_parser<std::uint32_t, 16>;
      std::uint32_t value = 0;
      const bool converted = qi::parse(nc.begin(), nc.end(), hex{}, value);
      return boost::make_optional(converted, value);
    }

    struct server_parameters
    {
      server_parameters()
        : nonce(), opaque(), realm(), stale(), value_generator()
        , index(boost::fusion::size(digest_algorithms))
      {}

      template<typename DigestIter>
      explicit server_parameters(const auth_message& request, const DigestIter& digest)
        : nonce(request.nonce)
        , opaque(request.opaque)
        , stale(request.stale)
        , realm(request.realm)
        , value_generator()
        , index(boost::fusion::distance(boost::fusion::begin(digest_algorithms), digest))
      {
        using digest_type = typename boost::fusion::result_of::value_of<DigestIter>::type;

        // debug check internal state of the auth_message class
        assert(
          (std::is_same<digest_type, md5_>::value) ||
          boost::equals((*digest).name, request.algorithm, ascii_iequal)
        );
        if (request.qop.empty())
          value_generator = old_algorithm<digest_type>{*digest};
        else
        {
          for (auto elem = boost::make_split_iterator(request.qop, boost::token_finder(http_list_separator));
               !elem.eof();
               ++elem)
          {
            if (boost::equals(ceref(u8"auth"), *elem, ascii_iequal))
            {
              value_generator = auth_algorithm<digest_type>{*digest};
              break;
            }
          }
          if (!value_generator) // no supported qop mode
            index = boost::fusion::size(digest_algorithms);
        }
      }

      http::http_client_auth::session::keys take()
      {
        return {to_string(nonce), to_string(opaque), to_string(realm), std::move(value_generator)};
      }

      boost::iterator_range<iterator> nonce;
      boost::iterator_range<iterator> opaque;
      boost::iterator_range<iterator> realm;
      boost::iterator_range<iterator> stale;
      http::http_client_auth::session::keys::algorithm value_generator;
      unsigned index;
    };

    struct matches_algorithm
    {
      template<typename DigestIter>
      server_parameters operator()(server_parameters current, const DigestIter& digest) const
      {
        if (!current.value_generator)
        {
          if (boost::equals(response.algorithm, (*digest).name, ascii_iequal))
          {
            current = server_parameters{response, digest};
          }
        }
        return current;
      }
      const auth_message& response;
    };


    boost::iterator_range<iterator> algorithm;
    boost::iterator_range<iterator> cnonce;
    boost::iterator_range<iterator> nc;
    boost::iterator_range<iterator> nonce;
    boost::iterator_range<iterator> opaque;
    boost::iterator_range<iterator> qop;
    boost::iterator_range<iterator> realm;
    boost::iterator_range<iterator> response;
    boost::iterator_range<iterator> stale;
    boost::iterator_range<iterator> uri;
    boost::iterator_range<iterator> username; 
  }; // auth_message

  struct add_challenge
  {
    template<typename Digest>
    void operator()(const Digest& digest) const
    {
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
        
        fields.push_back(std::make_pair(std::string(server_auth_field), std::move(out)));
      }
    }

    const boost::string_ref nonce;
    std::list<std::pair<std::string, std::string>>& fields;
    const bool is_stale;
  };

  http::http_response_info create_digest_response(const boost::string_ref nonce, const bool is_stale)
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
      http_server_auth::http_server_auth(login credentials)
        : user(session{std::move(credentials)}) {
      }

      boost::optional<http_response_info> http_server_auth::do_get_response(const http_request_info& request)
      {
        assert(user);
        using field = std::pair<std::string, std::string>;

        const std::list<field>& fields = request.m_header_info.m_etc_fields;
        const auto auth = boost::find_if(fields, [] (const field& value) {
          return boost::equals(client_auth_field, value.first, ascii_iequal);
        });

        bool is_stale = false;
        if (auth != fields.end())
        {
          ++(user->counter);
          switch (auth_message::verify(request.m_http_method_str, auth->second, *user))
          {
          case auth_message::kPass:
            return boost::none;

          case auth_message::kStale:
            is_stale = true;
            break;

          default:
          case auth_message::kFail:
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

      http_client_auth::http_client_auth(login credentials)
        : user(session{std::move(credentials)}) {
      }

      http_client_auth::status http_client_auth::do_handle_401(const http_response_info& response)
      {
        assert(user);
        const bool first_auth = (user->counter == 0);
        user->server = auth_message::extract(response, first_auth);
        if (user->server.generator)
        {
          user->counter = 0;
          return kSuccess;
        }
        return first_auth ? kParseFailure : kBadPassword;
      }

      boost::optional<std::pair<std::string, std::string>> http_client_auth::do_get_auth_field(
        const boost::string_ref method, const boost::string_ref uri)
      {
        assert(user);
        if (user->server.generator)
        {
          ++(user->counter);
          return std::make_pair(std::string(client_auth_field), user->server.generator(*user, method, uri));
        }
        return boost::none;
      }
    }
  }
}

