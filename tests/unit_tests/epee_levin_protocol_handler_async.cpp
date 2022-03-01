// Copyright (c) 2014-2022, The Monero Project
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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "string_tools.h"
#include "net/levin_protocol_handler_async.h"
#include "net/net_utils_base.h"
#include "unit_tests_utils.h"

namespace
{
  struct test_levin_connection_context : public epee::net_utils::connection_context_base
  {
    static constexpr int handshake_command() noexcept { return 1001; }
    static constexpr bool handshake_complete() noexcept { return true; }
    size_t get_max_bytes(int command) const { return LEVIN_DEFAULT_MAX_PACKET_SIZE; }
  };

  typedef epee::levin::async_protocol_handler_config<test_levin_connection_context> test_levin_protocol_handler_config;
  typedef epee::levin::async_protocol_handler<test_levin_connection_context> test_levin_protocol_handler;

  struct test_levin_commands_handler : public epee::levin::levin_commands_handler<test_levin_connection_context>
  {
    test_levin_commands_handler()
      : m_return_code(LEVIN_OK)
      , m_last_command(-1)
    {
    }

    virtual int invoke(int command, const epee::span<const uint8_t> in_buff, epee::byte_stream& buff_out, test_levin_connection_context& context)
    {
      m_invoke_counter.inc();
      boost::unique_lock<boost::mutex> lock(m_mutex);
      m_last_command = command;
      m_last_in_buf = std::string((const char*)in_buff.data(), in_buff.size());
      buff_out.write(epee::to_span(m_invoke_out_buf));
      return m_return_code;
    }

    virtual int notify(int command, const epee::span<const uint8_t> in_buff, test_levin_connection_context& context)
    {
      m_notify_counter.inc();
      boost::unique_lock<boost::mutex> lock(m_mutex);
      m_last_command = command;
      m_last_in_buf = std::string((const char*)in_buff.data(), in_buff.size());
      return m_return_code;
    }

    virtual void callback(test_levin_connection_context& context)
    {
      m_callback_counter.inc();
      //std::cout << "test_levin_commands_handler::callback()" << std::endl;
    }

    virtual void on_connection_new(test_levin_connection_context& context)
    {
      m_new_connection_counter.inc();
      //std::cout << "test_levin_commands_handler::on_connection_new()" << std::endl;
    }

    virtual void on_connection_close(test_levin_connection_context& context)
    {
      m_close_connection_counter.inc();
      //std::cout << "test_levin_commands_handler::on_connection_close()" << std::endl;
    }

    size_t invoke_counter() const { return m_invoke_counter.get(); }
    size_t notify_counter() const { return m_notify_counter.get(); }
    size_t callback_counter() const { return m_callback_counter.get(); }
    size_t new_connection_counter() const { return m_new_connection_counter.get(); }
    size_t close_connection_counter() const { return m_close_connection_counter.get(); }

    int return_code() const { return m_return_code; }
    void return_code(int v) { m_return_code = v; }

    void invoke_out_buf(std::string v) { m_invoke_out_buf = epee::byte_slice{std::move(v)}; }

    int last_command() const { return m_last_command; }
    const std::string& last_in_buf() const { return m_last_in_buf; }

  private:
    unit_test::call_counter m_invoke_counter;
    unit_test::call_counter m_notify_counter;
    unit_test::call_counter m_callback_counter;
    unit_test::call_counter m_new_connection_counter;
    unit_test::call_counter m_close_connection_counter;

    boost::mutex m_mutex;

    int m_return_code;
    epee::byte_slice m_invoke_out_buf;

    int m_last_command;
    std::string m_last_in_buf;
  };

  class test_connection : public epee::net_utils::i_service_endpoint
  {
  public:
    test_connection(boost::asio::io_service& io_service, test_levin_protocol_handler_config& protocol_config)
      : m_io_service(io_service)
      , m_protocol_handler(this, protocol_config, m_context)
      , m_send_return(true)
    {
    }

    void start()
    {
      ASSERT_TRUE(m_protocol_handler.after_init_connection());
    }

