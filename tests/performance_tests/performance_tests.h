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

#include "misc_language.h"
#include "common/perf_timer.h"

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

struct Params
{
  bool verbose;
  bool stats;
  unsigned loop_multiplier;
};

template <typename T>
class test_runner
{
public:
  test_runner(const Params &params)
    : m_elapsed(0)
    , m_params(params)
    , m_per_call_timers(T::loop_count * params.loop_multiplier, {true})
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
    if (m_params.verbose)
      std::cout << "Warm up: " << timer.elapsed_ms() << " ms" << std::endl;

    timer.start();
    for (size_t i = 0; i < T::loop_count * m_params.loop_multiplier; ++i)
    {
      if (m_params.stats)
        m_per_call_timers[i].resume();
      if (!test.test())
        return false;
      if (m_params.stats)
        m_per_call_timers[i].pause();
    }
    m_elapsed = timer.elapsed_ms();

    return true;
  }

  int elapsed_time() const { return m_elapsed; }

  int time_per_call(int scale = 1) const
  {
    static_assert(0 < T::loop_count, "T::loop_count must be greater than 0");
    return m_elapsed * scale / (T::loop_count * m_params.loop_multiplier);
  }

  uint64_t per_call_min() const
  {
    uint64_t v = std::numeric_limits<uint64_t>::max();
    for (const auto &pt: m_per_call_timers)
      v = std::min(v, pt.value());
    return v;
  }

  uint64_t per_call_max() const
  {
    uint64_t v = std::numeric_limits<uint64_t>::min();
    for (const auto &pt: m_per_call_timers)
      v = std::max(v, pt.value());
    return v;
  }

  uint64_t per_call_mean() const
  {
    uint64_t v = 0;
    for (const auto &pt: m_per_call_timers)
      v += pt.value();
    return v / m_per_call_timers.size();
  }

  uint64_t per_call_median() const
  {
    std::vector<uint64_t> values;
    values.reserve(m_per_call_timers.size());
    for (const auto &pt: m_per_call_timers)
      values.push_back(pt.value());
    return epee::misc_utils::median(values);
  }

  uint64_t per_call_stddev() const
  {
    if (m_per_call_timers.size() <= 1)
      return 0;
    const uint64_t mean = per_call_mean();
    uint64_t acc = 0;
    for (const auto &pt: m_per_call_timers)
    {
      int64_t dv = pt.value() - mean;
      acc += dv * dv;
    }
    acc /= m_per_call_timers.size () - 1;
    return sqrt(acc);
  }

  uint64_t min_time_ns() const { return tools::ticks_to_ns(per_call_min()); }
  uint64_t max_time_ns() const { return tools::ticks_to_ns(per_call_max()); }
  uint64_t median_time_ns() const { return tools::ticks_to_ns(per_call_median()); }
  uint64_t standard_deviation_time_ns() const { return tools::ticks_to_ns(per_call_stddev()); }

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
  Params m_params;
  std::vector<tools::PerformanceTimer> m_per_call_timers;
};

template <typename T>
void run_test(const std::string &filter, const Params &params, const char* test_name)
{
  boost::smatch match;
  if (!filter.empty() && !boost::regex_match(std::string(test_name), match, boost::regex(filter)))
    return;

  test_runner<T> runner(params);
  if (runner.run())
  {
    if (params.verbose)
    {
      std::cout << test_name << " - OK:\n";
      std::cout << "  loop count:    " << T::loop_count * params.loop_multiplier << '\n';
      std::cout << "  elapsed:       " << runner.elapsed_time() << " ms\n";
      if (params.stats)
      {
        std::cout << "  min:       " << runner.min_time_ns() << " ns\n";
        std::cout << "  max:       " << runner.max_time_ns() << " ns\n";
        std::cout << "  median:    " << runner.median_time_ns() << " ns\n";
        std::cout << "  std dev:   " << runner.standard_deviation_time_ns() << " ns\n";
      }
    }
    else
    {
      std::cout << test_name << " (" << T::loop_count * params.loop_multiplier << " calls) - OK:";
    }
    const char *unit = "ms";
    uint64_t scale = 1000000;
    int time_per_call = runner.time_per_call();
    if (time_per_call < 30000) {
     time_per_call = runner.time_per_call(1000);
#ifdef _WIN32
     unit = "\xb5s";
#else
     unit = "µs";
#endif
     scale = 1000;
    }
    std::cout << (params.verbose ? "  time per call: " : " ") << time_per_call << " " << unit << "/call" << (params.verbose ? "\n" : "");
    if (params.stats)
    {
      uint64_t min_ns = runner.min_time_ns() / scale;
      uint64_t med_ns = runner.median_time_ns() / scale;
      uint64_t stddev_ns = runner.standard_deviation_time_ns() / scale;
      std::cout << " (min " << min_ns << " " << unit << ", median " << med_ns << " " << unit << ", std dev " << stddev_ns << " " << unit << ")";
    }
    std::cout << std::endl;
  }
  else
  {
    std::cout << test_name << " - FAILED" << std::endl;
  }
}

#define QUOTEME(x) #x
#define TEST_PERFORMANCE0(filter, params, test_class)         run_test< test_class >(filter, params, QUOTEME(test_class))
#define TEST_PERFORMANCE1(filter, params, test_class, a0)     run_test< test_class<a0> >(filter, params, QUOTEME(test_class<a0>))
#define TEST_PERFORMANCE2(filter, params, test_class, a0, a1) run_test< test_class<a0, a1> >(filter, params, QUOTEME(test_class) "<" QUOTEME(a0) ", " QUOTEME(a1) ">")
#define TEST_PERFORMANCE3(filter, params, test_class, a0, a1, a2) run_test< test_class<a0, a1, a2> >(filter, params, QUOTEME(test_class) "<" QUOTEME(a0) ", " QUOTEME(a1) ", " QUOTEME(a2) ">")
#define TEST_PERFORMANCE4(filter, params, test_class, a0, a1, a2, a3) run_test< test_class<a0, a1, a2, a3> >(filter, params, QUOTEME(test_class) "<" QUOTEME(a0) ", " QUOTEME(a1) ", " QUOTEME(a2) ", " QUOTEME(a3) ">")
#define TEST_PERFORMANCE5(filter, params, test_class, a0, a1, a2, a3, a4) run_test< test_class<a0, a1, a2, a3, a4> >(filter, params, QUOTEME(test_class) "<" QUOTEME(a0) ", " QUOTEME(a1) ", " QUOTEME(a2) ", " QUOTEME(a3) ", " QUOTEME(a4) ">")
#define TEST_PERFORMANCE6(filter, params, test_class, a0, a1, a2, a3, a4, a5) run_test< test_class<a0, a1, a2, a3, a4, a5> >(filter, params, QUOTEME(test_class) "<" QUOTEME(a0) ", " QUOTEME(a1) ", " QUOTEME(a2) ", " QUOTEME(a3) ", " QUOTEME(a4) ", " QUOTEME(a5) ">")
