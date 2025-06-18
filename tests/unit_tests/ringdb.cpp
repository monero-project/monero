// Copyright (c) 2018-2024, The Monero Project

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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <boost/filesystem.hpp>

#include "gtest/gtest.h"

#include "string_tools.h"
#include "crypto/crypto.h"
#include "crypto/random.h"
#include "crypto/chacha.h"
#include "ringct/rctOps.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "wallet/ringdb.h"

static crypto::chacha_key generate_chacha_key()
{
  crypto::chacha_key chacha_key;
  uint64_t password = crypto::rand<uint64_t>();
  crypto::generate_chacha_key(std::string((const char*)&password, sizeof(password)), chacha_key, 1);
  return chacha_key;
}

static crypto::key_image generate_key_image()
{
  crypto::key_image key_image;
  cryptonote::keypair keypair = cryptonote::keypair::generate(hw::get_device("default"));
  crypto::generate_key_image(keypair.pub, keypair.sec, key_image);
  return key_image;
}

static crypto::chacha_key get_KEY_1() { static const crypto::chacha_key KEY_1 = generate_chacha_key(); return KEY_1; }
static crypto::chacha_key get_KEY_2() { static const crypto::chacha_key KEY_2 = generate_chacha_key(); return KEY_2; }
static crypto::key_image get_KEY_IMAGE_1() { static const crypto::key_image KEY_IMAGE_1 = generate_key_image(); return KEY_IMAGE_1; }

#define KEY_1 get_KEY_1()
#define KEY_2 get_KEY_2()
#define KEY_IMAGE_1 get_KEY_IMAGE_1()

class RingDB: public tools::ringdb
{
public:
  RingDB(const char *genesis = ""): tools::ringdb(make_filename(), genesis) { }
  ~RingDB() { close(); boost::filesystem::remove_all(filename); free(filename); }

private:
  std::string make_filename()
  {
    boost::filesystem::path path =
      boost::filesystem::temp_directory_path();
#if defined(__MINGW32__) || defined(__MINGW__)
    filename = tempnam(path.string().c_str(), "monero-ringdb-test-");
    EXPECT_TRUE(filename != NULL);
#else
    path /= "monero-ringdb-test-XXXXXX";
    filename = strdup(path.string().c_str());
    EXPECT_TRUE(mkdtemp(filename) != NULL);
#endif
    return filename;
  }

private:
  char *filename;
};

TEST(ringdb, not_found)
{
  RingDB ringdb;
  std::vector<uint64_t> outs;
  ASSERT_FALSE(ringdb.get_ring(KEY_1, KEY_IMAGE_1, outs));
}

TEST(ringdb, found)
{
  RingDB ringdb;
  std::vector<uint64_t> outs, outs2;
  outs.push_back(43); outs.push_back(7320); outs.push_back(8429);
  ASSERT_TRUE(ringdb.set_ring(KEY_1, KEY_IMAGE_1, outs, false));
  ASSERT_TRUE(ringdb.get_ring(KEY_1, KEY_IMAGE_1, outs2));
  ASSERT_EQ(outs, outs2);
}

TEST(ringdb, convert)
{
  RingDB ringdb;
  std::vector<uint64_t> outs, outs2;
  outs.push_back(43); outs.push_back(7320); outs.push_back(8429);
  ASSERT_TRUE(ringdb.set_ring(KEY_1, KEY_IMAGE_1, outs, true));
  ASSERT_TRUE(ringdb.get_ring(KEY_1, KEY_IMAGE_1, outs2));
  ASSERT_EQ(outs2.size(), 3);
  ASSERT_EQ(outs2[0], 43);
  ASSERT_EQ(outs2[1], 43+7320);
  ASSERT_EQ(outs2[2], 43+7320+8429);
}

TEST(ringdb, different_genesis)
{
  RingDB ringdb;
  std::vector<uint64_t> outs, outs2;
  outs.push_back(43); outs.push_back(7320); outs.push_back(8429);
  ASSERT_TRUE(ringdb.set_ring(KEY_1, KEY_IMAGE_1, outs, false));
  ASSERT_FALSE(ringdb.get_ring(KEY_2, KEY_IMAGE_1, outs2));
}
