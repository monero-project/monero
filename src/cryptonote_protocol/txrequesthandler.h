// Copyright (c) 2014-2025, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list
//    of conditions and the following disclaimer in the documentation and/or
//    other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be
//    used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef CRYPTONOTE_PROTOCOL_TXREQUESTHANDLER_H
#define CRYPTONOTE_PROTOCOL_TXREQUESTHANDLER_H

#include <boost/functional/hash.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <thread>

#include "crypto/hash.h"
#include "request_manager.h"
#include "txrequestqueue.h"

class tx_request_handler {
public:
  typedef std::function<void(const crypto::hash &, tx_request_queue &,
                             const std::time_t)>
      tx_request_handler_runner;

private:
  // Disable everything that can be disabled
  // to prevent misuse of the class
  tx_request_handler() = delete;
  tx_request_handler(tx_request_handler &&other) noexcept = delete;
  tx_request_handler(const tx_request_handler &other) = delete;
  tx_request_handler &operator=(tx_request_handler &&other) = delete;
  tx_request_handler &operator=(const tx_request_handler &other) = delete;
  tx_request_handler *operator&() = delete;
  bool operator>(const tx_request_handler &other) const = delete;
  bool operator<(const tx_request_handler &other) const = delete;
  bool operator==(const tx_request_handler &other) const = delete;
  bool operator!=(const tx_request_handler &other) const = delete;
  void swap(tx_request_handler &) = delete;
  operator bool() const = delete;
  auto operator->() = delete;
  auto operator*() = delete;
  void *operator new(std::size_t) = delete;
  void operator delete(void *) = delete;
  void *operator new[](std::size_t) = delete;
  void operator delete[](void *) = delete;
  void operator()() = delete;

public:
  tx_request_handler(request_manager &request_manager,
                     tx_request_handler_runner &handler_runner,
                     std::time_t request_deadline = 30)
      : m_handler_thread(), m_stop_handler_thread(false),
        m_handler_runner(handler_runner), m_request_deadline(request_deadline),
        m_request_manager(request_manager) {}

  ~tx_request_handler() { deinit(); }

  /*
   * \brief Start the thread that will check the transaction requests
   *
   * This function will start a thread that will check the transaction
   * requests every m_request_deadline seconds. The thread will call the
   * m_handler_runner function for each request in the request manager.
   *
   * This function assumes that the request manager is already
   * initialized and that the request handler runner is already set.
   * The thread will run until the m_stop_handler_thread variable is set to
   * true.
   */
  void start() {
    m_handler_thread = std::thread([this]() {
      while (!m_stop_handler_thread) {
        tx_request_handler_runner &runner = m_handler_runner;
        std::this_thread::sleep_for(std::chrono::seconds(m_request_deadline));
        m_request_manager.for_each_request(runner, m_request_deadline);
      }
    });
  }

  void deinit() {
    m_stop_handler_thread.store(true);
    if (m_handler_thread.joinable()) {
      m_handler_thread.join();
    }
  }

  void update_request_deadline(std::time_t request_deadline) {
    m_request_deadline = request_deadline;
  }

  std::time_t get_request_deadline() const { return m_request_deadline; }

private:
  // Thread for checking the transaction requests
  std::thread m_handler_thread;
  std::atomic<bool> m_stop_handler_thread{false};
  std::atomic<std::time_t> m_request_deadline;
  tx_request_handler_runner &m_handler_runner;
  request_manager &m_request_manager;
};

#endif // CRYPTONOTE_PROTOCOL_TXREQUESTHANDLER_H