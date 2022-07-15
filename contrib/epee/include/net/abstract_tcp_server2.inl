/**
@file
@author from CrypoNote (see copyright below; Andrey N. Sabelnikov)
@monero rfree
@brief the connection templated-class for one peer connection
*/
// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 



#include <boost/foreach.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/chrono.hpp>
#include <boost/utility/value_init.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp> // TODO
#include <boost/thread/condition_variable.hpp> // TODO
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include "warnings.h"
#include "string_tools_lexical.h"
#include "misc_language.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <random>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net"

#define AGGRESSIVE_TIMEOUT_THRESHOLD 120 // sockets
#define NEW_CONNECTION_TIMEOUT_LOCAL 1200000 // 2 minutes
#define NEW_CONNECTION_TIMEOUT_REMOTE 10000 // 10 seconds
#define DEFAULT_TIMEOUT_MS_LOCAL 1800000 // 30 minutes
#define DEFAULT_TIMEOUT_MS_REMOTE 300000 // 5 minutes
#define TIMEOUT_EXTRA_MS_PER_BYTE 0.2


namespace epee
{
namespace net_utils
{
  template<typename T>
  T& check_and_get(std::shared_ptr<T>& ptr)
  {
    CHECK_AND_ASSERT_THROW_MES(bool(ptr), "shared_state cannot be null");
    return *ptr;
  }

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<typename T>
  unsigned int connection<T>::host_count(int delta)
  {
    static std::mutex hosts_mutex;
    std::lock_guard<std::mutex> guard(hosts_mutex);
    static std::map<std::string, unsigned int> hosts;
    unsigned int &val = hosts[m_host];
    if (delta > 0)
      MTRACE("New connection from host " << m_host << ": " << val);
    else if (delta < 0)
      MTRACE("Closed connection from host " << m_host << ": " << val);
    CHECK_AND_ASSERT_THROW_MES(delta >= 0 || val >= (unsigned)-delta, "Count would go negative");
    CHECK_AND_ASSERT_THROW_MES(delta <= 0 || val <= std::numeric_limits<unsigned int>::max() - (unsigned)delta, "Count would wrap");
    val += delta;
    return val;
  }

  template<typename T>
  typename connection<T>::duration_t connection<T>::get_default_timeout()
  {
    unsigned count{};
    try { count = host_count(); } catch (...) {}
    const unsigned shift = (
      connection_basic::get_state().sock_count > AGGRESSIVE_TIMEOUT_THRESHOLD ?
      std::min(std::max(count, 1u) - 1, 8u) :
      0
    );
    return (
      m_local ?
      std::chrono::milliseconds(DEFAULT_TIMEOUT_MS_LOCAL >> shift) :
      std::chrono::milliseconds(DEFAULT_TIMEOUT_MS_REMOTE >> shift)
    );
  }

  template<typename T>
  typename connection<T>::duration_t connection<T>::get_timeout_from_bytes_read(size_t bytes) const
  {
    return std::chrono::duration_cast<connection<T>::duration_t>(
      std::chrono::duration<double, std::chrono::milliseconds::period>(
        bytes * TIMEOUT_EXTRA_MS_PER_BYTE
      )
    );
  }

  template<typename T>
  void connection<T>::state_status_check()
  {
    switch (m_state.status)
    {
      case status_t::RUNNING:
        interrupt();
        break;
      case status_t::INTERRUPTED:
        on_interrupted();
        break;
      case status_t::TERMINATING:
        on_terminating();
        break;
      default:
        break;
    }
  }

  template<typename T>
  void connection<T>::start_timer(duration_t duration, bool add)
  {
    if (m_state.timers.general.wait_expire) {
      m_state.timers.general.cancel_expire = true;
      m_state.timers.general.reset_expire = true;
      ec_t ec;
      m_timers.general.expires_from_now(
        std::min(
          duration + (add ? m_timers.general.expires_from_now() : duration_t{}),
          get_default_timeout()
        ),
        ec
      );
    }
    else {
      ec_t ec;
      m_timers.general.expires_from_now(
        std::min(
          duration + (add ? m_timers.general.expires_from_now() : duration_t{}),
          get_default_timeout()
        ),
        ec
      );
      async_wait_timer();
    }
  }

  template<typename T>
  void connection<T>::async_wait_timer()
  {
    if (m_state.timers.general.wait_expire)
      return;
    m_state.timers.general.wait_expire = true;
    auto self = connection<T>::shared_from_this();
    m_timers.general.async_wait([this, self](const ec_t & ec){
      std::lock_guard<std::mutex> guard(m_state.lock);
      m_state.timers.general.wait_expire = false;
      if (m_state.timers.general.cancel_expire) {
        m_state.timers.general.cancel_expire = false;
        if (m_state.timers.general.reset_expire) {
          m_state.timers.general.reset_expire = false;
          async_wait_timer();
        }
        else if (m_state.status == status_t::INTERRUPTED)
          on_interrupted();
        else if (m_state.status == status_t::TERMINATING)
          on_terminating();
      }
      else if (m_state.status == status_t::RUNNING)
        interrupt();
      else if (m_state.status == status_t::INTERRUPTED)
        terminate();
    });
  }

  template<typename T>
  void connection<T>::cancel_timer()
  {
    if (!m_state.timers.general.wait_expire)
      return;
    m_state.timers.general.cancel_expire = true;
    m_state.timers.general.reset_expire = false;
    ec_t ec;
    m_timers.general.cancel(ec);
  }

  template<typename T>
  void connection<T>::start_handshake()
  {
    if (m_state.socket.wait_handshake)
      return;
    static_assert(
      epee::net_utils::get_ssl_magic_size() <= sizeof(m_state.data.read.buffer),
      ""
    );
    auto self = connection<T>::shared_from_this();
    if (!m_state.ssl.forced && !m_state.ssl.detected) {
      m_state.socket.wait_read = true;
      boost::asio::async_read(
        connection_basic::socket_.next_layer(),
        boost::asio::buffer(
          m_state.data.read.buffer.data(),
          m_state.data.read.buffer.size()
        ),
        boost::asio::transfer_exactly(epee::net_utils::get_ssl_magic_size()),
        m_strand.wrap(
          [this, self](const ec_t &ec, size_t bytes_transferred){
            std::lock_guard<std::mutex> guard(m_state.lock);
            m_state.socket.wait_read = false;
            if (m_state.socket.cancel_read) {
              m_state.socket.cancel_read = false;
              state_status_check();
            }
            else if (ec.value()) {
              terminate();
            }
            else if (
              !epee::net_utils::is_ssl(
                static_cast<const unsigned char *>(
                  m_state.data.read.buffer.data()
                ),
                bytes_transferred
              )
            ) {
              m_state.ssl.enabled = false;
              m_state.socket.handle_read = true;
              connection_basic::strand_.post(
                [this, self, bytes_transferred]{
                  bool success = m_handler.handle_recv(
                    reinterpret_cast<char *>(m_state.data.read.buffer.data()),
                    bytes_transferred
                  );
                  std::lock_guard<std::mutex> guard(m_state.lock);
                  m_state.socket.handle_read = false;
                  if (m_state.status == status_t::INTERRUPTED)
                    on_interrupted();
                  else if (m_state.status == status_t::TERMINATING)
                    on_terminating();
                  else if (!success)
                    interrupt();
                  else {
                    start_read();
                  }
                }
              );
            }
            else {
              m_state.ssl.detected = true;
              start_handshake();
            }
          }
        )
      );
      return;
    }

    m_state.socket.wait_handshake = true;
    auto on_handshake = [this, self](const ec_t &ec, size_t bytes_transferred){
      std::lock_guard<std::mutex> guard(m_state.lock);
      m_state.socket.wait_handshake = false;
      if (m_state.socket.cancel_handshake) {
        m_state.socket.cancel_handshake = false;
        state_status_check();
      }
      else if (ec.value()) {
        ec_t ec;
        connection_basic::socket_.next_layer().shutdown(
          socket_t::shutdown_both,
          ec
        );
        connection_basic::socket_.next_layer().close(ec);
        m_state.socket.connected = false;
        interrupt();
      }
      else {
        m_state.ssl.handshaked = true;
        start_write();
        start_read();
      }
    };
    const auto handshake = handshake_t::server;
    static_cast<shared_state&>(
      connection_basic::get_state()
    ).ssl_options().configure(connection_basic::socket_, handshake);
    m_strand.post(
      [this, self, on_handshake]{
        connection_basic::socket_.async_handshake(
          handshake,
          boost::asio::buffer(
            m_state.data.read.buffer.data(),
            m_state.ssl.forced ? 0 :
            epee::net_utils::get_ssl_magic_size()
          ),
          m_strand.wrap(on_handshake)
        );
      }
    );
  }

