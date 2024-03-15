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

#pragma once

#include "crypto/crypto.h"
#include "ringct/rctOps.h"

enum test_op
{
  op_sc_add,
  op_sc_sub,
  op_sc_mul,
  op_ge_add_raw,
  op_ge_add_p3_p3,
  op_zeroCommitCached,
  ops_fast,

  op_addKeys,
  op_scalarmultBase,
  op_scalarmultKey,
  op_scalarmultH,
  op_scalarmult8,
  op_scalarmult8_p3,
  op_ge_dsm_precomp,
  op_ge_double_scalarmult_base_vartime,
  op_ge_triple_scalarmult_base_vartime,
  op_ge_double_scalarmult_precomp_vartime,
  op_ge_triple_scalarmult_precomp_vartime,
  op_ge_double_scalarmult_precomp_vartime2,
  op_addKeys2,
  op_addKeys3,
  op_addKeys3_2,
  op_addKeys_aGbBcC,
  op_addKeys_aAbBcC,
  op_isInMainSubgroup,
  op_zeroCommitUncached,
};

template<test_op op>
class test_crypto_ops
{
public:
  static const size_t loop_count = op < ops_fast ? 10000000 : 1000;

  bool init()
  {
    scalar0 = rct::skGen();
    scalar1 = rct::skGen();
    scalar2 = rct::skGen();
    point0 = rct::scalarmultBase(rct::skGen());
    point1 = rct::scalarmultBase(rct::skGen());
    point2 = rct::scalarmultBase(rct::skGen());
    if (ge_frombytes_vartime(&p3_0, point0.bytes) != 0)
      return false;
    if (ge_frombytes_vartime(&p3_1, point1.bytes) != 0)
      return false;
    if (ge_frombytes_vartime(&p3_2, point2.bytes) != 0)
      return false;
    ge_p3_to_cached(&cached, &p3_0);
    rct::precomp(precomp0, point0);
    rct::precomp(precomp1, point1);
    rct::precomp(precomp2, point2);
    return true;
  }

  bool test()
  {
    rct::key key;
    ge_cached tmp_cached;
    ge_p1p1 tmp_p1p1;
    ge_p2 tmp_p2;
    ge_dsmp dsmp;
    switch (op)
    {
      case op_sc_add: sc_add(key.bytes, scalar0.bytes, scalar1.bytes); break;
      case op_sc_sub: sc_sub(key.bytes, scalar0.bytes, scalar1.bytes); break;
      case op_sc_mul: sc_mul(key.bytes, scalar0.bytes, scalar1.bytes); break;
      case op_ge_add_p3_p3: {
        ge_p3_to_cached(&tmp_cached, &p3_0);
        ge_add(&tmp_p1p1, &p3_1, &tmp_cached);
        ge_p1p1_to_p3(&p3_1, &tmp_p1p1);
        break;
      }
      case op_ge_add_raw: ge_add(&tmp_p1p1, &p3_1, &cached); break;
      case op_addKeys: rct::addKeys(key, point0, point1); break;
      case op_scalarmultBase: rct::scalarmultBase(scalar0); break;
      case op_scalarmultKey: rct::scalarmultKey(point0, scalar0); break;
      case op_scalarmultH: rct::scalarmultH(scalar0); break;
      case op_scalarmult8: rct::scalarmult8(point0); break;
      case op_scalarmult8_p3: rct::scalarmult8(p3_0,point0); break;
      case op_ge_dsm_precomp: ge_dsm_precomp(dsmp, &p3_0); break;
      case op_ge_double_scalarmult_base_vartime: ge_double_scalarmult_base_vartime(&tmp_p2, scalar0.bytes, &p3_0, scalar1.bytes); break;
      case op_ge_triple_scalarmult_base_vartime: ge_triple_scalarmult_base_vartime(&tmp_p2, scalar0.bytes, scalar1.bytes, precomp1, scalar2.bytes, precomp2); break;
      case op_ge_double_scalarmult_precomp_vartime: ge_double_scalarmult_precomp_vartime(&tmp_p2, scalar0.bytes, &p3_0, scalar1.bytes, precomp0); break;
      case op_ge_triple_scalarmult_precomp_vartime: ge_triple_scalarmult_precomp_vartime(&tmp_p2, scalar0.bytes, precomp0, scalar1.bytes, precomp1, scalar2.bytes, precomp2); break;
      case op_ge_double_scalarmult_precomp_vartime2: ge_double_scalarmult_precomp_vartime2(&tmp_p2, scalar0.bytes, precomp0, scalar1.bytes, precomp1); break;
      case op_addKeys2: rct::addKeys2(key, scalar0, scalar1, point0); break;
      case op_addKeys3: rct::addKeys3(key, scalar0, point0, scalar1, precomp1); break;
      case op_addKeys3_2: rct::addKeys3(key, scalar0, precomp0, scalar1, precomp1); break;
      case op_addKeys_aGbBcC: rct::addKeys_aGbBcC(key, scalar0, scalar1, precomp1, scalar2, precomp2); break;
      case op_addKeys_aAbBcC: rct::addKeys_aAbBcC(key, scalar0, precomp0, scalar1, precomp1, scalar2, precomp2); break;
      case op_isInMainSubgroup: rct::isInMainSubgroup(point0); break;
      case op_zeroCommitUncached: rct::zeroCommit(9001); break;
      case op_zeroCommitCached: rct::zeroCommit(9000); break;
      default: return false;
    }
    return true;
  }

private:
  rct::key scalar0, scalar1, scalar2;
  rct::key point0, point1, point2;
  ge_p3 p3_0, p3_1, p3_2;
  ge_cached cached;
  ge_dsmp precomp0, precomp1, precomp2;
};