    // Implement epee::net_utils::i_service_endpoint interface
    virtual bool do_send(epee::byte_slice message)
    {
      //std::cout << "test_connection::do_send()" << std::endl;
      m_send_counter.inc();
      boost::unique_lock<boost::mutex> lock(m_mutex);
      m_last_send_data.append(reinterpret_cast<const char*>(message.data()), message.size());
      return m_send_return;
    }

    virtual bool close()                              { /*std::cout << "test_connection::close()" << std::endl; */return true; }
    virtual bool send_done()                          { /*std::cout << "test_connection::send_done()" << std::endl; */return true; }
    virtual bool call_run_once_service_io()           { std::cout << "test_connection::call_run_once_service_io()" << std::endl; return true; }
    virtual bool request_callback()                   { std::cout << "test_connection::request_callback()" << std::endl; return true; }
    virtual boost::asio::io_service& get_io_service() { std::cout << "test_connection::get_io_service()" << std::endl; return m_io_service; }
    virtual bool add_ref()                            { std::cout << "test_connection::add_ref()" << std::endl; return true; }
    virtual bool release()                            { std::cout << "test_connection::release()" << std::endl; return true; }

    size_t send_counter() const { return m_send_counter.get(); }

    const std::string& last_send_data() const { return m_last_send_data; }
    void reset_last_send_data() { boost::unique_lock<boost::mutex> lock(m_mutex); m_last_send_data.clear(); }

    bool send_return() const { return m_send_return; }
    void send_return(bool v) { m_send_return = v; }

  public:
    test_levin_protocol_handler m_protocol_handler;

  private:
    boost::asio::io_service& m_io_service;
    test_levin_connection_context m_context;

    unit_test::call_counter m_send_counter;
    boost::mutex m_mutex;

    std::string m_last_send_data;

    bool m_send_return;
  };

  class async_protocol_handler_test : public ::testing::Test
  {
  public:
    const static uint64_t invoke_timeout = 5 * 1000;
    const static size_t max_packet_size = 10 * 1024 * 1024;

    typedef std::unique_ptr<test_connection> test_connection_ptr;

    async_protocol_handler_test():
      m_pcommands_handler(new test_levin_commands_handler()),
      m_commands_handler(*m_pcommands_handler)
    {
      m_handler_config.set_handler(m_pcommands_handler, [](epee::levin::levin_commands_handler<test_levin_connection_context> *handler) { delete handler; });
      m_handler_config.m_invoke_timeout = invoke_timeout;
      m_handler_config.m_initial_max_packet_size = max_packet_size;
      m_handler_config.m_max_packet_size = max_packet_size;
    }

    virtual void SetUp()
    {
    }

  protected:
    test_connection_ptr create_connection(bool start = true)
    {
      test_connection_ptr conn(new test_connection(m_io_service, m_handler_config));
      if (start)
      {
        conn->start();
      }
      return conn;
    }

  protected:
    boost::asio::io_service m_io_service;
    test_levin_protocol_handler_config m_handler_config;
    test_levin_commands_handler *m_pcommands_handler, &m_commands_handler;
  };

  class positive_test_connection_to_levin_protocol_handler_calls : public async_protocol_handler_test
  {
  };

  class test_levin_protocol_handler__hanle_recv_with_invalid_data : public async_protocol_handler_test
  {
  public:
    static const int expected_command = 5615871;
    static const int expected_return_code = 782546;

    test_levin_protocol_handler__hanle_recv_with_invalid_data()
      : m_expected_invoke_out_buf(512, 'y')
    {
    }

