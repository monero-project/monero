#include "bootstrap_daemon.h"

#include <stdexcept>

#include "crypto/crypto.h"
#include "cryptonote_core/cryptonote_core.h"
#include "misc_log_ex.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon.rpc.bootstrap_daemon"

namespace cryptonote
{

  bootstrap_daemon::bootstrap_daemon(std::function<boost::optional<std::string>()> get_next_public_node) noexcept
    : m_get_next_public_node(get_next_public_node)
  {
  }

  bootstrap_daemon::bootstrap_daemon(const std::string &address, const boost::optional<epee::net_utils::http::login> &credentials)
    : bootstrap_daemon(nullptr)
  {
    if (!set_server(address, credentials))
    {
      throw std::runtime_error("invalid bootstrap daemon address or credentials");
    }
  }

  std::string bootstrap_daemon::address() const noexcept
  {
    const auto& host = m_http_client.get_host();
    if (host.empty())
    {
      return std::string();
    }
    return host + ":" + m_http_client.get_port();
  }

  boost::optional<uint64_t> bootstrap_daemon::get_height()
  {
    cryptonote::COMMAND_RPC_GET_HEIGHT::request req;
    cryptonote::COMMAND_RPC_GET_HEIGHT::response res;

    if (!invoke_http_json("/getheight", req, res))
    {
      return boost::none;
    }

    if (res.status != CORE_RPC_STATUS_OK)
    {
      return boost::none;
    }

    return res.height;
  }

  bool bootstrap_daemon::handle_result(bool success)
  {
    if (!success && m_get_next_public_node)
    {
      m_http_client.disconnect();
    }

    return success;
  }

  bool bootstrap_daemon::set_server(const std::string &address, const boost::optional<epee::net_utils::http::login> &credentials /* = boost::none */)
  {
    if (!m_http_client.set_server(address, credentials))
    {
      MERROR("Failed to set bootstrap daemon address " << address);
      return false;
    }

    MINFO("Changed bootstrap daemon address to " << address);
    return true;
  }


  bool bootstrap_daemon::switch_server_if_needed()
  {
    if (!m_get_next_public_node || m_http_client.is_connected())
    {
      return true;
    }

    const boost::optional<std::string> address = m_get_next_public_node();
    if (address) {
      return set_server(*address);
    }

    return false;
  }

}
