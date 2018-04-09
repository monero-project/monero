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

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "ringct/rctSigs.h"
#include "serialization/binary_archive.h"
#include "serialization/json_archive.h"
#include "serialization/debug_archive.h"
#include "serialization/variant.h"
#include "serialization/vector.h"
#include "serialization/binary_utils.h"
#include "wallet/wallet2.h"
#include "gtest/gtest.h"
#include "unit_tests_utils.h"
#include "device/device.hpp"
using namespace std;
using namespace crypto;

struct Struct
{
  int32_t a;
  int32_t b;
  char blob[8];
};

template <class Archive>
struct serializer<Archive, Struct>
{
  static bool serialize(Archive &ar, Struct &s) {
    ar.begin_object();
    ar.tag("a");
    ar.serialize_int(s.a);
    ar.tag("b");
    ar.serialize_int(s.b);
    ar.tag("blob");
    ar.serialize_blob(s.blob, sizeof(s.blob));
    ar.end_object();
    return true;
  }
};

struct Struct1
{
  vector<boost::variant<Struct, int32_t>> si;
  vector<int16_t> vi;

  BEGIN_SERIALIZE_OBJECT()
    FIELD(si)
    FIELD(vi)
  END_SERIALIZE()
  /*template <bool W, template <bool> class Archive>
  bool do_serialize(Archive<W> &ar)
  {
    ar.begin_object();
    ar.tag("si");
    ::do_serialize(ar, si);
    ar.tag("vi");
    ::do_serialize(ar, vi);
    ar.end_object();
  }*/
};

struct Blob
{
  uint64_t a;
  uint32_t b;

  bool operator==(const Blob& rhs) const
  {
    return a == rhs.a;
  }
};

VARIANT_TAG(binary_archive, Struct, 0xe0);
VARIANT_TAG(binary_archive, int, 0xe1);
VARIANT_TAG(json_archive, Struct, "struct");
VARIANT_TAG(json_archive, int, "int");
VARIANT_TAG(debug_archive, Struct1, "struct1");
VARIANT_TAG(debug_archive, Struct, "struct");
VARIANT_TAG(debug_archive, int, "int");

BLOB_SERIALIZER(Blob);

bool try_parse(const string &blob)
{
  Struct1 s1;
  return serialization::parse_binary(blob, s1);
}

TEST(Serialization, BinaryArchiveInts) {
  uint64_t x = 0xff00000000, x1;

  ostringstream oss;
  binary_archive<true> oar(oss);
  oar.serialize_int(x);
  ASSERT_TRUE(oss.good());
  ASSERT_EQ(8, oss.str().size());
  ASSERT_EQ(string("\0\0\0\0\xff\0\0\0", 8), oss.str());

  istringstream iss(oss.str());
  binary_archive<false> iar(iss);
  iar.serialize_int(x1);
  ASSERT_EQ(8, iss.tellg());
  ASSERT_TRUE(iss.good());

  ASSERT_EQ(x, x1);
}

TEST(Serialization, BinaryArchiveVarInts) {
  uint64_t x = 0xff00000000, x1;

  ostringstream oss;
  binary_archive<true> oar(oss);
  oar.serialize_varint(x);
  ASSERT_TRUE(oss.good());
  ASSERT_EQ(6, oss.str().size());
  ASSERT_EQ(string("\x80\x80\x80\x80\xF0\x1F", 6), oss.str());

  istringstream iss(oss.str());
  binary_archive<false> iar(iss);
  iar.serialize_varint(x1);
  ASSERT_TRUE(iss.good());
  ASSERT_EQ(x, x1);
}

TEST(Serialization, Test1) {
  ostringstream str;
  binary_archive<true> ar(str);

  Struct1 s1;
  s1.si.push_back(0);
  {
    Struct s;
    s.a = 5;
    s.b = 65539;
    std::memcpy(s.blob, "12345678", 8);
    s1.si.push_back(s);
  }
  s1.si.push_back(1);
  s1.vi.push_back(10);
  s1.vi.push_back(22);

  string blob;
  ASSERT_TRUE(serialization::dump_binary(s1, blob));
  ASSERT_TRUE(try_parse(blob));

  ASSERT_EQ('\xE0', blob[6]);
  blob[6] = '\xE1';
  ASSERT_FALSE(try_parse(blob));
  blob[6] = '\xE2';
  ASSERT_FALSE(try_parse(blob));
}

TEST(Serialization, Overflow) {
  Blob x = { 0xff00000000 };
  Blob x1;

  string blob;
  ASSERT_TRUE(serialization::dump_binary(x, blob));
  ASSERT_EQ(sizeof(Blob), blob.size());

  ASSERT_TRUE(serialization::parse_binary(blob, x1));
  ASSERT_EQ(x, x1);

  vector<Blob> bigvector;
  ASSERT_FALSE(serialization::parse_binary(blob, bigvector));
  ASSERT_EQ(0, bigvector.size());
}

