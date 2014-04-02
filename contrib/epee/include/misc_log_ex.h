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


#ifndef _MISC_LOG_EX_H_
#define _MISC_LOG_EX_H_

//#include <windows.h>
#include <atomic>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <list>
#include <map>
#include <time.h>
#include <boost/cstdint.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#if defined(WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include "static_initializer.h"
#include "string_tools.h"
#include "time_helper.h"
#include "misc_os_dependent.h"

#include "syncobj.h"


#define LOG_LEVEL_SILENT     -1
#define LOG_LEVEL_0     0
#define LOG_LEVEL_1     1
#define LOG_LEVEL_2     2
#define LOG_LEVEL_3     3
#define LOG_LEVEL_4     4
#define LOG_LEVEL_MIN   LOG_LEVEL_SILENT
#define LOG_LEVEL_MAX   LOG_LEVEL_4


#define   LOGGER_NULL       0
#define   LOGGER_FILE       1
#define   LOGGER_DEBUGGER   2
#define   LOGGER_CONSOLE    3
#define   LOGGER_DUMP       4


#ifndef LOCAL_ASSERT
#include <assert.h>
#if (defined _MSC_VER)
#define LOCAL_ASSERT(expr) {if(epee::debug::get_set_enable_assert()){_ASSERTE(expr);}}
#else
#define LOCAL_ASSERT(expr)
#endif

#endif

namespace epee
{
namespace debug
{
  inline bool get_set_enable_assert(bool set = false, bool v = false)
  {
    static bool e = true;
    if(set)
      e = v;
    return e;
  }
}
namespace log_space
{
  class  logger;
  class  log_message;
  class  log_singletone;

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  enum console_colors
  {
    console_color_default,
    console_color_white,
    console_color_red,
    console_color_green,
    console_color_blue,
    console_color_cyan,
    console_color_magenta,
    console_color_yellow
  };


  struct ibase_log_stream
  {
    ibase_log_stream(){}
    virtual ~ibase_log_stream(){}
    virtual bool out_buffer( const char* buffer, int buffer_len , int log_level, int color, const char* plog_name = NULL)=0;
    virtual int get_type(){return 0;}

    virtual bool set_max_logfile_size(uint64_t max_size){return true;};
    virtual bool set_log_rotate_cmd(const std::string& cmd){return true;};
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  /*struct ibase_log_value
  {
  public:
    virtual void debug_out( std::stringstream*  p_stream)const = 0;
  };*/

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  /*class log_message: public std::stringstream
  {
  public:
    log_message(const  log_message& lm): std::stringstream(), std::stringstream::basic_ios()
    {}
    log_message(){}

    template<class T>
    log_message& operator<< (T t)
    {
      std::stringstream* pstrstr = this;
      (*pstrstr) << t;

      return *this;
    }
  };
  inline
  log_space::log_message& operator<<(log_space::log_message& sstream, const ibase_log_value& log_val)
  {
    log_val.debug_out(&sstream);
    return sstream;
  }
  */
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct delete_ptr
  {
    template <class P>
      void operator() (P p)
    {
      delete p.first;
    }
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  //------------------------------------------------------------------------
#define max_dbg_str_len 80
#ifdef _MSC_VER
  class debug_output_stream: public ibase_log_stream
  {
    virtual bool out_buffer( const char* buffer, int buffer_len , int log_level, int color, const char* plog_name = NULL)
    {
      for ( int i = 0; i <  buffer_len; i = i + max_dbg_str_len )
      {
        std::string   s( buffer + i, buffer_len- i < max_dbg_str_len ?
          buffer_len - i : max_dbg_str_len );

        ::OutputDebugStringA( s.c_str() );
      }
      return  true;
    }

  };
#endif

  inline bool is_stdout_a_tty()
  {
    static std::atomic<bool> initialized(false);
    static std::atomic<bool> is_a_tty(false);

    if (!initialized.load(std::memory_order_acquire))
    {
#if defined(WIN32)
      is_a_tty.store(0 != _isatty(_fileno(stdout)), std::memory_order_relaxed);
#else
      is_a_tty.store(0 != isatty(fileno(stdout)), std::memory_order_relaxed);
#endif
      initialized.store(true, std::memory_order_release);
    }

    return is_a_tty.load(std::memory_order_relaxed);
  }

  inline void set_console_color(int color, bool bright)
  {
    if (!is_stdout_a_tty())
      return;

    switch(color)
    {
    case console_color_default:
      {
#ifdef WIN32
        HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h_stdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE| (bright ? FOREGROUND_INTENSITY:0));
#else
        if(bright)
          std::cout << "\033[1;37m";
        else
          std::cout << "\033[0m";
#endif
      }
      break;
    case console_color_white:
      {
#ifdef WIN32
        HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h_stdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | (bright ? FOREGROUND_INTENSITY:0));
#else
        if(bright)
          std::cout << "\033[1;37m";
        else
          std::cout << "\033[0;37m";
#endif
      }
      break;
    case console_color_red:
      {
#ifdef WIN32
        HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h_stdout, FOREGROUND_RED | (bright ? FOREGROUND_INTENSITY:0));
#else
        if(bright)
          std::cout << "\033[1;31m";
        else
          std::cout << "\033[0;31m";
#endif
      }
      break;
    case console_color_green:
      {
#ifdef WIN32
        HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h_stdout, FOREGROUND_GREEN | (bright ? FOREGROUND_INTENSITY:0));
#else
        if(bright)
          std::cout << "\033[1;32m";
        else
          std::cout << "\033[0;32m";
#endif
      }
      break;

