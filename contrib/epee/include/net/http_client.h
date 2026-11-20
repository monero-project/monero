// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <boost/optional/optional.hpp>
#include <boost/utility/string_ref.hpp>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "net_helper.h"
#include "string_tools.h"
#include "string_tools_lexical.h"
#include "abstract_http_client.h"
#include "http_base.h"
#include "http_auth.h"
#include "net_parse_helpers.h"
#include "syncobj.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

namespace epee
{
namespace net_utils
{
namespace http
{
  struct i_response_handler
  {
    virtual ~i_response_handler() {}

    //! False stops the response
    virtual bool on_header(const http_response_info& headers) = 0;

    //! False stops the response
    virtual bool handle_target_data(std::string& piece_of_transfer) = 0;
  };

  //! Incremental HTTP response parser, backed by Boost.Beast
  class response_parser
  {
  public:
    enum class status
    {
      need_more,
      done,
      failed
    };

    response_parser(http_response_info& info, i_response_handler& handler, bool head);
    ~response_parser() noexcept;

    response_parser(const response_parser&) = delete;
    response_parser& operator=(const response_parser&) = delete;

    //! Bytes after the end of the response are an error
    status put(boost::string_ref data);

    status put_eof();

    //! Valid once `done`
    bool keep_alive() const noexcept;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
  };

  //! Empty if any part would need escaping (CR, LF, etc.)
  std::string make_request_header(boost::string_ref method, boost::string_ref uri,
                                  boost::string_ref host, std::size_t content_length,
                                  const fields_list& fields,
                                  const boost::optional<std::pair<std::string, std::string>>& auth);

  template<typename net_client_type>
  class http_simple_client_template : public i_response_handler, public abstract_http_client
  {
  private:
    net_client_type m_net_client;
    std::string m_host_buff;
    std::string m_port;
    http_client_auth m_auth;
    http_response_info m_response_info;
    bool m_auto_connect;
    critical_section m_lock;

  public:
    explicit http_simple_client_template()
      : i_response_handler(), abstract_http_client()
      , m_net_client()
      , m_host_buff()
      , m_port()
      , m_auth()
      , m_response_info()
      , m_auto_connect(true)
      , m_lock()
    {}

    const std::string &get_host() const { return m_host_buff; };
    const std::string &get_port() const { return m_port; };

    using abstract_http_client::set_server;

    void set_server(std::string host, std::string port, boost::optional<login> user, ssl_options_t ssl_options = ssl_support_t::e_ssl_support_autodetect) override
    {
      CRITICAL_REGION_LOCAL(m_lock);
      disconnect();
      m_net_client.set_ssl(std::move(ssl_options));
      m_host_buff = std::move(host);
      m_port = std::move(port);
      m_auth = user ? http_client_auth{std::move(*user)} : http_client_auth{};
    }

    void set_auto_connect(bool auto_connect) override
    {
      m_auto_connect = auto_connect;
    }

    template<typename F>
    void set_connector(F connector)
    {
      CRITICAL_REGION_LOCAL(m_lock);
      m_net_client.set_connector(std::move(connector));
    }

