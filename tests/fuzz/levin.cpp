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

#include "include_base_utils.h"
#include "file_io_utils.h"
#include "net/net_utils_base.h"
#include "net/abstract_tcp_server2.h"
#include "storages/levin_abstract_invoke2.h"
#include "net/levin_protocol_handler_async.h"
#include "fuzzer.h"

namespace
{
  class call_counter
  {
  public:
    call_counter() : m_counter(0) { }

    // memory_order_relaxed is enough for call counter
    void inc() volatile { m_counter.fetch_add(1, std::memory_order_relaxed); }
    size_t get() volatile const { return m_counter.load(std::memory_order_relaxed); }
    void reset() volatile { m_counter.store(0, std::memory_order_relaxed); }

  private:
    std::atomic<size_t> m_counter;
  };

  struct test_levin_connection_context : public epee::net_utils::connection_context_base
  {
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

    virtual int invoke(int command, const std::string& in_buff, std::string& buff_out, test_levin_connection_context& context)
    {
      m_invoke_counter.inc();
      boost::unique_lock<boost::mutex> lock(m_mutex);
      m_last_command = command;
      m_last_in_buf = in_buff;
      buff_out = m_invoke_out_buf;
      return m_return_code;
    }

    virtual int notify(int command, const std::string& in_buff, test_levin_connection_context& context)
    {
      m_notify_counter.inc();
      boost::unique_lock<boost::mutex> lock(m_mutex);
      m_last_command = command;
      m_last_in_buf = in_buff;
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

    const std::string& invoke_out_buf() const { return m_invoke_out_buf; }
    void invoke_out_buf(const std::string& v) { m_invoke_out_buf = v; }

    int last_command() const { return m_last_command; }
    const std::string& last_in_buf() const { return m_last_in_buf; }

  private:
    call_counter m_invoke_counter;
    call_counter m_notify_counter;
    call_counter m_callback_counter;
    call_counter m_new_connection_counter;
    call_counter m_close_connection_counter;

    boost::mutex m_mutex;

    int m_return_code;
    std::string m_invoke_out_buf;

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
      m_protocol_handler.after_init_connection();
    }

    // Implement epee::net_utils::i_service_endpoint interface
    virtual bool do_send(const void* ptr, size_t cb)
    {
      m_send_counter.inc();
      boost::unique_lock<boost::mutex> lock(m_mutex);
      m_last_send_data.append(reinterpret_cast<const char*>(ptr), cb);
      return m_send_return;
    }

    virtual bool close()                              { return true; }
    virtual bool call_run_once_service_io()           { return true; }
    virtual bool request_callback()                   { return true; }
    virtual boost::asio::io_service& get_io_service() { return m_io_service; }
    virtual bool add_ref()                            { return true; }
    virtual bool release()                            { return true; }

    size_t send_counter() const { return m_send_counter.get(); }

    const std::string& last_send_data() const { return m_last_send_data; }
    void reset_last_send_data() { boost::unique_lock<boost::mutex> lock(m_mutex); m_last_send_data.clear(); }

    bool send_return() const { return m_send_return; }
    void send_return(bool v) { m_send_return = v; }

  public:
    test_levin_connection_context m_context;
    test_levin_protocol_handler m_protocol_handler;

  private:
    boost::asio::io_service& m_io_service;

    call_counter m_send_counter;
    boost::mutex m_mutex;

    std::string m_last_send_data;

    bool m_send_return;
  };

#if 0
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

      m_req_head.m_signature = LEVIN_SIGNATURE;
      m_req_head.m_cb = m_in_data.size();
      m_req_head.m_have_to_return_data = true;
      m_req_head.m_command = expected_command;
      m_req_head.m_return_code = LEVIN_OK;
      m_req_head.m_flags = LEVIN_PACKET_REQUEST;
      m_req_head.m_protocol_version = LEVIN_PROTOCOL_VER_1;

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
#endif
}

class LevinFuzzer: public Fuzzer
{
public:
  LevinFuzzer() {} //: handler(endpoint, config, context) {}
  virtual int init();
  virtual int run(const std::string &filename);

private:
  //epee::net_utils::connection_context_base context;
  //epee::levin::async_protocol_handler<> handler;
};

int LevinFuzzer::init()
{
  return 0;
}

int LevinFuzzer::run(const std::string &filename)
{
  std::string s;

#if 0
  epee::levin::bucket_head2 req_head;
  req_head.m_signature = LEVIN_SIGNATURE;
  req_head.m_cb = 0;
  req_head.m_have_to_return_data = true;
  req_head.m_command = 2000;
  req_head.m_flags = LEVIN_PACKET_REQUEST;
  req_head.m_protocol_version = LEVIN_PROTOCOL_VER_1;
  req_head.m_return_code = 0;
  FILE *f=fopen("/tmp/out.levin", "w");
  fwrite(&req_head,sizeof(req_head),1, f);
  fclose(f);
#endif
  if (!epee::file_io_utils::load_file_to_string(filename, s))
  {
    std::cout << "Error: failed to load file " << filename << std::endl;
    return 1;
  }
  try
  {
    //std::unique_ptr<test_connection> conn = new test();
    boost::asio::io_service io_service;
    test_levin_protocol_handler_config m_handler_config;
    test_levin_commands_handler *m_pcommands_handler = new test_levin_commands_handler();
    m_handler_config.set_handler(m_pcommands_handler, [](epee::levin::levin_commands_handler<test_levin_connection_context> *handler) { delete handler; });
    std::unique_ptr<test_connection> conn(new test_connection(io_service, m_handler_config));
    conn->start();
    //m_commands_handler.invoke_out_buf(expected_out_data);
    //m_commands_handler.return_code(expected_return_code);
    conn->m_protocol_handler.handle_recv(s.data(), s.size());
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to test http client: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, const char **argv)
{
  LevinFuzzer fuzzer;
  return run_fuzzer(argc, argv, fuzzer);
}