    case console_color_blue:
      {
#ifdef WIN32
        HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h_stdout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);//(bright ? FOREGROUND_INTENSITY:0));
#else
        if(bright)
          std::cout << "\033[1;34m";
        else
          std::cout << "\033[0;34m";
#endif
      }
      break;

    case console_color_cyan:
      {
#ifdef WIN32
        HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h_stdout, FOREGROUND_GREEN | FOREGROUND_BLUE | (bright ? FOREGROUND_INTENSITY:0));
#else
        if(bright)
          std::cout << "\033[1;36m";
        else
          std::cout << "\033[0;36m";
#endif
      }
      break;

    case console_color_magenta:
      {
#ifdef WIN32
        HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h_stdout, FOREGROUND_BLUE | FOREGROUND_RED | (bright ? FOREGROUND_INTENSITY:0));
#else
        if(bright)
          std::cout << "\033[1;35m";
        else
          std::cout << "\033[0;35m";
#endif
      }
      break;

    case console_color_yellow:
      {
#ifdef WIN32
        HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h_stdout, FOREGROUND_RED | FOREGROUND_GREEN | (bright ? FOREGROUND_INTENSITY:0));
#else
        if(bright)
          std::cout << "\033[1;33m";
        else
          std::cout << "\033[0;33m";
