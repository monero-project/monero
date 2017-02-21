// Copyright (c) 2017, The Monero Project
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

#include <string>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include "cryptonote_config.h"
#include "include_base_utils.h"
#include "net/http_client.h"
#include "download.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.dl"

namespace tools
{
  static bool download_thread(const std::string &path, const std::string &url)
  {
    try
    {
      MINFO("Downloading " << url << " to " << path);
      std::ofstream f;
      f.open(path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
      if (!f.good()) {
        MERROR("Failed to open file " << path);
        return false;
      }
      class download_client: public epee::net_utils::http::http_simple_client
      {
      public:
        download_client(std::ofstream &f): f(f) {}
        virtual ~download_client() { f.close(); }
        virtual bool handle_target_data(std::string &piece_of_transfer)
        {
          try
          {
            f << piece_of_transfer;
            return f.good();
          }
          catch (const std::exception &e)
          {
            MERROR("Error writing data: " << e.what());
            return false;
          }
        }
      private:
        std::ofstream &f;
      } client(f);
      epee::net_utils::http::url_content u_c;
      if (!epee::net_utils::parse_url(url, u_c))
      {
        MWARNING("Failed to parse URL " << url);
        return false;
      }
      if (u_c.host.empty())
      {
        MWARNING("Failed to determine address from URL " << url);
        return false;
      }
      uint16_t port = u_c.port ? u_c.port : 80;
      MDEBUG("Connecting to " << u_c.host << ":" << port);
      client.set_server(u_c.host, std::to_string(port), boost::none);
      if (!client.connect(std::chrono::seconds(30)))
      {
        MERROR("Failed to connect to " << url);
        return false;
      }
      MDEBUG("GETting " << u_c.uri);
      const epee::net_utils::http::http_response_info *info = NULL;
      if (!client.invoke_get(u_c.uri, std::chrono::seconds(30), "", &info))
      {
        MERROR("Failed to connect to " << url);
        client.disconnect();
        return false;
      }
      if (!info)
      {
        MERROR("Failed invoking GET command to " << url << ", no status info returned");
        client.disconnect();
        return false;
      }
      MDEBUG("response code: " << info->m_response_code);
      MDEBUG("response comment: " << info->m_response_comment);
      MDEBUG("response body: " << info->m_body);
      for (const auto &f: info->m_additional_fields)
        MDEBUG("additional field: " << f.first << ": " << f.second);
      if (info->m_response_code != 200)
      {
        MERROR("Status code " << info->m_response_code);
        client.disconnect();
        return false;
      }
      client.disconnect();
      f.close();
      MDEBUG("Download complete");
      return true;
    }
    catch (const std::exception &e)
    {
      MERROR("Exception in download thread: " << e.what());
      return false;
    }
  }

  bool download(const std::string &path, const std::string &url)
  {
    bool success;
    std::unique_ptr<boost::thread> thread(new boost::thread([&](){ success = download_thread(path, url); }));
    thread->join();
    return success;
  }
}