TEST(Serialization, serializes_vector_uint64_as_varint)
{
  std::vector<uint64_t> v;
  string blob;

  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(1, blob.size());

  // +1 byte
  v.push_back(0);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(2, blob.size());

  // +1 byte
  v.push_back(1);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(3, blob.size());

  // +2 bytes
  v.push_back(0x80);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(5, blob.size());

  // +2 bytes
  v.push_back(0xFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(7, blob.size());

  // +2 bytes
  v.push_back(0x3FFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(9, blob.size());

  // +3 bytes
  v.push_back(0x40FF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(12, blob.size());

  // +10 bytes
  v.push_back(0xFFFFFFFFFFFFFFFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(22, blob.size());
}

TEST(Serialization, serializes_vector_int64_as_fixed_int)
{
  std::vector<int64_t> v;
  string blob;

  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(1, blob.size());

  // +8 bytes
  v.push_back(0);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(9, blob.size());

  // +8 bytes
  v.push_back(1);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(17, blob.size());

  // +8 bytes
  v.push_back(0x80);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(25, blob.size());

  // +8 bytes
  v.push_back(0xFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(33, blob.size());

  // +8 bytes
  v.push_back(0x3FFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(41, blob.size());

  // +8 bytes
  v.push_back(0x40FF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(49, blob.size());

  // +8 bytes
  v.push_back(0xFFFFFFFFFFFFFFFF);
  ASSERT_TRUE(serialization::dump_binary(v, blob));
  ASSERT_EQ(57, blob.size());
}

namespace
{
  template<typename T>
  std::vector<T> linearize_vector2(const std::vector< std::vector<T> >& vec_vec)
  {
    std::vector<T> res;
    BOOST_FOREACH(const auto& vec, vec_vec)
    {
      res.insert(res.end(), vec.begin(), vec.end());
    }
    return res;
  }
}

TEST(Serialization, serializes_transacion_signatures_correctly)
{
  using namespace cryptonote;

  transaction tx;
  transaction tx1;
  string blob;

  // Empty tx
  tx.set_null();
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(5, blob.size()); // 5 bytes + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx, tx1);
  ASSERT_EQ(linearize_vector2(tx.signatures), linearize_vector2(tx1.signatures));

  // Miner tx without signatures
  txin_gen txin_gen1;
  txin_gen1.height = 0;
  tx.set_null();
  tx.vin.push_back(txin_gen1);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(7, blob.size()); // 5 bytes + 2 bytes vin[0] + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx, tx1);
  ASSERT_EQ(linearize_vector2(tx.signatures), linearize_vector2(tx1.signatures));

  // Miner tx with empty signatures 2nd vector
  tx.signatures.resize(1);
  tx.invalidate_hashes();
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(7, blob.size()); // 5 bytes + 2 bytes vin[0] + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx, tx1);
  ASSERT_EQ(linearize_vector2(tx.signatures), linearize_vector2(tx1.signatures));

  // Miner tx with one signature
  tx.signatures[0].resize(1);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Miner tx with 2 empty vectors
  tx.signatures.resize(2);
  tx.signatures[0].resize(0);
  tx.signatures[1].resize(0);
  tx.invalidate_hashes();
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Miner tx with 2 signatures
  tx.signatures[0].resize(1);
  tx.signatures[1].resize(1);
  tx.invalidate_hashes();
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Two txin_gen, no signatures
  tx.vin.push_back(txin_gen1);
  tx.signatures.resize(0);
  tx.invalidate_hashes();
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(9, blob.size()); // 5 bytes + 2 * 2 bytes vins + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx, tx1);
  ASSERT_EQ(linearize_vector2(tx.signatures), linearize_vector2(tx1.signatures));

  // Two txin_gen, signatures vector contains only one empty element
  tx.signatures.resize(1);
  tx.invalidate_hashes();
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Two txin_gen, signatures vector contains two empty elements
  tx.signatures.resize(2);
  tx.invalidate_hashes();
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(9, blob.size()); // 5 bytes + 2 * 2 bytes vins + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx, tx1);
  ASSERT_EQ(linearize_vector2(tx.signatures), linearize_vector2(tx1.signatures));

  // Two txin_gen, signatures vector contains three empty elements
  tx.signatures.resize(3);
  tx.invalidate_hashes();
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Two txin_gen, signatures vector contains two non empty elements
  tx.signatures.resize(2);
  tx.signatures[0].resize(1);
  tx.signatures[1].resize(1);
  tx.invalidate_hashes();
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // A few bytes instead of signature
  tx.vin.clear();
  tx.vin.push_back(txin_gen1);
  tx.signatures.clear();
  tx.invalidate_hashes();
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  blob.append(std::string(sizeof(crypto::signature) / 2, 'x'));
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));

  // blob contains one signature
  blob.append(std::string(sizeof(crypto::signature) / 2, 'y'));
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));

  // Not enough signature vectors for all inputs
  txin_to_key txin_to_key1;
  txin_to_key1.amount = 1;
  memset(&txin_to_key1.k_image, 0x42, sizeof(crypto::key_image));
  txin_to_key1.key_offsets.push_back(12);
  txin_to_key1.key_offsets.push_back(3453);
  tx.vin.clear();
  tx.vin.push_back(txin_to_key1);
  tx.vin.push_back(txin_to_key1);
  tx.signatures.resize(1);
  tx.signatures[0].resize(2);
  tx.invalidate_hashes();
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Too much signatures for two inputs
  tx.signatures.resize(3);
  tx.signatures[0].resize(2);
  tx.signatures[1].resize(2);
  tx.signatures[2].resize(2);
  tx.invalidate_hashes();
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // First signatures vector contains too little elements
  tx.signatures.resize(2);
  tx.signatures[0].resize(1);
  tx.signatures[1].resize(2);
  tx.invalidate_hashes();
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // First signatures vector contains too much elements
  tx.signatures.resize(2);
  tx.signatures[0].resize(3);
  tx.signatures[1].resize(2);
  tx.invalidate_hashes();
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // There are signatures for each input
  tx.signatures.resize(2);
  tx.signatures[0].resize(2);
  tx.signatures[1].resize(2);
  tx.invalidate_hashes();
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx, tx1);
  ASSERT_EQ(linearize_vector2(tx.signatures), linearize_vector2(tx1.signatures));

  // Blob doesn't contain enough data
  blob.resize(blob.size() - sizeof(crypto::signature) / 2);
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));

  // Blob contains too much data
  blob.resize(blob.size() + sizeof(crypto::signature));
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));

  // Blob contains one excess signature
  blob.resize(blob.size() + sizeof(crypto::signature) / 2);
  ASSERT_FALSE(serialization::parse_binary(blob, tx1));
}