#endif
      }
      break;

    }
  }

  inline void reset_console_color() {
    if (!is_stdout_a_tty())
      return;

#ifdef WIN32
    HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h_stdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    std::cout << "\033[0m";
    std::cout.flush();
#endif
  }

  class console_output_stream: public ibase_log_stream
  {
#ifdef _MSC_VER
    bool m_have_to_kill_console;
#endif

  public:
    console_output_stream()
    {
#ifdef _MSC_VER

      if(!::GetStdHandle(STD_OUTPUT_HANDLE))
        m_have_to_kill_console = true;
      else
        m_have_to_kill_console = false;

      ::AllocConsole();
#endif
    }

    ~console_output_stream()
    {
#ifdef _MSC_VER
      if(m_have_to_kill_console)
        ::FreeConsole();
#endif
    }
    int get_type(){return LOGGER_CONSOLE;}



    virtual bool out_buffer( const char* buffer, int buffer_len , int log_level, int color, const char* plog_name = NULL)
    {
      if(plog_name)
        return true; //skip alternative logs from console

      set_console_color(color, log_level < 1);

#ifdef _MSC_VER
      const char* ptarget_buf = NULL;
      char* pallocated_buf = NULL;

      //
      int i = 0;
      for(; i < buffer_len; i++)
        if(buffer[i] == '\a') break;
      if(i == buffer_len)
        ptarget_buf = buffer;
      else
      {
        pallocated_buf = new char[buffer_len];
        ptarget_buf = pallocated_buf;
        for(i = 0; i < buffer_len; i++)
        {
          if(buffer[i] == '\a')
            pallocated_buf[i] = '^';
          else
            pallocated_buf[i] = buffer[i];
        }
      }

      //uint32_t b = 0;
      //::WriteConsoleA(::GetStdHandle(STD_OUTPUT_HANDLE), ptarget_buf, buffer_len, (DWORD*)&b, 0);
      std::cout << ptarget_buf;
      if(pallocated_buf) delete [] pallocated_buf;
#else
      std::string buf(buffer, buffer_len);
      for(size_t i = 0; i!= buf.size(); i++)
      {
        if(buf[i] == 7 || buf[i] == -107)
          buf[i] = '^';
      }

      std::cout << buf;
#endif
      reset_console_color();
      return  true;
    }


  };

  inline bool rotate_log_file(const char* pfile_path)
  {
#ifdef _MSC_VER
    if(!pfile_path)
      return false;

    std::string file_path = pfile_path;
    std::string::size_type a = file_path .rfind('.');
    if ( a != std::string::npos )
      file_path .erase( a, file_path .size());

    ::DeleteFileA( (file_path + ".0").c_str() );
    ::MoveFileA( (file_path + ".log").c_str(), (file_path + ".0").c_str() );
#else
    return false;//not implemented yet
#endif
    return true;
  }




  //--------------------------------------------------------------------------//
  class file_output_stream : public ibase_log_stream
  {
  public:
    typedef std::map<std::string, std::ofstream*> named_log_streams;

    file_output_stream( std::string default_log_file_name, std::string log_path )
    {
      m_default_log_filename = default_log_file_name;
      m_max_logfile_size = 0;
      m_default_log_path = log_path;
      m_pdefault_file_stream = add_new_stream_and_open(default_log_file_name.c_str());
    }

    ~file_output_stream()
    {
      for(named_log_streams::iterator it = m_log_file_names.begin(); it!=m_log_file_names.end(); it++)
      {
        if ( it->second->is_open() )
        {
          it->second->flush();
          it->second->close();
        }
        delete it->second;
      }
    }
  private:
    named_log_streams m_log_file_names;
    std::string       m_default_log_path;
    std::ofstream*    m_pdefault_file_stream;
    std::string     m_log_rotate_cmd;
    std::string     m_default_log_filename;
    uint64_t   m_max_logfile_size;


    std::ofstream*    add_new_stream_and_open(const char* pstream_name)
    {
      //log_space::rotate_log_file((m_default_log_path + "\\" + pstream_name).c_str());

      std::ofstream* pstream = (m_log_file_names[pstream_name] = new std::ofstream);
      std::string target_path = m_default_log_path + "/" + pstream_name;
      pstream->open( target_path.c_str(), std::ios_base::out | std::ios::app /*ios_base::trunc */);
      if(pstream->fail())
        return NULL;
      return pstream;
    }

    bool set_max_logfile_size(uint64_t max_size)
    {
      m_max_logfile_size = max_size;
      return true;
    }

    bool set_log_rotate_cmd(const std::string& cmd)
    {
      m_log_rotate_cmd = cmd;
      return true;
    }



    virtual bool out_buffer( const char* buffer, int buffer_len, int log_level, int color, const char* plog_name = NULL )
    {
      std::ofstream*    m_target_file_stream = m_pdefault_file_stream;
      if(plog_name)
      { //find named stream
        named_log_streams::iterator it = m_log_file_names.find(plog_name);
        if(it == m_log_file_names.end())
          m_target_file_stream = add_new_stream_and_open(plog_name);
        else
          m_target_file_stream = it->second;
      }
      if(!m_target_file_stream || !m_target_file_stream->is_open())
        return false;//TODO: add assert here

      m_target_file_stream->write(buffer, buffer_len );
      m_target_file_stream->flush();

      if(m_max_logfile_size)
      {
        std::ofstream::pos_type pt =  m_target_file_stream->tellp();
        uint64_t current_sz = pt;
        if(current_sz > m_max_logfile_size)
        {
          std::cout << "current_sz= " << current_sz << " m_max_logfile_size= " << m_max_logfile_size << std::endl;
          std::string log_file_name;
          if(!plog_name)
            log_file_name = m_default_log_filename;
          else
            log_file_name = plog_name;

          m_target_file_stream->close();
          std::string new_log_file_name = log_file_name;

          time_t tm = 0;
          time(&tm);

          int err_count = 0;
          boost::system::error_code ec;
          do
          {
            new_log_file_name = string_tools::cut_off_extension(log_file_name);
            if(err_count)
              new_log_file_name += misc_utils::get_time_str_v2(tm) + "(" + boost::lexical_cast<std::string>(err_count) + ")" + ".log";
            else
              new_log_file_name += misc_utils::get_time_str_v2(tm) + ".log";

            err_count++;
          }while(boost::filesystem::exists(m_default_log_path + "/" + new_log_file_name, ec));

          std::string new_log_file_path = m_default_log_path + "/" + new_log_file_name;
          boost::filesystem::rename(m_default_log_path + "/" + log_file_name, new_log_file_path, ec);
          if(ec)
          {
            std::cout << "Filed to rename, ec = " << ec.message() << std::endl;
          }

          if(m_log_rotate_cmd.size())
          {

            std::string m_log_rotate_cmd_local_copy = m_log_rotate_cmd;
            //boost::replace_all(m_log_rotate_cmd, "[*SOURCE*]", new_log_file_path);
            boost::replace_all(m_log_rotate_cmd_local_copy, "[*TARGET*]", new_log_file_path);

            misc_utils::call_sys_cmd(m_log_rotate_cmd_local_copy);
          }

          m_target_file_stream->open( (m_default_log_path + "/" + log_file_name).c_str(), std::ios_base::out | std::ios::app /*ios_base::trunc */);
          if(m_target_file_stream->fail())
            return false;
        }
      }

      return  true;
    }
    int get_type(){return LOGGER_FILE;}
  };
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class log_stream_splitter
  {
  public:
    typedef std::list<std::pair<ibase_log_stream*, int> > streams_container;

    log_stream_splitter(){}
    ~log_stream_splitter()
    {
      //free pointers
      std::for_each(m_log_streams.begin(), m_log_streams.end(), delete_ptr());
    }

    bool set_max_logfile_size(uint64_t max_size)
    {
      for(streams_container::iterator it = m_log_streams.begin(); it!=m_log_streams.end();it++)
        it->first->set_max_logfile_size(max_size);
      return true;
    }

    bool set_log_rotate_cmd(const std::string& cmd)
    {
      for(streams_container::iterator it = m_log_streams.begin(); it!=m_log_streams.end();it++)
        it->first->set_log_rotate_cmd(cmd);
      return true;
    }

    bool do_log_message(const std::string& rlog_mes, int log_level, int color, const char* plog_name = NULL)
    {
      std::string str_mess = rlog_mes;
      size_t str_len = str_mess.size();
      const char* pstr = str_mess.c_str();
      for(streams_container::iterator it = m_log_streams.begin(); it!=m_log_streams.end();it++)
        if(it->second >= log_level)
          it->first->out_buffer(pstr, (int)str_len, log_level, color, plog_name);
      return true;
    }

    bool add_logger( int type, const char* pdefault_file_name, const char* pdefault_log_folder, int log_level_limit = LOG_LEVEL_4 )
    {
      ibase_log_stream* ls = NULL;

      switch( type )
      {
      case LOGGER_FILE:
        ls = new file_output_stream( pdefault_file_name, pdefault_log_folder );
        break;

      case LOGGER_DEBUGGER:
#ifdef _MSC_VER
        ls = new debug_output_stream( );
#else
        return false;//not implemented yet
#endif
        break;
      case LOGGER_CONSOLE:
        ls = new console_output_stream( );
        break;
      }

      if ( ls ) {
        m_log_streams.push_back(streams_container::value_type(ls, log_level_limit));
        return true;
      }
      return ls ? true:false;
    }
    bool add_logger( ibase_log_stream* pstream, int log_level_limit = LOG_LEVEL_4 )
    {
      m_log_streams.push_back(streams_container::value_type(pstream, log_level_limit) );
      return true;
    }

    bool remove_logger(int type)
    {
      streams_container::iterator it = m_log_streams.begin();
      for(;it!=m_log_streams.end(); it++)
      {
        if(it->first->get_type() == type)
        {
          delete it->first;
          m_log_streams.erase(it);
          return true;
        }
      }
      return false;

    }

  protected:
  private:

        streams_container m_log_streams;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  inline int get_set_log_detalisation_level(bool is_need_set = false, int log_level_to_set = LOG_LEVEL_1);
  inline int  get_set_time_level(bool is_need_set = false, int time_log_level = LOG_LEVEL_0);
  inline bool get_set_need_thread_id(bool is_need_set = false, bool is_need_val = false);
  inline bool get_set_need_proc_name(bool is_need_set = false, bool is_need_val = false);


  inline std::string get_daytime_string2()
  {
    boost::posix_time::ptime p = boost::posix_time::microsec_clock::local_time();
    return misc_utils::get_time_str_v3(p);
  }
  inline std::string get_day_time_string()
  {
    return get_daytime_string2();
    //time_t tm = 0;
    //time(&tm);
    //return  misc_utils::get_time_str(tm);
  }

  inline std::string get_time_string()
  {
            return get_daytime_string2();

  }
#ifdef _MSC_VER
  inline std::string get_time_string_adv(SYSTEMTIME* pst = NULL)
  {
    SYSTEMTIME st = {0};
    if(!pst)
    {
      pst = &st;
      GetSystemTime(&st);
    }
    std::stringstream str_str;
    str_str.fill('0');
    str_str << std::setw(2) << pst->wHour << "_"
        << std::setw(2) << pst->wMinute << "_"
        << std::setw(2) << pst->wSecond << "_"
        << std::setw(3) << pst->wMilliseconds;
    return str_str.str();
  }
#endif





    class logger
  {
  public:
    friend class log_singletone;

    logger()
    {
      CRITICAL_REGION_BEGIN(m_critical_sec);
      init();
      CRITICAL_REGION_END();
    }
    ~logger()
    {
    }

    bool set_max_logfile_size(uint64_t max_size)
    {
      CRITICAL_REGION_BEGIN(m_critical_sec);
      m_log_target.set_max_logfile_size(max_size);
      CRITICAL_REGION_END();
      return true;
    }

    bool set_log_rotate_cmd(const std::string& cmd)
    {
      CRITICAL_REGION_BEGIN(m_critical_sec);
      m_log_target.set_log_rotate_cmd(cmd);
      CRITICAL_REGION_END();
      return true;
    }

    bool take_away_journal(std::list<std::string>& journal)
    {
      CRITICAL_REGION_BEGIN(m_critical_sec);
      m_journal.swap(journal);
      CRITICAL_REGION_END();
      return true;
    }

    bool do_log_message(const std::string& rlog_mes, int log_level, int color, bool add_to_journal = false, const char* plog_name = NULL)
    {
      CRITICAL_REGION_BEGIN(m_critical_sec);
      m_log_target.do_log_message(rlog_mes, log_level, color, plog_name);
      if(add_to_journal)
        m_journal.push_back(rlog_mes);

      return true;
      CRITICAL_REGION_END();
    }

    bool add_logger( int type, const char* pdefault_file_name, const char* pdefault_log_folder , int log_level_limit = LOG_LEVEL_4)
    {
      CRITICAL_REGION_BEGIN(m_critical_sec);
      return m_log_target.add_logger( type, pdefault_file_name, pdefault_log_folder, log_level_limit);
      CRITICAL_REGION_END();
    }
    bool add_logger( ibase_log_stream* pstream, int log_level_limit = LOG_LEVEL_4)
    {
      CRITICAL_REGION_BEGIN(m_critical_sec);
      return m_log_target.add_logger(pstream, log_level_limit);
      CRITICAL_REGION_END();
    }

    bool remove_logger(int type)
    {
      CRITICAL_REGION_BEGIN(m_critical_sec);
      return m_log_target.remove_logger(type);
      CRITICAL_REGION_END();
    }


    bool set_thread_prefix(const std::string& prefix)
    {
      CRITICAL_REGION_BEGIN(m_critical_sec);
      m_thr_prefix_strings[misc_utils::get_thread_string_id()] = prefix;
      CRITICAL_REGION_END();
      return true;
    }


    std::string get_default_log_file()
    {
      return m_default_log_file;
    }

    std::string get_default_log_folder()
    {
      return m_default_log_folder;
    }

  protected:
  private:
    bool init()
    {
      //

      m_process_name = string_tools::get_current_module_name();

      init_log_path_by_default();

      //init default set of loggers
      init_default_loggers();

      std::stringstream ss;
      ss  << get_time_string() << " Init logging. Level=" << get_set_log_detalisation_level()
        << " Log path=" << m_default_log_folder << std::endl;
      this->do_log_message(ss.str(), console_color_white, LOG_LEVEL_0);
      return true;
    }
    bool init_default_loggers()
    {
      //TODO:
      return true;
    }

    bool init_log_path_by_default()
    {
      //load process name
      m_default_log_folder = string_tools::get_current_module_folder();

      m_default_log_file =  m_process_name;
      std::string::size_type a = m_default_log_file.rfind('.');
      if ( a != std::string::npos )
        m_default_log_file.erase( a, m_default_log_file.size());
      m_default_log_file += ".log";

      return true;
    }

    log_stream_splitter m_log_target;

    std::string m_default_log_folder;
    std::string m_default_log_file;
    std::string m_process_name;
    std::map<std::string, std::string> m_thr_prefix_strings;
    std::list<std::string> m_journal;
    critical_section m_critical_sec;
  };
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class log_singletone
  {
  public:
    friend class initializer<log_singletone>;
    friend class logger;
    static int get_log_detalisation_level()
    {
      get_or_create_instance();//to initialize logger, if it not initialized
      return get_set_log_detalisation_level();
    }

    static bool is_filter_error(int error_code)
    {
      return false;
    }


    static bool do_log_message(const std::string& rlog_mes, int log_level, int color, bool keep_in_journal, const char* plog_name = NULL)
    {
      logger* plogger = get_or_create_instance();
      bool res = false;
      if(plogger)
        res = plogger->do_log_message(rlog_mes, log_level, color, keep_in_journal, plog_name);
      else
      { //globally uninitialized, create new logger for each call of do_log_message() and then delete it
        plogger = new logger();
        //TODO: some extra initialization
        res = plogger->do_log_message(rlog_mes, log_level, color, keep_in_journal, plog_name);
        delete plogger;
        plogger = NULL;
      }
      return res;
    }

    static bool take_away_journal(std::list<std::string>& journal)
    {
      logger* plogger = get_or_create_instance();
      bool res = false;
      if(plogger)
        res = plogger->take_away_journal(journal);

      return res;
    }

    static bool set_max_logfile_size(uint64_t file_size)
    {
      logger* plogger = get_or_create_instance();
      if(!plogger) return false;
      return plogger->set_max_logfile_size(file_size);
    }


    static bool set_log_rotate_cmd(const std::string& cmd)
    {
      logger* plogger = get_or_create_instance();
      if(!plogger) return false;
      return plogger->set_log_rotate_cmd(cmd);
    }


    static bool add_logger( int type, const char* pdefault_file_name, const char* pdefault_log_folder, int log_level_limit = LOG_LEVEL_4)
    {
      logger* plogger = get_or_create_instance();
      if(!plogger) return false;
      return plogger->add_logger(type, pdefault_file_name, pdefault_log_folder, log_level_limit);
    }

    static std::string get_default_log_file()
    {
      logger* plogger = get_or_create_instance();
      if(plogger)
        return plogger->get_default_log_file();

      return "";
    }

    static std::string get_default_log_folder()
    {
      logger* plogger = get_or_create_instance();
      if(plogger)
        return plogger->get_default_log_folder();

      return "";
    }

    static bool add_logger( ibase_log_stream* pstream, int log_level_limit = LOG_LEVEL_4 )
    {
      logger* plogger = get_or_create_instance();
      if(!plogger) return false;
      return plogger->add_logger(pstream, log_level_limit);
    }


    static bool remove_logger( int type )
    {
      logger* plogger = get_or_create_instance();
      if(!plogger) return false;
      return plogger->remove_logger(type);
    }
PUSH_WARNINGS
DISABLE_GCC_WARNING(maybe-uninitialized)
    static int get_set_log_detalisation_level(bool is_need_set = false, int log_level_to_set = LOG_LEVEL_1)
    {
      static int log_detalisation_level = LOG_LEVEL_1;
      if(is_need_set)
        log_detalisation_level = log_level_to_set;
      return log_detalisation_level;
    }
POP_WARNINGS
    static int  get_set_time_level(bool is_need_set = false, int time_log_level = LOG_LEVEL_0)
    {
      static int val_time_log_level = LOG_LEVEL_0;
      if(is_need_set)
        val_time_log_level = time_log_level;

      return val_time_log_level;
    }

    static int  get_set_process_level(bool is_need_set = false, int process_log_level = LOG_LEVEL_0)
    {
      static int val_process_log_level = LOG_LEVEL_0;
      if(is_need_set)
        val_process_log_level = process_log_level;

      return val_process_log_level;
    }

    /*static int  get_set_tid_level(bool is_need_set = false, int tid_log_level = LOG_LEVEL_0)
    {
      static int val_tid_log_level = LOG_LEVEL_0;
      if(is_need_set)
        val_tid_log_level = tid_log_level;

      return val_tid_log_level;
    }*/

    static bool get_set_need_thread_id(bool is_need_set = false, bool is_need_val = false)
    {
      static bool is_need = false;
      if(is_need_set)
        is_need = is_need_val;

      return is_need;
    }

    static bool get_set_need_proc_name(bool is_need_set = false, bool is_need_val = false)
    {
      static bool is_need = true;
      if(is_need_set)
        is_need = is_need_val;

      return is_need;
    }
    static uint64_t get_set_err_count(bool is_need_set = false, uint64_t err_val = false)
    {
      static uint64_t err_count = 0;
      if(is_need_set)
        err_count = err_val;

      return err_count;
    }


#ifdef _MSC_VER


    static void SetThreadName( DWORD dwThreadID, const char* threadName)
    {
#define MS_VC_EXCEPTION 0x406D1388

#pragma pack(push,8)
      typedef struct tagTHREADNAME_INFO
      {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
      } THREADNAME_INFO;
#pragma pack(pop)



      Sleep(10);
      THREADNAME_INFO info;
      info.dwType = 0x1000;
      info.szName = (char*)threadName;
      info.dwThreadID = dwThreadID;
      info.dwFlags = 0;

      __try
      {
        RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
      }
    }
#endif

    static bool set_thread_log_prefix(const std::string& prefix)
    {
#ifdef _MSC_VER
      SetThreadName(-1, prefix.c_str());
#endif


      logger* plogger = get_or_create_instance();
      if(!plogger) return false;
      return plogger->set_thread_prefix(prefix);
    }


    static std::string get_prefix_entry()
    {
      std::stringstream str_prefix;
      //write time entry
      if ( get_set_time_level() <= get_set_log_detalisation_level() )
        str_prefix << get_day_time_string() << " ";

      //write process info
      logger* plogger = get_or_create_instance();
      //bool res = false;
      if(!plogger)
      { //globally uninitialized, create new logger for each call of get_prefix_entry() and then delete it
        plogger = new logger();
      }

      //if ( get_set_need_proc_name() && get_set_process_level() <= get_set_log_detalisation_level()  )
      //    str_prefix << "[" << plogger->m_process_name << " (id=" << GetCurrentProcessId() << ")] ";
//#ifdef _MSC_VER_EX
      if ( get_set_need_thread_id() /*&& get_set_tid_level() <= get_set_log_detalisation_level()*/ )
        str_prefix << "tid:" << misc_utils::get_thread_string_id() << " ";
//#endif

      if(plogger->m_thr_prefix_strings.size())
      {
        CRITICAL_REGION_LOCAL(plogger->m_critical_sec);
        std::string thr_str = misc_utils::get_thread_string_id();
        std::map<std::string, std::string>::iterator it = plogger->m_thr_prefix_strings.find(thr_str);
        if(it!=plogger->m_thr_prefix_strings.end())
        {
          str_prefix << it->second;
        }
      }


      if(get_set_is_uninitialized())
        delete plogger;

      return str_prefix.str();
    }

  private:
    log_singletone(){}//restric to create an instance
    //static initializer<log_singletone> m_log_initializer;//must be in one .cpp file (for example main.cpp) via DEFINE_LOGGING macro

    static bool init()
    {
      return true;/*do nothing here*/
    }
    static bool un_init()
    {
      //delete object
      logger* plogger = get_set_instance_internal();
      if(plogger) delete plogger;
      //set uninitialized
      get_set_is_uninitialized(true, true);
      get_set_instance_internal(true, NULL);
      return true;
    }

    static logger* get_or_create_instance()
    {
      logger* plogger = get_set_instance_internal();
      if(!plogger)
        if(!get_set_is_uninitialized())
          get_set_instance_internal(true, plogger = new logger);

      return plogger;
    }

    static logger* get_set_instance_internal(bool is_need_set = false, logger* pnew_logger_val = NULL)
    {
      static logger* val_plogger = NULL;

      if(is_need_set)
        val_plogger = pnew_logger_val;

      return val_plogger;
    }

    static bool get_set_is_uninitialized(bool is_need_set = false, bool is_uninitialized = false)
    {
      static bool val_is_uninitialized = false;

      if(is_need_set)
        val_is_uninitialized = is_uninitialized;

      return val_is_uninitialized;
    }
    //static int get_set_error_filter(bool is_need_set = false)
  };

  const static initializer<log_singletone> log_initializer;
  /************************************************************************/
  /*                                                                      */
//  /************************************************************************/
//  class log_array_value
//  {
//    int       num;
//    log_message&    m_ref_log_mes;
//
//  public:
//
//    log_array_value( log_message& log_mes ) : num(0), m_ref_log_mes(log_mes) {}
//
//    void operator ( )( ibase_log_value *val ) {
//      m_ref_log_mes << "\n[" << num++ << "] "/* << val*/; }
//
//
//    template<class T>
//      void operator ()(T &value )
//    {
//      m_ref_log_mes << "\n[" << num++ << "] " << value;
//    }
//  };

  class log_frame
  {
    std::string   m_name;
    int       m_level;
    const char*           m_plog_name;
  public:

    log_frame(const std::string& name,  int dlevel = LOG_LEVEL_2 , const char* plog_name = NULL)
    {
#ifdef _MSC_VER
      int lasterr=::GetLastError();
#endif
      m_plog_name = plog_name;
      if ( dlevel <= log_singletone::get_log_detalisation_level() )
      {
        m_name = name;
        std::stringstream ss;
        ss << log_space::log_singletone::get_prefix_entry() << "-->>" << m_name << std::endl;
        log_singletone::do_log_message(ss.str(), dlevel, console_color_default, false, m_plog_name);
      }
      m_level = dlevel;
#ifdef _MSC_VER
      ::SetLastError(lasterr);
#endif
    }
    ~log_frame()
    {
#ifdef _MSC_VER
      int lasterr=::GetLastError();
#endif

      if (m_level <= log_singletone::get_log_detalisation_level() )
      {
        std::stringstream ss;
        ss << log_space::log_singletone::get_prefix_entry() << "<<--" << m_name << std::endl;
        log_singletone::do_log_message(ss.str(), m_level, console_color_default, false,m_plog_name);
      }
#ifdef _MSC_VER
      ::SetLastError(lasterr);
#endif
    }
  };

  inline int  get_set_time_level(bool is_need_set, int time_log_level)
  {
    return log_singletone::get_set_time_level(is_need_set, time_log_level);
  }
  inline int get_set_log_detalisation_level(bool is_need_set, int log_level_to_set)
  {
    return log_singletone::get_set_log_detalisation_level(is_need_set, log_level_to_set);
  }
  inline std::string get_prefix_entry()
  {
    return log_singletone::get_prefix_entry();
  }
  inline bool get_set_need_thread_id(bool is_need_set, bool is_need_val)
  {
    return log_singletone::get_set_need_thread_id(is_need_set, is_need_val);
  }
  inline bool get_set_need_proc_name(bool is_need_set, bool is_need_val )
  {
    return log_singletone::get_set_need_proc_name(is_need_set, is_need_val);
  }

  inline std::string get_win32_err_descr(int err_no)
  {
#ifdef _MSC_VER
    LPVOID lpMsgBuf;

    FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      err_no,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (char*) &lpMsgBuf,
      0, NULL );

    std::string fix_sys_message = "(null)";
    if(lpMsgBuf) fix_sys_message = (char*)lpMsgBuf;
    std::string::size_type a;
    if ( (a = fix_sys_message.rfind( '\n' )) != std::string::npos )
      fix_sys_message.erase(a);
    if ( (a = fix_sys_message.rfind( '\r' )) != std::string::npos )
      fix_sys_message.erase(a);

    LocalFree(lpMsgBuf);
    return fix_sys_message;
#else
    return "Not implemented yet";
#endif
  }

  inline bool getwin32_err_text(std::stringstream& ref_message, int error_no)
  {
    ref_message << "win32 error:" << get_win32_err_descr(error_no);
    return true;
  }
}
#if defined(_DEBUG) || defined(__GNUC__)
  #define  ENABLE_LOGGING_INTERNAL