  template<typename T>
  void connection<T>::start_read()
  {
    if (m_state.timers.throttle.in.wait_expire || m_state.socket.wait_read ||
      m_state.socket.handle_read
    ) {
      return;
    }
    auto self = connection<T>::shared_from_this();
    if (m_connection_type != e_connection_type_RPC) {
      auto calc_duration = []{
        CRITICAL_REGION_LOCAL(
          network_throttle_manager_t::m_lock_get_global_throttle_in
        );
        return std::chrono::duration_cast<connection<T>::duration_t>(
            std::chrono::duration<double, std::chrono::seconds::period>(
              std::min(
                network_throttle_manager_t::get_global_throttle_in(
                ).get_sleep_time_after_tick(1),
                1.0
              )
            )
        );
      };
      const auto duration = calc_duration();
      if (duration > duration_t{}) {
        ec_t ec;
        m_timers.throttle.in.expires_from_now(duration, ec);
        m_state.timers.throttle.in.wait_expire = true;
        m_timers.throttle.in.async_wait([this, self](const ec_t &ec){
          std::lock_guard<std::mutex> guard(m_state.lock);
          m_state.timers.throttle.in.wait_expire = false;
          if (m_state.timers.throttle.in.cancel_expire) {
            m_state.timers.throttle.in.cancel_expire = false;
            state_status_check();
          }
          else if (ec.value())
            interrupt();
          else
            start_read();
        });
        return;
      }
    }
    m_state.socket.wait_read = true;
    auto on_read = [this, self](const ec_t &ec, size_t bytes_transferred){
      std::lock_guard<std::mutex> guard(m_state.lock);
      m_state.socket.wait_read = false;
      if (m_state.socket.cancel_read) {
        m_state.socket.cancel_read = false;
        state_status_check();
      }
      else if (ec.value())
        terminate();
      else {
        {
          m_state.stat.in.throttle.handle_trafic_exact(bytes_transferred);
          const auto speed = m_state.stat.in.throttle.get_current_speed();
          m_conn_context.m_current_speed_down = speed;
          m_conn_context.m_max_speed_down = std::max(
            m_conn_context.m_max_speed_down,
            speed
          );
          {
            CRITICAL_REGION_LOCAL(
              network_throttle_manager_t::m_lock_get_global_throttle_in
            );
            network_throttle_manager_t::get_global_throttle_in(
            ).handle_trafic_exact(bytes_transferred);
          }
          connection_basic::logger_handle_net_read(bytes_transferred);
          m_conn_context.m_last_recv = time(NULL);
          m_conn_context.m_recv_cnt += bytes_transferred;
          start_timer(get_timeout_from_bytes_read(bytes_transferred), true);
        }

        // Post handle_recv to a separate `strand_`, distinct from `m_strand`
        // which is listening for reads/writes. This avoids a circular dep.
        // handle_recv can queue many writes, and `m_strand` will process those
        // writes until the connection terminates without deadlocking waiting
        // for handle_recv.
        m_state.socket.handle_read = true;
        connection_basic::strand_.post(
          [this, self, bytes_transferred]{
            bool success = m_handler.handle_recv(
              reinterpret_cast<char *>(m_state.data.read.buffer.data()),
              bytes_transferred
            );
            std::lock_guard<std::mutex> guard(m_state.lock);
            m_state.socket.handle_read = false;
            if (m_state.status == status_t::INTERRUPTED)
              on_interrupted();
            else if (m_state.status == status_t::TERMINATING)
              on_terminating();
            else if (!success)
              interrupt();
            else {
              start_read();
            }
          }
        );
      }
    };
    if (!m_state.ssl.enabled)
      connection_basic::socket_.next_layer().async_read_some(
        boost::asio::buffer(
          m_state.data.read.buffer.data(),
          m_state.data.read.buffer.size()
        ),
        m_strand.wrap(on_read)
      );
    else
      m_strand.post(
        [this, self, on_read]{
          connection_basic::socket_.async_read_some(
            boost::asio::buffer(
              m_state.data.read.buffer.data(),
              m_state.data.read.buffer.size()
            ),
            m_strand.wrap(on_read)
          );
        }
      );
  }

  template<typename T>
  void connection<T>::start_write()
  {
    if (m_state.timers.throttle.out.wait_expire || m_state.socket.wait_write ||
      m_state.data.write.queue.empty() ||
      (m_state.ssl.enabled && !m_state.ssl.handshaked)
    ) {
      return;
    }
    auto self = connection<T>::shared_from_this();
    if (m_connection_type != e_connection_type_RPC) {
      auto calc_duration = [this]{
        CRITICAL_REGION_LOCAL(
          network_throttle_manager_t::m_lock_get_global_throttle_out
        );
        return std::chrono::duration_cast<connection<T>::duration_t>(
            std::chrono::duration<double, std::chrono::seconds::period>(
              std::min(
                network_throttle_manager_t::get_global_throttle_out(
                ).get_sleep_time_after_tick(
                  m_state.data.write.queue.back().size()
                ),
                1.0
              )
            )
        );
      };
      const auto duration = calc_duration();
      if (duration > duration_t{}) {
        ec_t ec;
        m_timers.throttle.out.expires_from_now(duration, ec);
        m_state.timers.throttle.out.wait_expire = true;
        m_timers.throttle.out.async_wait([this, self](const ec_t &ec){
          std::lock_guard<std::mutex> guard(m_state.lock);
          m_state.timers.throttle.out.wait_expire = false;
          if (m_state.timers.throttle.out.cancel_expire) {
            m_state.timers.throttle.out.cancel_expire = false;
            state_status_check();
          }
          else if (ec.value())
            interrupt();
          else
            start_write();
        });
      }
    }

    m_state.socket.wait_write = true;
    auto on_write = [this, self](const ec_t &ec, size_t bytes_transferred){
      std::lock_guard<std::mutex> guard(m_state.lock);
      m_state.socket.wait_write = false;
      if (m_state.socket.cancel_write) {
        m_state.socket.cancel_write = false;
        m_state.data.write.queue.clear();
        state_status_check();
      }
      else if (ec.value()) {
        m_state.data.write.queue.clear();
        interrupt();
      }
      else {
        {
          m_state.stat.out.throttle.handle_trafic_exact(bytes_transferred);
          const auto speed = m_state.stat.out.throttle.get_current_speed();
          m_conn_context.m_current_speed_up = speed;
          m_conn_context.m_max_speed_down = std::max(
            m_conn_context.m_max_speed_down,
            speed
          );
          {
            CRITICAL_REGION_LOCAL(
              network_throttle_manager_t::m_lock_get_global_throttle_out
            );
            network_throttle_manager_t::get_global_throttle_out(
            ).handle_trafic_exact(bytes_transferred);
          }
          connection_basic::logger_handle_net_write(bytes_transferred);
          m_conn_context.m_last_send = time(NULL);
          m_conn_context.m_send_cnt += bytes_transferred;

          start_timer(get_default_timeout(), true);
        }
        assert(bytes_transferred == m_state.data.write.queue.back().size());
        m_state.data.write.queue.pop_back();
        m_state.condition.notify_all();
        start_write();
      }
    };
    if (!m_state.ssl.enabled)
      boost::asio::async_write(
        connection_basic::socket_.next_layer(),
        boost::asio::buffer(
          m_state.data.write.queue.back().data(),
          m_state.data.write.queue.back().size()
        ),
        m_strand.wrap(on_write)
      );
    else
      m_strand.post(
        [this, self, on_write]{
          boost::asio::async_write(
            connection_basic::socket_,
            boost::asio::buffer(
              m_state.data.write.queue.back().data(),
              m_state.data.write.queue.back().size()
            ),
            m_strand.wrap(on_write)
          );
        }
      );
  }