TEST(Serialization, serializes_ringct_types)
{
  string blob;
  rct::key key0, key1;
  rct::keyV keyv0, keyv1;
  rct::keyM keym0, keym1;
  rct::ctkey ctkey0, ctkey1;
  rct::ctkeyV ctkeyv0, ctkeyv1;
  rct::ctkeyM ctkeym0, ctkeym1;
  rct::ecdhTuple ecdh0, ecdh1;
  rct::boroSig boro0, boro1;
  rct::mgSig mg0, mg1;
  rct::rangeSig rg0, rg1;
  rct::rctSig s0, s1;
  cryptonote::transaction tx0, tx1;

  key0 = rct::skGen();
  ASSERT_TRUE(serialization::dump_binary(key0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, key1));
  ASSERT_TRUE(key0 == key1);

  keyv0 = rct::skvGen(30);
  for (size_t n = 0; n < keyv0.size(); ++n)
    keyv0[n] = rct::skGen();
  ASSERT_TRUE(serialization::dump_binary(keyv0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, keyv1));
  ASSERT_TRUE(keyv0.size() == keyv1.size());
  for (size_t n = 0; n < keyv0.size(); ++n)
  {
    ASSERT_TRUE(keyv0[n] == keyv1[n]);
  }

  keym0 = rct::keyMInit(9, 12);
  for (size_t n = 0; n < keym0.size(); ++n)
    for (size_t i = 0; i < keym0[n].size(); ++i)
      keym0[n][i] = rct::skGen();
  ASSERT_TRUE(serialization::dump_binary(keym0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, keym1));
  ASSERT_TRUE(keym0.size() == keym1.size());
  for (size_t n = 0; n < keym0.size(); ++n)
  {
    ASSERT_TRUE(keym0[n].size() == keym1[n].size());
    for (size_t i = 0; i < keym0[n].size(); ++i)
    {
      ASSERT_TRUE(keym0[n][i] == keym1[n][i]);
    }
  }

  rct::skpkGen(ctkey0.dest, ctkey0.mask);
  ASSERT_TRUE(serialization::dump_binary(ctkey0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, ctkey1));
  ASSERT_TRUE(!memcmp(&ctkey0, &ctkey1, sizeof(ctkey0)));

  ctkeyv0 = std::vector<rct::ctkey>(14);
  for (size_t n = 0; n < ctkeyv0.size(); ++n)
    rct::skpkGen(ctkeyv0[n].dest, ctkeyv0[n].mask);
  ASSERT_TRUE(serialization::dump_binary(ctkeyv0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, ctkeyv1));
  ASSERT_TRUE(ctkeyv0.size() == ctkeyv1.size());
  for (size_t n = 0; n < ctkeyv0.size(); ++n)
  {
    ASSERT_TRUE(!memcmp(&ctkeyv0[n], &ctkeyv1[n], sizeof(ctkeyv0[n])));
  }

  ctkeym0 = std::vector<rct::ctkeyV>(9);
  for (size_t n = 0; n < ctkeym0.size(); ++n)
  {
    ctkeym0[n] = std::vector<rct::ctkey>(11);
    for (size_t i = 0; i < ctkeym0[n].size(); ++i)
      rct::skpkGen(ctkeym0[n][i].dest, ctkeym0[n][i].mask);
  }
  ASSERT_TRUE(serialization::dump_binary(ctkeym0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, ctkeym1));
  ASSERT_TRUE(ctkeym0.size() == ctkeym1.size());
  for (size_t n = 0; n < ctkeym0.size(); ++n)
  {
    ASSERT_TRUE(ctkeym0[n].size() == ctkeym1[n].size());
    for (size_t i = 0; i < ctkeym0.size(); ++i)
    {
      ASSERT_TRUE(!memcmp(&ctkeym0[n][i], &ctkeym1[n][i], sizeof(ctkeym0[n][i])));
    }
  }

  ecdh0.mask = rct::skGen();
  ecdh0.amount = rct::skGen();
  ecdh0.senderPk = rct::skGen();
  ASSERT_TRUE(serialization::dump_binary(ecdh0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, ecdh1));
  ASSERT_TRUE(!memcmp(&ecdh0.mask, &ecdh1.mask, sizeof(ecdh0.mask)));
  ASSERT_TRUE(!memcmp(&ecdh0.amount, &ecdh1.amount, sizeof(ecdh0.amount)));
  // senderPk is not serialized

  for (size_t n = 0; n < 64; ++n)
  {
    boro0.s0[n] = rct::skGen();
    boro0.s1[n] = rct::skGen();
  }
  boro0.ee = rct::skGen();
  ASSERT_TRUE(serialization::dump_binary(boro0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, boro1));
  ASSERT_TRUE(!memcmp(&boro0, &boro1, sizeof(boro0)));

  // create a full rct signature to use its innards
  rct::ctkeyV sc, pc;
  rct::ctkey sctmp, pctmp;
  tie(sctmp, pctmp) = rct::ctskpkGen(6000);
  sc.push_back(sctmp);
  pc.push_back(pctmp);
  tie(sctmp, pctmp) = rct::ctskpkGen(7000);
  sc.push_back(sctmp);
  pc.push_back(pctmp);
  vector<uint64_t> amounts;
  rct::keyV amount_keys;
  //add output 500
  amounts.push_back(500);
  amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
  rct::keyV destinations;
  rct::key Sk, Pk;
  rct::skpkGen(Sk, Pk);
  destinations.push_back(Pk);
  //add output for 12500
  amounts.push_back(12500);
  amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
  rct::skpkGen(Sk, Pk);
  destinations.push_back(Pk);
  //compute rct data with mixin 500
  s0 = rct::genRct(rct::zero(), sc, pc, destinations, amounts, amount_keys, NULL, NULL, 3, hw::get_device("default"));

  mg0 = s0.p.MGs[0];
  ASSERT_TRUE(serialization::dump_binary(mg0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, mg1));
  ASSERT_TRUE(mg0.ss.size() == mg1.ss.size());
  for (size_t n = 0; n < mg0.ss.size(); ++n)
  {
    ASSERT_TRUE(mg0.ss[n] == mg1.ss[n]);
  }
  ASSERT_TRUE(mg0.cc == mg1.cc);

  // mixRing and II are not serialized, they are meant to be reconstructed
  ASSERT_TRUE(mg1.II.empty());

  rg0 = s0.p.rangeSigs.front();
  ASSERT_TRUE(serialization::dump_binary(rg0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, rg1));
  ASSERT_TRUE(!memcmp(&rg0, &rg1, sizeof(rg0)));

#if 0
  ASSERT_TRUE(serialization::dump_binary(s0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, s1));
  ASSERT_TRUE(s0.type == s1.type);
  ASSERT_TRUE(s0.p.rangeSigs.size() == s1.p.rangeSigs.size());
  for (size_t n = 0; n < s0.p.rangeSigs.size(); ++n)
  {
    ASSERT_TRUE(!memcmp(&s0.p.rangeSigs[n], &s1.p.rangeSigs[n], sizeof(s0.p.rangeSigs[n])));
  }
  ASSERT_TRUE(s0.p.MGs.size() == s1.p.MGs.size());
  ASSERT_TRUE(s0.p.MGs[0].ss.size() == s1.p.MGs[0].ss.size());
  for (size_t n = 0; n < s0.p.MGs[0].ss.size(); ++n)
  {
    ASSERT_TRUE(s0.p.MGs[0].ss[n] == s1.p.MGs[0].ss[n]);
  }
  ASSERT_TRUE(s0.p.MGs[0].cc == s1.p.MGs[0].cc);
  // mixRing and II are not serialized, they are meant to be reconstructed
  ASSERT_TRUE(s1.p.MGs[0].II.empty());

  // mixRing and II are not serialized, they are meant to be reconstructed
  ASSERT_TRUE(s1.mixRing.size() == 0);

  ASSERT_TRUE(s0.ecdhInfo.size() == s1.ecdhInfo.size());
  for (size_t n = 0; n < s0.ecdhInfo.size(); ++n)
  {
    ASSERT_TRUE(!memcmp(&s0.ecdhInfo[n], &s1.ecdhInfo[n], sizeof(s0.ecdhInfo[n])));
  }
  ASSERT_TRUE(s0.outPk.size() == s1.outPk.size());
  for (size_t n = 0; n < s0.outPk.size(); ++n)
  {
    // serialization only does the mask
    ASSERT_TRUE(!memcmp(&s0.outPk[n].mask, &s1.outPk[n].mask, sizeof(s0.outPk[n].mask)));
  }
#endif

  tx0.set_null();
  tx0.version = 2;
  cryptonote::txin_to_key txin_to_key1{};
  txin_to_key1.amount = 100;
  txin_to_key1.key_offsets.resize(4);
  cryptonote::txin_to_key txin_to_key2{};
  txin_to_key2.amount = 200;
  txin_to_key2.key_offsets.resize(4);
  tx0.vin.push_back(txin_to_key1);
  tx0.vin.push_back(txin_to_key2);
  tx0.vout.push_back(cryptonote::tx_out());
  tx0.vout.push_back(cryptonote::tx_out());
  tx0.rct_signatures = s0;
  ASSERT_EQ(tx0.rct_signatures.p.rangeSigs.size(), 2);
  ASSERT_TRUE(serialization::dump_binary(tx0, blob));
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx1.rct_signatures.p.rangeSigs.size(), 2);
  std::string blob2;
  ASSERT_TRUE(serialization::dump_binary(tx1, blob2));
  ASSERT_TRUE(blob == blob2);
}