#endif

#if defined(ENABLE_RELEASE_LOGGING)
  #define  ENABLE_LOGGING_INTERNAL
#endif


#if defined(ENABLE_LOGGING_INTERNAL)

#define LOG_PRINT_NO_PREFIX2(log_name, x, y) {if ( y <= epee::log_space::log_singletone::get_log_detalisation_level() )\
  {std::stringstream ss________; ss________ << x << std::endl; epee::log_space::log_singletone::do_log_message(ss________.str() , y, epee::log_space::console_color_default, false, log_name);}}

#define LOG_PRINT_NO_PREFIX_NO_POSTFIX2(log_name, x, y) {if ( y <= epee::log_space::log_singletone::get_log_detalisation_level() )\
  {std::stringstream ss________; ss________ << x; epee::log_space::log_singletone::do_log_message(ss________.str(), y, epee::log_space::console_color_default, false, log_name);}}


#define LOG_PRINT_NO_POSTFIX2(log_name, x, y) {if ( y <= epee::log_space::log_singletone::get_log_detalisation_level() )\
  {std::stringstream ss________; ss________ << epee::log_space::log_singletone::get_prefix_entry() << x; epee::log_space::log_singletone::do_log_message(ss________.str(), y, epee::log_space::console_color_default, false, log_name);}}


