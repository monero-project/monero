// Copyright (c) 2014-2018, The Monero Project
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
#include <stdint.h>

#include <boost/chrono.hpp>
#include <boost/regex.hpp>

class performance_timer
{
public:
  typedef boost::chrono::high_resolution_clock clock;

  performance_timer()
  {
    m_base = clock::now();
  }

  void start()
  {
    m_start = clock::now();
  }

  int elapsed_ms()
  {
    clock::duration elapsed = clock::now() - m_start;
    return static_cast<int>(boost::chrono::duration_cast<boost::chrono::milliseconds>(elapsed).count());
  }

private:
  clock::time_point m_base;
  clock::time_point m_start;
};


template <typename T>
class test_runner
{
public:
  test_runner()
    : m_elapsed(0)
  {
  }

  bool run()
  {
    T test;
    if (!test.init())
      return false;

    performance_timer timer;
    timer.start();
    warm_up();
    std::cout << "Warm up: " << timer.elapsed_ms() << " ms" << std::endl;

    timer.start();
    for (size_t i = 0; i < T::loop_count; ++i)
    {
      if (!test.test())
        return false;
    }
    m_elapsed = timer.elapsed_ms();

    return true;
  }

  int elapsed_time() const { return m_elapsed; }

  int time_per_call(int scale = 1) const
  {
    static_assert(0 < T::loop_count, "T::loop_count must be greater than 0");
    return m_elapsed * scale / T::loop_count;
  }

private:
  /**
   * Warm up processor core, enabling turbo boost, etc.
   */
  uint64_t warm_up()
  {
    const size_t warm_up_rounds = 1000 * 1000 * 1000;
    m_warm_up = 0;
    for (size_t i = 0; i < warm_up_rounds; ++i)
    {
      ++m_warm_up;
    }
    return m_warm_up;
  }

private:
  volatile uint64_t m_warm_up;  ///<! This field is intended for preclude compiler optimizations
  int m_elapsed;
};

template <typename T>
void run_test(const std::string &filter, const char* test_name)
{
  boost::smatch match;
  if (!filter.empty() && !boost::regex_match(std::string(test_name), match, boost::regex(filter)))
    return;

  test_runner<T> runner;
  if (runner.run())
  {
    std::cout << test_name << " - OK:\n";
    std::cout << "  loop count:    " << T::loop_count << '\n';
    std::cout << "  elapsed:       " << runner.elapsed_time() << " ms\n";
    const char *unit = "ms";
    int time_per_call = runner.time_per_call();
    if (time_per_call < 30000) {
     time_per_call = runner.time_per_call(1000);
#ifdef _WIN32
     unit = "\xb5s";
#else
     unit = "µs";
#endif
    }
    std::cout << "  time per call: " << time_per_call << " " << unit << "/call\n" << std::endl;
  }
  else
  {
    std::cout << test_name << " - FAILED" << std::endl;
  }
}

#define QUOTEME(x) #x
#define TEST_PERFORMANCE0(filter, test_class)         run_test< test_class >(filter, QUOTEME(test_class))
#define TEST_PERFORMANCE1(filter, test_class, a0)     run_test< test_class<a0> >(filter, QUOTEME(test_class<a0>))
#define TEST_PERFORMANCE2(filter, test_class, a0, a1) run_test< test_class<a0, a1> >(filter, QUOTEME(test_class) "<" QUOTEME(a0) ", " QUOTEME(a1) ">")
#define TEST_PERFORMANCE3(filter, test_class, a0, a1, a2) run_test< test_class<a0, a1, a2> >(filter, QUOTEME(test_class) "<" QUOTEME(a0) ", " QUOTEME(a1) ", " QUOTEME(a2) ">")
