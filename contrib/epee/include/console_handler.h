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
//

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace epee
{
  class async_stdin_reader
  {
  public:
    async_stdin_reader()
      : m_run(true)
      , m_has_read_request(false)
      , m_read_status(state_init)
    {
      m_reader_thread = std::thread(std::bind(&async_stdin_reader::reader_thread_func, this));
    }

    ~async_stdin_reader()
    {
      stop();
    }

    // Not thread safe. Only one thread can call this method at once.
    bool get_line(std::string& line)
    {
      if (!start_read())
        return false;

      std::unique_lock<std::mutex> lock(m_response_mutex);
      while (state_init == m_read_status)
      {
        m_response_cv.wait(lock);
      }

      bool res = false;
      if (state_success == m_read_status)
      {
        line = m_line;
        res = true;
      }

      m_read_status = state_init;

      return res;
    }

    void stop()
    {
      if (m_run)
      {
        m_run.store(false, std::memory_order_relaxed);

#if defined(WIN32)
        ::CloseHandle(::GetStdHandle(STD_INPUT_HANDLE));
#endif

        m_request_cv.notify_one();
        m_reader_thread.join();
      }
    }

  private:
    bool start_read()
    {
      std::unique_lock<std::mutex> lock(m_request_mutex);
      if (!m_run.load(std::memory_order_relaxed) || m_has_read_request)
        return false;

      m_has_read_request = true;
      m_request_cv.notify_one();
      return true;
    }

    bool wait_read()
    {
      std::unique_lock<std::mutex> lock(m_request_mutex);
      while (m_run.load(std::memory_order_relaxed) && !m_has_read_request)
      {
        m_request_cv.wait(lock);
      }

      if (m_has_read_request)
      {
        m_has_read_request = false;
        return true;
      }

      return false;
    }

    bool wait_stdin_data()
    {
#if !defined(WIN32)
      int stdin_fileno = ::fileno(stdin);

      while (m_run.load(std::memory_order_relaxed))
      {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(stdin_fileno, &read_set);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        int retval = ::select(stdin_fileno + 1, &read_set, NULL, NULL, &tv);
        if (retval < 0)
          return false;
        else if (0 < retval)
          return true;
      }
#endif

      return true;
    }

    void reader_thread_func()
    {
      while (true)
      {
        if (!wait_read())
          break;

        std::string line;
        bool read_ok = true;
        if (wait_stdin_data())
        {
          if (m_run.load(std::memory_order_relaxed))
          {
            std::getline(std::cin, line);
            read_ok = !std::cin.eof() && !std::cin.fail();
          }
        }
        else
        {
          read_ok = false;
        }

        {
          std::unique_lock<std::mutex> lock(m_response_mutex);
          if (m_run.load(std::memory_order_relaxed))
          {
            m_line = std::move(line);
            m_read_status = read_ok ? state_success : state_error;
          }
          else
          {
            m_read_status = state_cancelled;
          }
          m_response_cv.notify_one();
        }
      }
    }

    enum t_state
    {
      state_init,
      state_success,
      state_error,
      state_cancelled
    };

  private:
    std::thread m_reader_thread;
    std::atomic<bool> m_run;

    std::string m_line;
    bool m_has_read_request;
    t_state m_read_status;

    std::mutex m_request_mutex;
    std::mutex m_response_mutex;
    std::condition_variable m_request_cv;
    std::condition_variable m_response_cv;
  };


  template<class t_server>
  bool empty_commands_handler(t_server* psrv, const std::string& command)
  {
    return true;
  }


  class async_console_handler
  {
  public:
    async_console_handler()
    {
    }

    template<class t_server, class chain_handler>
    bool run(t_server* psrv, chain_handler ch_handler, const std::string& prompt = "#", const std::string& usage = "")
    {
      return run(prompt, usage, [&](const std::string& cmd) { return ch_handler(psrv, cmd); }, [&] { psrv->send_stop_signal(); });
    }

    template<class chain_handler>
    bool run(chain_handler ch_handler, const std::string& prompt = "#", const std::string& usage = "")
    {
      return run(prompt, usage, [&](const std::string& cmd) { return ch_handler(cmd); }, [] { });
    }

    void stop()
    {
      m_running = false;
      m_stdin_reader.stop();
    }

  private:
    template<typename t_cmd_handler, typename t_exit_handler>
    bool run(const std::string& prompt, const std::string& usage, const t_cmd_handler& cmd_handler, const t_exit_handler& exit_handler)
    {
      TRY_ENTRY();
      bool continue_handle = true;
      while(continue_handle)
      {
        if (!m_running)
        {
          break;
        }
        if (!prompt.empty())
        {
          epee::log_space::set_console_color(epee::log_space::console_color_yellow, true);
          std::cout << prompt;
          if (' ' != prompt.back())
            std::cout << ' ';
          epee::log_space::reset_console_color();
          std::cout.flush();
        }

        std::string command;
        if(!m_stdin_reader.get_line(command))
        {
          LOG_PRINT("Failed to read line.", LOG_LEVEL_0);
        }
        string_tools::trim(command);

        LOG_PRINT_L2("Read command: " << command);
        if(0 == command.compare("exit") || 0 == command.compare("q"))
        {
          continue_handle = false;
        }else if (command.empty())
        {
          continue;
        }
        else if(cmd_handler(command))
        {
          continue;
        } else
        {
          std::cout << "unknown command: " << command << std::endl;
          std::cout << usage;
        }
      }
      exit_handler();
      return true;
      CATCH_ENTRY_L0("console_handler", false);
    }

  private:
    async_stdin_reader m_stdin_reader;
    bool m_running = true;
  };


  template<class t_server, class t_handler>
  bool start_default_console(t_server* ptsrv, t_handler handlr, const std::string& prompt, const std::string& usage = "")
  {
    std::shared_ptr<async_console_handler> console_handler = std::make_shared<async_console_handler>();
    boost::thread([=](){console_handler->run<t_server, t_handler>(ptsrv, handlr, prompt, usage);}).detach();
    return true;
  }

  template<class t_server>
  bool start_default_console(t_server* ptsrv, const std::string& prompt, const std::string& usage = "")
  {
    return start_default_console(ptsrv, empty_commands_handler<t_server>, prompt, usage);
  }

  template<class t_server, class t_handler>
    bool no_srv_param_adapter(t_server* ptsrv, const std::string& cmd, t_handler handlr)
    {
      return handlr(cmd);
    }

  template<class t_server, class t_handler>
  bool run_default_console_handler_no_srv_param(t_server* ptsrv, t_handler handlr, const std::string& prompt, const std::string& usage = "")
  {
    async_console_handler console_handler;
    return console_handler.run(ptsrv, boost::bind<bool>(no_srv_param_adapter<t_server, t_handler>, _1, _2, handlr), prompt, usage);
  }

  template<class t_server, class t_handler>
  bool start_default_console_handler_no_srv_param(t_server* ptsrv, t_handler handlr, const std::string& prompt, const std::string& usage = "")
  {
    boost::thread( boost::bind(run_default_console_handler_no_srv_param<t_server, t_handler>, ptsrv, handlr, prompt, usage) );
    return true;
  }

  /*template<class a>
  bool f(int i, a l)
  {
    return true;
  }*/
  /*
  template<class chain_handler>
  bool default_console_handler2(chain_handler ch_handler, const std::string usage)
  */


  /*template<class t_handler>
  bool start_default_console2(t_handler handlr, const std::string& usage = "")
  {
    //std::string usage_local = usage;
    boost::thread( boost::bind(default_console_handler2<t_handler>, handlr, usage) );
    //boost::function<bool ()> p__ = boost::bind(f<t_handler>, 1, handlr);
    //boost::function<bool ()> p__ = boost::bind(default_console_handler2<t_handler>, handlr, usage);
    //boost::thread tr(p__);
    return true;
  }*/

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class console_handlers_binder
  {
    typedef boost::function<bool (const std::vector<std::string> &)> console_command_handler;
    typedef std::map<std::string, std::pair<console_command_handler, std::string> > command_handlers_map;
    std::unique_ptr<boost::thread> m_console_thread;
    command_handlers_map m_command_handlers;
    async_console_handler m_console_handler;
  public:
    std::string get_usage()
    {
      std::stringstream ss;
      size_t max_command_len = 0;
      for(auto& x:m_command_handlers)
        if(x.first.size() > max_command_len)
          max_command_len = x.first.size();

      for(auto& x:m_command_handlers)
      {
        ss.width(max_command_len + 3);
        ss << std::left <<  x.first << x.second.second << ENDL;
      }
      return ss.str();
    }
    void set_handler(const std::string& cmd, const console_command_handler& hndlr, const std::string& usage = "")
    {
      command_handlers_map::mapped_type & vt = m_command_handlers[cmd];
      vt.first = hndlr;
      vt.second = usage;
    }
    bool process_command_vec(const std::vector<std::string>& cmd)
    {
      if(!cmd.size())
        return false;
      auto it = m_command_handlers.find(cmd.front());
      if(it == m_command_handlers.end())
        return false;
      std::vector<std::string> cmd_local(cmd.begin()+1, cmd.end());
      return it->second.first(cmd_local);
    }

    bool process_command_str(const std::string& cmd)
    {
      std::vector<std::string> cmd_v;
      boost::split(cmd_v,cmd,boost::is_any_of(" "), boost::token_compress_on);
      return process_command_vec(cmd_v);
    }

    /*template<class t_srv>
    bool start_handling(t_srv& srv, const std::string& usage_string = "")
    {
      start_default_console_handler_no_srv_param(&srv, boost::bind(&console_handlers_binder::process_command_str, this, _1));
      return true;
    }*/

    bool start_handling(const std::string& prompt, const std::string& usage_string = "")
    {
      m_console_thread.reset(new boost::thread(boost::bind(&console_handlers_binder::run_handling, this, prompt, usage_string)));
      m_console_thread->detach();
      return true;
    }

    void stop_handling()
    {
      m_console_handler.stop();
    }

    bool run_handling(const std::string& prompt, const std::string& usage_string)
    {
      return m_console_handler.run(boost::bind(&console_handlers_binder::process_command_str, this, _1), prompt, usage_string);
    }

    /*template<class t_srv>
    bool run_handling(t_srv& srv, const std::string& usage_string)
    {
      return run_default_console_handler_no_srv_param(&srv, boost::bind<bool>(&console_handlers_binder::process_command_str, this, _1), usage_string);
    }*/
  };

  /* work around because of broken boost bind */
  template<class t_server>
  class srv_console_handlers_binder: public console_handlers_binder
  {
    bool process_command_str(t_server* /*psrv*/, const std::string& cmd)
    {
      return console_handlers_binder::process_command_str(cmd);
    }
  public:
    bool start_handling(t_server* psrv, const std::string& prompt, const std::string& usage_string = "")
    {
      boost::thread(boost::bind(&srv_console_handlers_binder<t_server>::run_handling, this, psrv, prompt, usage_string)).detach();
      return true;
    }

    bool run_handling(t_server* psrv, const std::string& prompt, const std::string& usage_string)
    {
      return m_console_handler.run(psrv, boost::bind(&srv_console_handlers_binder<t_server>::process_command_str, this, _1, _2), prompt, usage_string);
    }

    void stop_handling()
    {
      m_console_handler.stop();
    }

  private:
    async_console_handler m_console_handler;
  };
}