#define LOG_PRINT2(log_name, x, y) {if ( y <= epee::log_space::log_singletone::get_log_detalisation_level() )\
  {std::stringstream ss________; ss________ << epee::log_space::log_singletone::get_prefix_entry() << x << std::endl;epee::log_space::log_singletone::do_log_message(ss________.str(), y, epee::log_space::console_color_default, false, log_name);}}

#define LOG_PRINT_COLOR2(log_name, x, y, color) {if ( y <= epee::log_space::log_singletone::get_log_detalisation_level() )\
  {std::stringstream ss________; ss________ << epee::log_space::log_singletone::get_prefix_entry() << x << std::endl;epee::log_space::log_singletone::do_log_message(ss________.str(), y, color, false, log_name);}}


#define LOG_PRINT2_JORNAL(log_name, x, y) {if ( y <= epee::log_space::log_singletone::get_log_detalisation_level() )\
  {std::stringstream ss________; ss________ << epee::log_space::log_singletone::get_prefix_entry() << x << std::endl;epee::log_space::log_singletone::do_log_message(ss________.str(), y, epee::log_space::console_color_default, true, log_name);}}


#define LOG_ERROR2(log_name, x) { \
  std::stringstream ss________; ss________ << epee::log_space::log_singletone::get_prefix_entry() << "ERROR " << __FILE__ << ":" << __LINE__ << " " << x << std::endl; epee::log_space::log_singletone::do_log_message(ss________.str(), LOG_LEVEL_0, epee::log_space::console_color_red, true, log_name);LOCAL_ASSERT(0); epee::log_space::log_singletone::get_set_err_count(true, epee::log_space::log_singletone::get_set_err_count()+1);}

