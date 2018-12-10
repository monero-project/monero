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

#ifdef WITH_DEVICE_TREZOR_WEBUSB
#include <libusb.h>
#endif

#include <boost/endian/conversion.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/format.hpp>
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

  bool t_deserialize(const std::string & in, json & out){
    if (out.Parse(in.c_str()).HasParseError()) {
      throw exc::CommunicationException("JSON parse error");
    }
    return true;
  }

  static std::string json_get_string(const rapidjson::Value & in){
    return std::string(in.GetString());
  }

  //
  // Helpers
  //

#define PROTO_HEADER_SIZE 6

  static size_t message_size(const google::protobuf::Message &req){
    return static_cast<size_t>(req.ByteSize());
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
    if (!req.SerializeToArray(buff + 6, msg_size)){
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

    std::unique_ptr<uint8_t[]> req_buff(new uint8_t[buff_size]);
    uint8_t * req_buff_raw = req_buff.get();
    req_buff_raw[0] = '#';
    req_buff_raw[1] = '#';

    serialize_message(req, msg_size, req_buff_raw + 2, buff_size - 2);

    size_t offset = 0;
    uint8_t chunk_buff[REPLEN];

    // Chunk by chunk upload
    while(offset < buff_size){
      auto to_copy = std::min((size_t)(buff_size - offset), (size_t)(REPLEN - 1));

      chunk_buff[0] = '?';
      memcpy(chunk_buff + 1, req_buff_raw + offset, to_copy);

      // Pad with zeros
      if (to_copy < REPLEN - 1){
        memset(chunk_buff + 1 + to_copy, 0, REPLEN - 1 - to_copy);
      }

      transport.write_chunk(chunk_buff, REPLEN);
      offset += REPLEN - 1;
    }
  }

  void ProtocolV1::read(Transport & transport, std::shared_ptr<google::protobuf::Message> & msg, messages::MessageType * msg_type){
    char chunk[REPLEN];

    // Initial chunk read
    size_t nread = transport.read_chunk(chunk, REPLEN);
    if (nread != REPLEN){
      throw exc::CommunicationException("Read chunk has invalid size");
    }

    if (strncmp(chunk, "?##", 3) != 0){
      throw exc::CommunicationException("Malformed chunk");
    }

    uint16_t tag;
    uint32_t len;
    nread -= 3 + 6;
    deserialize_message_header(chunk + 3, tag, len);

    std::string data_acc(chunk + 3 + 6, nread);
    data_acc.reserve(len);

    while(nread < len){
      const size_t cur = transport.read_chunk(chunk, REPLEN);
      if (chunk[0] != '?'){
        throw exc::CommunicationException("Chunk malformed");
      }

      data_acc.append(chunk + 1, cur - 1);
      nread += cur - 1;
    }

    if (msg_type){
      *msg_type = static_cast<messages::MessageType>(tag);
    }

    if (nread < len){
      throw exc::CommunicationException("Response incomplete");
    }

    std::shared_ptr<google::protobuf::Message> msg_wrap(MessageMapper::get_message(tag));
    if (!msg_wrap->ParseFromArray(data_acc.c_str(), len)){
      throw exc::CommunicationException("Message could not be parsed");
    }

    msg = msg_wrap;
  }

  //
  // Bridge transport
  //

  const char * BridgeTransport::PATH_PREFIX = "bridge:";

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
      t->m_device_info.emplace();
      t->m_device_info->CopyFrom(*itr, t->m_device_info->GetAllocator());
      res.push_back(t);
    }
  }

  void BridgeTransport::open() {
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
  }

  void BridgeTransport::close() {
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

    std::unique_ptr<uint8_t[]> req_buff(new uint8_t[buff_size]);
    uint8_t * req_buff_raw = req_buff.get();

    serialize_message(req, msg_size, req_buff_raw, buff_size);

    std::string uri = "/call/" + m_session.get();
    std::string req_hex = epee::to_hex::string(epee::span<const std::uint8_t>(req_buff_raw, buff_size));
    std::string res_hex;

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

    std::string bin_data;
    if (!epee::string_tools::parse_hexstr_to_binbuff(m_response.get(), bin_data)){
      throw exc::CommunicationException("Response is not well hexcoded");
    }

    uint16_t msg_tag;
    uint32_t msg_len;
    deserialize_message_header(bin_data.c_str(), msg_tag, msg_len);
    if (bin_data.size() != msg_len + 6){
      throw exc::CommunicationException("Response is not well hexcoded");
    }

    if (msg_type){
      *msg_type = static_cast<messages::MessageType>(msg_tag);
    }

    std::shared_ptr<google::protobuf::Message> msg_wrap(MessageMapper::get_message(msg_tag));
    if (!msg_wrap->ParseFromArray(bin_data.c_str() + 6, msg_len)){
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

  UdpTransport::UdpTransport(boost::optional<std::string> device_path,
                             boost::optional<std::shared_ptr<Protocol>> proto) :
      m_io_service(), m_deadline(m_io_service)
  {
    m_device_port = DEFAULT_PORT;
    if (device_path) {
      const std::string device_str = device_path.get();
      auto delim = device_str.find(':');
      if (delim == std::string::npos) {
        m_device_host = device_str;
      } else {
        m_device_host = device_str.substr(0, delim);
        m_device_port = std::stoi(device_str.substr(delim + 1));
      }
    } else {
      m_device_host = DEFAULT_HOST;
    }

    if (m_device_port <= 1024 || m_device_port > 65535){
      throw std::invalid_argument("Port number invalid");
    }

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

      m_socket->send_to(boost::asio::buffer(req.c_str(), req.size()), m_endpoint);
      receive(res, 8, nullptr, false, timeout);

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
    udp::resolver resolver(m_io_service);
    udp::resolver::query query(udp::v4(), m_device_host, std::to_string(m_device_port));
    m_endpoint = *resolver.resolve(query);

    m_socket.reset(new udp::socket(m_io_service));
    m_socket->open(udp::v4());

    m_deadline.expires_at(boost::posix_time::pos_infin);
    check_deadline();

    m_proto->session_begin(*this);
  }

  void UdpTransport::close() {
    if (!m_socket){
      throw exc::CommunicationException("Socket is already closed");
    }

    m_proto->session_end(*this);
    m_socket->close();
    m_socket = nullptr;
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
                                 boost::bind(&UdpTransport::handle_receive, _1, _2, &ec, &length));

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
    return info->idVendor == 0x534C && info->idProduct == 0x0001;
  }

  static bool is_trezor2(libusb_device_descriptor * info){
    return info->idVendor == 0x1209 && info->idProduct == 0x53C1;
  }

  static bool is_trezor2_bl(libusb_device_descriptor * info){
    return info->idVendor == 0x1209 && info->idProduct == 0x53C0;
  }

  static uint8_t get_trezor_dev_mask(libusb_device_descriptor * info){
    uint8_t mask = 0;
    CHECK_AND_ASSERT_THROW_MES(info, "Empty device descriptor");
    mask |= is_trezor1(info) ? 1 : 0;
    mask |= is_trezor2(info) ? 2 : 0;
    mask |= is_trezor2_bl(info) ? 4 : 0;
    return mask;
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
  ): m_conn_count(0),
     m_usb_session(nullptr), m_usb_device(nullptr), m_usb_device_handle(nullptr),
     m_bus_id(-1), m_device_addr(-1)
  {
    if (descriptor){
      libusb_device_descriptor * desc = new libusb_device_descriptor;
      memcpy(desc, descriptor.get(), sizeof(libusb_device_descriptor));
      this->m_usb_device_desc.reset(desc);
    }

    m_proto = proto ? proto.get() : std::make_shared<ProtocolV1>();

#ifdef WITH_TREZOR_DEBUG
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

      const auto trezor_mask = get_trezor_dev_mask(&desc);
      if (!trezor_mask){
        continue;
      }

      MTRACE("Found Trezor device: " << desc.idVendor << ":" << desc.idProduct << " mask " << (int)trezor_mask);

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
    const int interface = get_interface();
    if (m_conn_count > 0){
      MTRACE("Already opened, count: " << m_conn_count);
      m_conn_count += 1;
      return;
    }

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

      const auto trezor_mask = get_trezor_dev_mask(&desc);
      if (!trezor_mask) {
        continue;
      }

      auto bus_id = libusb_get_bus_number(devs[i]);
      std::vector<uint8_t> path;

      // Port resolution may fail. Non-critical error, just addressing precision is decreased.
      get_libusb_ports(devs[i], path);

      MTRACE("Found Trezor device: " << desc.idVendor << ":" << desc.idProduct
                                     << ", mask: " << (int)trezor_mask
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

    m_conn_count += 1;
    m_proto->session_begin(*this);
    
#undef TREZOR_DESTROY_SESSION
  };

  void WebUsbTransport::close() {
    m_conn_count -= 1;

    if (m_conn_count < 0){
      MERROR("Close counter is negative: " << m_conn_count);

    } else if (m_conn_count == 0){
      MTRACE("Closing webusb device");

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
    }
  };


  int WebUsbTransport::get_interface() const{
    const int INTERFACE_NORMAL = 0;
#ifdef WITH_TREZOR_DEBUG
    const int INTERFACE_DEBUG = 1;
    return m_debug_mode ? INTERFACE_DEBUG : INTERFACE_NORMAL;
#else
    return INTERFACE_NORMAL;
#endif
  }

  unsigned char WebUsbTransport::get_endpoint() const{
    const unsigned char ENDPOINT_NORMAL = 1;
#ifdef WITH_TREZOR_DEBUG
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


