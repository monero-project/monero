// Copyright (c) 2017-2025, The Monero Project
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

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <cstddef>
#include <deque>
#include <functional>
#include <utility>
#include <vector>
#include <stdexcept>

namespace tools
{
//! A global thread pool
class threadpool
{
public:
  static threadpool& getInstanceForCompute() {
    static threadpool instance;
    return instance;
  }
  static threadpool& getInstanceForIO() {
    static threadpool instance(8);
    return instance;
  }
  static threadpool *getNewForUnitTests(unsigned max_threads = 0) {
    return new threadpool(max_threads);
  }

  // The waiter lets the caller know when all of its
  // tasks are completed.
  class waiter {
    boost::mutex mt;
    boost::condition_variable cv;
    threadpool &pool;
    int num;
    bool error_flag;
    public:
    void inc();
    void dec();
    bool wait();  //! Wait for a set of tasks to finish, returns false iff any error
    void set_error() noexcept { error_flag = true; }
    bool error() const noexcept { return error_flag; }
    waiter(threadpool &pool) : pool(pool), num(0), error_flag(false) {}
    ~waiter();
  };

  // Submit a task to the pool. The waiter pointer may be
  // NULL if the caller doesn't care to wait for the
  // task to finish.
  void submit(waiter *waiter, std::function<void()> f, bool leaf = false);

  // destroy and recreate threads
  void recycle();

  unsigned int get_max_concurrency() const;

  ~threadpool();

  private:
    threadpool(unsigned int max_threads = 0);
    void destroy();
    void create(unsigned int max_threads);
    typedef struct entry {
      waiter *wo;
      std::function<void()> f;
      bool leaf;
    } entry;
    std::deque<entry> queue;
    boost::condition_variable has_work;
    boost::mutex mutex;
    std::vector<boost::thread> threads;
    unsigned int active;
    unsigned int max;
    bool running;
    void run(bool flush = false);
};

}
