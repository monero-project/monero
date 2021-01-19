#include "bootstrap_daemon.h"

#include <stdexcept>

#include <boost/thread/locks.hpp>

#include "crypto/crypto.h"
#include "cryptonote_core/cryptonote_core.h"
#include "misc_log_ex.h"
#include "net/parse.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon.rpc.bootstrap_daemon"

namespace cryptonote
{

  bootstrap_daemon::bootstrap_daemon(
    std::function<std::map<std::string, bool>()> get_public_nodes,
    bool rpc_payment_enabled,
    const std::string &proxy)
    : m_selector(new bootstrap_node::selector_auto(std::move(get_public_nodes)))
    , m_rpc_payment_enabled(rpc_payment_enabled)
  {
    set_proxy(proxy);
  }

  bootstrap_daemon::bootstrap_daemon(
    const std::string &address,
    boost::optional<epee::net_utils::http::login> credentials,
    bool rpc_payment_enabled,
    const std::string &proxy)
    : m_selector(nullptr)
    , m_rpc_payment_enabled(rpc_payment_enabled)
  {
    set_proxy(proxy);
    if (!set_server(address, std::move(credentials)))
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

  boost::optional<std::pair<uint64_t, uint64_t>> bootstrap_daemon::get_height()
  {
    cryptonote::COMMAND_RPC_GET_INFO::request req;
    cryptonote::COMMAND_RPC_GET_INFO::response res;

    if (!invoke_http_json("/getinfo", req, res))
    {
      return boost::none;
    }

    if (res.status != CORE_RPC_STATUS_OK)
    {
      return boost::none;
    }

    return {{res.height, res.target_height}};
  }

  bool bootstrap_daemon::handle_result(bool success, const std::string &status)
  {
    const bool failed = !success || (!m_rpc_payment_enabled && status == CORE_RPC_STATUS_PAYMENT_REQUIRED);
    if (failed && m_selector)
    {
      const std::string current_address = address();
      m_http_client.disconnect();

      const boost::unique_lock<boost::mutex> lock(m_selector_mutex);
      m_selector->handle_result(current_address, !failed);
    }

    return success;
  }

  void bootstrap_daemon::set_proxy(const std::string &address)
  {
    if (!address.empty() && !net::get_tcp_endpoint(address))
    {
      throw std::runtime_error("invalid proxy address format");
    }
    if (!m_http_client.set_proxy(address))
    {
      throw std::runtime_error("failed to set proxy address");
    }
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
    if (m_http_client.is_connected() || !m_selector)
    {
      return true;
    }

    boost::optional<bootstrap_node::node_info> node;
    {
      const boost::unique_lock<boost::mutex> lock(m_selector_mutex);
      node = m_selector->next_node();
    }
    if (node) {
      return set_server(node->address, node->credentials);
    }

    return false;
  }

}
