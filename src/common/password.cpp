// Copyright (c) 2014-2024, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "password.h"

#include <iostream>
#include <stdio.h>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#define EOT 0x4

namespace
{
#if defined(_WIN32)
  bool is_cin_tty() noexcept
  {
    return 0 != _isatty(_fileno(stdin));
  }

  bool read_from_tty(epee::wipeable_string& pass, bool hide_input)
  {
    HANDLE h_cin = ::GetStdHandle(STD_INPUT_HANDLE);

    DWORD mode_old;
    ::GetConsoleMode(h_cin, &mode_old);
    DWORD mode_new = mode_old & ~((hide_input ? ENABLE_ECHO_INPUT : 0) | ENABLE_LINE_INPUT);
    ::SetConsoleMode(h_cin, mode_new);

    bool r = true;
    pass.reserve(tools::password_container::max_password_size);
    std::vector<int> chlen;
    chlen.reserve(tools::password_container::max_password_size);
    while (pass.size() < tools::password_container::max_password_size)
    {
      DWORD read;
      wchar_t ucs2_ch;
      r = (TRUE == ::ReadConsoleW(h_cin, &ucs2_ch, 1, &read, NULL));
      r &= (1 == read);

      if (!r)
      {
        break;
      }
      else if (ucs2_ch == L'\r')
      {
        std::cout << std::endl;
        break;
      }
      else if (ucs2_ch == L'\b')
      {
        if (!pass.empty())
        {
          int len = chlen.back();
          chlen.pop_back();
          while(len-- > 0) 
            pass.pop_back();
        }
        continue;
      }
      
      char utf8_ch[8] = {0};
      int len;
      if((len = WideCharToMultiByte(CP_UTF8, 0, &ucs2_ch, 1, utf8_ch, sizeof(utf8_ch), NULL, NULL)) <= 0)
        break;

      if(pass.size() + len >= tools::password_container::max_password_size)
        break;

      chlen.push_back(len);
      pass += utf8_ch;
    }

    ::SetConsoleMode(h_cin, mode_old);

    return r;
  }

#else // end WIN32 

  bool is_cin_tty() noexcept
  {
    return 0 != isatty(fileno(stdin));
  }

  int getch(bool hide_input) noexcept
  {
    struct termios tty_old;
    tcgetattr(STDIN_FILENO, &tty_old);

    struct termios tty_new;
    tty_new = tty_old;
    tty_new.c_lflag &= ~(ICANON | (hide_input ? ECHO : 0));
    tcsetattr(STDIN_FILENO, TCSANOW, &tty_new);

    int ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &tty_old);

    return ch;
  }

  bool read_from_tty(epee::wipeable_string& aPass, bool hide_input)
  {
    static constexpr const char BACKSPACE = 127;

    aPass.reserve(tools::password_container::max_password_size);
    while (aPass.size() < tools::password_container::max_password_size)
    {
      int ch = getch(hide_input);
      if (EOF == ch || ch == EOT)
      {
        return false;
      }
      else if (ch == '\n' || ch == '\r')
      {
        std::cout << std::endl;
        break;
      }
      else if (ch == BACKSPACE)
      {
        if (!aPass.empty())
        {
          aPass.pop_back();
          if (!hide_input)
            std::cout << "\b\b\b   \b\b\b" << std::flush;
        }
        else
        {
          if (!hide_input)
            std::cout << "\b\b  \b\b" << std::flush;
        }
      }
      else
      {
        aPass.push_back(ch);
      }
    }

    return true;
  }

#endif // end !WIN32

  bool read_from_tty(const bool verify, const char *message, bool hide_input, epee::wipeable_string& pass1, epee::wipeable_string& pass2)
  {
    while (true)
    {
      if (message)
        std::cout << message <<": " << std::flush;
      if (!read_from_tty(pass1, hide_input))
        return false;
      if (verify)
      {
        std::cout << "Confirm password: ";
        if (!read_from_tty(pass2, hide_input))
          return false;
        if(pass1!=pass2)
        {
          std::cout << "Passwords do not match! Please try again." << std::endl;
          pass1.clear();
          pass2.clear();
        }
        else //new password matches
          return true;
      }
      else
        return true;
        //No need to verify password entered at this point in the code
    }

    return false;
  }

  bool read_from_file(epee::wipeable_string& pass)
  {
    pass.reserve(tools::password_container::max_password_size);
    for (size_t i = 0; i < tools::password_container::max_password_size; ++i)
    {
      char ch = static_cast<char>(std::cin.get());
      if (std::cin.eof() || ch == '\n' || ch == '\r')
      {
        break;
      }
      else if (std::cin.fail())
      {
        return false;
      }
      else
      {
        pass.push_back(ch);
      }
    }
    return true;
  }

} // anonymous namespace

namespace tools 
{
  // deleted via private member
  password_container::password_container() noexcept : m_password() {}
  password_container::password_container(std::string&& password) noexcept
    : m_password(std::move(password)) 
  {
  }
  password_container::password_container(const epee::wipeable_string& password) noexcept
    : m_password(password)
  {
  }

  password_container::~password_container() noexcept
  {
    m_password.clear();
  }

  std::atomic<bool> password_container::is_prompting(false);

  boost::optional<password_container> password_container::prompt(const bool verify, const char *message, bool hide_input)
  {
    is_prompting = true;
    password_container pass1{};
    password_container pass2{};
    if (is_cin_tty() ? read_from_tty(verify, message, hide_input, pass1.m_password, pass2.m_password) : read_from_file(pass1.m_password))
    {
      is_prompting = false;
      return {std::move(pass1)};
    }

    is_prompting = false;
    return boost::none;
  }

  boost::optional<login> login::parse(std::string&& userpass, bool verify, const std::function<boost::optional<password_container>(bool)> &prompt)
  {
    login out{};

    const auto loc = userpass.find(':');
    if (loc == std::string::npos)
    {
      auto result = prompt(verify);
      if (!result)
        return boost::none;

      out.password = std::move(*result);
    }
    else
    {
      out.password = password_container{userpass.substr(loc + 1)};
    }

    out.username = userpass.substr(0, loc);
    password_container wipe{std::move(userpass)};
    return {std::move(out)};
  }
} 
