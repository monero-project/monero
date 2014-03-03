// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include <string>

namespace tools
{
  class password_container
  {
  public:
    static const size_t max_password_size = 1024;

    password_container();
    password_container(std::string&& password);
    password_container(password_container&& rhs);
    ~password_container();

    void clear();
    bool empty() const { return m_empty; }
    const std::string& password() const { return m_password; }
    void password(std::string&& val) { m_password = std::move(val); m_empty = false; }
    bool read_password();

  private:
    bool read_from_file();
    bool read_from_tty();

  private:
    bool m_empty;
    std::string m_password;
  };
}
