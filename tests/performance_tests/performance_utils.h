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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <iostream>

#include <boost/config.hpp>

#ifdef BOOST_WINDOWS
#include <windows.h>
#endif

void set_process_affinity(int core)
{
#if defined (__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    return;
#elif defined(BOOST_WINDOWS)
  DWORD_PTR mask = 1;
  for (int i = 0; i < core; ++i)
  {
    mask <<= 1;
  }
  ::SetProcessAffinityMask(::GetCurrentProcess(), core);
#elif defined(BOOST_HAS_PTHREADS)
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  if (0 != ::pthread_setaffinity_np(::pthread_self(), sizeof(cpuset), &cpuset))
  {
    std::cout << "pthread_setaffinity_np - ERROR" << std::endl;
  }
#endif
}

void set_thread_high_priority()
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    return;
#elif defined(BOOST_WINDOWS)
  ::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#elif defined(BOOST_HAS_PTHREADS)
  pthread_attr_t attr;
  int policy = 0;
  int max_prio_for_policy = 0;

  ::pthread_attr_init(&attr);
  ::pthread_attr_getschedpolicy(&attr, &policy);
  max_prio_for_policy = ::sched_get_priority_max(policy);

  if (0 != ::pthread_setschedprio(::pthread_self(), max_prio_for_policy))
  {
    std::cout << "pthread_setschedprio - ERROR" << std::endl;
  }

  ::pthread_attr_destroy(&attr);
#endif
}
