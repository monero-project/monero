// Copyright (c) 2014-2024, The Monero Project
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

#include "common/base58.cpp"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "serialization/binary_utils.h"

using namespace tools;

#define MAKE_STR(arr) std::string(arr, sizeof(arr) - 1)

namespace
{
  void do_test_uint_8be_to_64(uint64_t expected, const uint8_t* data, size_t size)
  {
    uint64_t val = base58::uint_8be_to_64(data, size);
    ASSERT_EQ(val, expected);
  }

  void do_test_uint_64_to_8be(uint64_t num, const std::string& expected_str)
  {
    std::string data(expected_str.size(), '\x0');
    base58::uint_64_to_8be(num, data.size(), reinterpret_cast<uint8_t*>(&data[0]));
    ASSERT_EQ(data, expected_str);
  }

  void do_test_encode_block(const std::string& block, const std::string& expected)
  {
    ASSERT_TRUE(1 <= block.size() && block.size() <= base58::full_block_size);
    std::string enc(base58::encoded_block_sizes[block.size()], base58::alphabet[0]);
    base58::encode_block(block.data(), block.size(), &enc[0]);
    ASSERT_EQ(enc, expected);

    std::string dec(block.size(), '\0');
    ASSERT_TRUE(base58::decode_block(enc.data(), enc.size(), &dec[0]));
    ASSERT_EQ(block, dec);
  }

  void do_test_decode_block_pos(const std::string& enc, const std::string& expected)
  {
    std::string data(base58::decoded_block_sizes::instance(enc.size()), '\0');
    ASSERT_TRUE(base58::decode_block(enc.data(), enc.size(), &data[0]));
    ASSERT_EQ(data, expected);
  }

  void do_test_decode_block_neg(const std::string& enc)
  {
    std::string data(base58::full_block_size, '\0');
    ASSERT_FALSE(base58::decode_block(enc.data(), enc.size(), &data[0]));
  }

  void do_test_encode(const std::string& data, const std::string& expected)
  {
    std::string enc = base58::encode(data);
    ASSERT_EQ(enc, expected);

    std::string dec;
    ASSERT_TRUE(base58::decode(enc, dec));
    ASSERT_EQ(dec, data);
  }

  void do_test_decode_pos(const std::string& enc, const std::string& expected)
  {
    std::string dec;
    ASSERT_TRUE(base58::decode(enc, dec));
    ASSERT_EQ(dec, expected);
  }

  void do_test_decode_neg(const std::string& enc)
  {
    std::string dec;
    ASSERT_FALSE(base58::decode(enc, dec));
  }

  void do_test_encode_decode_addr(uint64_t tag, const std::string& data, const std::string& expected)
  {
    std::string addr = base58::encode_addr(tag, data);
    ASSERT_EQ(addr, expected);

    uint64_t dec_tag;
    std::string dec_data;
    ASSERT_TRUE(base58::decode_addr(addr, dec_tag, dec_data));
    ASSERT_EQ(tag, dec_tag);
    ASSERT_EQ(data, dec_data);
  }
}

