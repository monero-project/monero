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

#include "gtest/gtest.h"
#include "ringct/rctOps.h"
#include "device/device_default.hpp"

TEST(device, name)
{
  hw::core::device_default dev;
  ASSERT_TRUE(dev.set_name("test"));
  ASSERT_EQ(dev.get_name(), "test");
}

/*
TEST(device, locking)
{
  hw::core::device_default dev;
  ASSERT_TRUE(dev.try_lock());
  ASSERT_FALSE(dev.try_lock());
  dev.unlock();
  ASSERT_TRUE(dev.try_lock());
  dev.unlock();
  dev.lock();
  ASSERT_FALSE(dev.try_lock());
  dev.unlock();
  ASSERT_TRUE(dev.try_lock());
  dev.unlock();
}
*/

TEST(device, open_close)
{
  hw::core::device_default dev;
  crypto::secret_key key;
  ASSERT_TRUE(dev.open_tx(key));
  ASSERT_TRUE(dev.close_tx());
}

TEST(device, ops)
{
  hw::core::device_default dev;
  rct::key resd, res;
  crypto::key_derivation derd, der;
  rct::key sk, pk;
  crypto::secret_key sk0, sk1;
  crypto::public_key pk0, pk1;
  crypto::ec_scalar ressc0, ressc1;
  crypto::key_image ki0, ki1;

  rct::skpkGen(sk, pk);
  rct::scalarmultBase((rct::key&)pk0, (rct::key&)sk0);
  rct::scalarmultBase((rct::key&)pk1, (rct::key&)sk1);

  dev.scalarmultKey(resd, pk, sk);
  rct::scalarmultKey(res, pk, sk);
  ASSERT_EQ(resd, res);

  dev.scalarmultBase(resd, sk);
  rct::scalarmultBase(res, sk);
  ASSERT_EQ(resd, res);

  dev.sc_secret_add((crypto::secret_key&)resd, sk0, sk1);
  sc_add((unsigned char*)&res, (unsigned char*)&sk0, (unsigned char*)&sk1);
  ASSERT_EQ(resd, res);

  dev.generate_key_derivation(pk0, sk0, derd);
  crypto::generate_key_derivation(pk0, sk0, der);
  ASSERT_FALSE(memcmp(&derd, &der, sizeof(der)));

  dev.derivation_to_scalar(der, 0, ressc0);
  crypto::derivation_to_scalar(der, 0, ressc1);
  ASSERT_FALSE(memcmp(&ressc0, &ressc1, sizeof(ressc1)));

  dev.derive_secret_key(der, 0, rct::rct2sk(sk), sk0);
  crypto::derive_secret_key(der, 0, rct::rct2sk(sk), sk1);
  ASSERT_EQ(sk0, sk1);

  dev.derive_public_key(der, 0, rct::rct2pk(pk), pk0);
  crypto::derive_public_key(der, 0, rct::rct2pk(pk), pk1);
  ASSERT_EQ(pk0, pk1);

  dev.secret_key_to_public_key(rct::rct2sk(sk), pk0);
  crypto::secret_key_to_public_key(rct::rct2sk(sk), pk1);
  ASSERT_EQ(pk0, pk1);

  dev.generate_key_image(pk0, sk0, ki0);
  crypto::generate_key_image(pk0, sk0, ki1);
  ASSERT_EQ(ki0, ki1);
}

TEST(device, ecdh32)
{
  hw::core::device_default dev;
  rct::ecdhTuple tuple, tuple2;
  rct::key key = rct::skGen();
  tuple.mask = rct::skGen();
  tuple.amount = rct::skGen();
  tuple2 = tuple;
  dev.ecdhEncode(tuple, key, false);
  dev.ecdhDecode(tuple, key, false);
  ASSERT_EQ(tuple2.mask, tuple.mask);
  ASSERT_EQ(tuple2.amount, tuple.amount);
}