TEST(Serialization, portability_wallet)
{
  const cryptonote::network_type nettype = cryptonote::TESTNET;
  const bool restricted = false;
  tools::wallet2 w(nettype, restricted);
  const boost::filesystem::path wallet_file = unit_test::data_dir / "wallet_9svHk1";
  string password = "test";
  bool r = false;
  try
  {
    w.load(wallet_file.string(), password);
    r = true;
  }
  catch (const exception& e)
  {}
  ASSERT_TRUE(r);
  /*
  fields of tools::wallet2 to be checked: 
    std::vector<crypto::hash>                                       m_blockchain
    std::vector<transfer_details>                                   m_transfers               // TODO
    cryptonote::account_public_address                              m_account_public_address
    std::unordered_map<crypto::key_image, size_t>                   m_key_images
    std::unordered_map<crypto::hash, unconfirmed_transfer_details>  m_unconfirmed_txs
    std::unordered_multimap<crypto::hash, payment_details>          m_payments
    std::unordered_map<crypto::hash, crypto::secret_key>            m_tx_keys
    std::unordered_map<crypto::hash, confirmed_transfer_details>    m_confirmed_txs
    std::unordered_map<crypto::hash, std::string>                   m_tx_notes
    std::unordered_map<crypto::hash, payment_details>               m_unconfirmed_payments
    std::unordered_map<crypto::public_key, size_t>                  m_pub_keys
    std::vector<tools::wallet2::address_book_row>                   m_address_book
  */
  // blockchain
  ASSERT_TRUE(w.m_blockchain.size() == 1);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(w.m_blockchain[0]) == "48ca7cd3c8de5b6a4d53d2861fbdaedca141553559f9be9520068053cda8430b");
  // transfers (TODO)
  ASSERT_TRUE(w.m_transfers.size() == 3);
  // account public address
  ASSERT_TRUE(epee::string_tools::pod_to_hex(w.m_account_public_address.m_view_public_key) == "e47d4b6df6ab7339539148c2a03ad3e2f3434e5ab2046848e1f21369a3937cad");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(w.m_account_public_address.m_spend_public_key) == "13daa2af00ad26a372d317195de0bdd716f7a05d33bc4d7aff1664b6ee93c060");
  // key images
  ASSERT_TRUE(w.m_key_images.size() == 3);
  {
    crypto::key_image ki[3];
    epee::string_tools::hex_to_pod("c5680d3735b90871ca5e3d90cd82d6483eed1151b9ab75c2c8c3a7d89e00a5a8", ki[0]);
    epee::string_tools::hex_to_pod("d54cbd435a8d636ad9b01b8d4f3eb13bd0cf1ce98eddf53ab1617f9b763e66c0", ki[1]);
    epee::string_tools::hex_to_pod("6c3cd6af97c4070a7aef9b1344e7463e29c7cd245076fdb65da447a34da3ca76", ki[2]);
    ASSERT_EQ_MAP(0, w.m_key_images, ki[0]);
    ASSERT_EQ_MAP(1, w.m_key_images, ki[1]);
    ASSERT_EQ_MAP(2, w.m_key_images, ki[2]);
  }
  // unconfirmed txs
  ASSERT_TRUE(w.m_unconfirmed_txs.size() == 0);
  // payments
  ASSERT_TRUE(w.m_payments.size() == 2);
  {
    auto pd0 = w.m_payments.begin();
    auto pd1 = pd0;
    ++pd1;
    ASSERT_TRUE(epee::string_tools::pod_to_hex(pd0->first) == "0000000000000000000000000000000000000000000000000000000000000000");
    ASSERT_TRUE(epee::string_tools::pod_to_hex(pd1->first) == "0000000000000000000000000000000000000000000000000000000000000000");
    if (epee::string_tools::pod_to_hex(pd0->second.m_tx_hash) == "ec34c9bb12b99af33d49691384eee5bed9171498ff04e59516505f35d1fc5efc")
      swap(pd0, pd1);
    ASSERT_TRUE(epee::string_tools::pod_to_hex(pd0->second.m_tx_hash) == "15024343b38e77a1a9860dfed29921fa17e833fec837191a6b04fa7cb9605b8e");
    ASSERT_TRUE(epee::string_tools::pod_to_hex(pd1->second.m_tx_hash) == "ec34c9bb12b99af33d49691384eee5bed9171498ff04e59516505f35d1fc5efc");
    ASSERT_TRUE(pd0->second.m_amount == 13400845012231);
    ASSERT_TRUE(pd1->second.m_amount == 1200000000000);
    ASSERT_TRUE(pd0->second.m_block_height == 818424);
    ASSERT_TRUE(pd1->second.m_block_height == 818522);
    ASSERT_TRUE(pd0->second.m_unlock_time == 818484);
    ASSERT_TRUE(pd1->second.m_unlock_time == 0);
    ASSERT_TRUE(pd0->second.m_timestamp == 1483263366);
    ASSERT_TRUE(pd1->second.m_timestamp == 1483272963);
  }
  // tx keys
  ASSERT_TRUE(w.m_tx_keys.size() == 2);
  {
    const std::vector<std::pair<std::string, std::string>> txid_txkey =
    {
      {"b9aac8c020ab33859e0c0b6331f46a8780d349e7ac17b067116e2d87bf48daad", "bf3614c6de1d06c09add5d92a5265d8c76af706f7bc6ac830d6b0d109aa87701"},
      {"6e7013684d35820f66c6679197ded9329bfe0e495effa47e7b25258799858dba", "e556884246df5a787def6732c6ea38f1e092fa13e5ea98f732b99c07a6332003"},
    };
    for (size_t i = 0; i < txid_txkey.size(); ++i)
    {
      crypto::hash txid;
      crypto::secret_key txkey;
      epee::string_tools::hex_to_pod(txid_txkey[i].first, txid);
      epee::string_tools::hex_to_pod(txid_txkey[i].second, txkey);
      ASSERT_EQ_MAP(txkey, w.m_tx_keys, txid);
    }
  }
  // confirmed txs
  ASSERT_TRUE(w.m_confirmed_txs.size() == 1);
  // tx notes
  ASSERT_TRUE(w.m_tx_notes.size() == 2);
  {
    crypto::hash h[2];
    epee::string_tools::hex_to_pod("15024343b38e77a1a9860dfed29921fa17e833fec837191a6b04fa7cb9605b8e", h[0]);
    epee::string_tools::hex_to_pod("6e7013684d35820f66c6679197ded9329bfe0e495effa47e7b25258799858dba", h[1]);
    ASSERT_EQ_MAP("sample note", w.m_tx_notes, h[0]);
    ASSERT_EQ_MAP("sample note 2", w.m_tx_notes, h[1]);
  }
  // unconfirmed payments
  ASSERT_TRUE(w.m_unconfirmed_payments.size() == 0);
  // pub keys
  ASSERT_TRUE(w.m_pub_keys.size() == 3);
  {
    crypto::public_key pubkey[3];
    epee::string_tools::hex_to_pod("33f75f264574cb3a9ea5b24220a5312e183d36dc321c9091dfbb720922a4f7b0", pubkey[0]);
    epee::string_tools::hex_to_pod("5066ff2ce9861b1d131cf16eeaa01264933a49f28242b97b153e922ec7b4b3cb", pubkey[1]);
    epee::string_tools::hex_to_pod("0d8467e16e73d16510452b78823e082e05ee3a63788d40de577cf31eb555f0c8", pubkey[2]);
    ASSERT_EQ_MAP(0, w.m_pub_keys, pubkey[0]);
    ASSERT_EQ_MAP(1, w.m_pub_keys, pubkey[1]);
    ASSERT_EQ_MAP(2, w.m_pub_keys, pubkey[2]);
  }
  // address book
  ASSERT_TRUE(w.m_address_book.size() == 1);
  {
    auto address_book_row = w.m_address_book.begin();
    ASSERT_TRUE(epee::string_tools::pod_to_hex(address_book_row->m_address.m_spend_public_key) == "9bc53a6ff7b0831c9470f71b6b972dbe5ad1e8606f72682868b1dda64e119fb3");
    ASSERT_TRUE(epee::string_tools::pod_to_hex(address_book_row->m_address.m_view_public_key) == "49fece1ef97dc0c0f7a5e2106e75e96edd910f7e86b56e1e308cd0cf734df191");
    ASSERT_TRUE(epee::string_tools::pod_to_hex(address_book_row->m_payment_id) == "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    ASSERT_TRUE(address_book_row->m_description == "testnet wallet 9y52S6");
  }
}

