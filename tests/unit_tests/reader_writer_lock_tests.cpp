// Copyright (c) 2018-2023, The Monero Project

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

#include "gtest/gtest.h"
#include "syncobj.h"
#include "boost/thread.hpp"

#include <random>

// number of times each test runs
int test_iteration = 2;

// total number of threads 
size_t total_number_of_threads;
// Between 20-80% of the total number of threads.
size_t number_of_writers;
// Remaining number of threads.
size_t number_of_readers;

// Cycles that readers are going to hold the lock. between 2-100 cycles
constexpr size_t reading_cycles_min = 2;
constexpr size_t reading_cycles_max = 25;
// Duration that readers are going to hold the lock. between 5-100 milliseconds
constexpr size_t reading_step_duration_min = 2;
constexpr size_t reading_step_duration_max = 10;

// Cycles that writers are going to hold the lock, between 2-100 cycles
constexpr size_t writing_cycles_min = 2;
constexpr size_t writing_cycles_max = 25;
// Duration that writers are going to hold the lock. between 5-100 milliseconds
constexpr size_t writing_step_duration_min = 2;
constexpr size_t writing_step_duration_max = 10;

epee::reader_writer_lock main_lock;

void calculate_parameters(size_t threads) {
  total_number_of_threads = threads;
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> d( total_number_of_threads * 0.2 , total_number_of_threads * 0.8); 

  number_of_writers = d(rng);
  number_of_readers = total_number_of_threads - number_of_writers;
}

void reader() {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> d(reading_cycles_min, reading_cycles_max); 
  size_t reading_cycles = d(rng);

  d = std::uniform_int_distribution<std::mt19937::result_type>(reading_step_duration_min, reading_step_duration_max);  
  bool release_required = main_lock.start_read();
  for(int i = 0; i < reading_cycles; i++) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(d(rng)));
  }
  bool recurse = !((bool) std::uniform_int_distribution<std::mt19937::result_type>(0, 10)(rng) % 4); // ~30%
  if (recurse) {
    reader();
  }  
  if (release_required) main_lock.end_read();
}

void writer() {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> d(writing_cycles_min, writing_cycles_max); 
  size_t writing_cycles = d(rng);

  d = std::uniform_int_distribution<std::mt19937::result_type>(writing_step_duration_min, writing_step_duration_max);  
  bool release_required = main_lock.start_write();
  for(int i = 0; i < writing_cycles; i++) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(d(rng)));
  }
  bool recurse = !((bool) std::uniform_int_distribution<std::mt19937::result_type>(0, 10)(rng) % 4); // ~20%
  if (recurse) {
    bool which = std::uniform_int_distribution<std::mt19937::result_type>(0, 10)(rng) % 2;
    if (which) {
      writer();
    }
    else {
      reader();
    }
  }  
  if (release_required) main_lock.end_write();
}

void RUN_TEST() {
  std::vector<boost::thread> threads;
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> d(0, 10); 

  int reader_count = 0;
  int writer_count = 0;
  while(reader_count  < number_of_readers 
        || writer_count  < number_of_writers ) {
    bool which_one = d(rng) % 2;
    if(which_one) {
      threads.push_back(boost::thread(reader)); reader_count++;
    }
    else {
      threads.push_back(boost::thread(writer)); writer_count++;
    }
  }

  std::for_each(threads.begin(), threads.end(), [] (boost::thread& thread) {
    if (thread.joinable()) thread.join();
  });
}

TEST(reader_writer_lock_tests, test_10)
{
  calculate_parameters(10);
  for(int i = 0; i < test_iteration; ++i)
    RUN_TEST();
}

TEST(reader_writer_lock_tests, test_100)
{
  calculate_parameters(100);
  for(int i = 0; i < test_iteration; ++i)
    RUN_TEST();
}

TEST(reader_writer_lock_tests, test_500)
{
  calculate_parameters(500);
  for(int i = 0; i < test_iteration; ++i)
    RUN_TEST();
}

TEST(reader_writer_lock_tests, test_1000)
{
  calculate_parameters(1000);
  for(int i = 0; i < test_iteration; ++i)
    RUN_TEST();
}