    bool connect(std::chrono::milliseconds timeout) override
    {
      CRITICAL_REGION_LOCAL(m_lock);
      return m_net_client.connect(m_host_buff, m_port, timeout);
    }
    //---------------------------------------------------------------------------
    bool disconnect() override
    {
      CRITICAL_REGION_LOCAL(m_lock);
      return m_net_client.disconnect();
    }
    //---------------------------------------------------------------------------
    bool shutdown() override
    {
      return m_net_client.shutdown(); // no m_lock; interrupts an invoke blocked while holding it
    }
    //---------------------------------------------------------------------------
    bool is_connected(bool *ssl = NULL) override
    {
      CRITICAL_REGION_LOCAL(m_lock);
      return m_net_client.is_connected(ssl);
    }
    //---------------------------------------------------------------------------
    bool handle_target_data(std::string& piece_of_transfer) override
    {
      CRITICAL_REGION_LOCAL(m_lock);
      m_response_info.m_body += piece_of_transfer;
      piece_of_transfer.clear();
      return true;
    }
    //---------------------------------------------------------------------------
    bool on_header(const http_response_info &headers) override
    {
      return true;
    }
    //---------------------------------------------------------------------------
    inline bool invoke_get(const boost::string_ref uri, std::chrono::milliseconds timeout, const std::string& body = std::string(), const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list()) override
    {
      CRITICAL_REGION_LOCAL(m_lock);
      return invoke(uri, "GET", body, timeout, ppresponse_info, additional_params);
    }
    //---------------------------------------------------------------------------
    inline bool invoke(const boost::string_ref uri, const boost::string_ref method, const boost::string_ref body, std::chrono::milliseconds timeout, const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list()) override
    {
      CRITICAL_REGION_LOCAL(m_lock);
      for (unsigned sends = 0; sends < 2; ++sends)
      {
        if(!is_connected())
        {
          if (!m_auto_connect)
          {
            MWARNING("Auto connect attempt to " << m_host_buff << ":" << m_port << " disabled");
            return false;
          }
          MDEBUG("Reconnecting...");
          if(!connect(timeout))
          {
            MDEBUG("Failed to connect to " << m_host_buff << ":" << m_port);
            return false;
          }
        }

        const std::string header = make_request_header(
          method, uri, m_host_buff, body.size(), additional_params, m_auth.get_auth_field(method, uri));
        CHECK_AND_ASSERT_MES(!header.empty(), false, "HTTP_CLIENT: Invalid request");

        bool res = m_net_client.send(header, timeout);
        CHECK_AND_ASSERT_MES(res, false, "HTTP_CLIENT: Failed to SEND");
        if(body.size())
          res = m_net_client.send(body, timeout);
        CHECK_AND_ASSERT_MES(res, false, "HTTP_CLIENT: Failed to SEND");

        if (!handle_reciev(timeout, method == "HEAD"))
          return false;
        if (m_response_info.m_response_code != 401)
        {
          if(ppresponse_info)
            *ppresponse_info = std::addressof(m_response_info);
          return true;
        }

        switch (m_auth.handle_401(m_response_info))
        {
        case http_client_auth::kSuccess:
          break;
        case http_client_auth::kBadPassword:
          sends = 2;
          break;
        default:
        case http_client_auth::kParseFailure:
          LOG_ERROR("Bad server response for authentication");
          return false;
        }
      }
      LOG_ERROR("Client has incorrect username/password for server requiring authentication");
      return false;
    }
    //---------------------------------------------------------------------------
    bool test(const std::string &s, std::chrono::milliseconds timeout) // TEST FUNC ONLY
    {
      CRITICAL_REGION_LOCAL(m_lock);
      m_net_client.set_test_data(s);
      return handle_reciev(timeout, false);
    }
    //---------------------------------------------------------------------------
    uint64_t get_bytes_sent() const override
    {
      return m_net_client.get_bytes_sent();
    }
    //---------------------------------------------------------------------------
    uint64_t get_bytes_received() const override
    {
      return m_net_client.get_bytes_received();
    }
    //---------------------------------------------------------------------------
    void wipe_response()
    {
      m_response_info.wipe();
    }
    //---------------------------------------------------------------------------
  private:
    //---------------------------------------------------------------------------
    inline bool handle_reciev(std::chrono::milliseconds timeout, bool head)
    {
      CRITICAL_REGION_LOCAL(m_lock);
      m_response_info.clear();
      response_parser::status status = response_parser::status::failed;
      bool keep_alive = false;
      try
      {
        response_parser parser{m_response_info, *this, head};
        std::string recv_buffer;
        do
        {
          if(!m_net_client.recv(recv_buffer, timeout))
          {
            MERROR("Unexpected recv fail");
            status = response_parser::status::failed;
            break;
          }
          status = recv_buffer.empty() ? parser.put_eof() : parser.put(recv_buffer);
        } while (status == response_parser::status::need_more);
        keep_alive = status == response_parser::status::done && parser.keep_alive();
      }
      catch (const std::exception& e)
      {
        LOG_ERROR("http_simple_client_template::handle_reciev(): exception while processing response, resetting connection: " << e.what());
        status = response_parser::status::failed;
      }
      catch (...)
      {
        LOG_ERROR("http_simple_client_template::handle_reciev(): unknown exception while processing response, resetting connection");
        status = response_parser::status::failed;
      }

      if (status != response_parser::status::done)
      {
        m_response_info.clear();
        disconnect();
        return false;
      }
      if (!keep_alive)
        disconnect();
      return true;
    }
  };
  typedef http_simple_client_template<blocked_mode_client> http_simple_client;
}
}
}
