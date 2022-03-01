// Copyright (c) 2014-2022, The Monero Project
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
#pragma once

#include <boost/optional/optional.hpp>
#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include "wipeable_string.h"
#include "http_base.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

namespace epee
{
namespace net_utils
{
  namespace http
  {
    struct login
    {
      login() : username(), password() {}
      login(std::string username_, wipeable_string password_)
        : username(std::move(username_)), password(std::move(password_))
      {}

      std::string username;
      wipeable_string password;
    };

    //! Implements RFC 2617 digest auth. Digests from RFC 7616 can be added.
    class http_server_auth
    {
    public:
      struct session
      {
        session(login credentials_)
          : credentials(std::move(credentials_)), nonce(), counter(0)
        {}

        login credentials;
        std::string nonce;
        std::uint32_t counter;
      };

      http_server_auth() : user(), rng() {}
      http_server_auth(login credentials, std::function<void(size_t, uint8_t*)> r);

      //! \return Auth response, or `boost::none` iff `request` had valid auth.
      boost::optional<http_response_info> get_response(const http_request_info& request)
      {
        if (user)
          return do_get_response(request);
        return boost::none;
      }

    private:
      boost::optional<http_response_info> do_get_response(const http_request_info& request);

      boost::optional<session> user;

      std::function<void(size_t, uint8_t*)> rng;
    };

    //! Implements RFC 2617 digest auth. Digests from RFC 7616 can be added.
    class http_client_auth
    {
    public:
      enum status : std::uint8_t { kSuccess = 0, kBadPassword, kParseFailure };

      struct session
      {
        session(login credentials_)
          : credentials(std::move(credentials_)), server(), counter(0)
        {}

        struct keys
        {
          using algorithm =
            std::function<std::string(const session&, boost::string_ref, boost::string_ref)>;

          keys() : nonce(), opaque(), realm(), generator() {}
          keys(std::string nonce_, std::string opaque_, std::string realm_, algorithm generator_)
            : nonce(std::move(nonce_))
            , opaque(std::move(opaque_))
            , realm(std::move(realm_))
            , generator(std::move(generator_))
          {}

          std::string nonce;
          std::string opaque;
          std::string realm;
          algorithm generator;
        };

        login credentials;
        keys server;
        std::uint32_t counter;
      };

      http_client_auth() : user() {}
      http_client_auth(login credentials);

      /*!
        Clients receiving a 401 response code from the server should call this
        function to process the server auth. Then, before every client request,
        `get_auth_field()` should be called to retrieve the newest
        authorization request.

        \return `kBadPassword` if client will never be able to authenticate,
          `kParseFailure` if all server authentication responses were invalid,
          and `kSuccess` if `get_auth_field` is ready to generate authorization
          fields.
      */
      status handle_401(const http_response_info& response)
      {
        if (user)
          return do_handle_401(response);
        return kBadPassword;
      }

      /*!
        After calling `handle_401`, clients should call this function to
        generate an authentication field for every request.

        \return A HTTP "Authorization" field if `handle_401(...)` previously
          returned `kSuccess`.
      */
      boost::optional<std::pair<std::string, std::string>> get_auth_field(
        const boost::string_ref method, const boost::string_ref uri)
      {
        if (user)
          return do_get_auth_field(method, uri);
        return boost::none;
      }

    private:
      status do_handle_401(const http_response_info&);
      boost::optional<std::pair<std::string, std::string>> do_get_auth_field(boost::string_ref, boost::string_ref);

      boost::optional<session> user;
    };
  }
}
}
