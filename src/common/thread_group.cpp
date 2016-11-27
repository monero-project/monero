// Copyright (c) 2014-2016, The Monero Project
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
#include "common/thread_group.h"

#include <boost/thread/locks.hpp>
#include <cassert>
#include <limits>
#include <stdexcept>

#include "common/util.h"

namespace tools
{
std::size_t thread_group::optimal() {
  static_assert(
    std::numeric_limits<unsigned>::max() <= std::numeric_limits<std::size_t>::max(), 
    "unexpected truncation"
  );
  const std::size_t hardware = get_max_concurrency();
  return hardware ? (hardware - 1) : 0;
}

std::size_t thread_group::optimal_with_max(std::size_t count) {
  return count ? std::min(count - 1, optimal()) : 0;
}

thread_group::thread_group(std::size_t count) : internal() {
  if (count) {
    internal.emplace(count);
  }
}

thread_group::data::data(std::size_t count)
  : threads()
  , head{nullptr}
  , last(std::addressof(head))
  , mutex()
  , has_work()
  , stop(false) {
  threads.reserve(count);
  while (count--) {
    threads.push_back(boost::thread(&thread_group::data::run, this));
  }
}

thread_group::data::~data() noexcept {
  {
    const boost::unique_lock<boost::mutex> lock(mutex);
    stop = true;
  }
  has_work.notify_all();
  for (auto& worker : threads) {
    try {
      worker.join();
    }
    catch(...) {}
  }
}

std::unique_ptr<thread_group::data::work> thread_group::data::get_next() noexcept {
  std::unique_ptr<work> rc = std::move(head.ptr);
  if (rc != nullptr) {
    head.ptr = std::move(rc->next.ptr);
    if (head.ptr == nullptr) {
      last = std::addressof(head);
    }
  }
  return rc;
}

bool thread_group::data::try_run_one() noexcept {
  /* This function and `run()` can both throw when acquiring the lock, or in
  dispatched function. It is tough to recover from either, particularly the
  lock case. These functions are marked as noexcept so that if either call
  throws, the entire process is terminated. Users of the `dispatch` call are
  expected to make their functions noexcept, or use std::packaged_task to copy
  exceptions so that the process will continue in all but the most pessimistic
  cases (std::bad_alloc). This was the existing behavior;
  `asio::io_service::run` propogates errors from dispatched calls, and uncaught
  exceptions on threads result in process termination. */
  std::unique_ptr<work> next = nullptr;
  {
    const boost::unique_lock<boost::mutex> lock(mutex);
    next = get_next();
  }
  if (next) {
    assert(next->f);
    next->f();
    return true;
  }
  return false;
}

void thread_group::data::run() noexcept {
  // see `try_run_one()` source for additional information
  while (true) {
    std::unique_ptr<work> next = nullptr;
    {
      boost::unique_lock<boost::mutex> lock(mutex);
      has_work.wait(lock, [this] { return head.ptr != nullptr || stop; });
      if (stop) {
        return;
      }
      next = get_next();
    }
    assert(next != nullptr);
    assert(next->f);
    next->f();
  }
}

void thread_group::data::dispatch(std::function<void()> f) {
  std::unique_ptr<work> latest(new work{std::move(f), node{nullptr}});
  node* const latest_node = std::addressof(latest->next);
  {
    const boost::unique_lock<boost::mutex> lock(mutex);
    assert(last != nullptr);
    assert(last->ptr == nullptr);

    last->ptr = std::move(latest);
    last = latest_node;
  }
  has_work.notify_one(); 
}
}
