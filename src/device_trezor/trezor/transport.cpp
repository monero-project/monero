// Copyright (c) 2017-2024, The Monero Project
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

#ifdef WITH_DEVICE_TREZOR_WEBUSB
#include <libusb.h>
#endif

#include <algorithm>
#include <functional>
#include <boost/endian/conversion.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "common/apply_permutation.h"
#include "transport.hpp"
#include "messages/messages-common.pb.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "device.trezor.transport"

using namespace std;
using json = rapidjson::Document;


namespace hw{
namespace trezor{

  bool t_serialize(const std::string & in, std::string & out){
    out = in;
    return true;
  }

  bool t_serialize(const epee::wipeable_string & in, std::string & out){
    out.assign(in.data(), in.size());
    return true;
  }

  bool t_serialize(const json_val & in, std::string & out){
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    in.Accept(writer);
    out = sb.GetString();
    return true;
  }

  std::string t_serialize(const json_val & in){
    std::string ret;
    t_serialize(in, ret);
    return ret;
  }

  bool t_deserialize(const std::string & in, std::string & out){
    out = in;
    return true;
  }

  bool t_deserialize(std::string & in, epee::wipeable_string & out){
    out = epee::wipeable_string(in);
    return true;
  }

  bool t_deserialize(const std::string & in, json & out){
    if (out.Parse(in.c_str()).HasParseError()) {
      throw exc::CommunicationException("JSON parse error");
    }
    return true;
  }

  static std::string json_get_string(const rapidjson::Value & in){
    return std::string(in.GetString());
  }

  uint64_t pack_version(uint32_t major, uint32_t minor, uint32_t patch)
  {
    // packing (major, minor, patch) to 64 B: 16 B | 24 B | 24 B
    const unsigned bits_1 = 16;
    const unsigned bits_2 = 24;
    const uint32_t mask_1 = (1 << bits_1) - 1;
    const uint32_t mask_2 = (1 << bits_2) - 1;
    CHECK_AND_ASSERT_THROW_MES(major <= mask_1 && minor <= mask_2 && patch <= mask_2, "Version numbers overflow packing scheme");
    return patch | (((uint64_t)minor) << bits_2) | (((uint64_t)major) << (bits_1 + bits_2));
  }

  typedef struct {
    uint16_t trezor_type;
    uint16_t id_vendor;
    uint16_t id_product;
  } trezor_usb_desc_t;

  static trezor_usb_desc_t TREZOR_DESC_T1 = {1, 0x534C, 0x0001};
  static trezor_usb_desc_t TREZOR_DESC_T2 = {2, 0x1209, 0x53C1};
  static trezor_usb_desc_t TREZOR_DESC_T2_BL = {3, 0x1209, 0x53C0};

  static trezor_usb_desc_t TREZOR_DESCS[] = {
      TREZOR_DESC_T1,
      TREZOR_DESC_T2,
      TREZOR_DESC_T2_BL,
  };

  static size_t TREZOR_DESCS_LEN = sizeof(TREZOR_DESCS)/sizeof(TREZOR_DESCS[0]);

  static ssize_t get_device_idx(uint16_t id_vendor, uint16_t id_product){
    for(size_t i = 0; i < TREZOR_DESCS_LEN; ++i){
      if (TREZOR_DESCS[i].id_vendor == id_vendor && TREZOR_DESCS[i].id_product == id_product){
        return i;
      }
    }

    return -1;
  }

  static bool is_device_supported(ssize_t device_idx){
    CHECK_AND_ASSERT_THROW_MES(device_idx < (ssize_t)TREZOR_DESCS_LEN, "Device desc idx too big");
    if (device_idx < 0){
      return false;
    }

#ifdef TREZOR_1_SUPPORTED
    return true;
#else
    return TREZOR_DESCS[device_idx].trezor_type != 1;
#endif
  }

  //
  // Helpers
  //

#define PROTO_MAGIC_SIZE 3
#define PROTO_HEADER_SIZE 6

  static size_t message_size(const google::protobuf::Message &req){
#if GOOGLE_PROTOBUF_VERSION < 3006001
    return size_t(req.ByteSize());
#else
    return req.ByteSizeLong();
#endif
  }

  static size_t serialize_message_buffer_size(size_t msg_size) {
    return PROTO_HEADER_SIZE + msg_size;  // tag 2B + len 4B
  }

  static void serialize_message_header(void * buff, uint16_t tag, uint32_t len){
    uint16_t wire_tag = boost::endian::native_to_big(static_cast<uint16_t>(tag));
    uint32_t wire_len = boost::endian::native_to_big(static_cast<uint32_t>(len));
    memcpy(buff, (void *) &wire_tag, 2);
    memcpy((uint8_t*)buff + 2, (void *) &wire_len, 4);
  }

  static void deserialize_message_header(const void * buff, uint16_t & tag, uint32_t & len){
    uint16_t wire_tag;
    uint32_t wire_len;
    memcpy(&wire_tag, buff, 2);
    memcpy(&wire_len, (uint8_t*)buff + 2, 4);

    tag = boost::endian::big_to_native(wire_tag);
    len = boost::endian::big_to_native(wire_len);
  }

