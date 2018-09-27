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
static __thread bool is_leaf = false;

namespace tools
{
threadpool::threadpool(unsigned int max_threads) : running(true), active(0) {
  boost::thread::attributes attrs;
  attrs.set_stack_size(THREAD_STACK_SIZE);
  max = max_threads ? max_threads : tools::get_max_concurrency();
  size_t i = max ? max - 1 : 0;
  while(i--) {
    threads.push_back(boost::thread(attrs, boost::bind(&threadpool::run, this, false)));
  }
}

threadpool::~threadpool() {
  {
    const boost::unique_lock<boost::mutex> lock(mutex);
    running = false;
    has_work.notify_all();
  }
  for (size_t i = 0; i<threads.size(); i++) {
    try { threads[i].join(); }
    catch (...) { /* ignore */ }
  }
}

void threadpool::submit(waiter *obj, std::function<void()> f, bool leaf) {
  CHECK_AND_ASSERT_THROW_MES(!is_leaf, "A leaf routine is using a thread pool");
  boost::unique_lock<boost::mutex> lock(mutex);
  if (!leaf && ((active == max && !queue.empty()) || depth > 0)) {
    // if all available threads are already running
    // and there's work waiting, just run in current thread
    lock.unlock();
    ++depth;
    is_leaf = leaf;
    f();
    --depth;
    is_leaf = false;
  } else {
    if (obj)
      obj->inc();
    if (leaf)
      queue.push_front({obj, f, leaf});
    else
      queue.push_back({obj, f, leaf});
    has_work.notify_one();
  }
}

unsigned int threadpool::get_max_concurrency() const {
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
    wait(NULL);
  }
  catch (const std::exception &e)
  {
    /* ignored */
  }
}

void threadpool::waiter::wait(threadpool *tpool) {
  if (tpool)
    tpool->run(true);
  boost::unique_lock<boost::mutex> lock(mt);
  while(num)
    cv.wait(lock);
}

void threadpool::waiter::inc() {
  const boost::unique_lock<boost::mutex> lock(mt);
  num++;
}

void threadpool::waiter::dec() {
  const boost::unique_lock<boost::mutex> lock(mt);
  num--;
  if (!num)
    cv.notify_all();
}

void threadpool::run(bool flush) {
  boost::unique_lock<boost::mutex> lock(mutex);
  while (running) {
    entry e;
    while(queue.empty() && running)
    {
      if (flush)
        return;
      has_work.wait(lock);
    }
    if (!running) break;

    active++;
    e = queue.front();
    queue.pop_front();
    lock.unlock();
    ++depth;
    is_leaf = e.leaf;
    e.f();
    --depth;
    is_leaf = false;

    if (e.wo)
      e.wo->dec();
    lock.lock();
    active--;
  }
}
}