    virtual void SetUp()
    {
      async_protocol_handler_test::SetUp();

      m_conn = create_connection();

      m_in_data.assign(256, 't');

      m_req_head.m_signature = SWAP64LE(LEVIN_SIGNATURE);
      m_req_head.m_cb = SWAP64LE(m_in_data.size());
      m_req_head.m_have_to_return_data = true;
      m_req_head.m_command = SWAP32LE(expected_command);
      m_req_head.m_return_code = SWAP32LE(LEVIN_OK);
      m_req_head.m_flags = SWAP32LE(LEVIN_PACKET_REQUEST);
      m_req_head.m_protocol_version = SWAP32LE(LEVIN_PROTOCOL_VER_1);

      m_commands_handler.return_code(expected_return_code);
      m_commands_handler.invoke_out_buf(m_expected_invoke_out_buf);
    }

  protected:
    void prepare_buf()
    {
      m_buf.assign(reinterpret_cast<const char*>(&m_req_head), sizeof(m_req_head));
      m_buf += m_in_data;
    }

  protected:
    test_connection_ptr m_conn;
    epee::levin::bucket_head2 m_req_head;
    std::string m_in_data;
    std::string m_buf;
    std::string m_expected_invoke_out_buf;
  };
}

TEST_F(positive_test_connection_to_levin_protocol_handler_calls, new_handler_is_not_initialized)
{
  test_connection_ptr conn = create_connection(false);
  ASSERT_FALSE(conn->m_protocol_handler.m_connection_initialized);
  ASSERT_EQ(0, m_handler_config.get_connections_count());
  ASSERT_EQ(0, m_commands_handler.new_connection_counter());
  conn.reset();
  ASSERT_EQ(0, m_handler_config.get_connections_count());
  ASSERT_EQ(0, m_commands_handler.close_connection_counter());
}

TEST_F(positive_test_connection_to_levin_protocol_handler_calls, handler_initialization_and_destruction_is_correct)
{
  test_connection_ptr conn = create_connection();
  ASSERT_TRUE(conn->m_protocol_handler.m_connection_initialized);
  ASSERT_EQ(1, m_handler_config.get_connections_count());
  ASSERT_EQ(1, m_commands_handler.new_connection_counter());
  conn.reset();
  ASSERT_EQ(0, m_handler_config.get_connections_count());
  ASSERT_EQ(1, m_commands_handler.close_connection_counter());
}

TEST_F(positive_test_connection_to_levin_protocol_handler_calls, concurent_handler_initialization_and_destruction_is_correct)
{
  const size_t connection_count = 10000;
  auto create_and_destroy_connections = [this]()
  {
    std::vector<test_connection_ptr> connections(connection_count);
    for (size_t i = 0; i < connection_count; ++i)
    {
      connections[i] = create_connection();
    }

    for (size_t i = 0; i < connection_count; ++i)
    {
      connections[i].reset();
    }
  };

  const size_t thread_count = boost::thread::hardware_concurrency();
  std::vector<boost::thread> threads(thread_count);
  for (boost::thread& th : threads)
  {
    th = boost::thread(create_and_destroy_connections);
  }

  for (boost::thread& th : threads)
  {
    th.join();
  }

  ASSERT_EQ(0, m_handler_config.get_connections_count());
  ASSERT_EQ(connection_count * thread_count, m_commands_handler.new_connection_counter());
  ASSERT_EQ(connection_count * thread_count, m_commands_handler.close_connection_counter());
}

