// Copyright (c) 2018-2022, The Monero Project

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

#include "misc_log_ex.h"
#include "mlocker.h"

#if defined __GNUC__ && !defined _WIN32
#define HAVE_MLOCK 1
#endif

#ifdef HAVE_MLOCK

#define BASE(data) (char*)(((uintptr_t)(data.get() + page_size - 1)) / page_size * page_size)

TEST(mlocker, distinct_1)
{
  const size_t page_size = epee::mlocker::get_page_size();
  ASSERT_TRUE(page_size > 0);
  const size_t base_pages = epee::mlocker::get_num_locked_pages();
  const size_t base_objects = epee::mlocker::get_num_locked_objects();
  std::unique_ptr<char[]> data{new char[8 * page_size]};
  std::shared_ptr<epee::mlocker> m0{new epee::mlocker(BASE(data), 1)};
  std::shared_ptr<epee::mlocker> m1{new epee::mlocker(BASE(data) + 2 * page_size, 1)};
  std::shared_ptr<epee::mlocker> m2{new epee::mlocker(BASE(data) + 3 * page_size, 1)};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 3);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 3);
  m0 = NULL;
  m1 = NULL;
  m2 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 0);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 0);
}

TEST(mlocker, distinct_full_page)
{
  const size_t page_size = epee::mlocker::get_page_size();
  ASSERT_TRUE(page_size > 0);
  const size_t base_pages = epee::mlocker::get_num_locked_pages();
  const size_t base_objects = epee::mlocker::get_num_locked_objects();
  std::unique_ptr<char[]> data{new char[8 * page_size]};
  std::shared_ptr<epee::mlocker> m0{new epee::mlocker(BASE(data), page_size)};
  std::shared_ptr<epee::mlocker> m1{new epee::mlocker(BASE(data) + 2 * page_size, page_size)};
  std::shared_ptr<epee::mlocker> m2{new epee::mlocker(BASE(data) + 3 * page_size, page_size)};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 3);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 3);
  m0 = NULL;
  m1 = NULL;
  m2 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 0);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 0);
}

TEST(mlocker, identical)
{
  const size_t page_size = epee::mlocker::get_page_size();
  ASSERT_TRUE(page_size >= 32);
  const size_t base_pages = epee::mlocker::get_num_locked_pages();
  const size_t base_objects = epee::mlocker::get_num_locked_objects();
  std::unique_ptr<char[]> data{new char[8 * page_size]};
  std::shared_ptr<epee::mlocker> m0{new epee::mlocker(BASE(data) + page_size, 32)};
  std::shared_ptr<epee::mlocker> m1{new epee::mlocker(BASE(data) + page_size, 32)};
  std::shared_ptr<epee::mlocker> m2{new epee::mlocker(BASE(data) + page_size, 32)};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 1);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 3);
  m1 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 1);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 2);
  m0 = NULL;
  m2 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 0);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 0);
}

TEST(mlocker, overlapping_small)
{
  const size_t page_size = epee::mlocker::get_page_size();
  ASSERT_TRUE(page_size >= 64);
  const size_t base_pages = epee::mlocker::get_num_locked_pages();
  const size_t base_objects = epee::mlocker::get_num_locked_objects();
  std::unique_ptr<char[]> data{new char[8 * page_size]};
  std::shared_ptr<epee::mlocker> m0{new epee::mlocker(BASE(data), 32)};
  std::shared_ptr<epee::mlocker> m1{new epee::mlocker(BASE(data) + 16, 32)};
  std::shared_ptr<epee::mlocker> m2{new epee::mlocker(BASE(data) + 8, 32)};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 1);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 3);
  m1 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 1);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 2);
  m2 = NULL;
  m0 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 0);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 0);
}

TEST(mlocker, multi_page)
{
  const size_t page_size = epee::mlocker::get_page_size();
  ASSERT_TRUE(page_size > 0);
  const size_t base_pages = epee::mlocker::get_num_locked_pages();
  const size_t base_objects = epee::mlocker::get_num_locked_objects();
  std::unique_ptr<char[]> data{new char[8 * page_size]};
  std::shared_ptr<epee::mlocker> m0{new epee::mlocker(BASE(data) + page_size, page_size * 3)};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 3);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 1);
  std::shared_ptr<epee::mlocker> m1{new epee::mlocker(BASE(data) + page_size * 7, page_size)};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 4);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 2);
  m0 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 1);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 1);
  m1 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 0);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 0);
}

TEST(mlocker, cross_page)
{
  const size_t page_size = epee::mlocker::get_page_size();
  ASSERT_TRUE(page_size > 32);
  const size_t base_pages = epee::mlocker::get_num_locked_pages();
  const size_t base_objects = epee::mlocker::get_num_locked_objects();
  std::unique_ptr<char[]> data{new char[2 * page_size]};
  std::shared_ptr<epee::mlocker> m0{new epee::mlocker(BASE(data) + page_size - 1, 2)};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 2);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 1);
  m0 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 0);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 0);
}

TEST(mlocker, redundant)
{
  const size_t page_size = epee::mlocker::get_page_size();
  ASSERT_TRUE(page_size > 0);
  const size_t base_pages = epee::mlocker::get_num_locked_pages();
  const size_t base_objects = epee::mlocker::get_num_locked_objects();
  std::unique_ptr<char[]> data{new char[2 * page_size]};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 0);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 0);
  std::shared_ptr<epee::mlocker> m0{new epee::mlocker(BASE(data), 32)};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 1);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 1);
  std::shared_ptr<epee::mlocker> m1{new epee::mlocker(BASE(data), 32)};
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 1);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 2);
  m1 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 1);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 1);
  m0 = NULL;
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 0);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 0);
}

TEST(mlocker, mlocked)
{
  const size_t base_pages = epee::mlocker::get_num_locked_pages();
  const size_t base_objects = epee::mlocker::get_num_locked_objects();
  {
    struct Foo { uint64_t u; };
    epee::mlocked<Foo> l;
    ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 1);
    ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 1);
  }
  ASSERT_EQ(epee::mlocker::get_num_locked_pages(), base_pages + 0);
  ASSERT_EQ(epee::mlocker::get_num_locked_objects(), base_objects + 0);
}

#endif