  template<typename T>
  void connection<T>::start_shutdown()
  {
    if (m_state.socket.wait_shutdown)
      return;
    auto self = connection<T>::shared_from_this();
    m_state.socket.wait_shutdown = true;
    auto on_shutdown = [this, self](const ec_t &ec){
      std::lock_guard<std::mutex> guard(m_state.lock);
      m_state.socket.wait_shutdown = false;
      if (m_state.socket.cancel_shutdown) {
        m_state.socket.cancel_shutdown = false;
        switch (m_state.status)
        {
          case status_t::RUNNING:
            interrupt();
            break;
          case status_t::INTERRUPTED:
            terminate();
            break;
          case status_t::TERMINATING:
            on_terminating();
            break;
          default:
            break;
        }
      }
      else if (ec.value())
        terminate();
      else {
        cancel_timer();
        on_interrupted();
      }
    };
    m_strand.post(
      [this, self, on_shutdown]{
        connection_basic::socket_.async_shutdown(
          m_strand.wrap(on_shutdown)
        );
      }
    );
    start_timer(get_default_timeout());
  }

  template<typename T>
  void connection<T>::cancel_socket()
  {
    bool wait_socket = false;
    if (m_state.socket.wait_handshake)
      wait_socket = m_state.socket.cancel_handshake = true;
    if (m_state.timers.throttle.in.wait_expire) {
      m_state.timers.throttle.in.cancel_expire = true;
      ec_t ec;
      m_timers.throttle.in.cancel(ec);
    }
    if (m_state.socket.wait_read)
      wait_socket = m_state.socket.cancel_read = true;
    if (m_state.timers.throttle.out.wait_expire) {
      m_state.timers.throttle.out.cancel_expire = true;
      ec_t ec;
      m_timers.throttle.out.cancel(ec);
    }
    if (m_state.socket.wait_write)
      wait_socket = m_state.socket.cancel_write = true;
    if (m_state.socket.wait_shutdown)
      wait_socket = m_state.socket.cancel_shutdown = true;
    if (wait_socket) {
      ec_t ec;
      connection_basic::socket_.next_layer().cancel(ec);
    }
  }

  template<typename T>
  void connection<T>::cancel_handler()
  {
    if (m_state.protocol.released || m_state.protocol.wait_release)
      return;
    m_state.protocol.wait_release = true;
    m_state.lock.unlock();
    m_handler.release_protocol();
    m_state.lock.lock();
    m_state.protocol.wait_release = false;
    m_state.protocol.released = true;
    if (m_state.status == status_t::INTERRUPTED)
      on_interrupted();
    else if (m_state.status == status_t::TERMINATING)
      on_terminating();
  }

  template<typename T>
  void connection<T>::interrupt()
  {
    if (m_state.status != status_t::RUNNING)
      return;
    m_state.status = status_t::INTERRUPTED;
    cancel_timer();
    cancel_socket();
    on_interrupted();
    m_state.condition.notify_all();
    cancel_handler();
  }

  template<typename T>
  void connection<T>::on_interrupted()
  {
    assert(m_state.status == status_t::INTERRUPTED);
    if (m_state.timers.general.wait_expire)
      return;
    if (m_state.socket.wait_handshake)
      return;
    if (m_state.timers.throttle.in.wait_expire)
      return;
    if (m_state.socket.wait_read)
      return;
    if (m_state.socket.handle_read)
      return;
    if (m_state.timers.throttle.out.wait_expire)
      return;
    if (m_state.socket.wait_write)
      return;
    if (m_state.socket.wait_shutdown)
      return;
    if (m_state.protocol.wait_init)
      return;
    if (m_state.protocol.wait_callback)
      return;
    if (m_state.protocol.wait_release)
      return;
    if (m_state.socket.connected) {
      if (!m_state.ssl.enabled) {
        ec_t ec;
        connection_basic::socket_.next_layer().shutdown(
          socket_t::shutdown_both,
          ec
        );
        connection_basic::socket_.next_layer().close(ec);
        m_state.socket.connected = false;
        m_state.status = status_t::WASTED;
      }
      else
        start_shutdown();
    }
    else
      m_state.status = status_t::WASTED;
  }

  template<typename T>
  void connection<T>::terminate()
  {
    if (m_state.status != status_t::RUNNING &&
      m_state.status != status_t::INTERRUPTED
    )
      return;
    m_state.status = status_t::TERMINATING;
    cancel_timer();
    cancel_socket();
    on_terminating();
    m_state.condition.notify_all();
    cancel_handler();
  }

  template<typename T>
  void connection<T>::on_terminating()
  {
    assert(m_state.status == status_t::TERMINATING);
    if (m_state.timers.general.wait_expire)
      return;
    if (m_state.socket.wait_handshake)
      return;
    if (m_state.timers.throttle.in.wait_expire)
      return;
    if (m_state.socket.wait_read)
      return;
    if (m_state.socket.handle_read)
      return;
    if (m_state.timers.throttle.out.wait_expire)
      return;
    if (m_state.socket.wait_write)
      return;
    if (m_state.socket.wait_shutdown)
      return;
    if (m_state.protocol.wait_init)
      return;
    if (m_state.protocol.wait_callback)
      return;
    if (m_state.protocol.wait_release)
      return;
    if (m_state.socket.connected) {
      ec_t ec;
      connection_basic::socket_.next_layer().shutdown(
        socket_t::shutdown_both,
        ec
      );
      connection_basic::socket_.next_layer().close(ec);
      m_state.socket.connected = false;
    }
    m_state.status = status_t::WASTED;
  }