#define TEST_uint_8be_to_64(expected, str)                                                        \
  TEST(base58_uint_8be_to_64, handles_bytes_##expected)                                           \
  {                                                                                               \
    std::string data = str;                                                                       \
    do_test_uint_8be_to_64(expected, reinterpret_cast<const uint8_t*>(data.data()), data.size()); \
  }

TEST_uint_8be_to_64(0x0000000000000001, "\x1");
TEST_uint_8be_to_64(0x0000000000000102, "\x1\x2");
TEST_uint_8be_to_64(0x0000000000010203, "\x1\x2\x3");
TEST_uint_8be_to_64(0x0000000001020304, "\x1\x2\x3\x4");
TEST_uint_8be_to_64(0x0000000102030405, "\x1\x2\x3\x4\x5");
TEST_uint_8be_to_64(0x0000010203040506, "\x1\x2\x3\x4\x5\x6");
TEST_uint_8be_to_64(0x0001020304050607, "\x1\x2\x3\x4\x5\x6\x7");
TEST_uint_8be_to_64(0x0102030405060708, "\x1\x2\x3\x4\x5\x6\x7\x8");


#define TEST_uint_64_to_8be(num, expected_str)     \
  TEST(base58_uint_64_to_8be, handles_bytes_##num) \
  {                                                \
    do_test_uint_64_to_8be(num, expected_str);     \
  }

TEST_uint_64_to_8be(0x0000000000000001, "\x1");
TEST_uint_64_to_8be(0x0000000000000102, "\x1\x2");
TEST_uint_64_to_8be(0x0000000000010203, "\x1\x2\x3");
TEST_uint_64_to_8be(0x0000000001020304, "\x1\x2\x3\x4");
TEST_uint_64_to_8be(0x0000000102030405, "\x1\x2\x3\x4\x5");
TEST_uint_64_to_8be(0x0000010203040506, "\x1\x2\x3\x4\x5\x6");
TEST_uint_64_to_8be(0x0001020304050607, "\x1\x2\x3\x4\x5\x6\x7");
TEST_uint_64_to_8be(0x0102030405060708, "\x1\x2\x3\x4\x5\x6\x7\x8");

TEST(reverse_alphabet, is_correct)
{
  ASSERT_EQ(-1, base58::reverse_alphabet::instance(0));
  ASSERT_EQ(-1, base58::reverse_alphabet::instance(std::numeric_limits<char>::min()));
  ASSERT_EQ(-1, base58::reverse_alphabet::instance(std::numeric_limits<char>::max()));
  ASSERT_EQ(-1, base58::reverse_alphabet::instance('1' - 1));
  ASSERT_EQ(-1, base58::reverse_alphabet::instance('z' + 1));
  ASSERT_EQ(-1, base58::reverse_alphabet::instance('0'));
  ASSERT_EQ(-1, base58::reverse_alphabet::instance('I'));
  ASSERT_EQ(-1, base58::reverse_alphabet::instance('O'));
  ASSERT_EQ(-1, base58::reverse_alphabet::instance('l'));
  ASSERT_EQ(0,  base58::reverse_alphabet::instance('1'));
  ASSERT_EQ(8,  base58::reverse_alphabet::instance('9'));
  ASSERT_EQ(base58::alphabet_size - 1, base58::reverse_alphabet::instance('z'));
}


#define TEST_encode_block(block, expected)            \
  TEST(base58_encode_block, handles_##expected)       \
  {                                                   \
    do_test_encode_block(MAKE_STR(block), #expected); \
  }

TEST_encode_block("\x00",                             11);
TEST_encode_block("\x39",                             1z);
TEST_encode_block("\xFF",                             5Q);

TEST_encode_block("\x00\x00",                         111);
TEST_encode_block("\x00\x39",                         11z);
TEST_encode_block("\x01\x00",                         15R);
TEST_encode_block("\xFF\xFF",                         LUv);

TEST_encode_block("\x00\x00\x00",                     11111);
TEST_encode_block("\x00\x00\x39",                     1111z);
TEST_encode_block("\x01\x00\x00",                     11LUw);
TEST_encode_block("\xFF\xFF\xFF",                     2UzHL);

TEST_encode_block("\x00\x00\x00\x39",                 11111z);
TEST_encode_block("\xFF\xFF\xFF\xFF",                 7YXq9G);
TEST_encode_block("\x00\x00\x00\x00\x39",             111111z);
TEST_encode_block("\xFF\xFF\xFF\xFF\xFF",             VtB5VXc);
TEST_encode_block("\x00\x00\x00\x00\x00\x39",         11111111z);
TEST_encode_block("\xFF\xFF\xFF\xFF\xFF\xFF",         3CUsUpv9t);
TEST_encode_block("\x00\x00\x00\x00\x00\x00\x39",     111111111z);
TEST_encode_block("\xFF\xFF\xFF\xFF\xFF\xFF\xFF",     Ahg1opVcGW);
TEST_encode_block("\x00\x00\x00\x00\x00\x00\x00\x39", 1111111111z);
TEST_encode_block("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", jpXCZedGfVQ);

TEST_encode_block("\x00\x00\x00\x00\x00\x00\x00\x00", 11111111111);
TEST_encode_block("\x00\x00\x00\x00\x00\x00\x00\x01", 11111111112);
TEST_encode_block("\x00\x00\x00\x00\x00\x00\x00\x08", 11111111119);
TEST_encode_block("\x00\x00\x00\x00\x00\x00\x00\x09", 1111111111A);
TEST_encode_block("\x00\x00\x00\x00\x00\x00\x00\x3A", 11111111121);
TEST_encode_block("\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 1Ahg1opVcGW);
TEST_encode_block("\x06\x15\x60\x13\x76\x28\x79\xF7", 22222222222);
TEST_encode_block("\x05\xE0\x22\xBA\x37\x4B\x2A\x00", 1z111111111);


#define TEST_decode_block_pos(enc, expected)            \
  TEST(base58_decode_block, handles_pos_##enc)          \
  {                                                     \
    do_test_decode_block_pos(#enc, MAKE_STR(expected)); \
  }

#define TEST_decode_block_neg(enc)             \
  TEST(base58_decode_block, handles_neg_##enc) \
  {                                            \
    do_test_decode_block_neg(#enc);            \
  }

// 1-byte block
TEST_decode_block_neg(1);
TEST_decode_block_neg(z);
// 2-bytes block
TEST_decode_block_pos(11,          "\x00");
TEST_decode_block_pos(5Q,          "\xFF");
TEST_decode_block_neg(5R);
TEST_decode_block_neg(zz);
// 3-bytes block
TEST_decode_block_pos(111,         "\x00\x00");
TEST_decode_block_pos(LUv,         "\xFF\xFF");
TEST_decode_block_neg(LUw);
TEST_decode_block_neg(zzz);
// 4-bytes block
TEST_decode_block_neg(1111);
TEST_decode_block_neg(zzzz);
// 5-bytes block
TEST_decode_block_pos(11111,       "\x00\x00\x00");
TEST_decode_block_pos(2UzHL,       "\xFF\xFF\xFF");
TEST_decode_block_neg(2UzHM);
TEST_decode_block_neg(zzzzz);
// 6-bytes block
TEST_decode_block_pos(111111,      "\x00\x00\x00\x00");
TEST_decode_block_pos(7YXq9G,      "\xFF\xFF\xFF\xFF");
TEST_decode_block_neg(7YXq9H);
TEST_decode_block_neg(zzzzzz);
// 7-bytes block
TEST_decode_block_pos(1111111,     "\x00\x00\x00\x00\x00");
TEST_decode_block_pos(VtB5VXc,     "\xFF\xFF\xFF\xFF\xFF");
TEST_decode_block_neg(VtB5VXd);
TEST_decode_block_neg(zzzzzzz);
// 8-bytes block
TEST_decode_block_neg(11111111);
TEST_decode_block_neg(zzzzzzzz);
// 9-bytes block
TEST_decode_block_pos(111111111,   "\x00\x00\x00\x00\x00\x00");
TEST_decode_block_pos(3CUsUpv9t,   "\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_block_neg(3CUsUpv9u);
TEST_decode_block_neg(zzzzzzzzz);
// 10-bytes block
TEST_decode_block_pos(1111111111,  "\x00\x00\x00\x00\x00\x00\x00");
TEST_decode_block_pos(Ahg1opVcGW,  "\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_block_neg(Ahg1opVcGX);
TEST_decode_block_neg(zzzzzzzzzz);
// 11-bytes block
TEST_decode_block_pos(11111111111, "\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_decode_block_pos(jpXCZedGfVQ, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_block_neg(jpXCZedGfVR);
TEST_decode_block_neg(zzzzzzzzzzz);
// Invalid symbols
TEST_decode_block_neg(01111111111);
TEST_decode_block_neg(11111111110);
TEST_decode_block_neg(11111011111);
TEST_decode_block_neg(I1111111111);
TEST_decode_block_neg(O1111111111);
TEST_decode_block_neg(l1111111111);
TEST_decode_block_neg(_1111111111);


#define TEST_encode(expected, data)            \
  TEST(base58_encode, handles_##expected)      \
  {                                            \
    do_test_encode(MAKE_STR(data), #expected); \
  }

TEST_encode(11,                     "\x00");
TEST_encode(111,                    "\x00\x00");
TEST_encode(11111,                  "\x00\x00\x00");
TEST_encode(111111,                 "\x00\x00\x00\x00");
TEST_encode(1111111,                "\x00\x00\x00\x00\x00");
TEST_encode(111111111,              "\x00\x00\x00\x00\x00\x00");
TEST_encode(1111111111,             "\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(11111111111,            "\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(1111111111111,          "\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(11111111111111,         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(1111111111111111,       "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(11111111111111111,      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(111111111111111111,     "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(11111111111111111111,   "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(111111111111111111111,  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(1111111111111111111111, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode(22222222222VtB5VXc,     "\x06\x15\x60\x13\x76\x28\x79\xF7\xFF\xFF\xFF\xFF\xFF");


#define TEST_decode_pos(enc, expected)            \
  TEST(base58_decode_pos, handles_pos_##enc)      \
  {                                               \
    do_test_decode_pos(#enc, MAKE_STR(expected)); \
  }

#define TEST_decode_neg(enc)                 \
  TEST(base58_decode_neg, handles_neg_##enc) \
  {                                          \
    do_test_decode_neg(#enc);                \
  }

TEST_decode_pos(,                       "");
TEST_decode_pos(5Q,                     "\xFF");
TEST_decode_pos(LUv,                    "\xFF\xFF");
TEST_decode_pos(2UzHL,                  "\xFF\xFF\xFF");
TEST_decode_pos(7YXq9G,                 "\xFF\xFF\xFF\xFF");
TEST_decode_pos(VtB5VXc,                "\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(3CUsUpv9t,              "\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(Ahg1opVcGW,             "\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(jpXCZedGfVQ,            "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(jpXCZedGfVQ5Q,          "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(jpXCZedGfVQLUv,         "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(jpXCZedGfVQ2UzHL,       "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(jpXCZedGfVQ7YXq9G,      "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(jpXCZedGfVQVtB5VXc,     "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(jpXCZedGfVQ3CUsUpv9t,   "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(jpXCZedGfVQAhg1opVcGW,  "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_decode_pos(jpXCZedGfVQjpXCZedGfVQ, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
// Invalid length
TEST_decode_neg(1);
TEST_decode_neg(z);
TEST_decode_neg(1111);
TEST_decode_neg(zzzz);
TEST_decode_neg(11111111);
TEST_decode_neg(zzzzzzzz);
TEST_decode_neg(123456789AB1);
TEST_decode_neg(123456789ABz);
TEST_decode_neg(123456789AB1111);
TEST_decode_neg(123456789ABzzzz);
TEST_decode_neg(123456789AB11111111);
TEST_decode_neg(123456789ABzzzzzzzz);
// Overflow
TEST_decode_neg(5R);
TEST_decode_neg(zz);
TEST_decode_neg(LUw);
TEST_decode_neg(zzz);
TEST_decode_neg(2UzHM);
TEST_decode_neg(zzzzz);
TEST_decode_neg(7YXq9H);
TEST_decode_neg(zzzzzz);
TEST_decode_neg(VtB5VXd);
TEST_decode_neg(zzzzzzz);
TEST_decode_neg(3CUsUpv9u);
TEST_decode_neg(zzzzzzzzz);
TEST_decode_neg(Ahg1opVcGX);
TEST_decode_neg(zzzzzzzzzz);
TEST_decode_neg(jpXCZedGfVR);
TEST_decode_neg(zzzzzzzzzzz);
TEST_decode_neg(123456789AB5R);
TEST_decode_neg(123456789ABzz);
TEST_decode_neg(123456789ABLUw);
TEST_decode_neg(123456789ABzzz);
TEST_decode_neg(123456789AB2UzHM);
TEST_decode_neg(123456789ABzzzzz);
TEST_decode_neg(123456789AB7YXq9H);
TEST_decode_neg(123456789ABzzzzzz);
TEST_decode_neg(123456789ABVtB5VXd);
TEST_decode_neg(123456789ABzzzzzzz);
TEST_decode_neg(123456789AB3CUsUpv9u);
TEST_decode_neg(123456789ABzzzzzzzzz);
TEST_decode_neg(123456789ABAhg1opVcGX);
TEST_decode_neg(123456789ABzzzzzzzzzz);
TEST_decode_neg(123456789ABjpXCZedGfVR);
TEST_decode_neg(123456789ABzzzzzzzzzzz);
TEST_decode_neg(zzzzzzzzzzz11);
// Invalid symbols
TEST_decode_neg(10);
TEST_decode_neg(11I);
TEST_decode_neg(11O11);
TEST_decode_neg(11l111);
TEST_decode_neg(11_11111111);
TEST_decode_neg(1101111111111);
TEST_decode_neg(11I11111111111111);
TEST_decode_neg(11O1111111111111111111);
TEST_decode_neg(1111111111110);
TEST_decode_neg(111111111111l1111);
TEST_decode_neg(111111111111_111111111);


#define TEST_encode_decode_addr(addr, tag, data)                      \
  TEST(base58_encode_decode_addr, handles_##addr)                     \
  {                                                                   \
    do_test_encode_decode_addr(UINT64_C(tag), MAKE_STR(data), #addr); \
  }

TEST_encode_decode_addr(21D35quxec71111111111111111111111111111111111111111111111111111111111111111111111111111116Q5tCH, 6,
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode_decode_addr(2Aui6ejTFscjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQVqegMoV, 6,
                        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_encode_decode_addr(1119XrkPuSmLzdHXgVgrZKjepg5hZAxffLzdHXgVgrZKjepg5hZAxffLzdHXgVgrZKjepg5hZAxffLzdHXgVgrZKVphZRvn, 0,
                        "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA\xBB\xCC\xDD\xEE\xFF\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA\xBB\xCC\xDD\xEE\xFF"
                        "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA\xBB\xCC\xDD\xEE\xFF\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA\xBB\xCC\xDD\xEE\xFF");
TEST_encode_decode_addr(111111111111111111111111111111111111111111111111111111111111111111111111111111111111111115TXfiA, 0,
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode_decode_addr(PuT7GAdgbA83qvSEivPLYo11111111111111111111111111111111111111111111111111111111111111111111111111111169tWrH, 0x1122334455667788,
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
TEST_encode_decode_addr(PuT7GAdgbA841d7FXjswpJjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQjpXCZedGfVQVq4LL1v, 0x1122334455667788,
                        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
TEST_encode_decode_addr(PuT7GAdgbA819VwdWVDP,            0x1122334455667788, "\x11");
TEST_encode_decode_addr(PuT7GAdgbA81efAfdCjPg,           0x1122334455667788, "\x22\x22");
TEST_encode_decode_addr(PuT7GAdgbA83sryEt3YC8Q,          0x1122334455667788, "\x33\x33\x33");
TEST_encode_decode_addr(PuT7GAdgbA83tWUuc54PFP3b,        0x1122334455667788, "\x44\x44\x44\x44");
TEST_encode_decode_addr(PuT7GAdgbA83u9zaKrtRKZ1J6,       0x1122334455667788, "\x55\x55\x55\x55\x55");
TEST_encode_decode_addr(PuT7GAdgbA83uoWF3eanGG1aRoG,     0x1122334455667788, "\x66\x66\x66\x66\x66\x66");
TEST_encode_decode_addr(PuT7GAdgbA83vT1umSHMYJ4oNVdu,    0x1122334455667788, "\x77\x77\x77\x77\x77\x77\x77");
TEST_encode_decode_addr(PuT7GAdgbA83w6XaVDyvpoGQBEWbB,   0x1122334455667788, "\x88\x88\x88\x88\x88\x88\x88\x88");
TEST_encode_decode_addr(PuT7GAdgbA83wk3FD1gW7J2KVGofA1r, 0x1122334455667788, "\x99\x99\x99\x99\x99\x99\x99\x99\x99");
TEST_encode_decode_addr(15p2yAV,                         0, "");
TEST_encode_decode_addr(FNQ3D6A,                         0x7F, "");
TEST_encode_decode_addr(26k9QWweu,                       0x80, "");
TEST_encode_decode_addr(3BzAD7n3y,                       0xFF, "");
TEST_encode_decode_addr(11efCaY6UjG7JrxuB,               0, "\x11\x22\x33\x44\x55\x66\x77");
TEST_encode_decode_addr(21rhHRT48LN4PriP9,               6, "\x11\x22\x33\x44\x55\x66\x77");


#define TEST_decode_addr_neg(addr, test_name)                     \
  TEST(base58_decode_addr_neg, test_name)                         \
  {                                                               \
    uint64_t tag;                                                 \
    std::string data;                                             \
    ASSERT_FALSE(base58::decode_addr(MAKE_STR(addr), tag, data)); \
  }

TEST_decode_addr_neg("zuT7GAdgbA819VwdWVDP", decode_fails_due_overflow);
TEST_decode_addr_neg("0uT7GAdgbA819VwdWVDP", decode_fails_due_invalid_char_0);
TEST_decode_addr_neg("IuT7GAdgbA819VwdWVDP", decode_fails_due_invalid_char_I);
TEST_decode_addr_neg("OuT7GAdgbA819VwdWVDP", decode_fails_due_invalid_char_O);
TEST_decode_addr_neg("luT7GAdgbA819VwdWVDP", decode_fails_due_invalid_char_l);
TEST_decode_addr_neg("\0uT7GAdgbA819VwdWVDP", decode_fails_due_invalid_char_00);
TEST_decode_addr_neg("PuT7GAdgbA819VwdWVD", decode_fails_due_invalid_lenght);
TEST_decode_addr_neg("11efCaY6UjG7JrxuC", handles_invalid_checksum);
TEST_decode_addr_neg("jerj2e4mESo", handles_non_correct_tag); // "jerj2e4mESo" == "\xFF\x00\xFF\xFF\x5A\xD9\xF1\x1C"
TEST_decode_addr_neg("1", decode_fails_due_invalid_block_len_0);
TEST_decode_addr_neg("1111", decode_fails_due_invalid_block_len_1);
TEST_decode_addr_neg("11", decode_fails_due_address_too_short_0);
TEST_decode_addr_neg("111", decode_fails_due_address_too_short_1);
TEST_decode_addr_neg("11111", decode_fails_due_address_too_short_2);
TEST_decode_addr_neg("111111", decode_fails_due_address_too_short_3);
TEST_decode_addr_neg("999999", decode_fails_due_address_too_short_4);
TEST_decode_addr_neg("ZZZZZZ", decode_fails_due_address_too_short_5);

namespace
{
  std::string test_serialized_keys = MAKE_STR(
    "\xf7\x24\xbc\x5c\x6c\xfb\xb9\xd9\x76\x02\xc3\x00\x42\x3a\x2f\x28"
    "\x64\x18\x74\x51\x3a\x03\x57\x78\xa0\xc1\x77\x8d\x83\x32\x01\xe9"
    "\x22\x09\x39\x68\x9e\xdf\x1a\xbd\x5b\xc1\xd0\x31\xf7\x3e\xcd\x6c"
    "\x99\x3a\xdd\x66\xd6\x80\x88\x70\x45\x6a\xfe\xb8\xe7\xee\xb6\x8d");
  // DON'T ever use this as a destination for funds, as the keys are right above this comment...
  std::string test_keys_addr_str = "4AzKEX4gXdJdNeM6dfiBFL7kqund3HYGvMBF3ttsNd9SfzgYB6L7ep1Yg1osYJzLdaKAYSLVh6e6jKnAuzj3bw1oGy9kXCb";
}

TEST(get_account_address_as_str, works_correctly)
{
  cryptonote::account_public_address addr;
  ASSERT_TRUE(serialization::parse_binary(test_serialized_keys, addr));
  std::string addr_str = cryptonote::get_account_address_as_str(cryptonote::MAINNET, false, addr);
  ASSERT_EQ(addr_str, test_keys_addr_str);
}

TEST(get_account_address_from_str, handles_valid_address)
{
  cryptonote::address_parse_info info;
  ASSERT_TRUE(cryptonote::get_account_address_from_str(info, cryptonote::MAINNET, test_keys_addr_str));

  std::string blob;
  ASSERT_TRUE(serialization::dump_binary(info.address, blob));
  ASSERT_EQ(blob, test_serialized_keys);
}

TEST(get_account_address_from_str, fails_on_invalid_address_format)
{
  cryptonote::address_parse_info info;
  std::string addr_str = test_keys_addr_str;
  addr_str[0] = '0';

  ASSERT_FALSE(cryptonote::get_account_address_from_str(info, cryptonote::MAINNET, addr_str));
}

TEST(get_account_address_from_str, fails_on_invalid_address_prefix)
{
  std::string addr_str = base58::encode_addr(0, test_serialized_keys);

  cryptonote::address_parse_info info;
  ASSERT_FALSE(cryptonote::get_account_address_from_str(info, cryptonote::MAINNET, addr_str));
}

TEST(get_account_address_from_str, fails_on_invalid_address_content)
{
  std::string addr_str = base58::encode_addr(config::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX, test_serialized_keys.substr(1));

  cryptonote::address_parse_info info;
  ASSERT_FALSE(cryptonote::get_account_address_from_str(info, cryptonote::MAINNET, addr_str));
}

TEST(get_account_address_from_str, fails_on_invalid_address_spend_key)
{
  std::string serialized_keys_copy = test_serialized_keys;
  serialized_keys_copy[0] = '\0';
  std::string addr_str = base58::encode_addr(config::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX, serialized_keys_copy);

  cryptonote::address_parse_info info;
  ASSERT_FALSE(cryptonote::get_account_address_from_str(info, cryptonote::MAINNET, addr_str));
}

TEST(get_account_address_from_str, fails_on_invalid_address_view_key)
{
  std::string serialized_keys_copy = test_serialized_keys;
  serialized_keys_copy.back() = '\x01';
  std::string addr_str = base58::encode_addr(config::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX, serialized_keys_copy);

  cryptonote::address_parse_info info;
  ASSERT_FALSE(cryptonote::get_account_address_from_str(info, cryptonote::MAINNET, addr_str));
}

TEST(get_account_address_from_str, parses_old_address_format)
{
  cryptonote::address_parse_info info;
  ASSERT_TRUE(cryptonote::get_account_address_from_str(info, cryptonote::MAINNET, "002391bbbb24dea6fd95232e97594a27769d0153d053d2102b789c498f57a2b00b69cd6f2f5c529c1660f2f4a2b50178d6640c20ce71fe26373041af97c5b10236fc"));
}
