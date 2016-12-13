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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "password_container.h"

#include <iostream>
#include <memory.h>
#include <stdio.h>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace tools
{
  namespace
  {
    bool is_cin_tty();
  }
  // deleted via private member
  password_container::password_container()
    : m_empty(true),m_verify(true)
  {

  }
  password_container::password_container(bool verify)
    : m_empty(true),m_verify(verify)
  {

  }

  password_container::password_container(std::string&& password)
    : m_empty(false)
    , m_password(std::move(password))
	, m_verify(false)
  {

  }


  password_container::password_container(password_container&& rhs)
    : m_empty(std::move(rhs.m_empty))
    , m_password(std::move(rhs.m_password))
    , m_verify(std::move(rhs.m_verify))
  {
  }
  password_container::~password_container()
  {
    clear();
  }

  void password_container::clear()
  {
    if (0 < m_password.capacity())
    {
      m_password.replace(0, m_password.capacity(), m_password.capacity(), '\0');
      m_password.resize(0);
    }
    m_empty = true;
  }

  bool password_container::read_password(const char *message)
  {
    clear();

    bool r;
    if (is_cin_tty())
    {
     r = read_from_tty_double_check(message);
    }
    else
    {
      r = read_from_file();
    }

    if (r)
    {
      m_empty = false;
    }
    else
    {
      clear();
    }

    return r;
  }

  bool password_container::read_from_file()
  {
    m_password.reserve(max_password_size);
    for (size_t i = 0; i < max_password_size; ++i)
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
        m_password.push_back(ch);
      }
    }

    return true;
  }

bool password_container::read_from_tty_double_check(const char *message) {
    std::string pass1;
    std::string pass2;
    bool match=false;
    bool doNotVerifyEntry=false;
    if (m_verify){message = "Enter a password for your new wallet";}
    do{
        if (message)
            std::cout << message <<": ";
        if (!password_container::read_from_tty(pass1))
            return false;
        if (m_verify==true){//double check password; 
            std::cout << "Confirm Password: ";
            if (!password_container::read_from_tty(pass2))
                return false;
            if(pass1!=pass2){ //new password entered did not match

                std::cout << "Passwords do not match! Please try again." << std::endl;
                pass1="";
                pass2="";
                match=false;
            }
            else{//new password matches
                match=true;
            }
        }
        else
            doNotVerifyEntry=true; //do not verify
            //No need to verify password entered at this point in the code 
            
    }while(match==false && doNotVerifyEntry==false);

    m_password=pass1;
    return true;
  }


#if defined(_WIN32)

  namespace
  {
    bool is_cin_tty()
    {
      return 0 != _isatty(_fileno(stdin));
    }
  }

  bool password_container::read_from_tty(std::string & pass)
  {
    const char BACKSPACE = 8;

    HANDLE h_cin = ::GetStdHandle(STD_INPUT_HANDLE);

    DWORD mode_old;
    ::GetConsoleMode(h_cin, &mode_old);
    DWORD mode_new = mode_old & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    ::SetConsoleMode(h_cin, mode_new);

    bool r = true;
    pass.reserve(max_password_size);
    while (pass.size() < max_password_size)
    {
      DWORD read;
      char ch;
      r = (TRUE == ::ReadConsoleA(h_cin, &ch, 1, &read, NULL));
      r &= (1 == read);
      if (!r)
      {
        break;
      }
      else if (ch == '\n' || ch == '\r')
      {
        std::cout << std::endl;
        break;
      }
      else if (ch == BACKSPACE)
      {
        if (!pass.empty())
        {
          pass.back() = '\0';
          pass.resize(pass.size() - 1);
          std::cout << "\b \b";
        }
      }
      else
      {
        pass.push_back(ch);
        std::cout << '*';
      }
    }

    ::SetConsoleMode(h_cin, mode_old);

    return r;
  }

#else

  namespace
  {
    bool is_cin_tty()
    {
      return 0 != isatty(fileno(stdin));
    }

    int getch()
    {
      struct termios tty_old;
      tcgetattr(STDIN_FILENO, &tty_old);

      struct termios tty_new;
      tty_new = tty_old;
      tty_new.c_lflag &= ~(ICANON | ECHO);
      tcsetattr(STDIN_FILENO, TCSANOW, &tty_new);

      int ch = getchar();

      tcsetattr(STDIN_FILENO, TCSANOW, &tty_old);

      return ch;
    }
  }
  bool password_container::read_from_tty(std::string &aPass)
  {
    const char BACKSPACE = 127;

    aPass.reserve(max_password_size);
    while (aPass.size() < max_password_size)
    {
      int ch = getch();
      if (EOF == ch)
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
          aPass.back() = '\0';
          aPass.resize(aPass.size() - 1);
          std::cout << "\b \b";
        }
      }
      else
      {
        aPass.push_back(ch);
        std::cout << '*';
      }
    }

    return true;
  }

#endif
}
