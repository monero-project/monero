// Copyright (c) 2014-2017, The Monero Project
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
#pragma once

#include <boost/optional/optional.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <cstddef>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

namespace tools 
{
//! Manages zero or more threads for work dispatching.
class thread_group
{
public:

  //! \return `get_max_concurrency() ? get_max_concurrency() - 1 : 0`
  static std::size_t optimal();

  //! \return `count ? min(count - 1, optimal()) : 0`
  static std::size_t optimal_with_max(std::size_t count);

  //! Create an optimal number of threads.
  explicit thread_group() : thread_group(optimal()) {}

  //! Create exactly `count` threads.
  explicit thread_group(std::size_t count);

  thread_group(thread_group const&) = delete;
  thread_group(thread_group&&) = delete;

  //! Joins threads, but does not necessarily run all dispatched functions.
  ~thread_group() = default;

  thread_group& operator=(thread_group const&) = delete;
  thread_group& operator=(thread_group&&) = delete;

  //! \return Number of threads owned by `this` group.
  std::size_t count() const noexcept {
    if (internal) {
      return internal->count();
    }
    return 0;
  }

  //! \return True iff a function was available and executed (on `this_thread`).
  bool try_run_one() noexcept {
    if (internal) {
      return internal->try_run_one();
    }
    return false;
  }

  /*! `f` is invoked immediately if `count() == 0`, otherwise execution of `f`
  is queued for next available thread. If `f` is queued, any exception leaving
  that function will result in process termination. Use std::packaged_task if
  exceptions need to be handled. */
  template<typename F>
  void dispatch(F&& f) {
    if (internal) {
      internal->dispatch(std::forward<F>(f));
    }
    else {
      f();
    }
  }

private:
  class data {
  public:
    data(std::size_t count);
    ~data() noexcept;

    std::size_t count() const noexcept {
      return threads.size();
    }

    bool try_run_one() noexcept;
    void dispatch(std::function<void()> f);

  private:
    struct work;

    struct node {
      std::unique_ptr<work> ptr;
    };

    struct work {
      std::function<void()> f;
      node next;
    };

    //! Requires lock on `mutex`.
    std::unique_ptr<work> get_next() noexcept;

    //! Blocks until destructor is invoked, only call from thread.
    void run() noexcept;

  private:
    std::vector<boost::thread> threads;
    node head;
    node* last;
    boost::condition_variable has_work;
    boost::mutex mutex;
    bool stop;
  };

private:
  // optionally construct elements, without separate heap allocation
  boost::optional<data> internal;
};

}
