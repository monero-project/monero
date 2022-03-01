// Copyright (c) 2014-2022, The Monero Project
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
#include "stats.h"
#include "common/perf_timer.h"
#include "common/timings.h"

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
  TimingsDatabase td;
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
    static_assert(0 < T::loop_count, "T::loop_count must be greater than 0");

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
    m_stats.reset(new Stats<tools::PerformanceTimer, uint64_t>(m_per_call_timers));

    return true;
  }

  int elapsed_time() const { return m_elapsed; }
  size_t get_size() const { return m_stats->get_size(); }

  int time_per_call(int scale = 1) const
  {
    static_assert(0 < T::loop_count, "T::loop_count must be greater than 0");
    return m_elapsed * scale / (T::loop_count * m_params.loop_multiplier);
  }

  uint64_t get_min() const { return m_stats->get_min(); }
  uint64_t get_max() const { return m_stats->get_max(); }
  double get_mean() const { return m_stats->get_mean(); }
  uint64_t get_median() const { return m_stats->get_median(); }
  double get_stddev() const { return m_stats->get_standard_deviation(); }
  double get_non_parametric_skew() const { return m_stats->get_non_parametric_skew(); }
  std::vector<uint64_t> get_quantiles(size_t n) const { return m_stats->get_quantiles(n); }

  bool is_same_distribution(size_t npoints, double mean, double stddev) const
  {
    return m_stats->is_same_distribution_99(npoints, mean, stddev);
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
  Params m_params;
  std::vector<tools::PerformanceTimer> m_per_call_timers;
  std::unique_ptr<Stats<tools::PerformanceTimer, uint64_t>> m_stats;
};

template <typename T>
void run_test(const std::string &filter, Params &params, const char* test_name)
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
        std::cout << "  min:       " << runner.get_min() << " ns\n";
        std::cout << "  max:       " << runner.get_max() << " ns\n";
        std::cout << "  median:    " << runner.get_median() << " ns\n";
        std::cout << "  std dev:   " << runner.get_stddev() << " ns\n";
      }
    }
    else
    {
      std::cout << test_name << " (" << T::loop_count * params.loop_multiplier << " calls) - OK:";
    }
    const char *unit = "ms";
    double scale = 1000000;
    uint64_t time_per_call = runner.time_per_call();
    if (time_per_call < 100) {
     scale = 1000;
     time_per_call = runner.time_per_call(1000);
#ifdef _WIN32
     unit = "us";
#else
     unit = "µs";
#endif
    }
    const auto quantiles = runner.get_quantiles(10);
    double min = runner.get_min();
    double max = runner.get_max();
    double med = runner.get_median();
    double mean = runner.get_mean();
    double stddev = runner.get_stddev();
    double npskew = runner.get_non_parametric_skew();

    std::vector<TimingsDatabase::instance> prev_instances = params.td.get(test_name);
    params.td.add(test_name, {time(NULL), runner.get_size(), min, max, mean, med, stddev, npskew, quantiles});

    std::cout << (params.verbose ? "  time per call: " : " ") << time_per_call << " " << unit << "/call" << (params.verbose ? "\n" : "");
    if (params.stats)
    {
      uint64_t mins = min / scale;
      uint64_t meds = med / scale;
      uint64_t p95s = quantiles[9] / scale;
      uint64_t stddevs = stddev / scale;
      std::string cmp;
      if (!prev_instances.empty())
      {
        const TimingsDatabase::instance &prev_instance = prev_instances.back();
        if (!runner.is_same_distribution(prev_instance.npoints, prev_instance.mean, prev_instance.stddev))
        {
          double pc = fabs(100. * (prev_instance.mean - runner.get_mean()) / prev_instance.mean);
          cmp = ", " + std::to_string(pc) + "% " + (mean > prev_instance.mean ? "slower" : "faster");
        }
cmp += "  -- " + std::to_string(prev_instance.mean);
      }
      std::cout << " (min " << mins << " " << unit << ", 90th " << p95s << " " << unit << ", median " << meds << " " << unit << ", std dev " << stddevs << " " << unit << ")" << cmp;
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
