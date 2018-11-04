// Copyright (c) 2017-2018, The Monero Project
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

#ifndef MONERO_TRANSPORT_H
#define MONERO_TRANSPORT_H


#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/array.hpp>
#include <boost/utility/string_ref.hpp>

#include <typeinfo>
#include <type_traits>
#include "net/http_client.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "exceptions.hpp"
#include "trezor_defs.hpp"
#include "messages_map.hpp"

#include "messages/messages.pb.h"
#include "messages/messages-common.pb.h"
#include "messages/messages-management.pb.h"
#include "messages/messages-monero.pb.h"

namespace hw {
namespace trezor {

  using json = rapidjson::Document;
  using json_val = rapidjson::Value;
  namespace http = epee::net_utils::http;

  const std::string DEFAULT_BRIDGE = "127.0.0.1:21325";

  // Base HTTP comm serialization.
  bool t_serialize(const std::string & in, std::string & out);
  bool t_serialize(const json_val & in, std::string & out);
  std::string t_serialize(const json_val & in);

  bool t_deserialize(const std::string & in, std::string & out);
  bool t_deserialize(const std::string & in, json & out);

  // Flexible json serialization. HTTP client tailored for bridge API
  template<class t_req, class t_res, class t_transport>
  bool invoke_bridge_http(const boost::string_ref uri, const t_req & out_struct, t_res & result_struct, t_transport& transport, const boost::string_ref method = "POST", std::chrono::milliseconds timeout = std::chrono::seconds(180))
  {
    std::string req_param;
    t_serialize(out_struct, req_param);

    http::fields_list additional_params;
    additional_params.push_back(std::make_pair("Origin","https://monero.trezor.io"));
    additional_params.push_back(std::make_pair("Content-Type","application/json; charset=utf-8"));

    const http::http_response_info* pri = nullptr;
    if(!transport.invoke(uri, method, req_param, timeout, &pri, std::move(additional_params)))
    {
      MERROR("Failed to invoke http request to  " << uri);
      return false;
    }

    if(!pri)
    {
      MERROR("Failed to invoke http request to  " << uri << ", internal error (null response ptr)");
      return false;
    }

    if(pri->m_response_code != 200)
    {
      MERROR("Failed to invoke http request to  " << uri << ", wrong response code: " << pri->m_response_code
             << " Response Body: " << pri->m_body);
      return false;
    }

    return t_deserialize(pri->m_body, result_struct);
  }

  // Forward decl
  class Transport;
  class Protocol;

  // Communication protocol
  class Protocol {
  public:
    Protocol() = default;
    virtual ~Protocol() = default;
    virtual void session_begin(Transport & transport){ };
    virtual void session_end(Transport & transport){ };
    virtual void write(Transport & transport, const google::protobuf::Message & req)= 0;
    virtual void read(Transport & transport, std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type=nullptr)= 0;
  };

  class ProtocolV1 : public Protocol {
  public:
    ProtocolV1() = default;
    virtual ~ProtocolV1() = default;

    void write(Transport & transport, const google::protobuf::Message & req) override;
    void read(Transport & transport, std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type=nullptr) override;
  };


  // Base transport
  typedef std::vector<std::shared_ptr<Transport>> t_transport_vect;

  class Transport {
  public:
    Transport() = default;
    virtual ~Transport() = default;

    virtual bool ping() { return false; };
    virtual std::string get_path() const { return ""; };
    virtual void enumerate(t_transport_vect & res){};
    virtual void open(){};
    virtual void close(){};
    virtual void write(const google::protobuf::Message & req) =0;
    virtual void read(std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type=nullptr) =0;

    virtual void write_chunk(const void * buff, size_t size) { };
    virtual size_t read_chunk(void * buff, size_t size) { return 0; };
    virtual std::ostream& dump(std::ostream& o) const { return o << "Transport<>"; }
  };

  // Bridge transport
  class BridgeTransport : public Transport {
  public:
    BridgeTransport(
        boost::optional<std::string> device_path = boost::none,
        boost::optional<std::string> bridge_host = boost::none):
        m_device_path(device_path),
        m_bridge_host(bridge_host ? bridge_host.get() : DEFAULT_BRIDGE),
        m_response(boost::none),
        m_session(boost::none),
        m_device_info(boost::none)
    {
      m_http_client.set_server(m_bridge_host, boost::none, false);
    }

    virtual ~BridgeTransport() = default;

    static const char * PATH_PREFIX;

    std::string get_path() const override;
    void enumerate(t_transport_vect & res) override;

    void open() override;
    void close() override;

    void write(const google::protobuf::Message &req) override;
    void read(std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type=nullptr) override;

    const boost::optional<json> & device_info() const;
    std::ostream& dump(std::ostream& o) const override;

