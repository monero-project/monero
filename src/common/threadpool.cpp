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
#include "misc_log_ex.h"
#include "common/threadpool.h"

#include <cassert>
#include <limits>
#include <stdexcept>

#include "cryptonote_config.h"
#include "common/util.h"

static __thread int depth = 0;

namespace tools
{
threadpool::threadpool() : running(true), active(0) {
  boost::thread::attributes attrs;
  attrs.set_stack_size(THREAD_STACK_SIZE);
  max = tools::get_max_concurrency();
  size_t i = max;
  while(i--) {
    threads.push_back(boost::thread(attrs, boost::bind(&threadpool::run, this)));
  }
}

threadpool::~threadpool() {
  {
    const boost::unique_lock<boost::mutex> lock(mutex);
    running = false;
    has_work.notify_all();
  }
  for (size_t i = 0; i<threads.size(); i++) {
    threads[i].join();
  }
}

void threadpool::submit(waiter *obj, std::function<void()> f) {
  entry e = {obj, f};
  boost::unique_lock<boost::mutex> lock(mutex);
  if ((active == max && !queue.empty()) || depth > 0) {
    // if all available threads are already running
    // and there's work waiting, just run in current thread
    lock.unlock();
    ++depth;
    f();
    --depth;
  } else {
    if (obj)
      obj->inc();
    queue.push_back(e);
    has_work.notify_one();
  }
}

int threadpool::get_max_concurrency() {
  return max;
}

threadpool::waiter::~waiter()
{
  {
    boost::unique_lock<boost::mutex> lock(mt);
    if (num)
      MERROR("wait should have been called before waiter dtor - waiting now");
  }
  try
  {
    wait();
  }
  catch (const std::exception &e)
  {
    /* ignored */
  }
}

void threadpool::waiter::wait() {
  boost::unique_lock<boost::mutex> lock(mt);
  while(num) cv.wait(lock);
}

void threadpool::waiter::inc() {
  const boost::unique_lock<boost::mutex> lock(mt);
  num++;
}

void threadpool::waiter::dec() {
  const boost::unique_lock<boost::mutex> lock(mt);
  num--;
  if (!num)
    cv.notify_one();
}

void threadpool::run() {
  boost::unique_lock<boost::mutex> lock(mutex);
  while (running) {
    entry e;
    while(queue.empty() && running)
      has_work.wait(lock);
    if (!running) break;

    active++;
    e = queue.front();
    queue.pop_front();
    lock.unlock();
    ++depth;
    e.f();
    --depth;

    if (e.wo)
      e.wo->dec();
    lock.lock();
    active--;
  }
}
}