  template<typename T>
  bool connection<T>::send(epee::byte_slice message)
  {
    std::lock_guard<std::mutex> guard(m_state.lock);
    if (m_state.status != status_t::RUNNING || m_state.socket.wait_handshake)
      return false;

    // Wait for the write queue to fall below the max. If it doesn't after a
    // randomized delay, drop the connection.
    auto wait_consume = [this] {
      auto random_delay = []{
        using engine = std::mt19937;
        std::random_device dev;
        std::seed_seq::result_type rand[
          engine::state_size // Use complete bit space
        ]{};
        std::generate_n(rand, engine::state_size, std::ref(dev));
        std::seed_seq seed(rand, rand + engine::state_size);
        engine rng(seed);
        return std::chrono::milliseconds(
          std::uniform_int_distribution<>(5000, 6000)(rng)
        );
      };
      if (m_state.data.write.queue.size() <= ABSTRACT_SERVER_SEND_QUE_MAX_COUNT)
        return true;
      m_state.data.write.wait_consume = true;
      bool success = m_state.condition.wait_for(
        m_state.lock,
        random_delay(),
        [this]{
          return (
            m_state.status != status_t::RUNNING ||
            m_state.data.write.queue.size() <=
              ABSTRACT_SERVER_SEND_QUE_MAX_COUNT
          );
        }
      );
      m_state.data.write.wait_consume = false;
      if (!success) {
        terminate();
        return false;
      }
      else
        return m_state.status == status_t::RUNNING;
    };
    auto wait_sender = [this] {
      m_state.condition.wait(
        m_state.lock,
        [this] {
          return (
            m_state.status != status_t::RUNNING ||
            !m_state.data.write.wait_consume
          );
        }
      );
      return m_state.status == status_t::RUNNING;
    };
    if (!wait_sender())
      return false;
    constexpr size_t CHUNK_SIZE = 32 * 1024;
    if (m_connection_type == e_connection_type_RPC ||
      message.size() <= 2 * CHUNK_SIZE
    ) {
      if (!wait_consume())
        return false;
      m_state.data.write.queue.emplace_front(std::move(message));
      start_write();
    }
    else {
      while (!message.empty()) {
        if (!wait_consume())
          return false;
        m_state.data.write.queue.emplace_front(
          message.take_slice(CHUNK_SIZE)
        );
        start_write();
      }
    }
    m_state.condition.notify_all();
    return true;
  }

  template<typename T>
  bool connection<T>::start_internal(
    bool is_income,
    bool is_multithreaded,
    boost::optional<network_address> real_remote
  )
  {
    std::unique_lock<std::mutex> guard(m_state.lock);
    if (m_state.status != status_t::TERMINATED)
      return false;
    if (!real_remote) {
      ec_t ec;
      auto endpoint = connection_basic::socket_.next_layer().remote_endpoint(
        ec
      );
      if (ec.value())
        return false;
      real_remote = (
        endpoint.address().is_v6() ?
        network_address{
          ipv6_network_address{endpoint.address().to_v6(), endpoint.port()}
        } :
        network_address{
          ipv4_network_address{
            uint32_t{
              boost::asio::detail::socket_ops::host_to_network_long(
                endpoint.address().to_v4().to_ulong()
              )
            },
            endpoint.port()
          }
        }
      );
    }
    auto *filter = static_cast<shared_state&>(
      connection_basic::get_state()
    ).pfilter;
    if (filter && !filter->is_remote_host_allowed(*real_remote))
      return false;
    ec_t ec;
    #if !defined(_WIN32) || !defined(__i686)
    connection_basic::socket_.next_layer().set_option(
      boost::asio::detail::socket_option::integer<IPPROTO_IP, IP_TOS>{
        connection_basic::get_tos_flag()
      },
      ec
    );
    if (ec.value())
      return false;
    #endif
    connection_basic::socket_.next_layer().set_option(
      boost::asio::ip::tcp::no_delay{false},
      ec
    );
    if (ec.value())
      return false;
    connection_basic::m_is_multithreaded = is_multithreaded;
    m_conn_context.set_details(
      boost::uuids::random_generator()(),
      *real_remote,
      is_income,
      connection_basic::m_ssl_support == ssl_support_t::e_ssl_support_enabled
    );
    m_host = real_remote->host_str();
    try { host_count(1); } catch(...) { /* ignore */ }
    m_local = real_remote->is_loopback() || real_remote->is_local();
    m_state.ssl.enabled = (
      connection_basic::m_ssl_support != ssl_support_t::e_ssl_support_disabled
    );
    m_state.ssl.forced = (
      connection_basic::m_ssl_support == ssl_support_t::e_ssl_support_enabled
    );
    m_state.socket.connected = true;
    m_state.status = status_t::RUNNING;
    start_timer(
      std::chrono::milliseconds(
        m_local ? NEW_CONNECTION_TIMEOUT_LOCAL : NEW_CONNECTION_TIMEOUT_REMOTE
      )
    );
    m_state.protocol.wait_init = true;
    guard.unlock();
    m_handler.after_init_connection();
    guard.lock();
    m_state.protocol.wait_init = false;
    m_state.protocol.initialized = true;
    if (m_state.status == status_t::INTERRUPTED)
      on_interrupted();
    else if (m_state.status == status_t::TERMINATING)
      on_terminating();
    else if (!is_income || !m_state.ssl.enabled)
      start_read();
    else
      start_handshake();
    return true;
  }

  template<typename T>
  connection<T>::connection(
    io_context_t &io_context,
    std::shared_ptr<shared_state> shared_state,
    t_connection_type connection_type,
    ssl_support_t ssl_support
  ):
    connection(
      std::move(socket_t{io_context}),
      std::move(shared_state),
      connection_type,
      ssl_support
    )
  {
  }

  template<typename T>
  connection<T>::connection(
    socket_t &&socket,
    std::shared_ptr<shared_state> shared_state,
    t_connection_type connection_type,
    ssl_support_t ssl_support
  ):
    connection_basic(std::move(socket), shared_state, ssl_support),
    m_handler(this, *shared_state, m_conn_context),
    m_connection_type(connection_type),
    m_io_context{GET_IO_SERVICE(connection_basic::socket_)},
    m_strand{m_io_context},
    m_timers{m_io_context}
  {
  }

  template<typename T>
  connection<T>::~connection() noexcept(false)
  {
    std::lock_guard<std::mutex> guard(m_state.lock);
    assert(m_state.status == status_t::TERMINATED ||
      m_state.status == status_t::WASTED ||
      m_io_context.stopped()
    );
    if (m_state.status != status_t::WASTED)
      return;
    try { host_count(-1); } catch (...) { /* ignore */ }
  }

  template<typename T>
  bool connection<T>::start(
    bool is_income,
    bool is_multithreaded
  )
  {
    return start_internal(is_income, is_multithreaded, {});
  }

  template<typename T>
  bool connection<T>::start(
    bool is_income,
    bool is_multithreaded,
    network_address real_remote
  )
  {
    return start_internal(is_income, is_multithreaded, real_remote);
  }

  template<typename T>
  void connection<T>::save_dbg_log()
  {
    std::lock_guard<std::mutex> guard(m_state.lock);
    std::string address;
    std::string port;
    ec_t ec;
    auto endpoint = connection_basic::socket().remote_endpoint(ec);
    if (ec.value()) {
      address = "<not connected>";
      port = "<not connected>";
    }
    else {
      address = endpoint.address().to_string();
      port = std::to_string(endpoint.port());
    }
    MDEBUG(
      " connection type " << std::to_string(m_connection_type) <<
      " " << connection_basic::socket().local_endpoint().address().to_string() <<
      ":" << connection_basic::socket().local_endpoint().port() <<
      " <--> " << m_conn_context.m_remote_address.str() <<
      " (via " << address << ":" << port << ")"
    );
  }

  template<typename T>
  bool connection<T>::speed_limit_is_enabled() const
  {
    return m_connection_type != e_connection_type_RPC;
  }

  template<typename T>
  bool connection<T>::cancel()
  {
    return close();
  }

  template<typename T>
  bool connection<T>::do_send(byte_slice message)
  {
    return send(std::move(message));
  }

