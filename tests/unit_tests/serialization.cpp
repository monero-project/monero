// Copyright (c) 2014-2016, The Monero Project
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
#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_basic_impl.h"
#include "ringct/rctSigs.h"
#include "serialization/serialization.h"
#include "serialization/binary_archive.h"
#include "serialization/json_archive.h"
#include "serialization/debug_archive.h"
#include "serialization/variant.h"
#include "serialization/vector.h"
#include "serialization/binary_utils.h"
#include "gtest/gtest.h"
using namespace std;

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
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Miner tx with 2 signatures
  tx.signatures[0].resize(1);
  tx.signatures[1].resize(1);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Two txin_gen, no signatures
  tx.vin.push_back(txin_gen1);
  tx.signatures.resize(0);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(9, blob.size()); // 5 bytes + 2 * 2 bytes vins + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx, tx1);
  ASSERT_EQ(linearize_vector2(tx.signatures), linearize_vector2(tx1.signatures));

  // Two txin_gen, signatures vector contains only one empty element
  tx.signatures.resize(1);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Two txin_gen, signatures vector contains two empty elements
  tx.signatures.resize(2);
  ASSERT_TRUE(serialization::dump_binary(tx, blob));
  ASSERT_EQ(9, blob.size()); // 5 bytes + 2 * 2 bytes vins + 0 bytes extra + 0 bytes signatures
  ASSERT_TRUE(serialization::parse_binary(blob, tx1));
  ASSERT_EQ(tx, tx1);
  ASSERT_EQ(linearize_vector2(tx.signatures), linearize_vector2(tx1.signatures));

  // Two txin_gen, signatures vector contains three empty elements
  tx.signatures.resize(3);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Two txin_gen, signatures vector contains two non empty elements
  tx.signatures.resize(2);
  tx.signatures[0].resize(1);
  tx.signatures[1].resize(1);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // A few bytes instead of signature
  tx.vin.clear();
  tx.vin.push_back(txin_gen1);
  tx.signatures.clear();
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
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // Too much signatures for two inputs
  tx.signatures.resize(3);
  tx.signatures[0].resize(2);
  tx.signatures[1].resize(2);
  tx.signatures[2].resize(2);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // First signatures vector contains too little elements
  tx.signatures.resize(2);
  tx.signatures[0].resize(1);
  tx.signatures[1].resize(2);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // First signatures vector contains too much elements
  tx.signatures.resize(2);
  tx.signatures[0].resize(3);
  tx.signatures[1].resize(2);
  ASSERT_FALSE(serialization::dump_binary(tx, blob));

  // There are signatures for each input
  tx.signatures.resize(2);
  tx.signatures[0].resize(2);
  tx.signatures[1].resize(2);
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
  s0 = rct::genRct(rct::zero(), sc, pc, destinations, amounts, amount_keys, 3);

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
