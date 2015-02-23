#pragma once

#include "string_tools.h"
#include "net/http_client.h"

namespace tools {

class t_http_connection {
private:
  epee::net_utils::http::http_simple_client * mp_http_client;
  bool m_ok;
public:
  static unsigned int const TIMEOUT = 200000;

  t_http_connection(
      epee::net_utils::http::http_simple_client * p_http_client
    , uint32_t ip
    , uint16_t port
    )
    : mp_http_client(p_http_client)
  {
    // TODO fix http client so that it accepts properly typed arguments
    std::string ip_str = epee::string_tools::get_ip_string_from_int32(ip);
    std::string port_str = boost::lexical_cast<std::string>(port);
    m_ok = mp_http_client->connect(ip_str, port_str, TIMEOUT);
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

} // namespace tools