  template<typename T>
  bool connection<T>::send_done()
  {
    return true;
  }

  template<typename T>
  bool connection<T>::close()
  {
    std::lock_guard<std::mutex> guard(m_state.lock);
    if (m_state.status != status_t::RUNNING)
      return false;
    terminate();
    return true;
  }

  template<typename T>
  bool connection<T>::call_run_once_service_io()
  {
    if(connection_basic::m_is_multithreaded) {
      if (!m_io_context.poll_one())
        misc_utils::sleep_no_w(1);
    }
    else {
      if (!m_io_context.run_one())
        return false;
    }
    return true;
  }

  template<typename T>
  bool connection<T>::request_callback()
  {
    std::lock_guard<std::mutex> guard(m_state.lock);
    if (m_state.status != status_t::RUNNING)
      return false;
    auto self = connection<T>::shared_from_this();
    ++m_state.protocol.wait_callback;
    connection_basic::strand_.post([this, self]{
      m_handler.handle_qued_callback();
      std::lock_guard<std::mutex> guard(m_state.lock);
      --m_state.protocol.wait_callback;
      if (m_state.status == status_t::INTERRUPTED)
        on_interrupted();
      else if (m_state.status == status_t::TERMINATING)
        on_terminating();
    });
    return true;
  }

  template<typename T>
  typename connection<T>::io_context_t &connection<T>::get_io_service()
  {
    return m_io_context;
  }

  template<typename T>
  bool connection<T>::add_ref()
  {
    try {
      auto self = connection<T>::shared_from_this();
      std::lock_guard<std::mutex> guard(m_state.lock);
      this->self = std::move(self);
      ++m_state.protocol.reference_counter;
      return true;
    }
    catch (boost::bad_weak_ptr &exception) {
      return false;
    }
  }

  template<typename T>
  bool connection<T>::release()
  {
    connection_ptr self;
    std::lock_guard<std::mutex> guard(m_state.lock);
    if (!(--m_state.protocol.reference_counter))
      self = std::move(this->self);
    return true;
  }

  template<typename T>
  void connection<T>::setRpcStation()
  {
    std::lock_guard<std::mutex> guard(m_state.lock);
    m_connection_type = e_connection_type_RPC;
  }

  template<class t_protocol_handler>
  boosted_tcp_server<t_protocol_handler>::boosted_tcp_server( t_connection_type connection_type ) :
    m_state(std::make_shared<typename connection<t_protocol_handler>::shared_state>()),
    m_io_service_local_instance(new worker()),
    io_service_(m_io_service_local_instance->io_service),
    acceptor_(io_service_),
    acceptor_ipv6(io_service_),
    default_remote(),
    m_stop_signal_sent(false), m_port(0), 
    m_threads_count(0),
    m_thread_index(0),
		m_connection_type( connection_type ),
    new_connection_(),
    new_connection_ipv6()
  {
    create_server_type_map();
    m_thread_name_prefix = "NET";
  }