#define OUTPUT_EXPORT_FILE_MAGIC "Monero output export\003"
TEST(Serialization, portability_outputs)
{
  // read file
  const boost::filesystem::path filename = unit_test::data_dir / "outputs";
  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(filename.string(), data);
  ASSERT_TRUE(r);
  const size_t magiclen = strlen(OUTPUT_EXPORT_FILE_MAGIC);
  ASSERT_FALSE(data.size() < magiclen || memcmp(data.data(), OUTPUT_EXPORT_FILE_MAGIC, magiclen));
  // decrypt (copied from wallet2::decrypt)
  auto decrypt = [] (const std::string &ciphertext, const crypto::secret_key &skey, bool authenticated) -> string
  {
    const size_t prefix_size = sizeof(chacha_iv) + (authenticated ? sizeof(crypto::signature) : 0);
    if(ciphertext.size() < prefix_size)
      return {};
    crypto::chacha_key key;
    crypto::generate_chacha_key(&skey, sizeof(skey), key);
    const crypto::chacha_iv &iv = *(const crypto::chacha_iv*)&ciphertext[0];
    std::string plaintext;
    plaintext.resize(ciphertext.size() - prefix_size);
    if (authenticated)
    {
      crypto::hash hash;
      crypto::cn_fast_hash(ciphertext.data(), ciphertext.size() - sizeof(signature), hash);
      crypto::public_key pkey;
      crypto::secret_key_to_public_key(skey, pkey);
      const crypto::signature &signature = *(const crypto::signature*)&ciphertext[ciphertext.size() - sizeof(crypto::signature)];
      if(!crypto::check_signature(hash, pkey, signature))
        return {};
    }
    crypto::chacha8(ciphertext.data() + sizeof(iv), ciphertext.size() - prefix_size, key, iv, &plaintext[0]);
    return std::move(plaintext);
  };
  crypto::secret_key view_secret_key;
  epee::string_tools::hex_to_pod("339673bb1187e2f73ba7841ab6841c5553f96e9f13f8fe6612e69318db4e9d0a", view_secret_key);
  bool authenticated = true;
  data = decrypt(std::string(data, magiclen), view_secret_key, authenticated);
  ASSERT_FALSE(data.empty());
  // check public view/spend keys
  const size_t headerlen = 2 * sizeof(crypto::public_key);
  ASSERT_FALSE(data.size() < headerlen);
  const crypto::public_key &public_spend_key = *(const crypto::public_key*)&data[0];
  const crypto::public_key &public_view_key = *(const crypto::public_key*)&data[sizeof(crypto::public_key)];
  ASSERT_TRUE(epee::string_tools::pod_to_hex(public_spend_key) == "13daa2af00ad26a372d317195de0bdd716f7a05d33bc4d7aff1664b6ee93c060");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(public_view_key) == "e47d4b6df6ab7339539148c2a03ad3e2f3434e5ab2046848e1f21369a3937cad");
  r = false;
  std::vector<tools::wallet2::transfer_details> outputs;
  try
  {
    std::istringstream iss(std::string(data, headerlen));
    boost::archive::portable_binary_iarchive ar(iss);
    ar >> outputs;
    r = true;
  }
  catch (...)
  {}
  ASSERT_TRUE(r);
  /*
  fields of tools::wallet2::transfer_details to be checked: 
    uint64_t                        m_block_height
    cryptonote::transaction_prefix  m_tx                        // TODO
    crypto::hash                    m_txid
    size_t                          m_internal_output_index
    uint64_t                        m_global_output_index
    bool                            m_spent
    uint64_t                        m_spent_height
    crypto::key_image               m_key_image
    rct::key                        m_mask
    uint64_t                        m_amount
    bool                            m_rct
    bool                            m_key_image_known
    size_t                          m_pk_index
  */
  ASSERT_TRUE(outputs.size() == 3);
  auto& td0 = outputs[0];
  auto& td1 = outputs[1];
  auto& td2 = outputs[2];
  ASSERT_TRUE(td0.m_block_height == 818424);
  ASSERT_TRUE(td1.m_block_height == 818522);
  ASSERT_TRUE(td2.m_block_height == 818522);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td0.m_txid) == "15024343b38e77a1a9860dfed29921fa17e833fec837191a6b04fa7cb9605b8e");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td1.m_txid) == "ec34c9bb12b99af33d49691384eee5bed9171498ff04e59516505f35d1fc5efc");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td2.m_txid) == "6e7013684d35820f66c6679197ded9329bfe0e495effa47e7b25258799858dba");
  ASSERT_TRUE(td0.m_internal_output_index == 0);
  ASSERT_TRUE(td1.m_internal_output_index == 0);
  ASSERT_TRUE(td2.m_internal_output_index == 1);
  ASSERT_TRUE(td0.m_global_output_index == 19642);
  ASSERT_TRUE(td1.m_global_output_index == 19757);
  ASSERT_TRUE(td2.m_global_output_index == 19760);
  ASSERT_TRUE (td0.m_spent);
  ASSERT_FALSE(td1.m_spent);
  ASSERT_FALSE(td2.m_spent);
  ASSERT_TRUE(td0.m_spent_height == 0);
  ASSERT_TRUE(td1.m_spent_height == 0);
  ASSERT_TRUE(td2.m_spent_height == 0);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td0.m_key_image) == "c5680d3735b90871ca5e3d90cd82d6483eed1151b9ab75c2c8c3a7d89e00a5a8");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td1.m_key_image) == "d54cbd435a8d636ad9b01b8d4f3eb13bd0cf1ce98eddf53ab1617f9b763e66c0");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td2.m_key_image) == "6c3cd6af97c4070a7aef9b1344e7463e29c7cd245076fdb65da447a34da3ca76");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td0.m_mask) == "0100000000000000000000000000000000000000000000000000000000000000");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td1.m_mask) == "d3997a7b27fa199a377643b88cbd3f20f447496746dabe92d288730ecaeda007");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td2.m_mask) == "789bafff169ef206aa21219342c69ca52ce1d78d776c10b21d14bdd960fc7703");
  ASSERT_TRUE(td0.m_amount == 13400845012231);
  ASSERT_TRUE(td1.m_amount == 1200000000000);
  ASSERT_TRUE(td2.m_amount == 11066009260865);
  ASSERT_TRUE(td0.m_rct);
  ASSERT_TRUE(td1.m_rct);
  ASSERT_TRUE(td2.m_rct);
  ASSERT_TRUE(td0.m_key_image_known);
  ASSERT_TRUE(td1.m_key_image_known);
  ASSERT_TRUE(td2.m_key_image_known);
  ASSERT_TRUE(td0.m_pk_index == 0);
  ASSERT_TRUE(td1.m_pk_index == 0);
  ASSERT_TRUE(td2.m_pk_index == 0);
}