  static void serialize_message(const google::protobuf::Message &req, size_t msg_size, uint8_t * buff, size_t buff_size) {
    auto msg_wire_num = MessageMapper::get_message_wire_number(req);
    const auto req_buffer_size = serialize_message_buffer_size(msg_size);
    if (req_buffer_size > buff_size){
      throw std::invalid_argument("Buffer too small");
    }

    serialize_message_header(buff, msg_wire_num, msg_size);
    if (!req.SerializeToArray(buff + PROTO_HEADER_SIZE, msg_size)){
      throw exc::EncodingException("Message serialization error");
    }
  }

  //
  // Communication protocol
  //

#define REPLEN 64

  void ProtocolV1::write(Transport & transport, const google::protobuf::Message & req){
    const auto msg_size = message_size(req);
    const auto buff_size = serialize_message_buffer_size(msg_size) + 2;

    epee::wipeable_string req_buff;
    epee::wipeable_string chunk_buff;

    req_buff.resize(buff_size);
    chunk_buff.resize(REPLEN);

    uint8_t * req_buff_raw = reinterpret_cast<uint8_t *>(req_buff.data());
    uint8_t * chunk_buff_raw = reinterpret_cast<uint8_t *>(chunk_buff.data());

    req_buff_raw[0] = '#';
    req_buff_raw[1] = '#';

    serialize_message(req, msg_size, req_buff_raw + 2, buff_size - 2);

    size_t offset = 0;

    // Chunk by chunk upload
    while(offset < buff_size){
      auto to_copy = std::min((size_t)(buff_size - offset), (size_t)(REPLEN - 1));

      chunk_buff_raw[0] = '?';
      memcpy(chunk_buff_raw + 1, req_buff_raw + offset, to_copy);

      // Pad with zeros
      if (to_copy < REPLEN - 1){
        memset(chunk_buff_raw + 1 + to_copy, 0, REPLEN - 1 - to_copy);
      }

      transport.write_chunk(chunk_buff_raw, REPLEN);
      offset += REPLEN - 1;
    }
  }

  void ProtocolV1::read(Transport & transport, std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type){
    epee::wipeable_string chunk_buff;
    chunk_buff.resize(REPLEN);
    char * chunk_buff_raw = chunk_buff.data();

    // Initial chunk read
    size_t nread = transport.read_chunk(chunk_buff_raw, REPLEN);
    if (nread != REPLEN){
      throw exc::CommunicationException("Read chunk has invalid size");
    }

    if (memcmp(chunk_buff_raw, "?##", PROTO_MAGIC_SIZE) != 0){
      throw exc::CommunicationException("Malformed chunk");
    }

    uint16_t tag;
    uint32_t len;
    nread -= PROTO_MAGIC_SIZE + PROTO_HEADER_SIZE;
    deserialize_message_header(chunk_buff_raw + PROTO_MAGIC_SIZE, tag, len);

    epee::wipeable_string data_acc(chunk_buff_raw + PROTO_MAGIC_SIZE + PROTO_HEADER_SIZE, nread);
    data_acc.reserve(len);

    while(nread < len){
      const size_t cur = transport.read_chunk(chunk_buff_raw, REPLEN);
      if (chunk_buff_raw[0] != '?'){
        throw exc::CommunicationException("Chunk malformed");
      }

      data_acc.append(chunk_buff_raw + 1, cur - 1);
      nread += cur - 1;
    }

    if (msg_type){
      *msg_type = static_cast<messages::MessageType>(tag);
    }

    if (nread < len){
      throw exc::CommunicationException("Response incomplete");
    }

    std::shared_ptr<google::protobuf::Message> msg_wrap(MessageMapper::get_message(tag));
    if (!msg_wrap->ParseFromArray(data_acc.data(), len)){
      throw exc::CommunicationException("Message could not be parsed");
    }

    msg = msg_wrap;
  }

  static void assert_port_number(uint32_t port)
  {
    CHECK_AND_ASSERT_THROW_MES(port >= 1024 && port < 65535, "Invalid port number: " << port);
  }

  Transport::Transport(): m_open_counter(0) {

  }

  bool Transport::pre_open(){
    if (m_open_counter > 0){
      MTRACE("Already opened, count: " << m_open_counter);
      m_open_counter += 1;
      return false;

    } else if (m_open_counter < 0){
      MTRACE("Negative open value: " << m_open_counter);

    }

    // Caller should set m_open_counter to 1 after open
    m_open_counter = 0;
    return true;
  }

  bool Transport::pre_close(){
    m_open_counter -= 1;

    if (m_open_counter < 0){
      MDEBUG("Already closed. Counter " << m_open_counter);

    } else if (m_open_counter == 0) {
      return true;

    }

    return false;
  }

  //
  // Bridge transport
  //

  const char * BridgeTransport::PATH_PREFIX = "bridge:";

