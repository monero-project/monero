#ifndef JSONRPC_PROTOCOL_HANDLER_H
#define	JSONRPC_PROTOCOL_HANDLER_H

#include <cstdint>
#include <string>

#include "net/net_utils_base.h"
#include "jsonrpc_structs.h"
#include "storages/portable_storage.h"
#include "storages/portable_storage_template_helper.h"

namespace epee
{
namespace net_utils
{
  namespace jsonrpc2
  {
  inline
  std::string& make_error_resp_json(int64_t code, const std::string& message,
                                    std::string& response_data,
                                    const epee::serialization::storage_entry& id = nullptr)
  {
    epee::json_rpc::error_response rsp;
    rsp.id = id;
    rsp.jsonrpc = "2.0";
    rsp.error.code = code;
    rsp.error.message = message;
    epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(rsp), response_data, 0, false);
    response_data += "\n";
    return response_data;
  }

    template<class t_connection_context>
    struct i_jsonrpc2_server_handler
    {
      virtual ~i_jsonrpc2_server_handler()
      {}
      virtual bool handle_rpc_request(const std::string& req_data,
                                      std::string& resp_data,
                                      t_connection_context& conn_context) = 0;
      virtual bool init_server_thread()
      { return true; }
      virtual bool deinit_server_thread()
      { return true; }
    };
    
    template<class t_connection_context>
    struct jsonrpc2_server_config
    {
      i_jsonrpc2_server_handler<t_connection_context>* m_phandler;
      critical_section m_lock;
    };
    
    template<class t_connection_context = net_utils::connection_context_base>
    class jsonrpc2_connection_handler
    {
    public:
      typedef t_connection_context connection_context;
      typedef jsonrpc2_server_config<t_connection_context> config_type;

      jsonrpc2_connection_handler(i_service_endpoint* psnd_hndlr,
                                  config_type& config,
                                  t_connection_context& conn_context)
        : m_psnd_hndlr(psnd_hndlr),
          m_config(config),
          m_conn_context(conn_context),
          m_is_stop_handling(false)
      {}
      virtual ~jsonrpc2_connection_handler()
      {}

      bool release_protocol()
      {
        return true;
      }
      virtual bool thread_init()
      {
        return true;
      }
      virtual bool thread_deinit()
      {
        return true;
      }
      void handle_qued_callback()   
      {}
      bool after_init_connection()
      {
        return true;
      }
      virtual bool handle_recv(const void* ptr, size_t cb)
      {
        std::string buf((const char*)ptr, cb);
        LOG_PRINT_L0("JSONRPC2_RECV: " << ptr << "\r\n" << buf);

        bool res = handle_buff_in(buf);
        return res;
      }
    private:
      bool handle_buff_in(std::string& buf)
      {
        if(m_cache.size())
          m_cache += buf;
        else
          m_cache.swap(buf);

        m_is_stop_handling = false;
        while (!m_is_stop_handling) {
          std::string::size_type pos = match_end_of_request(m_cache);
          if (std::string::npos == pos) {
            m_is_stop_handling = true;
            if (m_cache.size() > 4096) {
              LOG_ERROR("jsonrpc2_connection_handler::handle_buff_in: Too long request");
              return false;
            }
            break;
          } else {
            extract_cached_request_and_handle(pos);
          }

          if (!m_cache.size()) {
            m_is_stop_handling = true;
          }
        }

        return true;
      }
      bool extract_cached_request_and_handle(std::string::size_type pos)
      {
        std::string request_data(m_cache.begin(), m_cache.begin() + pos);
        m_cache.erase(0, pos);
        return handle_request_and_send_response(request_data);
      }
      bool handle_request_and_send_response(const std::string& request_data)
      {
        CHECK_AND_ASSERT_MES(m_config.m_phandler, false, "m_config.m_phandler is NULL!!!!");
        std::string response_data;

        LOG_PRINT_L3("JSONRPC2_REQUEST: >> \r\n" << request_data);
        bool rpc_result = m_config.m_phandler->handle_rpc_request(request_data, response_data, m_conn_context);
        LOG_PRINT_L3("JSONRPC2_RESPONSE: << \r\n" << response_data);

        m_psnd_hndlr->do_send((void*)response_data.data(), response_data.size());
        return rpc_result;
      }
      std::string::size_type match_end_of_request(const std::string& buf)
      {
        std::string::size_type res = buf.find("\n");
        if(std::string::npos != res) {
          return res + 2;
        }
        return res;
      }

    protected:
      i_service_endpoint* m_psnd_hndlr;

    private:
      config_type& m_config;
      t_connection_context& m_conn_context;
      std::string m_cache;
      bool m_is_stop_handling;
    };
  }
}
}

#endif	/* JSONRPC_PROTOCOL_HANDLER_H */