#define UNSIGNED_TX_PREFIX "Monero unsigned tx set\003"
TEST(Serialization, portability_unsigned_tx)
{
  const boost::filesystem::path filename = unit_test::data_dir / "unsigned_monero_tx";
  std::string s;
  const cryptonote::network_type nettype = cryptonote::TESTNET;
  bool r = epee::file_io_utils::load_file_to_string(filename.string(), s);
  ASSERT_TRUE(r);
  const size_t magiclen = strlen(UNSIGNED_TX_PREFIX);
  ASSERT_FALSE(strncmp(s.c_str(), UNSIGNED_TX_PREFIX, magiclen));
  tools::wallet2::unsigned_tx_set exported_txs;
  s = s.substr(magiclen);
  r = false;
  try
  {
    std::istringstream iss(s);
    boost::archive::portable_binary_iarchive ar(iss);
    ar >> exported_txs;
    r = true;
  }
  catch (...)  
  {}
  ASSERT_TRUE(r);
  /*
  fields of tools::wallet2::unsigned_tx_set to be checked:
    std::vector<tx_construction_data> txes
    std::vector<wallet2::transfer_details> m_transfers

  fields of toolw::wallet2::tx_construction_data to be checked:
    std::vector<cryptonote::tx_source_entry>      sources
    cryptonote::tx_destination_entry              change_dts
    std::vector<cryptonote::tx_destination_entry> splitted_dsts
    std::list<size_t>                             selected_transfers
    std::vector<uint8_t>                          extra
    uint64_t                                      unlock_time
    bool                                          use_rct
    std::vector<cryptonote::tx_destination_entry> dests
  
  fields of cryptonote::tx_source_entry to be checked:
    std::vector<std::pair<uint64_t, rct::ctkey>>  outputs
    size_t                                        real_output
    crypto::public_key                            real_out_tx_key
    size_t                                        real_output_in_tx_index
    uint64_t                                      amount
    bool                                          rct
    rct::key                                      mask
  
  fields of cryptonote::tx_destination_entry to be checked:
    uint64_t                amount
    account_public_address  addr
  */
  // txes
  ASSERT_TRUE(exported_txs.txes.size() == 1);
  auto& tcd = exported_txs.txes[0];
  // tcd.sources
  ASSERT_TRUE(tcd.sources.size() == 1);
  auto& tse = tcd.sources[0];
  // tcd.sources[0].outputs
  ASSERT_TRUE(tse.outputs.size() == 5);
  auto& out0 = tse.outputs[0];
  auto& out1 = tse.outputs[1];
  auto& out2 = tse.outputs[2];
  auto& out3 = tse.outputs[3];
  auto& out4 = tse.outputs[4];
  ASSERT_TRUE(out0.first == 6295);
  ASSERT_TRUE(out1.first == 14302);
  ASSERT_TRUE(out2.first == 17598);
  ASSERT_TRUE(out3.first == 18671);
  ASSERT_TRUE(out4.first == 19760);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out0.second) == "e7272cb589954ddeedd20de9411ed57265f154d41f33cec9ff69e5d642e09814096490b0ac85308342acf436cc0270d53abef9dc04c6202f2459e879bfd40ce6");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out1.second) == "c3a9f49d1fe75939cc3feb39871ce0a7366c2879a63faa1a5cf34e65723b120a272ff0c7d84ab8b6ee3528d196450b0e28b3fed276bc2597a2b5b17afb9354ab");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out2.second) == "176e239c8c39000c2275e2f63ed7d55c55e0843524091522bbd3d3b869044969021fad70fc1244115449d4754829ae7c47346342ee5d52a2cdd47dfc351d0ab0");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out3.second) == "ef12d7946302fb064f2ba9df1a73d72233ac74664ed3b370580fa3bdc377542ad93f64898bd95851d6efe0d7bf2dbbea9b7c6b3c57e2c807e7b17d55b4622259");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out4.second) == "0d8467e16e73d16510452b78823e082e05ee3a63788d40de577cf31eb555f0c8525096cbc88d00a841eed66f3cdb6f0a018e6ce9fb9433ed61afba15cbbebd04");
  // tcd.sources[0].{real_output, real_out_tx_key, real_output_in_tx_index, amount, rct, mask}
  ASSERT_TRUE(tse.real_output == 4);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(tse.real_out_tx_key) == "4d86c7ba1c285fe4bc1cd7b54ba894fa89fa02fc6b0bbeea67d53251acd14a05");
  ASSERT_TRUE(tse.real_output_in_tx_index == 1); 
  ASSERT_TRUE(tse.amount == 11066009260865);
  ASSERT_TRUE(tse.rct);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(tse.mask) == "789bafff169ef206aa21219342c69ca52ce1d78d776c10b21d14bdd960fc7703");
  // tcd.change_dts
  ASSERT_TRUE(tcd.change_dts.amount == 9631208773403);
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, tcd.change_dts.addr) == "9svHk1wHPo3ULf2AZykghzcye6sitaRE4MaDjPC6uanTHCynHjJHZaiAb922PojE1GexhhRt1LVf5DC43feyrRZMLXQr3mk");
  // tcd.splitted_dsts
  ASSERT_TRUE(tcd.splitted_dsts.size() == 2);
  auto& splitted_dst0 = tcd.splitted_dsts[0];
  auto& splitted_dst1 = tcd.splitted_dsts[1];
  ASSERT_TRUE(splitted_dst0.amount == 1400000000000);
  ASSERT_TRUE(splitted_dst1.amount == 9631208773403);
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, splitted_dst0.addr) == "9xnhrMczQkPeoGi6dyu6BgKAYX4tZsDs6KHCkyTStDBKL4M4pM1gfCR3utmTAcSaKHGa1R5o266FbdnubErmij3oMdLyYgA");
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, splitted_dst1.addr) == "9svHk1wHPo3ULf2AZykghzcye6sitaRE4MaDjPC6uanTHCynHjJHZaiAb922PojE1GexhhRt1LVf5DC43feyrRZMLXQr3mk");
  // tcd.selected_transfers
  ASSERT_TRUE(tcd.selected_transfers.size() == 1);
  ASSERT_TRUE(tcd.selected_transfers.front() == 2);
  // tcd.extra
  ASSERT_TRUE(tcd.extra.size() == 68);
  // tcd.{unlock_time, use_rct}
  ASSERT_TRUE(tcd.unlock_time == 0);
  ASSERT_TRUE(tcd.use_rct);
  // tcd.dests
  ASSERT_TRUE(tcd.dests.size() == 1);
  auto& dest = tcd.dests[0];
  ASSERT_TRUE(dest.amount == 1400000000000);
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, dest.addr) == "9xnhrMczQkPeoGi6dyu6BgKAYX4tZsDs6KHCkyTStDBKL4M4pM1gfCR3utmTAcSaKHGa1R5o266FbdnubErmij3oMdLyYgA");
  // transfers
  ASSERT_TRUE(exported_txs.transfers.size() == 3);
  auto& td0 = exported_txs.transfers[0];
  auto& td1 = exported_txs.transfers[1];
  auto& td2 = exported_txs.transfers[2];
  ASSERT_TRUE(td0.m_block_height == 818424);
  ASSERT_TRUE(td1.m_block_height == 818522);
  ASSERT_TRUE(td2.m_block_height == 818522);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td0.m_txid) == "15024343b38e77a1a9860dfed29921fa17e833fec837191a6b04fa7cb9605b8e");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td1.m_txid) == "ec34c9bb12b99af33d49691384eee5bed9171498ff04e59516505f35d1fc5efc");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td2.m_txid) == "6e7013684d35820f66c6679197ded9329bfe0e495effa47e7b25258799858dba");
  ASSERT_TRUE(td0.m_internal_output_index == 0);
  ASSERT_TRUE(td1.m_internal_output_index == 0);
  ASSERT_TRUE(td2.m_internal_output_index == 1);
  ASSERT_TRUE(td0.m_global_output_index == 19642);
  ASSERT_TRUE(td1.m_global_output_index == 19757);
  ASSERT_TRUE(td2.m_global_output_index == 19760);
  ASSERT_TRUE (td0.m_spent);
  ASSERT_FALSE(td1.m_spent);
  ASSERT_FALSE(td2.m_spent);
  ASSERT_TRUE(td0.m_spent_height == 0);
  ASSERT_TRUE(td1.m_spent_height == 0);
  ASSERT_TRUE(td2.m_spent_height == 0);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td0.m_key_image) == "c5680d3735b90871ca5e3d90cd82d6483eed1151b9ab75c2c8c3a7d89e00a5a8");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td1.m_key_image) == "d54cbd435a8d636ad9b01b8d4f3eb13bd0cf1ce98eddf53ab1617f9b763e66c0");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td2.m_key_image) == "6c3cd6af97c4070a7aef9b1344e7463e29c7cd245076fdb65da447a34da3ca76");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td0.m_mask) == "0100000000000000000000000000000000000000000000000000000000000000");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td1.m_mask) == "d3997a7b27fa199a377643b88cbd3f20f447496746dabe92d288730ecaeda007");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(td2.m_mask) == "789bafff169ef206aa21219342c69ca52ce1d78d776c10b21d14bdd960fc7703");
  ASSERT_TRUE(td0.m_amount == 13400845012231);
  ASSERT_TRUE(td1.m_amount == 1200000000000);
  ASSERT_TRUE(td2.m_amount == 11066009260865);
  ASSERT_TRUE(td0.m_rct);
  ASSERT_TRUE(td1.m_rct);
  ASSERT_TRUE(td2.m_rct);
  ASSERT_TRUE(td0.m_key_image_known);
  ASSERT_TRUE(td1.m_key_image_known);
  ASSERT_TRUE(td2.m_key_image_known);
  ASSERT_TRUE(td0.m_pk_index == 0);
  ASSERT_TRUE(td1.m_pk_index == 0);
  ASSERT_TRUE(td2.m_pk_index == 0);
}

