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

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#include "cryptonote_basic/cryptonote_format_utils.h"

#define VEC_FROM_ARR(vec)                                               \
  std::vector<uint64_t> vec;                                            \
  for (size_t i = 0; i < sizeof(vec##_arr) / sizeof(vec##_arr[0]); ++i) \
  {                                                                     \
    vec.push_back(vec##_arr[i]);                                        \
  }

namespace
{
  struct chunk_handler_t
  {
    void operator()(uint64_t chunk) const
    {
      m_chunks.push_back(chunk);
    }

    mutable std::vector<uint64_t> m_chunks;
  };

  struct dust_handler_t
  {
    dust_handler_t()
      : m_dust(0)
      , m_has_dust(false)
    {
    }

    void operator()(uint64_t dust) const
    {
      m_dust = dust;
      m_has_dust = true;
    }

    mutable uint64_t m_dust;
    mutable bool m_has_dust;
  };

  class decompose_amount_into_digits_test : public ::testing::Test
  {
  protected:
    chunk_handler_t m_chunk_handler;
    dust_handler_t m_dust_handler;
  };
}

TEST_F(decompose_amount_into_digits_test, is_correct_0)
{
  std::vector<uint64_t> expected_chunks;
  cryptonote::decompose_amount_into_digits(0, 0, m_chunk_handler, m_dust_handler);
  ASSERT_EQ(m_chunk_handler.m_chunks, expected_chunks);
  ASSERT_EQ(m_dust_handler.m_has_dust, false);
}

TEST_F(decompose_amount_into_digits_test, is_correct_1)
{
  std::vector<uint64_t> expected_chunks;
  cryptonote::decompose_amount_into_digits(0, 10, m_chunk_handler, m_dust_handler);
  ASSERT_EQ(m_chunk_handler.m_chunks, expected_chunks);
  ASSERT_EQ(m_dust_handler.m_has_dust, false);
}

TEST_F(decompose_amount_into_digits_test, is_correct_2)
{
  uint64_t expected_chunks_arr[] = {10};
  VEC_FROM_ARR(expected_chunks);
  cryptonote::decompose_amount_into_digits(10, 0, m_chunk_handler, m_dust_handler);
  ASSERT_EQ(m_chunk_handler.m_chunks, expected_chunks);
  ASSERT_EQ(m_dust_handler.m_has_dust, false);
}

TEST_F(decompose_amount_into_digits_test, is_correct_3)
{
  std::vector<uint64_t> expected_chunks;
  uint64_t expected_dust = 10;
  cryptonote::decompose_amount_into_digits(10, 10, m_chunk_handler, m_dust_handler);
  ASSERT_EQ(m_chunk_handler.m_chunks, expected_chunks);
  ASSERT_EQ(m_dust_handler.m_dust, expected_dust);
}

TEST_F(decompose_amount_into_digits_test, is_correct_4)
{
  uint64_t expected_dust = 8100;
  std::vector<uint64_t> expected_chunks;
  cryptonote::decompose_amount_into_digits(8100, 1000000, m_chunk_handler, m_dust_handler);
  ASSERT_EQ(m_chunk_handler.m_chunks, expected_chunks);
  ASSERT_EQ(m_dust_handler.m_dust, expected_dust);
}

TEST_F(decompose_amount_into_digits_test, is_correct_5)
{
  uint64_t expected_chunks_arr[] = {100, 900000, 8000000};
  VEC_FROM_ARR(expected_chunks);
  cryptonote::decompose_amount_into_digits(8900100, 10, m_chunk_handler, m_dust_handler);
  ASSERT_EQ(m_chunk_handler.m_chunks, expected_chunks);
  ASSERT_EQ(m_dust_handler.m_has_dust, false);
}

TEST_F(decompose_amount_into_digits_test, is_correct_6)
{
  uint64_t expected_chunks_arr[] = {900000, 8000000};
  VEC_FROM_ARR(expected_chunks);
  uint64_t expected_dust = 100;
  cryptonote::decompose_amount_into_digits(8900100, 1000, m_chunk_handler, m_dust_handler);
  ASSERT_EQ(m_chunk_handler.m_chunks, expected_chunks);
  ASSERT_EQ(m_dust_handler.m_dust, expected_dust);
}
