#pragma once

#include "net/http_client.h"

namespace daemonize {

class t_http_connection {
private:
  epee::net_utils::http::http_simple_client * mp_http_client;
  bool m_ok;
public:
  static unsigned int const TIMEOUT = 200000;

  t_http_connection(
      epee::net_utils::http::http_simple_client * p_http_client
    , std::string const & daemon_ip
    , std::string const & daemon_port
    )
    : mp_http_client(p_http_client)
  {
    m_ok = mp_http_client->connect(daemon_ip, daemon_port, TIMEOUT);
  }

  ~t_http_connection()
  {
    if (m_ok)
    {
      mp_http_client->disconnect();
    }
  }

  bool is_open()
  {
    return m_ok;
  }
}; // class t_http_connection

} // namespace daemonize