#define SIGNED_TX_PREFIX "Monero signed tx set\003"
TEST(Serialization, portability_signed_tx)
{
  const boost::filesystem::path filename = unit_test::data_dir / "signed_monero_tx";
  const cryptonote::network_type nettype = cryptonote::TESTNET;
  std::string s;
  bool r = epee::file_io_utils::load_file_to_string(filename.string(), s);
  ASSERT_TRUE(r);
  const size_t magiclen = strlen(SIGNED_TX_PREFIX);
  ASSERT_FALSE(strncmp(s.c_str(), SIGNED_TX_PREFIX, magiclen));
  tools::wallet2::signed_tx_set exported_txs;
  s = s.substr(magiclen);
  r = false;
  try
  {
    std::istringstream iss(s);
    boost::archive::portable_binary_iarchive ar(iss);
    ar >> exported_txs;
    r = true;
  }
  catch (...)
  {}
  ASSERT_TRUE(r);
  /*
  fields of tools::wallet2::signed_tx_set to be checked:
    std::vector<pending_tx>         ptx
    std::vector<crypto::key_image>  key_images
  
  fields of tools::walllet2::pending_tx to be checked:
    cryptonote::transaction                       tx                  // TODO
    uint64_t                                      dust
    uint64_t                                      fee
    bool                                          dust_added_to_fee
    cryptonote::tx_destination_entry              change_dts
    std::list<size_t>                             selected_transfers
    std::string                                   key_images
    crypto::secret_key                            tx_key
    std::vector<cryptonote::tx_destination_entry> dests
    tx_construction_data                          construction_data
  */
  // ptx
  ASSERT_TRUE(exported_txs.ptx.size() == 1);
  auto& ptx = exported_txs.ptx[0];
  // ptx.{dust, fee, dust_added_to_fee}
  ASSERT_TRUE (ptx.dust == 0);
  ASSERT_TRUE (ptx.fee == 34800487462);
  ASSERT_FALSE(ptx.dust_added_to_fee);
  // ptx.change.{amount, addr}
  ASSERT_TRUE(ptx.change_dts.amount == 9631208773403);
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, ptx.change_dts.addr) == "9svHk1wHPo3ULf2AZykghzcye6sitaRE4MaDjPC6uanTHCynHjJHZaiAb922PojE1GexhhRt1LVf5DC43feyrRZMLXQr3mk");
  // ptx.selected_transfers
  ASSERT_TRUE(ptx.selected_transfers.size() == 1);
  ASSERT_TRUE(ptx.selected_transfers.front() == 2);
  // ptx.{key_images, tx_key}
  ASSERT_TRUE(ptx.key_images == "<6c3cd6af97c4070a7aef9b1344e7463e29c7cd245076fdb65da447a34da3ca76> ");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(ptx.tx_key) == "0100000000000000000000000000000000000000000000000000000000000000");
  // ptx.dests
  ASSERT_TRUE(ptx.dests.size() == 1);
  ASSERT_TRUE(ptx.dests[0].amount == 1400000000000);
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, ptx.dests[0].addr) == "9xnhrMczQkPeoGi6dyu6BgKAYX4tZsDs6KHCkyTStDBKL4M4pM1gfCR3utmTAcSaKHGa1R5o266FbdnubErmij3oMdLyYgA");
  // ptx.construction_data
  auto& tcd = ptx.construction_data;
  ASSERT_TRUE(tcd.sources.size() == 1);
  auto& tse = tcd.sources[0];
  // ptx.construction_data.sources[0].outputs
  ASSERT_TRUE(tse.outputs.size() == 5);
  auto& out0 = tse.outputs[0];
  auto& out1 = tse.outputs[1];
  auto& out2 = tse.outputs[2];
  auto& out3 = tse.outputs[3];
  auto& out4 = tse.outputs[4];
  ASSERT_TRUE(out0.first == 6295);
  ASSERT_TRUE(out1.first == 14302);
  ASSERT_TRUE(out2.first == 17598);
  ASSERT_TRUE(out3.first == 18671);
  ASSERT_TRUE(out4.first == 19760);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out0.second) == "e7272cb589954ddeedd20de9411ed57265f154d41f33cec9ff69e5d642e09814096490b0ac85308342acf436cc0270d53abef9dc04c6202f2459e879bfd40ce6");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out1.second) == "c3a9f49d1fe75939cc3feb39871ce0a7366c2879a63faa1a5cf34e65723b120a272ff0c7d84ab8b6ee3528d196450b0e28b3fed276bc2597a2b5b17afb9354ab");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out2.second) == "176e239c8c39000c2275e2f63ed7d55c55e0843524091522bbd3d3b869044969021fad70fc1244115449d4754829ae7c47346342ee5d52a2cdd47dfc351d0ab0");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out3.second) == "ef12d7946302fb064f2ba9df1a73d72233ac74664ed3b370580fa3bdc377542ad93f64898bd95851d6efe0d7bf2dbbea9b7c6b3c57e2c807e7b17d55b4622259");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(out4.second) == "0d8467e16e73d16510452b78823e082e05ee3a63788d40de577cf31eb555f0c8525096cbc88d00a841eed66f3cdb6f0a018e6ce9fb9433ed61afba15cbbebd04");
  // ptx.construction_data.sources[0].{real_output, real_out_tx_key, real_output_in_tx_index, amount, rct, mask}
  ASSERT_TRUE(tse.real_output == 4);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(tse.real_out_tx_key) == "4d86c7ba1c285fe4bc1cd7b54ba894fa89fa02fc6b0bbeea67d53251acd14a05");
  ASSERT_TRUE(tse.real_output_in_tx_index == 1); 
  ASSERT_TRUE(tse.amount == 11066009260865);
  ASSERT_TRUE(tse.rct);
  ASSERT_TRUE(epee::string_tools::pod_to_hex(tse.mask) == "789bafff169ef206aa21219342c69ca52ce1d78d776c10b21d14bdd960fc7703");
  // ptx.construction_data.change_dts
  ASSERT_TRUE(tcd.change_dts.amount == 9631208773403);
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, tcd.change_dts.addr) == "9svHk1wHPo3ULf2AZykghzcye6sitaRE4MaDjPC6uanTHCynHjJHZaiAb922PojE1GexhhRt1LVf5DC43feyrRZMLXQr3mk");
  // ptx.construction_data.splitted_dsts
  ASSERT_TRUE(tcd.splitted_dsts.size() == 2);
  auto& splitted_dst0 = tcd.splitted_dsts[0];
  auto& splitted_dst1 = tcd.splitted_dsts[1];
  ASSERT_TRUE(splitted_dst0.amount == 1400000000000);
  ASSERT_TRUE(splitted_dst1.amount == 9631208773403);
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, splitted_dst0.addr) == "9xnhrMczQkPeoGi6dyu6BgKAYX4tZsDs6KHCkyTStDBKL4M4pM1gfCR3utmTAcSaKHGa1R5o266FbdnubErmij3oMdLyYgA");
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, splitted_dst1.addr) == "9svHk1wHPo3ULf2AZykghzcye6sitaRE4MaDjPC6uanTHCynHjJHZaiAb922PojE1GexhhRt1LVf5DC43feyrRZMLXQr3mk");
  // ptx.construction_data.selected_transfers
  ASSERT_TRUE(tcd.selected_transfers.size() == 1);
  ASSERT_TRUE(tcd.selected_transfers.front() == 2);
  // ptx.construction_data.extra
  ASSERT_TRUE(tcd.extra.size() == 68);
  // ptx.construction_data.{unlock_time, use_rct}
  ASSERT_TRUE(tcd.unlock_time == 0);
  ASSERT_TRUE(tcd.use_rct);
  // ptx.construction_data.dests
  ASSERT_TRUE(tcd.dests.size() == 1);
  auto& dest = tcd.dests[0];
  ASSERT_TRUE(dest.amount == 1400000000000);
  ASSERT_TRUE(cryptonote::get_account_address_as_str(nettype, false, dest.addr) == "9xnhrMczQkPeoGi6dyu6BgKAYX4tZsDs6KHCkyTStDBKL4M4pM1gfCR3utmTAcSaKHGa1R5o266FbdnubErmij3oMdLyYgA");
  // key_images
  ASSERT_TRUE(exported_txs.key_images.size() == 3);
  auto& ki0 = exported_txs.key_images[0];
  auto& ki1 = exported_txs.key_images[1];
  auto& ki2 = exported_txs.key_images[2];
  ASSERT_TRUE(epee::string_tools::pod_to_hex(ki0) == "c5680d3735b90871ca5e3d90cd82d6483eed1151b9ab75c2c8c3a7d89e00a5a8");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(ki1) == "d54cbd435a8d636ad9b01b8d4f3eb13bd0cf1ce98eddf53ab1617f9b763e66c0");
  ASSERT_TRUE(epee::string_tools::pod_to_hex(ki2) == "6c3cd6af97c4070a7aef9b1344e7463e29c7cd245076fdb65da447a34da3ca76");
}
