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
#pragma once

#include <boost/optional/optional.hpp>
#include <cstdint>
#include "http_base.h"
#include <string>
#include <utility>

namespace epee
{
namespace net_utils
{
  namespace http
  {
    //! Implements RFC 2617 digest auth. Digests from RFC 7616 can be added.
    class http_auth
    {
    public:
      struct login
      {
        login() : username(), password() {}
        login(std::string username_, std::string password_)
          : username(std::move(username_)), password(std::move(password_))
        {}

        std::string username;
        std::string password;
      };

      struct session
      {
        session(login credentials_)
          : credentials(std::move(credentials_)), nonce(), counter(0)
        {}

        login credentials;
        std::string nonce;
        std::uint32_t counter;
      };

      http_auth() : user() {}
      http_auth(login credentials);

      //! \return Auth response, or `boost::none` iff `request` had valid auth.
      boost::optional<http_response_info> get_response(const http_request_info& request)
      {
        if (user)
        {
          return process(request);
        }
        return boost::none;
      }

    private:
      boost::optional<http_response_info> process(const http_request_info& request);

      boost::optional<session> user;
    };
  }
}
}