  BridgeTransport::BridgeTransport(
        boost::optional<std::string> device_path,
        boost::optional<std::string> bridge_host):
    m_device_path(device_path),
    m_bridge_host(bridge_host ? bridge_host.get() : DEFAULT_BRIDGE),
    m_response(boost::none),
    m_session(boost::none),
    m_device_info(boost::none)
    {
      const char *env_bridge_port = nullptr;
      if (!bridge_host && (env_bridge_port = getenv("TREZOR_BRIDGE_PORT")) != nullptr)
      {
        uint16_t bridge_port;
        CHECK_AND_ASSERT_THROW_MES(epee::string_tools::get_xtype_from_string(bridge_port, env_bridge_port), "Invalid bridge port: " << env_bridge_port);
        assert_port_number(bridge_port);

        m_bridge_host = std::string("127.0.0.1:") + boost::lexical_cast<std::string>(env_bridge_port);
        MDEBUG("Bridge host: " << m_bridge_host);
      }

      m_http_client.set_server(m_bridge_host, boost::none, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
    }

  std::string BridgeTransport::get_path() const {
    if (!m_device_path){
      return "";
    }

    std::string path(PATH_PREFIX);
    return path + m_device_path.get();
  }

  void BridgeTransport::enumerate(t_transport_vect & res) {
    json bridge_res;
    std::string req;

    bool req_status = invoke_bridge_http("/enumerate", req, bridge_res, m_http_client);
    if (!req_status){
      throw exc::CommunicationException("Bridge enumeration failed");
    }

    for(rapidjson::Value::ConstValueIterator itr = bridge_res.Begin(); itr != bridge_res.End(); ++itr){
      auto element = itr->GetObject();
      auto t = std::make_shared<BridgeTransport>(boost::make_optional(json_get_string(element["path"])));

      auto itr_vendor = element.FindMember("vendor");
      auto itr_product = element.FindMember("product");
      if (itr_vendor != element.MemberEnd() && itr_product != element.MemberEnd()
        && itr_vendor->value.IsNumber() && itr_product->value.IsNumber()){
        try {
          const auto id_vendor = (uint16_t) itr_vendor->value.GetUint64();
          const auto id_product = (uint16_t) itr_product->value.GetUint64();
          const auto device_idx = get_device_idx(id_vendor, id_product);
          if (!is_device_supported(device_idx)){
            MDEBUG("Device with idx " << device_idx << " is not supported. Vendor: " << id_vendor << ", product: " << id_product);
            continue;
          }
        } catch(const std::exception &e){
          MERROR("Could not detect vendor & product: " << e.what());
        }
      }

      t->m_device_info.emplace();
      t->m_device_info->CopyFrom(*itr, t->m_device_info->GetAllocator());
      res.push_back(t);
    }
  }

  void BridgeTransport::open() {
    if (!pre_open()){
      return;
    }

    if (!m_device_path){
      throw exc::CommunicationException("Coud not open, empty device path");
    }

    std::string uri = "/acquire/" + m_device_path.get() + "/null";
    std::string req;
    json bridge_res;
    bool req_status = invoke_bridge_http(uri, req, bridge_res, m_http_client);
    if (!req_status){
      throw exc::CommunicationException("Failed to acquire device");
    }

    m_session = boost::make_optional(json_get_string(bridge_res["session"]));
    m_open_counter = 1;
  }

  void BridgeTransport::close() {
    if (!pre_close()){
      return;
    }

    MTRACE("Closing Trezor:BridgeTransport");
    if (!m_device_path || !m_session){
      throw exc::CommunicationException("Device not open");
    }

    std::string uri = "/release/" + m_session.get();
    std::string req;
    json bridge_res;
    bool req_status = invoke_bridge_http(uri, req, bridge_res, m_http_client);
    if (!req_status){
      throw exc::CommunicationException("Failed to release device");
    }

    m_session = boost::none;
  }

  void BridgeTransport::write(const google::protobuf::Message &req) {
    m_response = boost::none;

    const auto msg_size = message_size(req);
    const auto buff_size = serialize_message_buffer_size(msg_size);
    epee::wipeable_string req_buff;
    req_buff.resize(buff_size);

    uint8_t * req_buff_raw = reinterpret_cast<uint8_t *>(req_buff.data());

    serialize_message(req, msg_size, req_buff_raw, buff_size);

    std::string uri = "/call/" + m_session.get();
    epee::wipeable_string res_hex;
    epee::wipeable_string req_hex = epee::to_hex::wipeable_string(epee::span<const std::uint8_t>(req_buff_raw, buff_size));

    bool req_status = invoke_bridge_http(uri, req_hex, res_hex, m_http_client);
    if (!req_status){
      throw exc::CommunicationException("Call method failed");
    }

    m_response = res_hex;
  }

  void BridgeTransport::read(std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type) {
    if (!m_response){
      throw exc::CommunicationException("Could not read, no response stored");
    }

    boost::optional<epee::wipeable_string> bin_data = m_response->parse_hexstr();
    if (!bin_data){
      throw exc::CommunicationException("Response is not well hexcoded");
    }

    uint16_t msg_tag;
    uint32_t msg_len;
    deserialize_message_header(bin_data->data(), msg_tag, msg_len);
    if (bin_data->size() != msg_len + PROTO_HEADER_SIZE){
      throw exc::CommunicationException("Response is not well hexcoded");
    }

    if (msg_type){
      *msg_type = static_cast<messages::MessageType>(msg_tag);
    }

    std::shared_ptr<google::protobuf::Message> msg_wrap(MessageMapper::get_message(msg_tag));
    if (!msg_wrap->ParseFromArray(bin_data->data() + PROTO_HEADER_SIZE, msg_len)){
      throw exc::EncodingException("Response is not well hexcoded");
    }
    msg = msg_wrap;
  }

  const boost::optional<json> & BridgeTransport::device_info() const {
    return m_device_info;
  }

  std::ostream& BridgeTransport::dump(std::ostream& o) const {
    return o << "BridgeTransport<path=" << (m_device_path ? get_path() : "None")
             << ", info=" << (m_device_info ? t_serialize(m_device_info.get()) : "None")
             << ", session=" << (m_session ? m_session.get() : "None")
             << ">";
  }

  //
  // UdpTransport
  //
  const char * UdpTransport::PATH_PREFIX = "udp:";
  const char * UdpTransport::DEFAULT_HOST = "127.0.0.1";
  const int UdpTransport::DEFAULT_PORT = 21324;

  static void parse_udp_path(std::string &host, int &port, std::string path)
  {
    if (boost::starts_with(path, UdpTransport::PATH_PREFIX))
    {
      path = path.substr(strlen(UdpTransport::PATH_PREFIX));
    }

    auto delim = path.find(':');
    if (delim == std::string::npos) {
      host = path;
    } else {
      host = path.substr(0, delim);
      port = std::stoi(path.substr(delim + 1));
    }
  }

  UdpTransport::UdpTransport(boost::optional<std::string> device_path,
                             boost::optional<std::shared_ptr<Protocol>> proto) :
      m_io_service(), m_deadline(m_io_service)
  {
    m_device_host = DEFAULT_HOST;
    m_device_port = DEFAULT_PORT;
    const char *env_trezor_path = nullptr;

    if (device_path) {
      parse_udp_path(m_device_host, m_device_port, device_path.get());
    } else if ((env_trezor_path = getenv("TREZOR_PATH")) != nullptr && boost::starts_with(env_trezor_path, UdpTransport::PATH_PREFIX)){
      parse_udp_path(m_device_host, m_device_port, std::string(env_trezor_path));
      MDEBUG("Applied TREZOR_PATH: " << m_device_host << ":" << m_device_port);
    } else {
      m_device_host = DEFAULT_HOST;
    }

    assert_port_number((uint32_t)m_device_port);
    if (m_device_host != "localhost" && m_device_host != DEFAULT_HOST){
      throw std::invalid_argument("Local endpoint allowed only");
    }

    m_proto = proto ? proto.get() : std::make_shared<ProtocolV1>();
  }

  std::string UdpTransport::get_path() const {
    std::string path(PATH_PREFIX);
    return path + m_device_host + ":" + std::to_string(m_device_port);
  }

  void UdpTransport::require_socket(){
    if (!m_socket){
      throw exc::NotConnectedException("Socket not connected");
    }
  }

  bool UdpTransport::ping(){
    return ping_int();
  }

  bool UdpTransport::ping_int(boost::posix_time::time_duration timeout){
    require_socket();
    try {
      std::string req = "PINGPING";
      char res[8];

      const auto written = m_socket->send_to(boost::asio::buffer(req.c_str(), req.size()), m_endpoint);
      if (written != req.size())
        return false;
      memset(res, 0, sizeof(res));
      const auto received = receive(res, 8, nullptr, false, timeout);
      if (received != 8)
        return false;

      return memcmp(res, "PONGPONG", 8) == 0;

    } catch(...){
      return false;
    }
  }

  void UdpTransport::enumerate(t_transport_vect & res) {
    std::shared_ptr<UdpTransport> t = std::make_shared<UdpTransport>();
    bool t_works = false;

    try{
      t->open();
      t_works = t->ping();
    } catch(...) {

    }
    t->close();
    if (t_works){
      res.push_back(t);
    }
  }

  void UdpTransport::open() {
    if (!pre_open()){
      return;
    }

    udp::resolver resolver(m_io_service);
    udp::resolver::query query(udp::v4(), m_device_host, std::to_string(m_device_port));
    m_endpoint = *resolver.resolve(query);

    m_socket.reset(new udp::socket(m_io_service));
    m_socket->open(udp::v4());

    m_deadline.expires_at(boost::posix_time::pos_infin);
    check_deadline();

    m_proto->session_begin(*this);
    m_open_counter = 1;
  }

  void UdpTransport::close() {
    if (!pre_close()){
      return;
    }

    MTRACE("Closing Trezor:UdpTransport");
    if (!m_socket) {
      throw exc::CommunicationException("Socket is already closed");
    }

    m_proto->session_end(*this);
    m_socket->close();
    m_socket = nullptr;
  }

  std::shared_ptr<Transport> UdpTransport::find_debug() {
#ifdef WITH_TREZOR_DEBUGGING
    std::shared_ptr<UdpTransport> t = std::make_shared<UdpTransport>();
    t->m_proto = std::make_shared<ProtocolV1>();
    t->m_device_host = m_device_host;
    t->m_device_port = m_device_port + 1;
    return t;
#else
    MINFO("Debug link is disabled in production");
    return nullptr;
#endif
  }

  void UdpTransport::write_chunk(const void * buff, size_t size){
    require_socket();

    if (size != 64){
      throw exc::CommunicationException("Invalid chunk size");
    }

    auto written = m_socket->send_to(boost::asio::buffer(buff, size), m_endpoint);
    if (size != written){
      throw exc::CommunicationException("Could not send the whole chunk");
    }
  }

  size_t UdpTransport::read_chunk(void * buff, size_t size){
    require_socket();
    if (size < 64){
      throw std::invalid_argument("Buffer too small");
    }

    ssize_t len;
    while(true) {
      try {
        boost::system::error_code ec;
        len = receive(buff, size, &ec, true);
        if (ec == boost::asio::error::operation_aborted) {
          continue;
        } else if (ec) {
          throw exc::CommunicationException(std::string("Comm error: ") + ec.message());
        }

        if (len != 64) {
          throw exc::CommunicationException("Invalid chunk size");
        }

        break;

      } catch(exc::CommunicationException const& e){
        throw;
      } catch(std::exception const& e){
        MWARNING("Error reading chunk, reason: " << e.what());
        throw exc::CommunicationException(std::string("Chunk read error: ") + std::string(e.what()));
      }
    }

    return static_cast<size_t>(len);
  }

  ssize_t UdpTransport::receive(void * buff, size_t size, boost::system::error_code * error_code, bool no_throw, boost::posix_time::time_duration timeout){
    boost::system::error_code ec;
    boost::asio::mutable_buffer buffer = boost::asio::buffer(buff, size);

    require_socket();

    // Set a deadline for the asynchronous operation.
    m_deadline.expires_from_now(timeout);

    // Set up the variables that receive the result of the asynchronous
    // operation. The error code is set to would_block to signal that the
    // operation is incomplete. Asio guarantees that its asynchronous
    // operations will never fail with would_block, so any other value in
    // ec indicates completion.
    ec = boost::asio::error::would_block;
    std::size_t length = 0;

    // Start the asynchronous operation itself. The handle_receive function
    // used as a callback will update the ec and length variables.
    m_socket->async_receive_from(boost::asio::buffer(buffer), m_endpoint,
                                 std::bind(&UdpTransport::handle_receive, std::placeholders::_1, std::placeholders::_2, &ec, &length));

    // Block until the asynchronous operation has completed.
    do {
      m_io_service.run_one();
    }
    while (ec == boost::asio::error::would_block);

    if (error_code){
      *error_code = ec;
    }

    if (no_throw){
      return length;
    }

    // Operation result
    if (ec == boost::asio::error::operation_aborted){
      throw exc::TimeoutException();

    } else if (ec) {
      MWARNING("Reading from UDP socket failed: " << ec.message());
      throw exc::CommunicationException();

    }

    return length;
  }

  void UdpTransport::write(const google::protobuf::Message &req) {
    m_proto->write(*this, req);
  }

  void UdpTransport::read(std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type) {
    m_proto->read(*this, msg, msg_type);
  }

  void UdpTransport::check_deadline(){
    if (!m_socket){
      return;  // no active socket.
    }

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (m_deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
      // The deadline has passed. The outstanding asynchronous operation needs
      // to be cancelled so that the blocked receive() function will return.
      //
      // Please note that cancel() has portability issues on some versions of
      // Microsoft Windows, and it may be necessary to use close() instead.
      // Consult the documentation for cancel() for further information.
      m_socket->cancel();

      // There is no longer an active deadline. The expiry is set to positive
      // infinity so that the actor takes no action until a new deadline is set.
      m_deadline.expires_at(boost::posix_time::pos_infin);
    }

    // Put the actor back to sleep.
    m_deadline.async_wait(boost::bind(&UdpTransport::check_deadline, this));
  }

  void UdpTransport::handle_receive(const boost::system::error_code &ec, std::size_t length,
                                    boost::system::error_code *out_ec, std::size_t *out_length) {
    *out_ec = ec;
    *out_length = length;
  }

  std::ostream& UdpTransport::dump(std::ostream& o) const {
    return o << "UdpTransport<path=" << get_path()
             << ", socket_alive=" << (m_socket ? "true" : "false")
             << ">";
  }

#ifdef WITH_DEVICE_TREZOR_WEBUSB

  static bool is_trezor1(libusb_device_descriptor * info){
    return info->idVendor == TREZOR_DESC_T1.id_vendor && info->idProduct == TREZOR_DESC_T1.id_product;
  }

  static bool is_trezor2(libusb_device_descriptor * info){
    return info->idVendor == TREZOR_DESC_T2.id_vendor && info->idProduct == TREZOR_DESC_T2.id_product;
  }

  static bool is_trezor2_bl(libusb_device_descriptor * info){
    return info->idVendor == TREZOR_DESC_T2_BL.id_vendor && info->idProduct == TREZOR_DESC_T2_BL.id_product;
  }

  static ssize_t get_trezor_dev_id(libusb_device_descriptor *info){
    CHECK_AND_ASSERT_THROW_MES(info, "Empty device descriptor");
    return get_device_idx(info->idVendor, info->idProduct);
  }

  static void set_libusb_log(libusb_context *ctx){
    CHECK_AND_ASSERT_THROW_MES(ctx, "Null libusb context");

    // http://libusb.sourceforge.net/api-1.0/group__libusb__lib.html
#if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000106)
#  define TREZOR_LIBUSB_SET_DEBUG(ctx, level) libusb_set_option(ctx,  LIBUSB_OPTION_LOG_LEVEL, level)
#else
#  define TREZOR_LIBUSB_SET_DEBUG(ctx, level) libusb_set_debug(ctx, level)
#endif

    if (ELPP->vRegistry()->allowed(el::Level::Debug, MONERO_DEFAULT_LOG_CATEGORY))
      TREZOR_LIBUSB_SET_DEBUG(ctx, 3);
    else if (ELPP->vRegistry()->allowed(el::Level::Warning, MONERO_DEFAULT_LOG_CATEGORY))
      TREZOR_LIBUSB_SET_DEBUG(ctx, 2);
    else if (ELPP->vRegistry()->allowed(el::Level::Error, MONERO_DEFAULT_LOG_CATEGORY))
      TREZOR_LIBUSB_SET_DEBUG(ctx, 1);

#undef TREZOR_LIBUSB_SET_DEBUG
  }

  static int get_libusb_ports(libusb_device *dev, std::vector<uint8_t> &path){
    uint8_t tmp_path[16];
    int r = libusb_get_port_numbers(dev, tmp_path, sizeof(tmp_path));
    CHECK_AND_ASSERT_MES(r != LIBUSB_ERROR_OVERFLOW, -1, "Libusb path array too small");
    CHECK_AND_ASSERT_MES(r >= 0, -1, "Libusb path array error");

    path.resize(r);
    for (int i = 0; i < r; i++){
      path[i] = tmp_path[i];
    }

    return 0;
  }

  static std::string get_usb_path(uint8_t bus_id, const std::vector<uint8_t> &path){
    std::stringstream ss;
    ss << WebUsbTransport::PATH_PREFIX << (boost::format("%03d") % ((int)bus_id));
    for(uint8_t port : path){
      ss << ":" << ((int) port);
    }
    return ss.str();
  }

  const char * WebUsbTransport::PATH_PREFIX = "webusb:";

  WebUsbTransport::WebUsbTransport(
      boost::optional<libusb_device_descriptor*> descriptor,
      boost::optional<std::shared_ptr<Protocol>> proto
  ): m_usb_session(nullptr), m_usb_device(nullptr), m_usb_device_handle(nullptr),
     m_bus_id(-1), m_device_addr(-1)
  {
    if (descriptor){
      libusb_device_descriptor * desc = new libusb_device_descriptor;
      memcpy(desc, descriptor.get(), sizeof(libusb_device_descriptor));
      this->m_usb_device_desc.reset(desc);
    }

    m_proto = proto ? proto.get() : std::make_shared<ProtocolV1>();

#ifdef WITH_TREZOR_DEBUGGING
    m_debug_mode = false;
#endif
  }

  WebUsbTransport::~WebUsbTransport(){
    if (m_usb_device){
      close();
    }

    if (m_usb_session) {
      libusb_exit(m_usb_session);
      m_usb_session = nullptr;
    }
  }

  void WebUsbTransport::require_device() const{
    if (!m_usb_device_desc){
      throw std::runtime_error("No USB device specified");
    }
  }

  void WebUsbTransport::require_connected() const{
    require_device();
    if (!m_usb_device_handle){
      throw std::runtime_error("USB Device not opened");
    }
  }

  void WebUsbTransport::enumerate(t_transport_vect & res) {
    int r;
    libusb_device **devs;
    libusb_context *ctx = nullptr;

    r = libusb_init(&ctx);
    CHECK_AND_ASSERT_THROW_MES(r >= 0, "Unable to init libusb");

    set_libusb_log(ctx);

    ssize_t cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0){
      libusb_exit(ctx);
      throw std::runtime_error("Unable to enumerate libusb devices");
    }

    MTRACE("Libusb devices: " << cnt);

    for(ssize_t i = 0; i < cnt; i++) {
      libusb_device_descriptor desc{};
      r = libusb_get_device_descriptor(devs[i], &desc);
      if (r < 0){
        MERROR("Unable to get libusb device descriptor " << i);
        continue;
      }

      const auto trezor_dev_idx = get_trezor_dev_id(&desc);
      if (!is_device_supported(trezor_dev_idx)){
        continue;
      }

      MTRACE("Found Trezor device: " << desc.idVendor << ":" << desc.idProduct << " dev_idx " << (int)trezor_dev_idx);

      auto t = std::make_shared<WebUsbTransport>(boost::make_optional(&desc));
      t->m_bus_id = libusb_get_bus_number(devs[i]);
      t->m_device_addr = libusb_get_device_address(devs[i]);

      // Port resolution may fail. Non-critical error, just addressing precision is decreased.
      get_libusb_ports(devs[i], t->m_port_numbers);

      res.push_back(t);
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(ctx);
  }

  std::string WebUsbTransport::get_path() const {
    if (!m_usb_device_desc){
      return "";
    }

    return get_usb_path(static_cast<uint8_t>(m_bus_id), m_port_numbers);
  };

  void WebUsbTransport::open() {
    if (!pre_open()){
      return;
    }
    const int interface = get_interface();

#define TREZOR_DESTROY_SESSION() do { libusb_exit(m_usb_session); m_usb_session = nullptr; } while(0)

    int r;
    libusb_device **devs = nullptr;

    if (m_usb_session) {
      TREZOR_DESTROY_SESSION();
    }

    r = libusb_init(&m_usb_session);
    CHECK_AND_ASSERT_THROW_MES(r >= 0, "Unable to init libusb");
    set_libusb_log(m_usb_session);

    bool found = false;
    int open_res = 0;

    ssize_t cnt = libusb_get_device_list(m_usb_session, &devs);
    if (cnt < 0){
      TREZOR_DESTROY_SESSION();
      throw std::runtime_error("Unable to enumerate libusb devices");
    }

    for (ssize_t i = 0; i < cnt; i++) {
      libusb_device_descriptor desc{};
      r = libusb_get_device_descriptor(devs[i], &desc);
      if (r < 0){
        MERROR("Unable to get libusb device descriptor " << i);
        continue;
      }

      const auto trezor_dev_idx = get_trezor_dev_id(&desc);
      if (!is_device_supported(trezor_dev_idx)){
        continue;
      }

      auto bus_id = libusb_get_bus_number(devs[i]);
      std::vector<uint8_t> path;

      // Port resolution may fail. Non-critical error, just addressing precision is decreased.
      get_libusb_ports(devs[i], path);

      MTRACE("Found Trezor device: " << desc.idVendor << ":" << desc.idProduct
                                     << ", dev_idx: " << (int)trezor_dev_idx
                                     << ". path: " << get_usb_path(bus_id, path));

      if (bus_id == m_bus_id && path == m_port_numbers) {
        found = true;
        m_usb_device = devs[i];
        open_res = libusb_open(m_usb_device, &m_usb_device_handle);
        break;
      }
    }

    libusb_free_device_list(devs, 1);

    if (!found){
      TREZOR_DESTROY_SESSION();
      throw exc::DeviceAcquireException("Device not found");

    } else if (found && open_res != 0) {
      m_usb_device_handle = nullptr;
      m_usb_device = nullptr;
      TREZOR_DESTROY_SESSION();
      throw exc::DeviceAcquireException("Unable to open libusb device");
    }

    r = libusb_claim_interface(m_usb_device_handle, interface);

    if (r != 0){
      libusb_close(m_usb_device_handle);
      m_usb_device_handle = nullptr;
      m_usb_device = nullptr;
      TREZOR_DESTROY_SESSION();
      throw exc::DeviceAcquireException("Unable to claim libusb device");
    }

    m_open_counter = 1;
    m_proto->session_begin(*this);
    
#undef TREZOR_DESTROY_SESSION
  };

  void WebUsbTransport::close() {
    if (!pre_close()){
      return;
    }

    MTRACE("Closing Trezor:WebUsbTransport");
    m_proto->session_end(*this);

    int r = libusb_release_interface(m_usb_device_handle, get_interface());
    if (r != 0){
      MERROR("Could not release libusb interface: " << r);
    }

    m_usb_device = nullptr;
    if (m_usb_device_handle) {
      libusb_close(m_usb_device_handle);
      m_usb_device_handle = nullptr;
    }

    if (m_usb_session) {
      libusb_exit(m_usb_session);
      m_usb_session = nullptr;
    }
  };

  std::shared_ptr<Transport> WebUsbTransport::find_debug() {
#ifdef WITH_TREZOR_DEBUGGING
    require_device();
    auto t = std::make_shared<WebUsbTransport>(boost::make_optional(m_usb_device_desc.get()));
    t->m_bus_id = m_bus_id;
    t->m_device_addr = m_device_addr;
    t->m_port_numbers = m_port_numbers;
    t->m_debug_mode = true;
    return t;
#else
      MINFO("Debug link is disabled in production");
      return nullptr;
#endif
    }

  int WebUsbTransport::get_interface() const{
    const int INTERFACE_NORMAL = 0;
#ifdef WITH_TREZOR_DEBUGGING
    const int INTERFACE_DEBUG = 1;
    return m_debug_mode ? INTERFACE_DEBUG : INTERFACE_NORMAL;
#else
    return INTERFACE_NORMAL;
#endif
  }

  unsigned char WebUsbTransport::get_endpoint() const{
    const unsigned char ENDPOINT_NORMAL = 1;
#ifdef WITH_TREZOR_DEBUGGING
    const unsigned char ENDPOINT_DEBUG = 2;
    return m_debug_mode ? ENDPOINT_DEBUG : ENDPOINT_NORMAL;
#else
    return ENDPOINT_NORMAL;
#endif
  }

  void WebUsbTransport::write(const google::protobuf::Message &req) {
    m_proto->write(*this, req);
  };

  void WebUsbTransport::read(std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type) {
    m_proto->read(*this, msg, msg_type);
  };

  void WebUsbTransport::write_chunk(const void * buff, size_t size) {
    require_connected();
    if (size != REPLEN){
      throw exc::CommunicationException("Invalid chunk size: ");
    }

    unsigned char endpoint = get_endpoint();
    endpoint = (endpoint & ~LIBUSB_ENDPOINT_DIR_MASK) | LIBUSB_ENDPOINT_OUT;

    int transferred = 0;
    int r = libusb_interrupt_transfer(m_usb_device_handle, endpoint, (unsigned char*)buff, (int)size, &transferred, 0);
    CHECK_AND_ASSERT_THROW_MES(r == 0, "Unable to transfer, r: " << r);
    if (transferred != (int)size){
      throw exc::CommunicationException("Could not transfer chunk");
    }
  };

  size_t WebUsbTransport::read_chunk(void * buff, size_t size) {
    require_connected();
    unsigned char endpoint = get_endpoint();
    endpoint = (endpoint & ~LIBUSB_ENDPOINT_DIR_MASK) | LIBUSB_ENDPOINT_IN;

    int transferred = 0;
    int r = libusb_interrupt_transfer(m_usb_device_handle, endpoint, (unsigned char*)buff, (int)size, &transferred, 0);
    CHECK_AND_ASSERT_THROW_MES(r == 0, "Unable to transfer, r: " << r);
    if (transferred != (int)size){
      throw exc::CommunicationException("Could not read the chunk");
    }

    return transferred;
  };

  std::ostream& WebUsbTransport::dump(std::ostream& o) const {
    o << "WebUsbTransport<path=" << get_path()
             << ", vendorId=" << (m_usb_device_desc ? std::to_string(m_usb_device_desc->idVendor) : "?")
             << ", productId=" << (m_usb_device_desc ? std::to_string(m_usb_device_desc->idProduct) : "?")
             << ", deviceType=";

    if (m_usb_device_desc){
      if (is_trezor1(m_usb_device_desc.get()))
        o << "TrezorOne";
      else if (is_trezor2(m_usb_device_desc.get()))
        o << "TrezorT";
      else if (is_trezor2_bl(m_usb_device_desc.get()))
        o << "TrezorT BL";
    } else {
      o << "?";
    }

    return o << ">";
  };

#endif  // WITH_DEVICE_TREZOR_WEBUSB

  void enumerate(t_transport_vect & res){
    BridgeTransport bt;
    try{
      bt.enumerate(res);
    } catch (const std::exception & e){
      MERROR("BridgeTransport enumeration failed:" << e.what());
    }

#ifdef WITH_DEVICE_TREZOR_WEBUSB
    hw::trezor::WebUsbTransport btw;
    try{
      btw.enumerate(res);
    } catch (const std::exception & e){
      MERROR("WebUsbTransport enumeration failed:" << e.what());
    }
#endif

#ifdef WITH_DEVICE_TREZOR_UDP
    hw::trezor::UdpTransport btu;
    try{
      btu.enumerate(res);
    } catch (const std::exception & e){
      MERROR("UdpTransport enumeration failed:" << e.what());
    }
#endif
  }

  void sort_transports_by_env(t_transport_vect & res){
    const char *env_trezor_path = getenv("TREZOR_PATH");
    if (!env_trezor_path){
      return;
    }
    
    // Sort transports by the longest matching prefix with TREZOR_PATH
    std::string trezor_path(env_trezor_path);
    std::vector<size_t> match_idx(res.size());
    std::vector<size_t> path_permutation(res.size());

    for(size_t i = 0; i < res.size(); ++i){
      auto cpath = res[i]->get_path();
      std::string * s1 = &trezor_path;
      std::string * s2 = &cpath;
      
      // first has to be shorter in std::mismatch(). Returns first non-matching iterators.
      if (s1->size() >= s2->size()){
        std::swap(s1, s2);
      }

      const auto mism = std::mismatch(s1->begin(), s1->end(), s2->begin());
      match_idx[i] = mism.first - s1->begin();
      path_permutation[i] = i;
    }

    std::sort(path_permutation.begin(), path_permutation.end(), [&](const size_t i0, const size_t i1) {
      return match_idx[i0] > match_idx[i1];
    });

    tools::apply_permutation(path_permutation, res);
  }

  std::shared_ptr<Transport> transport(const std::string & path){
    if (boost::starts_with(path, BridgeTransport::PATH_PREFIX)){
      return std::make_shared<BridgeTransport>(path.substr(strlen(BridgeTransport::PATH_PREFIX)));

    } else if (boost::starts_with(path, UdpTransport::PATH_PREFIX)){
      return std::make_shared<UdpTransport>(path.substr(strlen(UdpTransport::PATH_PREFIX)));

    } else {
      throw std::invalid_argument("Unknown Trezor device path: " + path);

    }
  }

  void throw_failure_exception(const messages::common::Failure * failure) {
    if (failure == nullptr){
      throw std::invalid_argument("Failure message cannot be null");
    }

    boost::optional<std::string> message = failure->has_message() ? boost::make_optional(failure->message()) : boost::none;
    boost::optional<uint32_t> code = failure->has_code() ? boost::make_optional(static_cast<uint32_t>(failure->code())) : boost::none;
    if (!code){
      throw exc::proto::FailureException(code, message);
    }

    auto ecode = failure->code();
    if (ecode == messages::common::Failure_FailureType_Failure_UnexpectedMessage){
      throw exc::proto::UnexpectedMessageException(code, message);
    } else if (ecode == messages::common::Failure_FailureType_Failure_ActionCancelled){
      throw exc::proto::CancelledException(code, message);
    } else if (ecode == messages::common::Failure_FailureType_Failure_PinExpected){
      throw exc::proto::PinExpectedException(code, message);
    } else if (ecode == messages::common::Failure_FailureType_Failure_PinInvalid){
      throw exc::proto::InvalidPinException(code, message);
    } else if (ecode == messages::common::Failure_FailureType_Failure_NotEnoughFunds){
      throw exc::proto::NotEnoughFundsException(code, message);
    } else if (ecode == messages::common::Failure_FailureType_Failure_NotInitialized){
      throw exc::proto::NotInitializedException(code, message);
    } else if (ecode == messages::common::Failure_FailureType_Failure_FirmwareError){
      throw exc::proto::FirmwareErrorException(code, message);
    } else {
      throw exc::proto::FailureException(code, message);
    }
  }

  GenericMessage::GenericMessage(messages::MessageType m_type, const shared_ptr<google::protobuf::Message> &m_msg)
        : m_type(m_type), m_msg(m_msg), m_empty(false) {}

  std::ostream& operator<<(std::ostream& o, hw::trezor::Transport const& t){
    return t.dump(o);
  }

  std::ostream& operator<<(std::ostream& o, std::shared_ptr<hw::trezor::Transport> const& t){
    if (!t){
      return o << "None";
    }

    return t->dump(o);
  }

}
}

