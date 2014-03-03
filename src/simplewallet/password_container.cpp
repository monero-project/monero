// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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

  password_container::password_container()
    : m_empty(true)
  {
  }

  password_container::password_container(std::string&& password)
    : m_empty(false)
    , m_password(std::move(password))
  {
  }

  password_container::password_container(password_container&& rhs)
    : m_empty(std::move(rhs.m_empty))
    , m_password(std::move(rhs.m_password))
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

  bool password_container::read_password()
  {
    clear();

    bool r;
    if (is_cin_tty())
    {
      std::cout << "password: ";
      r = read_from_tty();
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

#if defined(_WIN32)

  namespace
  {
    bool is_cin_tty()
    {
      return 0 != _isatty(_fileno(stdin));
    }
  }

  bool password_container::read_from_tty()
  {
    const char BACKSPACE = 8;

    HANDLE h_cin = ::GetStdHandle(STD_INPUT_HANDLE);

    DWORD mode_old;
    ::GetConsoleMode(h_cin, &mode_old);
    DWORD mode_new = mode_old & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    ::SetConsoleMode(h_cin, mode_new);

    bool r = true;
    m_password.reserve(max_password_size);
    while (m_password.size() < max_password_size)
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
        if (!m_password.empty())
        {
          m_password.back() = '\0';
          m_password.resize(m_password.size() - 1);
          std::cout << "\b \b";
        }
      }
      else
      {
        m_password.push_back(ch);
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

  bool password_container::read_from_tty()
  {
    const char BACKSPACE = 127;

    m_password.reserve(max_password_size);
    while (m_password.size() < max_password_size)
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
        if (!m_password.empty())
        {
          m_password.back() = '\0';
          m_password.resize(m_password.size() - 1);
          std::cout << "\b \b";
        }
      }
      else
      {
        m_password.push_back(ch);
        std::cout << '*';
      }
    }

    return true;
  }

#endif
}