  private:
    epee::net_utils::http::http_simple_client m_http_client;
    std::string m_bridge_host;
    boost::optional<std::string> m_device_path;
    boost::optional<std::string> m_session;
    boost::optional<std::string> m_response;
    boost::optional<json> m_device_info;
  };

  // UdpTransport transport
  using boost::asio::ip::udp;

  class UdpTransport : public Transport {
  public:

    explicit UdpTransport(
        boost::optional<std::string> device_path=boost::none,
        boost::optional<std::shared_ptr<Protocol>> proto=boost::none);

    virtual ~UdpTransport() = default;

    static const char * PATH_PREFIX;
    static const char * DEFAULT_HOST;
    static const int DEFAULT_PORT;

    bool ping() override;
    std::string get_path() const override;
    void enumerate(t_transport_vect & res) override;

    void open() override;
    void close() override;

    void write(const google::protobuf::Message &req) override;
    void read(std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type=nullptr) override;

    void write_chunk(const void * buff, size_t size) override;
    size_t read_chunk(void * buff, size_t size) override;

    std::ostream& dump(std::ostream& o) const override;

  private:
    void require_socket();
    ssize_t receive(void * buff, size_t size, boost::system::error_code * error_code=nullptr, bool no_throw=false, boost::posix_time::time_duration timeout=boost::posix_time::seconds(10));
    void check_deadline();
    static void handle_receive(const boost::system::error_code& ec, std::size_t length,
                               boost::system::error_code* out_ec, std::size_t* out_length);
    bool ping_int(boost::posix_time::time_duration timeout=boost::posix_time::milliseconds(1500));

    std::shared_ptr<Protocol> m_proto;
    std::string m_device_host;
    int m_device_port;

    std::unique_ptr<udp::socket> m_socket;
    boost::asio::io_service m_io_service;
    boost::asio::deadline_timer m_deadline;
    udp::endpoint m_endpoint;
  };

  //
  // General helpers
  //

  /**
   * Enumerates all transports
   */
  void enumerate(t_transport_vect & res);

  /**
   * Transforms path to the transport
   */
  std::shared_ptr<Transport> transport(const std::string & path);

  /**
   * Transforms path to the particular transport
   */
  template<class t_transport>
  std::shared_ptr<t_transport> transport_typed(const std::string & path){
    auto t = transport(path);
    if (!t){
      return nullptr;
    }

    return std::dynamic_pointer_cast<t_transport>(t);
  }

  // Exception carries unexpected message being received
  namespace exc {
    class UnexpectedMessageException: public ProtocolException {
    protected:
      hw::trezor::messages::MessageType recvType;
      std::shared_ptr<google::protobuf::Message> recvMsg;

    public:
      using ProtocolException::ProtocolException;
      UnexpectedMessageException(): ProtocolException("Trezor returned unexpected message") {};
      UnexpectedMessageException(hw::trezor::messages::MessageType recvType,
                                 const std::shared_ptr<google::protobuf::Message> & recvMsg)
          : recvType(recvType), recvMsg(recvMsg) {
        reason = std::string("Trezor returned unexpected message: ") + std::to_string(recvType);
      }
    };
  }

  /**
   * Throws corresponding failure exception.
   */
  [[ noreturn ]] void throw_failure_exception(const messages::common::Failure * failure);

  /**
   * Simple wrapper for write-read message exchange with expected message response type.
   *
   * @throws UnexpectedMessageException if the response message type is different than expected.
   * Exception contains message type and the message itself.
   */
  template<class t_message>
  std::shared_ptr<t_message>
      exchange_message(Transport & transport, const google::protobuf::Message & req,
                       boost::optional<messages::MessageType> resp_type = boost::none)
  {
    // Require strictly protocol buffers response in the template.
    BOOST_STATIC_ASSERT(boost::is_base_of<google::protobuf::Message, t_message>::value);

    // Write the request
    transport.write(req);

    // Read the response
    std::shared_ptr<google::protobuf::Message> msg_resp;
    hw::trezor::messages::MessageType msg_resp_type;
    transport.read(msg_resp, &msg_resp_type);

    // Determine type of expected message response
    messages::MessageType required_type = resp_type ? resp_type.get() : MessageMapper::get_message_wire_number<t_message>();

    if (msg_resp_type == required_type) {
      return message_ptr_retype<t_message>(msg_resp);
    } else if (msg_resp_type == messages::MessageType_Failure){
      throw_failure_exception(dynamic_cast<messages::common::Failure*>(msg_resp.get()));
    } else {
      throw exc::UnexpectedMessageException(msg_resp_type, msg_resp);
    }
  }

  std::ostream& operator<<(std::ostream& o, hw::trezor::Transport const& t);
  std::ostream& operator<<(std::ostream& o, std::shared_ptr<hw::trezor::Transport> const& t);
}}


#endif //MONERO_TRANSPORT_H
