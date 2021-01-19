#pragma  once

#include <functional>
#include <map>
#include <utility>

#include <boost/optional/optional.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/utility/string_ref.hpp>

#include "net/http.h"
#include "storages/http_abstract_invoke.h"

#include "bootstrap_node_selector.h"

namespace cryptonote
{

  class bootstrap_daemon
  {
  public:
    bootstrap_daemon(
      std::function<std::map<std::string, bool>()> get_public_nodes,
      bool rpc_payment_enabled,
      const std::string &proxy);
    bootstrap_daemon(
      const std::string &address,
      boost::optional<epee::net_utils::http::login> credentials,
      bool rpc_payment_enabled,
      const std::string &proxy);

    std::string address() const noexcept;
    boost::optional<std::pair<uint64_t, uint64_t>> get_height();
    bool handle_result(bool success, const std::string &status);

    template <class t_request, class t_response>
    bool invoke_http_json(const boost::string_ref uri, const t_request &out_struct, t_response &result_struct)
    {
      if (!switch_server_if_needed())
      {
        return false;
      }

      const bool result = epee::net_utils::invoke_http_json(uri, out_struct, result_struct, m_http_client);
      return handle_result(result, result_struct.status);
    }

    template <class t_request, class t_response>
    bool invoke_http_bin(const boost::string_ref uri, const t_request &out_struct, t_response &result_struct)
    {
      if (!switch_server_if_needed())
      {
        return false;
      }

      const bool result = epee::net_utils::invoke_http_bin(uri, out_struct, result_struct, m_http_client);
      return handle_result(result, result_struct.status);
    }

    template <class t_request, class t_response>
    bool invoke_http_json_rpc(const boost::string_ref command_name, const t_request &out_struct, t_response &result_struct)
    {
      if (!switch_server_if_needed())
      {
        return false;
      }

      const bool result = epee::net_utils::invoke_http_json_rpc(
        "/json_rpc",
        std::string(command_name.begin(), command_name.end()),
        out_struct,
        result_struct,
        m_http_client);
      return handle_result(result, result_struct.status);
    }

    void set_proxy(const std::string &address);

  private:
    bool set_server(const std::string &address, const boost::optional<epee::net_utils::http::login> &credentials = boost::none);
    bool switch_server_if_needed();

  private:
    net::http::client m_http_client;
    const bool m_rpc_payment_enabled;
    const std::unique_ptr<bootstrap_node::selector> m_selector;
    boost::mutex m_selector_mutex;
  };

}