#define LOG_FRAME2(log_name, x, y) epee::log_space::log_frame frame(x, y, log_name)

#else


#define LOG_PRINT_NO_PREFIX2(log_name, x, y)

#define LOG_PRINT_NO_PREFIX_NO_POSTFIX2(log_name, x, y)

#define LOG_PRINT_NO_POSTFIX2(log_name, x, y)

#define LOG_PRINT_COLOR2(log_name, x, y, color)

#define LOG_PRINT2_JORNAL(log_name, x, y)

#define LOG_PRINT2(log_name, x, y)

#define LOG_ERROR2(log_name, x)


#define LOG_FRAME2(log_name, x, y)


#endif


#ifndef LOG_DEFAULT_TARGET
  #define LOG_DEFAULT_TARGET    NULL
#endif


#define LOG_PRINT_NO_POSTFIX(mess, level) LOG_PRINT_NO_POSTFIX2(LOG_DEFAULT_TARGET, mess, level)
#define LOG_PRINT_NO_PREFIX(mess, level)  LOG_PRINT_NO_PREFIX2(LOG_DEFAULT_TARGET, mess, level)
#define LOG_PRINT_NO_PREFIX_NO_POSTFIX(mess, level) LOG_PRINT_NO_PREFIX_NO_POSTFIX2(LOG_DEFAULT_TARGET, mess, level)
#define LOG_PRINT(mess, level)        LOG_PRINT2(LOG_DEFAULT_TARGET, mess, level)

