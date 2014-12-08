/*!
 * \file json_rpc_http_server.h
 * \brief Header for Json_rpc_http_server class
 */

#ifndef JSON_RPC_HTTP_SERVER_H
#define JSON_RPC_HTTP_SERVER_H

#include "net_skeleton/net_skeleton.h"
#include <boost/thread.hpp>
#include <string>

/*!
 * \namespace RPC
 * \brief RPC related utilities
 */
namespace RPC
{
  /*!
   * \class Json_rpc_http_server
   * \brief JSON HTTP RPC Server implemented with net_skeleton (aka fossa).
   * 
   * Provides a higher level interface to C-like net_skeleton.
   */
  class Json_rpc_http_server
  {
    struct ns_mgr mgr; /*!< Connection manager */
    struct ns_connection *nc; /*!< Connection pointer */
    boost::thread *server_thread; /*!< Server runs on this thread */
    /*!
     * \brief Repeatedly loops processing requests if any.
     */
    void poll();
    std::string m_ip; /*!< IP address where its listening */
    std::string m_port; /*!< Port where its listening */
    bool m_is_running; /*!< Whether the server is currently running */
    void (*m_ev_handler)(struct ns_connection *nc, int ev, void *ev_data); /*!< Server event handler function pointer */
  public:

    /**
     * \brief Constructor
     * \param ip IP address to bind
     * \param port Port number to bind
     * \param ev_handler Event handler function pointer
     */
    Json_rpc_http_server(const std::string &ip, const std::string &port,
      void (*ev_handler)(struct ns_connection *nc, int ev, void *ev_data));

    /**
     * \brief Destructor
     */
    ~Json_rpc_http_server();

    /*!
     * \brief Starts the server
     * \return True if start was successful
     */
    bool start();

    /*!
     * \brief Stops the server
     */
    void stop();

    static int parse_error; /*!< JSON request passed couldn't be parsed */
    static int invalid_request; /*!< JSON request invalid */
    static int invalid_params; /*!< JSON request had faulty/missing params */
    static int internal_error; /*!< JSON request resulted in an internal error */
  };
}

#endif