TEST_F(positive_test_connection_to_levin_protocol_handler_calls, handler_processes_handle_read_as_invoke)
{
  // Setup
  const int expected_command = 2634981;
  const int expected_return_code = 6732;
  const std::string expected_out_data(128, 'w');

  test_connection_ptr conn = create_connection();

  std::string in_data(256, 'q');

  epee::levin::bucket_head2 req_head;
  req_head.m_signature = SWAP64LE(LEVIN_SIGNATURE);
  req_head.m_cb = SWAP64LE(in_data.size());
  req_head.m_have_to_return_data = true;
  req_head.m_command = SWAP32LE(expected_command);
  req_head.m_flags = SWAP32LE(LEVIN_PACKET_REQUEST);
  req_head.m_protocol_version = SWAP32LE(LEVIN_PROTOCOL_VER_1);

  std::string buf(reinterpret_cast<const char*>(&req_head), sizeof(req_head));
  buf += in_data;

  m_commands_handler.invoke_out_buf(expected_out_data);
  m_commands_handler.return_code(expected_return_code);

  // Test
  ASSERT_TRUE(conn->m_protocol_handler.handle_recv(buf.data(), buf.size()));

  //
  // Check
  //

  // Check connection and levin_commands_handler states
  ASSERT_EQ(1, m_commands_handler.invoke_counter());
  ASSERT_EQ(0, m_commands_handler.notify_counter());
  ASSERT_EQ(expected_command, m_commands_handler.last_command());
  ASSERT_EQ(in_data, m_commands_handler.last_in_buf());
  ASSERT_LE(1, conn->send_counter());

  // Parse send data
  std::string send_data = conn->last_send_data();
  epee::levin::bucket_head2 resp_head;
  ASSERT_LT(sizeof(resp_head), send_data.size());
  std::memcpy(std::addressof(resp_head), send_data.data(), sizeof(resp_head));
  std::string out_data = send_data.substr(sizeof(resp_head));

  // Check sent response
  ASSERT_EQ(expected_out_data, out_data);
  ASSERT_EQ(LEVIN_SIGNATURE, SWAP64LE(resp_head.m_signature));
  ASSERT_EQ(expected_command, SWAP32LE(resp_head.m_command));
  ASSERT_EQ(expected_return_code, SWAP32LE(resp_head.m_return_code));
  ASSERT_EQ(expected_out_data.size(), SWAP64LE(resp_head.m_cb));
  ASSERT_FALSE(resp_head.m_have_to_return_data);
  ASSERT_EQ(SWAP32LE(LEVIN_PROTOCOL_VER_1), resp_head.m_protocol_version);
  ASSERT_TRUE(0 != (SWAP32LE(resp_head.m_flags) & LEVIN_PACKET_RESPONSE));
}

TEST_F(positive_test_connection_to_levin_protocol_handler_calls, handler_processes_handle_read_as_notify)
{
  // Setup
  const int expected_command = 4673261;

  test_connection_ptr conn = create_connection();

  std::string in_data(256, 'e');

  epee::levin::bucket_head2 req_head;
  req_head.m_signature = SWAP64LE(LEVIN_SIGNATURE);
  req_head.m_cb = SWAP64LE(in_data.size());
  req_head.m_have_to_return_data = false;
  req_head.m_command = SWAP32LE(expected_command);
  req_head.m_flags = SWAP32LE(LEVIN_PACKET_REQUEST);
  req_head.m_protocol_version = SWAP32LE(LEVIN_PROTOCOL_VER_1);

  std::string buf(reinterpret_cast<const char*>(&req_head), sizeof(req_head));
  buf += in_data;

  // Test
  ASSERT_TRUE(conn->m_protocol_handler.handle_recv(buf.data(), buf.size()));

  // Check connection and levin_commands_handler states
  ASSERT_EQ(1, m_commands_handler.notify_counter());
  ASSERT_EQ(0, m_commands_handler.invoke_counter());
  ASSERT_EQ(expected_command, m_commands_handler.last_command());
  ASSERT_EQ(in_data, m_commands_handler.last_in_buf());
  ASSERT_LE(0, conn->send_counter());
  ASSERT_TRUE(conn->last_send_data().empty());
}

TEST_F(positive_test_connection_to_levin_protocol_handler_calls, handler_processes_qued_callback)
{
  test_connection_ptr conn = create_connection();

  conn->m_protocol_handler.handle_qued_callback();
  conn->m_protocol_handler.handle_qued_callback();
  conn->m_protocol_handler.handle_qued_callback();

  ASSERT_EQ(3, m_commands_handler.callback_counter());
}