#define LOG_PRINT_COLOR(mess, level, color)       LOG_PRINT_COLOR2(LOG_DEFAULT_TARGET, mess, level, color)
#define LOG_PRINT_RED(mess, level)        LOG_PRINT_COLOR2(LOG_DEFAULT_TARGET, mess, level, epee::log_space::console_color_red)
#define LOG_PRINT_GREEN(mess, level)        LOG_PRINT_COLOR2(LOG_DEFAULT_TARGET, mess, level, epee::log_space::console_color_green)
#define LOG_PRINT_BLUE(mess, level)       LOG_PRINT_COLOR2(LOG_DEFAULT_TARGET, mess, level, epee::log_space::console_color_blue)
#define LOG_PRINT_YELLOW(mess, level)       LOG_PRINT_COLOR2(LOG_DEFAULT_TARGET, mess, level, epee::log_space::console_color_yellow)
#define LOG_PRINT_CYAN(mess, level)       LOG_PRINT_COLOR2(LOG_DEFAULT_TARGET, mess, level, epee::log_space::console_color_cyan)
#define LOG_PRINT_MAGENTA(mess, level)       LOG_PRINT_COLOR2(LOG_DEFAULT_TARGET, mess, level, epee::log_space::console_color_magenta)

#define LOG_PRINT_RED_L0(mess)    LOG_PRINT_COLOR2(LOG_DEFAULT_TARGET, mess, LOG_LEVEL_0, epee::log_space::console_color_red)

