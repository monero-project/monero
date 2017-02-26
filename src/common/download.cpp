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
  struct download_thread_control
  {
    const std::string path;
    const std::string uri;
    std::function<void(bool)> result_cb;
    std::function<bool(size_t, ssize_t)> progress_cb;
    bool stop;
    bool stopped;
    bool success;
    std::shared_ptr<boost::thread> thread;
    boost::mutex mutex;

    download_thread_control(const std::string &path, const std::string &uri, std::function<void(bool)> result_cb, std::function<bool(size_t, ssize_t)> progress_cb):
        path(path), uri(uri), result_cb(result_cb), progress_cb(progress_cb), stop(false), stopped(false), success(false) {}
    void lock() { mutex.lock(); }
    void unlock() { mutex.unlock(); }
  };

  static void download_thread(download_thread_control &control)
  {
    try
    {
      control.lock();
      MINFO("Downloading " << control.uri << " to " << control.path);
      std::ofstream f;
      f.open(control.path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
      if (!f.good()) {
        MERROR("Failed to open file " << control.path);
        control.stopped = true;
        control.result_cb(control.success);
        control.unlock();
        return;
      }
      class download_client: public epee::net_utils::http::http_simple_client
      {
      public:
        download_client(download_thread_control &control, std::ofstream &f):
          control(control), f(f), content_length(-1), total(0) {}
        virtual ~download_client() { f.close(); }
        virtual bool on_header(const epee::net_utils::http::http_response_info &headers)
        {
          ssize_t length;
          if (epee::string_tools::get_xtype_from_string(length, headers.m_header_info.m_content_length) && length >= 0)
          {
            MINFO("Content-Length: " << length);
            content_length = length;
          }
          return true;
        }
        virtual bool handle_target_data(std::string &piece_of_transfer)
        {
          try
          {
            control.lock();
            if (control.stop)
            {
              control.unlock();
              return false;
            }
            f << piece_of_transfer;
            total += piece_of_transfer.size();
            if (control.progress_cb && !control.progress_cb(total, content_length))
            {
              control.unlock();
              return false;
            }
            control.unlock();
            return f.good();
          }
          catch (const std::exception &e)
          {
            MERROR("Error writing data: " << e.what());
            return false;
          }
        }
      private:
        download_thread_control &control;
        std::ofstream &f;
        ssize_t content_length;
        size_t total;
      } client(control, f);
      epee::net_utils::http::url_content u_c;
      if (!epee::net_utils::parse_url(control.uri, u_c))
      {
        MWARNING("Failed to parse URL " << control.uri);
        control.stopped = true;
        control.result_cb(control.success);
        control.unlock();
        return;
      }
      if (u_c.host.empty())
      {
        MWARNING("Failed to determine address from URL " << control.uri);
        control.stopped = true;
        control.result_cb(control.success);
        control.unlock();
        return;
      }
      control.unlock();

      uint16_t port = u_c.port ? u_c.port : 80;
      MDEBUG("Connecting to " << u_c.host << ":" << port);
      client.set_server(u_c.host, std::to_string(port), boost::none);
      if (!client.connect(std::chrono::seconds(30)))
      {
        control.lock();
        MERROR("Failed to connect to " << control.uri);
        control.stopped = true;
        control.result_cb(control.success);
        control.unlock();
        return;
      }
      MDEBUG("GETting " << u_c.uri);
      const epee::net_utils::http::http_response_info *info = NULL;
      if (!client.invoke_get(u_c.uri, std::chrono::seconds(30), "", &info))
      {
        control.lock();
        MERROR("Failed to connect to " << control.uri);
        client.disconnect();
        control.stopped = true;
        control.result_cb(control.success);
        control.unlock();
        return;
      }
      if (!info)
      {
        control.lock();
        MERROR("Failed invoking GET command to " << control.uri << ", no status info returned");
        client.disconnect();
        control.stopped = true;
        control.result_cb(control.success);
        control.unlock();
        return;
      }
      MDEBUG("response code: " << info->m_response_code);
      MDEBUG("response length: " << info->m_header_info.m_content_length);
      MDEBUG("response comment: " << info->m_response_comment);
      MDEBUG("response body: " << info->m_body);
      for (const auto &f: info->m_additional_fields)
        MDEBUG("additional field: " << f.first << ": " << f.second);
      if (info->m_response_code != 200)
      {
        control.lock();
        MERROR("Status code " << info->m_response_code);
        client.disconnect();
        control.stopped = true;
        control.result_cb(control.success);
        control.unlock();
        return;
      }
      client.disconnect();
      f.close();
      MDEBUG("Download complete");
      control.lock();
      control.success = true;
      control.result_cb(control.success);
      control.stopped = true;
      control.unlock();
    }
    catch (const std::exception &e)
    {
      control.lock();
      MERROR("Exception in download thread: " << e.what());
      control.stopped = true;
      control.unlock();
      return;
    }
  }

  bool download(const std::string &path, const std::string &url, std::function<bool(size_t, ssize_t)> cb)
  {
    bool success = false;
    download_async_handle handle = download_async(path, url, [&success](bool result) {success = result;}, cb);
    download_wait(handle);
    download_clear(handle);
    return success;
  }

  download_async_handle download_async(const std::string &path, const std::string &url, std::function<void(bool)> result, std::function<bool(size_t, ssize_t)> progress)
  {
    download_thread_control *control = new download_thread_control(path, url, result, progress);
    control->thread = std::shared_ptr<boost::thread>(new boost::thread([&](){ download_thread(*control); }));
    return (download_async_handle)control;
  }

  bool download_finished(download_async_handle h)
  {
    download_thread_control *control = (download_thread_control*)h;
    control->lock();
    bool stopped = control->stopped;
    control->unlock();
    return stopped;
  }

  bool download_error(download_async_handle h)
  {
    download_thread_control *control = (download_thread_control*)h;
    control->lock();
    bool success = control->success;
    control->unlock();
    return !success;
  }

  bool download_wait(download_async_handle h)
  {
    download_thread_control *control = (download_thread_control*)h;
    control->lock();
    if (control->stopped)
    {
      control->unlock();
      return true;
    }
    MTRACE("magic: joining: " << control);
    std::shared_ptr<boost::thread> thread = control->thread;
    control->unlock();
    thread->join();
    return true;
  }

  bool download_cancel(download_async_handle h)
  {
    download_thread_control *control = (download_thread_control*)h;
    control->lock();
    control->stop = true;
    std::shared_ptr<boost::thread> thread = control->thread;
    control->unlock();
    thread->join();
    return true;
  }

  void download_clear(download_async_handle h)
  {
    download_thread_control *control = (download_thread_control*)h;
    control->lock();
    if (!control->stopped)
      control->thread->detach();
    control->unlock();
    delete control;
  }
}