  template<class t_protocol_handler>
  boosted_tcp_server<t_protocol_handler>::boosted_tcp_server(boost::asio::io_service& extarnal_io_service, t_connection_type connection_type) :
    m_state(std::make_shared<typename connection<t_protocol_handler>::shared_state>()),
    io_service_(extarnal_io_service),
    acceptor_(io_service_),
    acceptor_ipv6(io_service_),
    default_remote(),
    m_stop_signal_sent(false), m_port(0),
    m_threads_count(0),
    m_thread_index(0),
		m_connection_type(connection_type),
    new_connection_(),
    new_connection_ipv6()
  {
    create_server_type_map();
    m_thread_name_prefix = "NET";
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  boosted_tcp_server<t_protocol_handler>::~boosted_tcp_server()
  {
    this->send_stop_signal();
    timed_wait_server_stop(10000);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::create_server_type_map() 
  {
		server_type_map["NET"] = e_connection_type_NET;
		server_type_map["RPC"] = e_connection_type_RPC;
		server_type_map["P2P"] = e_connection_type_P2P;
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
    bool boosted_tcp_server<t_protocol_handler>::init_server(uint32_t port,  const std::string& address,
	uint32_t port_ipv6, const std::string& address_ipv6, bool use_ipv6, bool require_ipv4,
	ssl_options_t ssl_options)
  {
    TRY_ENTRY();
    m_stop_signal_sent = false;
    m_port = port;
    m_port_ipv6 = port_ipv6;
    m_address = address;
    m_address_ipv6 = address_ipv6;
    m_use_ipv6 = use_ipv6;
    m_require_ipv4 = require_ipv4;

    if (ssl_options)
      m_state->configure_ssl(std::move(ssl_options));

    std::string ipv4_failed = "";
    std::string ipv6_failed = "";
    try
    {
      boost::asio::ip::tcp::resolver resolver(io_service_);
      boost::asio::ip::tcp::resolver::query query(address, boost::lexical_cast<std::string>(port), boost::asio::ip::tcp::resolver::query::canonical_name);
      boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
      acceptor_.open(endpoint.protocol());
#if !defined(_WIN32)
      acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
#endif
      acceptor_.bind(endpoint);
      acceptor_.listen();
      boost::asio::ip::tcp::endpoint binded_endpoint = acceptor_.local_endpoint();
      m_port = binded_endpoint.port();
      MDEBUG("start accept (IPv4)");
      new_connection_.reset(new connection<t_protocol_handler>(io_service_, m_state, m_connection_type, m_state->ssl_options().support));
      acceptor_.async_accept(new_connection_->socket(),
	boost::bind(&boosted_tcp_server<t_protocol_handler>::handle_accept_ipv4, this,
	boost::asio::placeholders::error));
    }
    catch (const std::exception &e)
    {
      ipv4_failed = e.what();
    }

    if (ipv4_failed != "")
    {
      MERROR("Failed to bind IPv4: " << ipv4_failed);
      if (require_ipv4)
      {
        throw std::runtime_error("Failed to bind IPv4 (set to required)");
      }
    }

    if (use_ipv6)
    {
      try
      {
        if (port_ipv6 == 0) port_ipv6 = port; // default arg means bind to same port as ipv4
        boost::asio::ip::tcp::resolver resolver(io_service_);
        boost::asio::ip::tcp::resolver::query query(address_ipv6, boost::lexical_cast<std::string>(port_ipv6), boost::asio::ip::tcp::resolver::query::canonical_name);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
        acceptor_ipv6.open(endpoint.protocol());
#if !defined(_WIN32)
        acceptor_ipv6.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
#endif
        acceptor_ipv6.set_option(boost::asio::ip::v6_only(true));
        acceptor_ipv6.bind(endpoint);
        acceptor_ipv6.listen();
        boost::asio::ip::tcp::endpoint binded_endpoint = acceptor_ipv6.local_endpoint();
        m_port_ipv6 = binded_endpoint.port();
        MDEBUG("start accept (IPv6)");
        new_connection_ipv6.reset(new connection<t_protocol_handler>(io_service_, m_state, m_connection_type, m_state->ssl_options().support));
        acceptor_ipv6.async_accept(new_connection_ipv6->socket(),
            boost::bind(&boosted_tcp_server<t_protocol_handler>::handle_accept_ipv6, this,
              boost::asio::placeholders::error));
      }
      catch (const std::exception &e)
      {
        ipv6_failed = e.what();
      }
    }

      if (use_ipv6 && ipv6_failed != "")
      {
        MERROR("Failed to bind IPv6: " << ipv6_failed);
        if (ipv4_failed != "")
        {
          throw std::runtime_error("Failed to bind IPv4 and IPv6");
        }
      }

    return true;
    }
    catch (const std::exception &e)
    {
      MFATAL("Error starting server: " << e.what());
      return false;
    }
    catch (...)
    {
      MFATAL("Error starting server");
      return false;
    }
  }
  //-----------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::init_server(const std::string port,  const std::string& address,
      const std::string port_ipv6, const std::string address_ipv6, bool use_ipv6, bool require_ipv4,
      ssl_options_t ssl_options)
  {
    uint32_t p = 0;
    uint32_t p_ipv6 = 0;

    if (port.size() && !string_tools::get_xtype_from_string(p, port)) {
      MERROR("Failed to convert port no = " << port);
      return false;
    }

    if (port_ipv6.size() && !string_tools::get_xtype_from_string(p_ipv6, port_ipv6)) {
      MERROR("Failed to convert port no = " << port_ipv6);
      return false;
    }
    return this->init_server(p, address, p_ipv6, address_ipv6, use_ipv6, require_ipv4, std::move(ssl_options));
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::worker_thread()
  {
    TRY_ENTRY();
    const uint32_t local_thr_index = m_thread_index++; // atomically increment, getting value before increment
    std::string thread_name = std::string("[") + m_thread_name_prefix;
    thread_name += boost::to_string(local_thr_index) + "]";
    MLOG_SET_THREAD_NAME(thread_name);
    //   _fact("Thread name: " << m_thread_name_prefix);
    while(!m_stop_signal_sent)
    {
      try
      {
        io_service_.run();
        return true;
      }
      catch(const std::exception& ex)
      {
        _erro("Exception at server worker thread, what=" << ex.what());
      }
      catch(...)
      {
        _erro("Exception at server worker thread, unknown execption");
      }
    }
    //_info("Worker thread finished");
    return true;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::worker_thread", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::set_threads_prefix(const std::string& prefix_name)
  {
    m_thread_name_prefix = prefix_name;
		auto it = server_type_map.find(m_thread_name_prefix);
		if (it==server_type_map.end()) throw std::runtime_error("Unknown prefix/server type:" + std::string(prefix_name));
    auto connection_type = it->second; // the value of type
    MINFO("Set server type to: " << connection_type << " from name: " << m_thread_name_prefix << ", prefix_name = " << prefix_name);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::set_connection_filter(i_connection_filter* pfilter)
  {
    assert(m_state != nullptr); // always set in constructor
    m_state->pfilter = pfilter;
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::run_server(size_t threads_count, bool wait, const boost::thread::attributes& attrs)
  {
    TRY_ENTRY();
    m_threads_count = threads_count;
    m_main_thread_id = boost::this_thread::get_id();
    MLOG_SET_THREAD_NAME("[SRV_MAIN]");
    while(!m_stop_signal_sent)
    {

      // Create a pool of threads to run all of the io_services.
      CRITICAL_REGION_BEGIN(m_threads_lock);
      for (std::size_t i = 0; i < threads_count; ++i)
      {
        boost::shared_ptr<boost::thread> thread(new boost::thread(
          attrs, boost::bind(&boosted_tcp_server<t_protocol_handler>::worker_thread, this)));
          _note("Run server thread name: " << m_thread_name_prefix);
        m_threads.push_back(thread);
      }
      CRITICAL_REGION_END();
      // Wait for all threads in the pool to exit.
      if (wait)
      {
		_fact("JOINING all threads");
        for (std::size_t i = 0; i < m_threads.size(); ++i) {
			m_threads[i]->join();
         }
         _fact("JOINING all threads - almost");
        m_threads.clear();
        _fact("JOINING all threads - DONE");

      } 
      else {
		_dbg1("Reiniting OK.");
        return true;
      }

      if(wait && !m_stop_signal_sent)
      {
        //some problems with the listening socket ?..
        _dbg1("Net service stopped without stop request, restarting...");
        if(!this->init_server(m_port, m_address, m_port_ipv6, m_address_ipv6, m_use_ipv6, m_require_ipv4))
        {
          _dbg1("Reiniting service failed, exit.");
          return false;
        }else
        {
          _dbg1("Reiniting OK.");
        }
      }
    }
    return true;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::run_server", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::is_thread_worker()
  {
    TRY_ENTRY();
    CRITICAL_REGION_LOCAL(m_threads_lock);
    BOOST_FOREACH(boost::shared_ptr<boost::thread>& thp,  m_threads)
    {
      if(thp->get_id() == boost::this_thread::get_id())
        return true;
    }
    if(m_threads_count == 1 && boost::this_thread::get_id() == m_main_thread_id)
      return true;
    return false;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::is_thread_worker", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::timed_wait_server_stop(uint64_t wait_mseconds)
  {
    TRY_ENTRY();
    boost::chrono::milliseconds ms(wait_mseconds);
    for (std::size_t i = 0; i < m_threads.size(); ++i)
    {
      if(m_threads[i]->joinable() && !m_threads[i]->try_join_for(ms))
      {
        _dbg1("Interrupting thread " << m_threads[i]->native_handle());
        m_threads[i]->interrupt();
      }
    }
    return true;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::timed_wait_server_stop", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::send_stop_signal()
  {
    m_stop_signal_sent = true;
    typename connection<t_protocol_handler>::shared_state *state = static_cast<typename connection<t_protocol_handler>::shared_state*>(m_state.get());
    state->stop_signal_sent = true;
    TRY_ENTRY();
    connections_mutex.lock();
    for (auto &c: connections_)
    {
      c->cancel();
    }
    connections_.clear();
    connections_mutex.unlock();
    io_service_.stop();
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::send_stop_signal()", void());
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::handle_accept_ipv4(const boost::system::error_code& e)
  {
    this->handle_accept(e, false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::handle_accept_ipv6(const boost::system::error_code& e)
  {
    this->handle_accept(e, true);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  void boosted_tcp_server<t_protocol_handler>::handle_accept(const boost::system::error_code& e, bool ipv6)
  {
    MDEBUG("handle_accept");

    boost::asio::ip::tcp::acceptor* current_acceptor = &acceptor_;
    connection_ptr* current_new_connection = &new_connection_;
    auto accept_function_pointer = &boosted_tcp_server<t_protocol_handler>::handle_accept_ipv4;
    if (ipv6)
    {
      current_acceptor = &acceptor_ipv6;
      current_new_connection = &new_connection_ipv6;
      accept_function_pointer = &boosted_tcp_server<t_protocol_handler>::handle_accept_ipv6;
    }

    try
    {
    if (!e)
    {
      if (m_connection_type == e_connection_type_RPC) {
        const char *ssl_message = "unknown";
        switch ((*current_new_connection)->get_ssl_support())
        {
          case epee::net_utils::ssl_support_t::e_ssl_support_disabled: ssl_message = "disabled"; break;
          case epee::net_utils::ssl_support_t::e_ssl_support_enabled: ssl_message = "enabled"; break;
          case epee::net_utils::ssl_support_t::e_ssl_support_autodetect: ssl_message = "autodetection"; break;
        }
        MDEBUG("New server for RPC connections, SSL " << ssl_message);
        (*current_new_connection)->setRpcStation(); // hopefully this is not needed actually
      }
      connection_ptr conn(std::move((*current_new_connection)));
      (*current_new_connection).reset(new connection<t_protocol_handler>(io_service_, m_state, m_connection_type, conn->get_ssl_support()));
      current_acceptor->async_accept((*current_new_connection)->socket(),
          boost::bind(accept_function_pointer, this,
            boost::asio::placeholders::error));

      boost::asio::socket_base::keep_alive opt(true);
      conn->socket().set_option(opt);

      bool res;
      if (default_remote.get_type_id() == net_utils::address_type::invalid)
        res = conn->start(true, 1 < m_threads_count);
      else
        res = conn->start(true, 1 < m_threads_count, default_remote);
      if (!res)
      {
        conn->cancel();
        return;
      }
      conn->save_dbg_log();
      return;
    }
    else
    {
      MERROR("Error in boosted_tcp_server<t_protocol_handler>::handle_accept: " << e);
    }
    }
    catch (const std::exception &e)
    {
      MERROR("Exception in boosted_tcp_server<t_protocol_handler>::handle_accept: " << e.what());
    }

    // error path, if e or exception
    assert(m_state != nullptr); // always set in constructor
    _erro("Some problems at accept: " << e.message() << ", connections_count = " << m_state->sock_count);
    misc_utils::sleep_no_w(100);
    (*current_new_connection).reset(new connection<t_protocol_handler>(io_service_, m_state, m_connection_type, (*current_new_connection)->get_ssl_support()));
    current_acceptor->async_accept((*current_new_connection)->socket(),
        boost::bind(accept_function_pointer, this,
          boost::asio::placeholders::error));
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::add_connection(t_connection_context& out, boost::asio::ip::tcp::socket&& sock, network_address real_remote, epee::net_utils::ssl_support_t ssl_support)
  {
    if(std::addressof(get_io_service()) == std::addressof(GET_IO_SERVICE(sock)))
    {
      connection_ptr conn(new connection<t_protocol_handler>(std::move(sock), m_state, m_connection_type, ssl_support));
      if(conn->start(false, 1 < m_threads_count, std::move(real_remote)))
      {
        conn->get_context(out);
        conn->save_dbg_log();
        return true;
      }
    }
    else
    {
	MWARNING(out << " was not added, socket/io_service mismatch");
    }
    return false;
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  typename boosted_tcp_server<t_protocol_handler>::try_connect_result_t boosted_tcp_server<t_protocol_handler>::try_connect(connection_ptr new_connection_l, const std::string& adr, const std::string& port, boost::asio::ip::tcp::socket &sock_, const boost::asio::ip::tcp::endpoint &remote_endpoint, const std::string &bind_ip, uint32_t conn_timeout, epee::net_utils::ssl_support_t ssl_support)
  {
    TRY_ENTRY();

    sock_.open(remote_endpoint.protocol());
    if(bind_ip != "0.0.0.0" && bind_ip != "0" && bind_ip != "" )
    {
      boost::asio::ip::tcp::endpoint local_endpoint(boost::asio::ip::address::from_string(bind_ip.c_str()), 0);
      boost::system::error_code ec;
      sock_.bind(local_endpoint, ec);
      if (ec)
      {
        MERROR("Error binding to " << bind_ip << ": " << ec.message());
        if (sock_.is_open())
          sock_.close();
        return CONNECT_FAILURE;
      }
    }

    /*
    NOTICE: be careful to make sync connection from event handler: in case if all threads suddenly do sync connect, there will be no thread to dispatch events from io service.
    */

    boost::system::error_code ec = boost::asio::error::would_block;

    //have another free thread(s), work in wait mode, without event handling
    struct local_async_context
    {
      boost::system::error_code ec;
      boost::mutex connect_mut;
      boost::condition_variable cond;
    };

    boost::shared_ptr<local_async_context> local_shared_context(new local_async_context());
    local_shared_context->ec = boost::asio::error::would_block;
    boost::unique_lock<boost::mutex> lock(local_shared_context->connect_mut);
    auto connect_callback = [](boost::system::error_code ec_, boost::shared_ptr<local_async_context> shared_context)
    {
      shared_context->connect_mut.lock(); shared_context->ec = ec_; shared_context->cond.notify_one(); shared_context->connect_mut.unlock();
    };

    sock_.async_connect(remote_endpoint, std::bind<void>(connect_callback, std::placeholders::_1, local_shared_context));
    while(local_shared_context->ec == boost::asio::error::would_block)
    {
      bool r = local_shared_context->cond.timed_wait(lock, boost::get_system_time() + boost::posix_time::milliseconds(conn_timeout));
      if (m_stop_signal_sent)
      {
        if (sock_.is_open())
          sock_.close();
        return CONNECT_FAILURE;
      }
      if(local_shared_context->ec == boost::asio::error::would_block && !r)
      {
        //timeout
        sock_.close();
        _dbg3("Failed to connect to " << adr << ":" << port << ", because of timeout (" << conn_timeout << ")");
        return CONNECT_FAILURE;
      }
    }
    ec = local_shared_context->ec;

    if (ec || !sock_.is_open())
    {
      _dbg3("Some problems at connect, message: " << ec.message());
      if (sock_.is_open())
        sock_.close();
      return CONNECT_FAILURE;
    }

    _dbg3("Connected success to " << adr << ':' << port);

    const ssl_support_t ssl_support = new_connection_l->get_ssl_support();
    if (ssl_support == epee::net_utils::ssl_support_t::e_ssl_support_enabled || ssl_support == epee::net_utils::ssl_support_t::e_ssl_support_autodetect)
    {
      // Handshake
      MDEBUG("Handshaking SSL...");
      if (!new_connection_l->handshake(boost::asio::ssl::stream_base::client))
      {
        if (ssl_support == epee::net_utils::ssl_support_t::e_ssl_support_autodetect)
        {
          boost::system::error_code ignored_ec;
          sock_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
          sock_.close();
          return CONNECT_NO_SSL;
        }
        MERROR("SSL handshake failed");
        if (sock_.is_open())
          sock_.close();
        return CONNECT_FAILURE;
      }
    }

    return CONNECT_SUCCESS;

    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::try_connect", CONNECT_FAILURE);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler>
  bool boosted_tcp_server<t_protocol_handler>::connect(const std::string& adr, const std::string& port, uint32_t conn_timeout, t_connection_context& conn_context, const std::string& bind_ip, epee::net_utils::ssl_support_t ssl_support)
  {
    TRY_ENTRY();

    connection_ptr new_connection_l(new connection<t_protocol_handler>(io_service_, m_state, m_connection_type, ssl_support) );
    connections_mutex.lock();
    connections_.insert(new_connection_l);
    MDEBUG("connections_ size now " << connections_.size());
    connections_mutex.unlock();
    epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){ CRITICAL_REGION_LOCAL(connections_mutex); connections_.erase(new_connection_l); });
    boost::asio::ip::tcp::socket&  sock_ = new_connection_l->socket();

    bool try_ipv6 = false;

    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), adr, port, boost::asio::ip::tcp::resolver::query::canonical_name);
    boost::system::error_code resolve_error;
    boost::asio::ip::tcp::resolver::iterator iterator;
    try
    {
      //resolving ipv4 address as ipv6 throws, catch here and move on
      iterator = resolver.resolve(query, resolve_error);
    }
    catch (const boost::system::system_error& e)
    {
      if (!m_use_ipv6 || (resolve_error != boost::asio::error::host_not_found &&
            resolve_error != boost::asio::error::host_not_found_try_again))
      {
        throw;
      }
      try_ipv6 = true;
    }
    catch (...)
    {
      throw;
    }

    std::string bind_ip_to_use;

    boost::asio::ip::tcp::resolver::iterator end;
    if(iterator == end)
    {
      if (!m_use_ipv6)
      {
        _erro("Failed to resolve " << adr);
        return false;
      }
      else
      {
        try_ipv6 = true;
        MINFO("Resolving address as IPv4 failed, trying IPv6");
      }
    }
    else
    {
      bind_ip_to_use = bind_ip;
    }

    if (try_ipv6)
    {
      boost::asio::ip::tcp::resolver::query query6(boost::asio::ip::tcp::v6(), adr, port, boost::asio::ip::tcp::resolver::query::canonical_name);

      iterator = resolver.resolve(query6, resolve_error);

      if(iterator == end)
      {
        _erro("Failed to resolve " << adr);
        return false;
      }
      else
      {
        if (bind_ip == "0.0.0.0")
        {
          bind_ip_to_use = "::";
        }
        else
        {
          bind_ip_to_use = "";
        }

      }

    }

    MDEBUG("Trying to connect to " << adr << ":" << port << ", bind_ip = " << bind_ip_to_use);

    //boost::asio::ip::tcp::endpoint remote_endpoint(boost::asio::ip::address::from_string(addr.c_str()), port);
    boost::asio::ip::tcp::endpoint remote_endpoint(*iterator);

    auto try_connect_result = try_connect(new_connection_l, adr, port, sock_, remote_endpoint, bind_ip_to_use, conn_timeout, ssl_support);
    if (try_connect_result == CONNECT_FAILURE)
      return false;
    if (ssl_support == epee::net_utils::ssl_support_t::e_ssl_support_autodetect && try_connect_result == CONNECT_NO_SSL)
    {
      // we connected, but could not connect with SSL, try without
      MERROR("SSL handshake failed on an autodetect connection, reconnecting without SSL");
      new_connection_l->disable_ssl();
      try_connect_result = try_connect(new_connection_l, adr, port, sock_, remote_endpoint, bind_ip_to_use, conn_timeout, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
      if (try_connect_result != CONNECT_SUCCESS)
        return false;
    }

    // start adds the connection to the config object's list, so we don't need to have it locally anymore
    connections_mutex.lock();
    connections_.erase(new_connection_l);
    connections_mutex.unlock();
    bool r = new_connection_l->start(false, 1 < m_threads_count);
    if (r)
    {
      new_connection_l->get_context(conn_context);
      //new_connection_l.reset(new connection<t_protocol_handler>(io_service_, m_config, m_sock_count, m_pfilter));
    }
    else
    {
      assert(m_state != nullptr); // always set in constructor
      _erro("[sock " << new_connection_l->socket().native_handle() << "] Failed to start connection, connections_count = " << m_state->sock_count);
    }
    
	new_connection_l->save_dbg_log();

    return r;

    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::connect", false);
  }
  //---------------------------------------------------------------------------------
  template<class t_protocol_handler> template<class t_callback>
  bool boosted_tcp_server<t_protocol_handler>::connect_async(const std::string& adr, const std::string& port, uint32_t conn_timeout, const t_callback &cb, const std::string& bind_ip, epee::net_utils::ssl_support_t ssl_support)
  {
    TRY_ENTRY();    
    connection_ptr new_connection_l(new connection<t_protocol_handler>(io_service_, m_state, m_connection_type, ssl_support) );
    connections_mutex.lock();
    connections_.insert(new_connection_l);
    MDEBUG("connections_ size now " << connections_.size());
    connections_mutex.unlock();
    epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){ CRITICAL_REGION_LOCAL(connections_mutex); connections_.erase(new_connection_l); });
    boost::asio::ip::tcp::socket&  sock_ = new_connection_l->socket();
    
    bool try_ipv6 = false;

    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), adr, port, boost::asio::ip::tcp::resolver::query::canonical_name);
    boost::system::error_code resolve_error;
    boost::asio::ip::tcp::resolver::iterator iterator;
    try
    {
      //resolving ipv4 address as ipv6 throws, catch here and move on
      iterator = resolver.resolve(query, resolve_error);
    }
    catch (const boost::system::system_error& e)
    {
      if (!m_use_ipv6 || (resolve_error != boost::asio::error::host_not_found &&
            resolve_error != boost::asio::error::host_not_found_try_again))
      {
        throw;
      }
      try_ipv6 = true;
    }
    catch (...)
    {
      throw;
    }

    boost::asio::ip::tcp::resolver::iterator end;
    if(iterator == end)
    {
      if (!try_ipv6)
      {
        _erro("Failed to resolve " << adr);
        return false;
      }
      else
      {
        MINFO("Resolving address as IPv4 failed, trying IPv6");
      }
    }

    if (try_ipv6)
    {
      boost::asio::ip::tcp::resolver::query query6(boost::asio::ip::tcp::v6(), adr, port, boost::asio::ip::tcp::resolver::query::canonical_name);

      iterator = resolver.resolve(query6, resolve_error);

      if(iterator == end)
      {
        _erro("Failed to resolve " << adr);
        return false;
      }
    }


    boost::asio::ip::tcp::endpoint remote_endpoint(*iterator);
     
    sock_.open(remote_endpoint.protocol());
    if(bind_ip != "0.0.0.0" && bind_ip != "0" && bind_ip != "" )
    {
      boost::asio::ip::tcp::endpoint local_endpoint(boost::asio::ip::address::from_string(bind_ip.c_str()), 0);
      boost::system::error_code ec;
      sock_.bind(local_endpoint, ec);
      if (ec)
      {
        MERROR("Error binding to " << bind_ip << ": " << ec.message());
        if (sock_.is_open())
          sock_.close();
        return false;
      }
    }
    
    boost::shared_ptr<boost::asio::deadline_timer> sh_deadline(new boost::asio::deadline_timer(io_service_));
    //start deadline
    sh_deadline->expires_from_now(boost::posix_time::milliseconds(conn_timeout));
    sh_deadline->async_wait([=](const boost::system::error_code& error)
      {
          if(error != boost::asio::error::operation_aborted) 
          {
            _dbg3("Failed to connect to " << adr << ':' << port << ", because of timeout (" << conn_timeout << ")");
            new_connection_l->socket().close();
          }
      });
    //start async connect
    sock_.async_connect(remote_endpoint, [=](const boost::system::error_code& ec_)
      {
        t_connection_context conn_context = AUTO_VAL_INIT(conn_context);
        boost::system::error_code ignored_ec;
        boost::asio::ip::tcp::socket::endpoint_type lep = new_connection_l->socket().local_endpoint(ignored_ec);
        if(!ec_)
        {//success
          if(!sh_deadline->cancel())
          {
            cb(conn_context, boost::asio::error::operation_aborted);//this mean that deadline timer already queued callback with cancel operation, rare situation
          }else
          {
            _dbg3("[sock " << new_connection_l->socket().native_handle() << "] Connected success to " << adr << ':' << port <<
              " from " << lep.address().to_string() << ':' << lep.port());

            // start adds the connection to the config object's list, so we don't need to have it locally anymore
            connections_mutex.lock();
            connections_.erase(new_connection_l);
            connections_mutex.unlock();
            bool r = new_connection_l->start(false, 1 < m_threads_count);
            if (r)
            {
              new_connection_l->get_context(conn_context);
              cb(conn_context, ec_);
            }
            else
            {
              _dbg3("[sock " << new_connection_l->socket().native_handle() << "] Failed to start connection to " << adr << ':' << port);
              cb(conn_context, boost::asio::error::fault);
            }
          }
        }else
        {
          _dbg3("[sock " << new_connection_l->socket().native_handle() << "] Failed to connect to " << adr << ':' << port <<
            " from " << lep.address().to_string() << ':' << lep.port() << ": " << ec_.message() << ':' << ec_.value());
          cb(conn_context, ec_);
        }
      });
    return true;
    CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::connect_async", false);
  }
  
} // namespace
} // namespace
