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
#include "common/task_region.h"

#include <boost/thread/locks.hpp>
#include <cassert>

/* `mark_completed` and `wait` can throw in the lock call, but its difficult to
recover from either. An exception in `wait` means the post condition of joining
all threads cannot be achieved, and an exception in `mark_completed` means
certain deadlock. `noexcept` qualifier will force a call to `std::terminate` if
locking throws an exception, which should only happen if a recursive lock
attempt is made (which is not possible since no external function is called
while holding the lock). */

namespace tools
{
void task_region_handle::state::mark_completed(id task_id) noexcept {
  assert(task_id != 0 && (task_id & (task_id - 1)) == 0); // power of 2 check
  if (pending.fetch_and(~task_id) == task_id) {
    // synchronize with wait call, but do not need to hold
    boost::unique_lock<boost::mutex>{sync_on_complete};
    all_complete.notify_all();
  }
}

void task_region_handle::state::abort() noexcept {
  state* current = this;
  while (current) {
    current->ready = 0;
    current = current->next.get();
  }
}

void task_region_handle::state::wait() noexcept {
  state* current = this;
  while (current) {
    {
      boost::unique_lock<boost::mutex> lock{current->sync_on_complete};
      current->all_complete.wait(lock, [current] { return current->pending == 0; });
    }
    current = current->next.get();
  }
}

void task_region_handle::state::wait(thread_group& threads) noexcept {
  state* current = this;
  while (current) {
    while (current->pending != 0) {
      if (!threads.try_run_one()) {
        current->wait();
        return;
      }
    }
    current = current->next.get();
  }
}

void task_region_handle::create_state() {
  st = std::make_shared<state>(std::move(st));
  next_id = 1;
}

void task_region_handle::do_wait() noexcept {
  assert(st);
  const std::shared_ptr<state> temp = std::move(st);
  temp->wait(threads);
}
}
