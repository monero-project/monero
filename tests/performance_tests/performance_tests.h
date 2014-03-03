// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <iostream>
#include <stdint.h>

#include <boost/chrono.hpp>

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

  int time_per_call() const
  {
    static_assert(0 < T::loop_count, "T::loop_count must be greater than 0");
    return m_elapsed / T::loop_count;
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
void run_test(const char* test_name)
{
  test_runner<T> runner;
  if (runner.run())
  {
    std::cout << test_name << " - OK:\n";
    std::cout << "  loop count:    " << T::loop_count << '\n';
    std::cout << "  elapsed:       " << runner.elapsed_time() << " ms\n";
    std::cout << "  time per call: " << runner.time_per_call() << " ms/call\n" << std::endl;
  }
  else
  {
    std::cout << test_name << " - FAILED" << std::endl;
  }
}

#define QUOTEME(x) #x
#define TEST_PERFORMANCE0(test_class)         run_test< test_class >(QUOTEME(test_class))
#define TEST_PERFORMANCE1(test_class, a0)     run_test< test_class<a0> >(QUOTEME(test_class<a0>))
#define TEST_PERFORMANCE2(test_class, a0, a1) run_test< test_class<a0, a1> >(QUOTEME(test_class) "<" QUOTEME(a0) ", " QUOTEME(a1) ">")
