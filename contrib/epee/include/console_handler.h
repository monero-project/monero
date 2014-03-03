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

namespace epee
{




  template<class t_server>
  bool empty_commands_handler(t_server* psrv, const std::string& command)
  {
    return true;
  }
  

  template<class t_server, class chain_handler>
  bool default_console_handler(t_server* psrv, chain_handler ch_handler, const std::string usage = "")
  {
    TRY_ENTRY();
    bool continue_handle = true;
    while(continue_handle)
    {
      char command_buff[400] = {0};
      std::string command;
      std::cin.getline(command_buff, 399);
      if(std::cin.eof() || std::cin.fail())
      {
        LOG_PRINT("std::cin.eof() or std::cin.fail(), stopping...", LOG_LEVEL_0);
        continue_handle = false;
        break;
      }
      command = command_buff;

      if(!command.compare("exit") || !command.compare("q") )
      {
        psrv->send_stop_signal();
        continue_handle = false;
      }else if ( !command.compare(0, 7, "set_log"))
      {
        //parse set_log command
        if(command.size() != 9)
        {
          std::cout << "wrong syntax: " << command << std::endl << "use set_log n" << std::endl;
          continue;
        }
        int n = 0;
        if(!string_tools::get_xtype_from_string(n, command.substr(8, 1)))
        {
          std::cout << "wrong syntax: " << command << std::endl << "use set_log n" << std::endl;
          continue;
        }
        log_space::get_set_log_detalisation_level(true, n);
        LOG_PRINT_L0("New log level set " << n);
      }
      else if(ch_handler(psrv, command))
        continue;
      else
      {	
        std::cout << "unknown command: " << command << std::endl;
        std::cout << usage;
      }
    }
    return true;
    CATCH_ENTRY_L0("console_handler", false);
  }

  template<class chain_handler>
  bool default_console_handler2(chain_handler ch_handler, const std::string usage)
  {
    TRY_ENTRY();
    bool continue_handle = true;
    while(continue_handle)
    {
      char command_buff[400] = {0};
      std::string command;
      std::cin.getline(command_buff, 399);
      if(std::cin.eof() || std::cin.fail())
      {

        LOG_PRINT("std::cin.eof() or std::cin.fail(), stopping...", LOG_LEVEL_0);
        continue_handle = false;
        break;
      }
      command = command_buff;

      if(!command.compare("exit") || !command.compare("q") )
      {
        continue_handle = false;
      }else if ( !command.compare(0, 7, "set_log"))
      {
        //parse set_log command
        if(command.size() != 9)
        {
          std::cout << "wrong syntax: " << command << std::endl << "use set_log n" << std::endl;
          continue;
        }
        int n = 0;
        if(!string_tools::get_xtype_from_string(n, command.substr(8, 1)))
        {
          std::cout << "wrong syntax: " << command << std::endl << "use set_log n" << std::endl;
          continue;
        }
        log_space::get_set_log_detalisation_level(true, n);
        LOG_PRINT_L0("New log level set " << n);
      }
      else if(ch_handler(command))
        continue;
      else
      {	
        std::cout << "unknown command: " << command << std::endl;
        std::cout << usage;
      }
    }
    return true;
    CATCH_ENTRY_L0("console_handler", false);
  }
  



  template<class t_server, class t_handler>
  bool start_default_console(t_server* ptsrv, t_handler handlr, const std::string& usage = "")
  {
    boost::thread( boost::bind(default_console_handler<t_server, t_handler>, ptsrv, handlr, usage) );
    return true;
  }

  template<class t_server>
  bool start_default_console(t_server* ptsrv, const std::string& usage = "")
  {
    return start_default_console(ptsrv, empty_commands_handler<t_server>, usage);
  }

  template<class t_server, class t_handler>
    bool no_srv_param_adapter(t_server* ptsrv, const std::string& cmd, t_handler handlr)
    {
      return handlr(cmd);
    }

  template<class t_server, class t_handler>
  bool run_default_console_handler_no_srv_param(t_server* ptsrv, t_handler handlr, const std::string& usage = "")
  {
    return default_console_handler(ptsrv, boost::bind<bool>(no_srv_param_adapter<t_server, t_handler>, _1, _2, handlr), usage);
  }

  template<class t_server, class t_handler>
  bool start_default_console_handler_no_srv_param(t_server* ptsrv, t_handler handlr, const std::string& usage = "")
  {
    boost::thread( boost::bind(run_default_console_handler_no_srv_param<t_server, t_handler>, ptsrv, handlr, usage) );
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
    
    bool start_handling(const std::string& usage_string = "")
    {
      m_console_thread.reset(new boost::thread(boost::bind(&console_handlers_binder::run_handling, this, usage_string) ));
      return true;
    }

    bool stop_handling()
    {
      if(m_console_thread.get())
        m_console_thread->interrupt();
      return true;
    }


    bool run_handling(const std::string usage_string)
    {
      return default_console_handler2(boost::bind(&console_handlers_binder::process_command_str, this, _1), usage_string);
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
    bool start_handling(t_server* psrv,  const std::string& usage_string = "")
    {
      boost::thread(boost::bind(&srv_console_handlers_binder<t_server>::run_handling, this, psrv, usage_string) );
      return true;
    }

    bool run_handling(t_server* psrv, const std::string usage_string)
    {
      return default_console_handler(psrv, boost::bind(&srv_console_handlers_binder<t_server>::process_command_str, this, _1, _2), usage_string);
    }
  };


}