TEST_F(positive_test_connection_to_levin_protocol_handler_calls, handler_processes_handle_read_as_dummy)
{
  // Setup
  const int expected_command = 4673261;
  const std::string in_data(256, 'e');

  epee::levin::message_writer message{};
  message.buffer.write(epee::to_span(in_data));

  const epee::byte_slice noise = epee::levin::make_noise_notify(1024);
  const epee::byte_slice notify = message.finalize_notify(expected_command);

  test_connection_ptr conn = create_connection();

  // Test
  ASSERT_TRUE(conn->m_protocol_handler.handle_recv(noise.data(), noise.size()));

  // Check connection and levin_commands_handler states
  ASSERT_EQ(0u, m_commands_handler.notify_counter());
  ASSERT_EQ(0u, m_commands_handler.invoke_counter());
  ASSERT_EQ(-1, m_commands_handler.last_command());
  ASSERT_TRUE(m_commands_handler.last_in_buf().empty());
  ASSERT_EQ(0u, conn->send_counter());
  ASSERT_TRUE(conn->last_send_data().empty());


  ASSERT_TRUE(conn->m_protocol_handler.handle_recv(notify.data(), notify.size()));

  // Check connection and levin_commands_handler states
  ASSERT_EQ(1u, m_commands_handler.notify_counter());
  ASSERT_EQ(0u, m_commands_handler.invoke_counter());
  ASSERT_EQ(expected_command, m_commands_handler.last_command());
  ASSERT_EQ(in_data, m_commands_handler.last_in_buf());
  ASSERT_EQ(0u, conn->send_counter());
  ASSERT_TRUE(conn->last_send_data().empty());
}

TEST_F(positive_test_connection_to_levin_protocol_handler_calls, handler_processes_handle_read_as_fragment)
{
  // Setup
  const int expected_command = 4673261;
  const int expected_fragmented_command = 46732;
  const std::string in_data(256, 'e');

  epee::levin::message_writer message{};
  message.buffer.write(epee::to_span(in_data));

  epee::levin::message_writer in_fragmented_data;
  in_fragmented_data.buffer.put_n('c', 1024 * 4);

  const epee::byte_slice noise = epee::levin::make_noise_notify(1024);
  const epee::byte_slice notify = message.finalize_notify(expected_command);
  epee::byte_slice fragmented = epee::levin::make_fragmented_notify(noise.size(), expected_fragmented_command, std::move(in_fragmented_data));

  EXPECT_EQ(5u, fragmented.size() / 1024);
  EXPECT_EQ(0u, fragmented.size() % 1024);

  test_connection_ptr conn = create_connection();

  while (!fragmented.empty())
  {
    if ((fragmented.size() / 1024) % 2 == 1)
    {
      ASSERT_TRUE(conn->m_protocol_handler.handle_recv(notify.data(), notify.size()));
    }

    ASSERT_EQ(3u - (fragmented.size() / 2048), m_commands_handler.notify_counter());
    ASSERT_EQ(0u, m_commands_handler.invoke_counter());
    ASSERT_EQ(expected_command, m_commands_handler.last_command());
    ASSERT_EQ(in_data, m_commands_handler.last_in_buf());
    ASSERT_EQ(0u, conn->send_counter());
    ASSERT_TRUE(conn->last_send_data().empty());

    epee::byte_slice next = fragmented.take_slice(1024);
    ASSERT_TRUE(conn->m_protocol_handler.handle_recv(next.data(), next.size()));
  }

  std::string compare_buffer(1024 * 4, 'c');
  compare_buffer.resize(((1024 - sizeof(epee::levin::bucket_head2)) * 5) - sizeof(epee::levin::bucket_head2)); // add padding zeroes

  ASSERT_EQ(4u, m_commands_handler.notify_counter());
  ASSERT_EQ(0u, m_commands_handler.invoke_counter());
  ASSERT_EQ(expected_fragmented_command, m_commands_handler.last_command());
  ASSERT_EQ(compare_buffer, m_commands_handler.last_in_buf());
  ASSERT_EQ(0u, conn->send_counter());
  ASSERT_TRUE(conn->last_send_data().empty());


  ASSERT_TRUE(conn->m_protocol_handler.handle_recv(notify.data(), notify.size()));

  ASSERT_EQ(5u, m_commands_handler.notify_counter());
  ASSERT_EQ(0u, m_commands_handler.invoke_counter());
  ASSERT_EQ(expected_command, m_commands_handler.last_command());
  ASSERT_EQ(in_data, m_commands_handler.last_in_buf());
  ASSERT_EQ(0u, conn->send_counter());
  ASSERT_TRUE(conn->last_send_data().empty());
}


TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_big_packet_1)
{
  std::string buf("yyyyyy");
  ASSERT_FALSE(m_conn->m_protocol_handler.handle_recv(buf.data(), max_packet_size + 1));
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_big_packet_2)
{
  prepare_buf();
  const size_t first_packet_size = sizeof(m_req_head) - 1;

  m_buf.resize(first_packet_size);
  ASSERT_TRUE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));

  ASSERT_FALSE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), max_packet_size - m_buf.size() + 1));
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_invalid_signature_for_full_header)
{
  m_req_head.m_signature = LEVIN_SIGNATURE ^ 1;
  prepare_buf();

  ASSERT_FALSE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_invalid_signature_for_partial_header)
{
  m_req_head.m_signature = LEVIN_SIGNATURE ^ 1;
  prepare_buf();
  m_buf.resize(sizeof(m_req_head.m_signature));

  ASSERT_FALSE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_big_cb)
{
  m_req_head.m_cb = max_packet_size + 1;
  prepare_buf();

  ASSERT_FALSE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, does_not_handle_data_after_close)
{
  prepare_buf();

  ASSERT_TRUE(m_conn->m_protocol_handler.close());
  ASSERT_FALSE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_network_error)
{
  prepare_buf();

  m_conn->send_return(false);
  ASSERT_FALSE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_chunked_header)
{
  prepare_buf();

  size_t buf1_size = sizeof(m_req_head) / 2;

  std::string buf1 = m_buf.substr(0, buf1_size);
  std::string buf2 = m_buf.substr(buf1_size);
  ASSERT_EQ(m_buf, buf1 + buf2);

  ASSERT_TRUE(m_conn->m_protocol_handler.handle_recv(buf1.data(), buf1.size()));
  ASSERT_EQ(0, m_commands_handler.invoke_counter());

  ASSERT_TRUE(m_conn->m_protocol_handler.handle_recv(buf2.data(), buf2.size()));
  ASSERT_EQ(1, m_commands_handler.invoke_counter());
} 


TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_chunked_body)
{
  prepare_buf();

  size_t buf1_size = sizeof(m_req_head) + (m_buf.size() - sizeof(m_req_head)) / 2;

  std::string buf1 = m_buf.substr(0, buf1_size);
  std::string buf2 = m_buf.substr(buf1_size);
  ASSERT_EQ(m_buf, buf1 + buf2);

  ASSERT_TRUE(m_conn->m_protocol_handler.handle_recv(buf1.data(), buf1.size()));
  ASSERT_EQ(0, m_commands_handler.invoke_counter());

  ASSERT_TRUE(m_conn->m_protocol_handler.handle_recv(buf2.data(), buf2.size()));
  ASSERT_EQ(1, m_commands_handler.invoke_counter());
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_two_requests_at_once)
{
  prepare_buf();
  m_buf.append(m_buf);

  ASSERT_TRUE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));
  ASSERT_EQ(2, m_commands_handler.invoke_counter());
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_unexpected_response)
{
  m_req_head.m_flags = SWAP32LE(LEVIN_PACKET_RESPONSE);
  prepare_buf();

  ASSERT_FALSE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));
}

TEST_F(test_levin_protocol_handler__hanle_recv_with_invalid_data, handles_short_fragment)
{
  m_req_head.m_cb = 1;
  m_req_head.m_flags = LEVIN_PACKET_BEGIN;
  m_req_head.m_command = 0;
  m_in_data.resize(1);
  prepare_buf();

  ASSERT_TRUE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));

  m_req_head.m_flags = LEVIN_PACKET_END;
  prepare_buf();

  ASSERT_FALSE(m_conn->m_protocol_handler.handle_recv(m_buf.data(), m_buf.size()));
}
