// Copyright (c) 2014-2018, The Monero Project
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

#include "misc_log_ex.h"
#include <iostream>

#ifdef HAVE_READLINE
  #include "readline_buffer.h"
  #define PAUSE_READLINE() \
    rdln::suspend_readline pause_readline; 
#else
  #define PAUSE_READLINE()
#endif

namespace tools
{

/************************************************************************/
/*                                                                      */
/************************************************************************/
class scoped_message_writer
{
private:
  bool m_flush;
  std::stringstream m_oss;
  epee::console_colors m_color;
  bool m_bright;
  el::Level m_log_level;
public:
  scoped_message_writer(
      epee::console_colors color = epee::console_color_default
    , bool bright = false
    , std::string&& prefix = std::string()
    , el::Level log_level = el::Level::Info
    )
    : m_flush(true)
    , m_color(color)
    , m_bright(bright)
    , m_log_level(log_level)
  {
    m_oss << prefix;
  }

  scoped_message_writer(scoped_message_writer&& rhs)
    : m_flush(std::move(rhs.m_flush))
#if defined(_MSC_VER)
    , m_oss(std::move(rhs.m_oss))
#else
      // GCC bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54316
    , m_oss(rhs.m_oss.str(), std::ios_base::out | std::ios_base::ate)
#endif
    , m_color(std::move(rhs.m_color))
    , m_log_level(std::move(rhs.m_log_level))
  {
    rhs.m_flush = false;
  }

  scoped_message_writer(scoped_message_writer& rhs) = delete;
  scoped_message_writer& operator=(scoped_message_writer& rhs) = delete;
  scoped_message_writer& operator=(scoped_message_writer&& rhs) = delete;

  template<typename T>
  std::ostream& operator<<(const T& val)
  {
    m_oss << val;
    return m_oss;
  }

  ~scoped_message_writer()
  {
    if (m_flush)
    {
      m_flush = false;

      MCLOG_FILE(m_log_level, "msgwriter", m_oss.str());

      if (epee::console_color_default == m_color)
      {
        std::cout << m_oss.str();
      }
      else
      {
        PAUSE_READLINE();
        set_console_color(m_color, m_bright);
        std::cout << m_oss.str();
        epee::reset_console_color();
      }
      std::cout << std::endl;
    }
  }
};

inline scoped_message_writer success_msg_writer(bool color = true)
{
  return scoped_message_writer(color ? epee::console_color_green : epee::console_color_default, false, std::string(), el::Level::Info);
}

inline scoped_message_writer msg_writer(epee::console_colors color = epee::console_color_default)
{
  return scoped_message_writer(color, false, std::string(), el::Level::Info);
}

inline scoped_message_writer fail_msg_writer()
{
  return scoped_message_writer(epee::console_color_red, true, "Error: ", el::Level::Error);
}

} // namespace tools
