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

#include <atomic>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <memory>
#include <type_traits>
#include <utility>

#include "common/thread_group.h"

namespace tools
{

/*! A model of the fork-join concept. `run(...)` "forks" (i.e. spawns new
tasks), and `~task_region_handle()` or `wait()` "joins" the spawned tasks.
`wait` will block until all tasks have completed, while `~task_region_handle()`
blocks until all tasks have completed or aborted.

Do _NOT_ give this object to separate thread of execution (which includes
`task_region_handle::run(...)`) because joining on a different thread is
undesireable (potential deadlock). 

This class cannot be constructed directly, use the function
`task_region(...)` instead.
*/
class task_region_handle
{
  struct state
  {
    using id = unsigned;

    explicit state(std::shared_ptr<state> next_src) noexcept
      : next(std::move(next_src))
      , ready(0)
      , pending(0)
      , sync_on_complete()
      , all_complete() {
    }
      
    state(const state&) = default;
    state(state&&) = default;
    ~state() = default;
    state& operator=(const state&) = default;
    state& operator=(state&&) = default;

    void track_id(id task_id) noexcept {
      pending |= task_id;
      ready |= task_id;
    }

    //! \return True only once whether a given id can execute
    bool can_run(id task_id) noexcept {
      return (ready.fetch_and(~task_id) & task_id);
    }

    //! Mark id as completed, and synchronize with waiting threads
    void mark_completed(id task_id) noexcept;

    //! Tell all unstarted functions in region to return immediately
    void abort() noexcept;

    //! Blocks until all functions in region have aborted or completed.
    void wait() noexcept;

    //! Same as `wait()`, except `this_thread` runs tasks while waiting.
    void wait(thread_group& threads) noexcept;

  private:
    /* This implementation is a bit pessimistic, it ensures that all copies
    of a wrapped task can only be executed once. `thread_group` should never
    do this, but some variable needs to track whether an abort should be done
    anyway... */
    std::shared_ptr<state> next;
    std::atomic<id> ready;   //!< Tracks whether a task has been invoked
    std::atomic<id> pending; //!< Tracks when a task has completed or aborted
    boost::mutex sync_on_complete;
    boost::condition_variable all_complete;
  };

  template<typename F>
  struct wrapper
  {
    wrapper(state::id id_src, std::shared_ptr<state> st_src, F f_src)
      : task_id(id_src), st(std::move(st_src)), f(std::move(f_src)) {
    }

    wrapper(const wrapper&) = default;
    wrapper(wrapper&&) = default;
    wrapper& operator=(const wrapper&) = default;
    wrapper& operator=(wrapper&&) = default;

    void operator()() {
      if (st) {
        if (st->can_run(task_id)) {
          f();
        }
        st->mark_completed(task_id);
      }
    }

  private:
    const state::id task_id;
    std::shared_ptr<state> st;
    F f;
  };

public:
  friend struct task_region_;

  task_region_handle() = delete;
  task_region_handle(const task_region_handle&) = delete;
  task_region_handle(task_region_handle&&) = delete;

  //! Cancels unstarted pending tasks, and waits for them to respond.
  ~task_region_handle() noexcept {
    if (st) {
      st->abort();
      st->wait(threads);
    }
  }

  task_region_handle& operator=(const task_region_handle&) = delete;
  task_region_handle& operator=(task_region_handle&&) = delete;

  /*! If the group has no threads, `f` is immediately run before returning.
  Otherwise, `f` is dispatched to the thread_group associated with `this`
  region. If `f` is dispatched to another thread, and it throws, the process
  will immediately terminate. See std::packaged_task for getting exceptions on
  functions executed on other threads. */
  template<typename F>
  void run(F&& f) {
    if (threads.count() == 0) {
      f();
    } else {
      if (!st || next_id == 0) {
        create_state();
      }
      const state::id this_id = next_id;
      next_id <<= 1;

      st->track_id(this_id);
      threads.dispatch(wrapper<F>{this_id, st, std::move(f)});
    }
  }

  //! Wait until all functions provided to `run` have completed.
  void wait() noexcept {
    if (st) {
      do_wait();
    }
  }
 
private:
  explicit task_region_handle(thread_group& threads_src)
    : st(nullptr), threads(threads_src), next_id(0) {
  }

  void create_state();
  void do_wait() noexcept;

  std::shared_ptr<state> st;
  thread_group& threads;
  state::id next_id;
};

/*! Function for creating a `task_region_handle`, which automatically calls
`task_region_handle::wait()` before returning. If a `thread_group` is not
provided, one is created with an optimal number of threads. The callback `f`
must have the signature `void(task_region_handle&)`. */
struct task_region_ {
  template<typename F>
  void operator()(thread_group& threads, F&& f) const {
    static_assert(
      std::is_same<void, typename std::result_of<F(task_region_handle&)>::type>::value,
      "f cannot have a return value"
    );
    task_region_handle region{threads};
    f(region);
    region.wait();
  }

  template<typename F>
  void operator()(thread_group&& threads, F&& f) const {
    (*this)(threads, std::forward<F>(f));
  }

  template<typename F>
  void operator()(F&& f) const {
    thread_group threads;
    (*this)(threads, std::forward<F>(f));
  }
};

constexpr const task_region_ task_region{};
}