#define LOG_PRINT_L0(mess)        LOG_PRINT(mess, LOG_LEVEL_0)
#define LOG_PRINT_L1(mess)        LOG_PRINT(mess, LOG_LEVEL_1)
#define LOG_PRINT_L2(mess)        LOG_PRINT(mess, LOG_LEVEL_2)
#define LOG_PRINT_L3(mess)        LOG_PRINT(mess, LOG_LEVEL_3)
#define LOG_PRINT_L4(mess)        LOG_PRINT(mess, LOG_LEVEL_4)
#define LOG_PRINT_J(mess, level)        LOG_PRINT2_JORNAL(LOG_DEFAULT_TARGET, mess, level)

#define LOG_ERROR(mess)           LOG_ERROR2(LOG_DEFAULT_TARGET, mess)
#define LOG_FRAME(mess, level)        LOG_FRAME2(LOG_DEFAULT_TARGET, mess, level)
#define LOG_VALUE(mess, level)        LOG_VALUE2(LOG_DEFAULT_TARGET, mess, level)
#define LOG_ARRAY(mess, level)        LOG_ARRAY2(LOG_DEFAULT_TARGET, mess, level)
//#define LOGWIN_PLATFORM_ERROR(err_no)       LOGWINDWOS_PLATFORM_ERROR2(LOG_DEFAULT_TARGET, err_no)
#define LOG_SOCKET_ERROR(err_no)      LOG_SOCKET_ERROR2(LOG_DEFAULT_TARGET, err_no)
//#define LOGWIN_PLATFORM_ERROR_UNCRITICAL(mess)  LOGWINDWOS_PLATFORM_ERROR_UNCRITICAL2(LOG_DEFAULT_TARGET, mess)

#define ENDL std::endl

#define TRY_ENTRY()   try {
#define CATCH_ENTRY(location, return_val) } \
  catch(const std::exception& ex) \
{ \
  (void)(ex); \
  LOG_ERROR("Exception at [" << location << "], what=" << ex.what()); \
  return return_val; \
}\
  catch(...)\
{\
  LOG_ERROR("Exception at [" << location << "], generic exception \"...\"");\
  return return_val; \
}

#define CATCH_ENTRY_L0(lacation, return_val) CATCH_ENTRY(lacation, return_val)
#define CATCH_ENTRY_L1(lacation, return_val) CATCH_ENTRY(lacation, return_val)
#define CATCH_ENTRY_L2(lacation, return_val) CATCH_ENTRY(lacation, return_val)
#define CATCH_ENTRY_L3(lacation, return_val) CATCH_ENTRY(lacation, return_val)
#define CATCH_ENTRY_L4(lacation, return_val) CATCH_ENTRY(lacation, return_val)


#define ASSERT_MES_AND_THROW(message) {LOG_ERROR(message); std::stringstream ss; ss << message; throw std::runtime_error(ss.str());}
#define CHECK_AND_ASSERT_THROW_MES(expr, message) {if(!(expr)) ASSERT_MES_AND_THROW(message);}


#ifndef CHECK_AND_ASSERT
#define CHECK_AND_ASSERT(expr, fail_ret_val)   do{if(!(expr)){LOCAL_ASSERT(expr); return fail_ret_val;};}while(0)
#endif

#define NOTHING

#ifndef CHECK_AND_ASSERT_MES
#define CHECK_AND_ASSERT_MES(expr, fail_ret_val, message)   do{if(!(expr)) {LOG_ERROR(message); return fail_ret_val;};}while(0)
#endif

#ifndef CHECK_AND_NO_ASSERT_MES
#define CHECK_AND_NO_ASSERT_MES(expr, fail_ret_val, message)   do{if(!(expr)) {LOG_PRINT_L0(message); /*LOCAL_ASSERT(expr);*/ return fail_ret_val;};}while(0)
#endif


#ifndef CHECK_AND_ASSERT_MES_NO_RET
#define CHECK_AND_ASSERT_MES_NO_RET(expr, message)   do{if(!(expr)) {LOG_ERROR(message); return;};}while(0)
#endif


#ifndef CHECK_AND_ASSERT_MES2
#define CHECK_AND_ASSERT_MES2(expr, message)   do{if(!(expr)) {LOG_ERROR(message); };}while(0)
#endif

}
#endif //_MISC_LOG_EX_H_
