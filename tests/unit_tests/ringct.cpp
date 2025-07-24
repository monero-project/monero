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
#include <algorithm>
#include <sstream>

#include "curve_trees.h"
#include "fcmp_pp/proof_len.h"
#include "fcmp_pp/prove.h"
#include "ringct/rctTypes.h"
#include "ringct/rctSigs.h"
#include "ringct/rctOps.h"
#include "device/device.hpp"
#include "string_tools.h"

using namespace std;
using namespace crypto;
using namespace rct;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static constexpr rct::xmr_amount MAX_AMOUNT_FCMP_PP = MONEY_SUPPLY /
  (FCMP_PLUS_PLUS_MAX_INPUTS + FCMP_PLUS_PLUS_MAX_OUTPUTS + 1);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
namespace
{
struct OutputContextsAndKeys
{
  std::vector<crypto::secret_key> x_vec;
  //std::vector<crypto::secret_key> y_vec;
  std::vector<crypto::secret_key> z_vec;
  std::vector<rct::xmr_amount> amount_vec;
  std::vector<fcmp_pp::curve_trees::OutputContext> outputs;
};
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static const OutputContextsAndKeys generate_random_outputs(const CurveTreesV1 &curve_trees,
  const std::size_t old_n_leaf_tuples,
  const std::size_t new_n_leaf_tuples)
{
  OutputContextsAndKeys outs;
  outs.x_vec.reserve(new_n_leaf_tuples);
  //outs.y_vec.reserve(new_n_leaf_tuples);
  outs.z_vec.reserve(new_n_leaf_tuples);
  outs.amount_vec.reserve(new_n_leaf_tuples);
  outs.outputs.reserve(new_n_leaf_tuples);

  for (std::size_t i = 0; i < new_n_leaf_tuples; ++i)
  {
    const std::uint64_t output_id = old_n_leaf_tuples + i;

    crypto::secret_key &x = outs.x_vec.emplace_back();
    //crypto::secret_key &y = outs.y_vec.emplace_back();
    crypto::secret_key &z = outs.z_vec.emplace_back();
    rct::xmr_amount &amount = outs.amount_vec.emplace_back();

    // Generate random O = G (no y component)
    crypto::public_key O;
    crypto::generate_keys(O, x, crypto::null_skey, false);

    // Generate random a, z, C = z G + a H
    rct::key C;
    amount = rct::randXmrAmount(MAX_AMOUNT_FCMP_PP);
    z = rct::rct2sk(rct::skGen());
    C = rct::commit(amount, rct::sk2rct(z));

    auto output_pair = fcmp_pp::curve_trees::OutputPair{
            .output_pubkey = O,
            .commitment    = C
        };

    auto output_context = fcmp_pp::curve_trees::OutputContext{
            .output_id   = output_id,
            .output_pair = std::move(output_pair)
        };

    outs.outputs.emplace_back(std::move(output_context));
  }

  return outs;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
static void make_balanced_rerandomized_output_set_rct(const ctkeyV &pre_rand_input_pairs,
  const ctkeyV &in_sks,
  const keyV &amount_keys,
  std::vector<FcmpRerandomizedOutputCompressed> &rerandomized_outputs_out)
{
  const size_t n_inputs = pre_rand_input_pairs.size();

  // collect K_o, C_a, z separately, generate r_o
  std::vector<crypto::public_key> input_onetime_addresses(n_inputs);
  std::vector<crypto::ec_point> input_amount_commitments(n_inputs);
  std::vector<crypto::secret_key> input_amount_blinding_factors(n_inputs);
  std::vector<crypto::secret_key> r_o(n_inputs);
  for (size_t i = 0; i < n_inputs; ++i)
  {
    const ctkey &pre_rand_input_pair = pre_rand_input_pairs.at(i);
    input_onetime_addresses[i] = rct::rct2pk(pre_rand_input_pair.dest);
    input_amount_commitments[i] = rct::rct2pk(pre_rand_input_pair.mask);
    input_amount_blinding_factors[i] = rct::rct2sk(in_sks.at(i).mask);
    crypto::random32_unbiased(to_bytes(r_o[i]));
  }

  // calculate output amount blinding factor sum
  crypto::secret_key output_amount_blinding_factor_sum;
  sc_0(to_bytes(output_amount_blinding_factor_sum));
  for (const key &amount_key : amount_keys)
  {
    const key z = genCommitmentMask(amount_key);
    sc_add(to_bytes(output_amount_blinding_factor_sum),
      to_bytes(output_amount_blinding_factor_sum),
      z.bytes);
  }

  fcmp_pp::make_balanced_rerandomized_output_set(input_onetime_addresses,
    input_amount_commitments,
    input_amount_blinding_factors,
    r_o,
    output_amount_blinding_factor_sum,
    rerandomized_outputs_out);
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
TEST(ringct, Borromean)
{
    int j = 0;

        //Tests for Borromean signatures
        //#boro true one, false one, C != sum Ci, and one out of the range..
        int N = 64;
        key64 xv;
        key64 P1v;
        key64 P2v;
        bits indi;

        for (j = 0 ; j < N ; j++) {
            indi[j] = (int)randXmrAmount(2);

            xv[j] = skGen();
            if ( (int)indi[j] == 0 ) {
                scalarmultBase(P1v[j], xv[j]);
            } else {
                addKeys1(P1v[j], xv[j], H2[j]);
            }
            subKeys(P2v[j], P1v[j], H2[j]);
        }

        //#true one
        boroSig bb = genBorromean(xv, P1v, P2v, indi);
        ASSERT_TRUE(verifyBorromean(bb, P1v, P2v));

        //#false one
        indi[3] = (indi[3] + 1) % 2;
        bb = genBorromean(xv, P1v, P2v, indi);
        ASSERT_FALSE(verifyBorromean(bb, P1v, P2v));

        //#true one again
        indi[3] = (indi[3] + 1) % 2;
        bb = genBorromean(xv, P1v, P2v, indi);
        ASSERT_TRUE(verifyBorromean(bb, P1v, P2v));

        //#false one
        bb = genBorromean(xv, P2v, P1v, indi);
        ASSERT_FALSE(verifyBorromean(bb, P1v, P2v));
}

TEST(ringct, MG_sigs)
{
    int j = 0;
    int N = 0;

        //Tests for MG Sigs
        //#MG sig: true one
        N = 3;// #cols
        int   R = 3;// #rows
        keyV xtmp = skvGen(R);
        keyM xm = keyMInit(R, N);// = [[None]*N] #just used to generate test public keys
        keyV sk = skvGen(R);
        keyM P  = keyMInit(R, N);// = keyM[[None]*N] #stores the public keys;
        int ind = 2;
        int i = 0;
        for (j = 0 ; j < R ; j++) {
            for (i = 0 ; i < N ; i++)
            {
                xm[i][j] = skGen();
                P[i][j] = scalarmultBase(xm[i][j]);
            }
        }
        for (j = 0 ; j < R ; j++) {
            sk[j] = xm[ind][j];
        }
        key message = identity();
        mgSig IIccss = MLSAG_Gen(message, P, sk, ind, R, hw::get_device("default"));
        ASSERT_TRUE(MLSAG_Ver(message, P, IIccss, R));

        //#MG sig: false one
        N = 3;// #cols
        R = 3;// #rows
        xtmp = skvGen(R);
        keyM xx(N, xtmp);// = [[None]*N] #just used to generate test public keys
        sk = skvGen(R);
        //P (N, xtmp);// = keyM[[None]*N] #stores the public keys;

        ind = 2;
        for (j = 0 ; j < R ; j++) {
            for (i = 0 ; i < N ; i++)
            {
                xx[i][j] = skGen();
                P[i][j] = scalarmultBase(xx[i][j]);
            }
            sk[j] = xx[ind][j];
        }
        sk[2] = skGen();//assume we don't know one of the private keys..
        IIccss = MLSAG_Gen(message, P, sk, ind, R, hw::get_device("default"));
        ASSERT_FALSE(MLSAG_Ver(message, P, IIccss, R));
}

TEST(ringct, CLSAG)
{
  const size_t N = 11;
  const size_t idx = 5;
  ctkeyV pubs;
  key p, t, t2, u;
  const key message = identity();
  ctkey backup;
  clsag clsag;

  for (size_t i = 0; i < N; ++i)
  {
    key sk;
    ctkey tmp;

    skpkGen(sk, tmp.dest);
    skpkGen(sk, tmp.mask);

    pubs.push_back(tmp);
  }

  // Set P[idx]
  skpkGen(p, pubs[idx].dest);

  // Set C[idx]
  t = skGen();
  u = skGen();
  addKeys2(pubs[idx].mask,t,u,H);

  // Set commitment offset
  key Cout;
  t2 = skGen();
  addKeys2(Cout,t2,u,H);

  // Prepare generation inputs
  ctkey insk;
  insk.dest = p;
  insk.mask = t;
  
  // bad message
  clsag = rct::proveRctCLSAGSimple(zero(),pubs,insk,t2,Cout,idx,hw::get_device("default"));
  ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));

  // bad index at creation
  try
  {
    clsag = rct::proveRctCLSAGSimple(message,pubs,insk,t2,Cout,(idx + 1) % N,hw::get_device("default"));
    ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  }
  catch (...) { /* either exception, or failure to verify above */ }

  // bad z at creation
  try
  {
    ctkey insk2;
    insk2.dest = insk.dest;
    insk2.mask = skGen();
    clsag = rct::proveRctCLSAGSimple(message,pubs,insk2,t2,Cout,idx,hw::get_device("default"));
    ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  }
  catch (...) { /* either exception, or failure to verify above */ }

  // bad C at creation
  backup = pubs[idx];
  pubs[idx].mask = scalarmultBase(skGen());
  try
  {
    clsag = rct::proveRctCLSAGSimple(message,pubs,insk,t2,Cout,idx,hw::get_device("default"));
    ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  }
  catch (...) { /* either exception, or failure to verify above */ }
  pubs[idx] = backup;

  // bad p at creation
  try
  {
    ctkey insk2;
    insk2.dest = skGen();
    insk2.mask = insk.mask;
    clsag = rct::proveRctCLSAGSimple(message,pubs,insk2,t2,Cout,idx,hw::get_device("default"));
    ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  }
  catch (...) { /* either exception, or failure to verify above */ }

  // bad P at creation
  backup = pubs[idx];
  pubs[idx].dest = scalarmultBase(skGen());
  try
  {
    clsag = rct::proveRctCLSAGSimple(message,pubs,insk,t2,Cout,idx,hw::get_device("default"));
    ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  }
  catch (...) { /* either exception, or failure to verify above */ }
  pubs[idx] = backup;

  // Test correct signature
  clsag = rct::proveRctCLSAGSimple(message,pubs,insk,t2,Cout,idx,hw::get_device("default"));
  ASSERT_TRUE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));

  // empty s
  auto sbackup = clsag.s;
  clsag.s.clear();
  ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  clsag.s = sbackup;

  // too few s elements
  key backup_key;
  backup_key = clsag.s.back();
  clsag.s.pop_back();
  ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  clsag.s.push_back(backup_key);

  // too many s elements
  clsag.s.push_back(skGen());
  ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  clsag.s.pop_back();

  // bad s in clsag at verification
  for (auto &s: clsag.s)
  {
    backup_key = s;
    s = skGen();
    ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
    s = backup_key;
  }

  // bad c1 in clsag at verification
  backup_key = clsag.c1;
  clsag.c1 = skGen();
  ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  clsag.c1 = backup_key;

  // bad I in clsag at verification
  backup_key = clsag.I;
  clsag.I = scalarmultBase(skGen());
  ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  clsag.I = backup_key;

  // bad D in clsag at verification
  backup_key = clsag.D;
  clsag.D = scalarmultBase(skGen());
  ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  clsag.D = backup_key;

  // D not in main subgroup in clsag at verification
  backup_key = clsag.D;
  rct::key x;
  ASSERT_TRUE(epee::string_tools::hex_to_pod("c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac03fa", x));
  clsag.D = rct::addKeys(clsag.D, x);
  ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  clsag.D = backup_key;

  // swapped I and D in clsag at verification
  std::swap(clsag.I, clsag.D);
  ASSERT_FALSE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
  std::swap(clsag.I, clsag.D);

  // check it's still good, in case we failed to restore
  ASSERT_TRUE(rct::verRctCLSAGSimple(message,clsag,pubs,Cout));
}

TEST(ringct, range_proofs)
{
        //Ring CT Stuff
        //ct range proofs
        ctkeyV sc, pc;
        ctkey sctmp, pctmp;
        std::vector<uint64_t> inamounts;
        //add fake input 6000
        inamounts.push_back(6000);
        tie(sctmp, pctmp) = ctskpkGen(inamounts.back());
        sc.push_back(sctmp);
        pc.push_back(pctmp);


        inamounts.push_back(7000);
        tie(sctmp, pctmp) = ctskpkGen(inamounts.back());
        sc.push_back(sctmp);
        pc.push_back(pctmp);
        vector<xmr_amount >amounts;
        rct::keyV amount_keys;
        key mask;

        //add output 500
        amounts.push_back(500);
        amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
        keyV destinations;
        key Sk, Pk;
        skpkGen(Sk, Pk);
        destinations.push_back(Pk);


        //add output for 12500
        amounts.push_back(12500);
        amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
        skpkGen(Sk, Pk);
        destinations.push_back(Pk);

        const rct::RCTConfig rct_config { RangeProofBorromean, 0 };

        //compute rct data with mixin 3 - should fail since full type with > 1 input
        bool ok = false;
        try { genRct(rct::zero(), sc, pc, destinations, amounts, amount_keys, 3, rct_config, hw::get_device("default")); }
        catch(...) { ok = true; }
        ASSERT_TRUE(ok);

        //compute rct data with mixin 3
        rctSig s = genRctSimple(rct::zero(), sc, pc, destinations, inamounts, amounts, amount_keys, 0, 3, rct_config, hw::get_device("default"));

        //verify rct data
        ASSERT_TRUE(verRctSimple(s));

        //decode received amount
        decodeRctSimple(s, amount_keys[1], 1, mask, hw::get_device("default"));

        // Ring CT with failing MG sig part should not verify!
        // Since sum of inputs != outputs

        amounts[1] = 12501;
        skpkGen(Sk, Pk);
        destinations[1] = Pk;


        //compute rct data with mixin 3
        s = genRctSimple(rct::zero(), sc, pc, destinations, inamounts, amounts, amount_keys, 0, 3, rct_config, hw::get_device("default"));

        //verify rct data
        ASSERT_FALSE(verRctSimple(s));

        //decode received amount
        decodeRctSimple(s, amount_keys[1], 1, mask, hw::get_device("default"));
}

TEST(ringct, range_proofs_with_fee)
{
        //Ring CT Stuff
        //ct range proofs
        ctkeyV sc, pc;
        ctkey sctmp, pctmp;
        std::vector<uint64_t> inamounts;
        //add fake input 6001
        inamounts.push_back(6001);
        tie(sctmp, pctmp) = ctskpkGen(inamounts.back());
        sc.push_back(sctmp);
        pc.push_back(pctmp);


        inamounts.push_back(7000);
        tie(sctmp, pctmp) = ctskpkGen(inamounts.back());
        sc.push_back(sctmp);
        pc.push_back(pctmp);
        vector<xmr_amount >amounts;
        keyV amount_keys;
        key mask;

        //add output 500
        amounts.push_back(500);
        amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
        keyV destinations;
        key Sk, Pk;
        skpkGen(Sk, Pk);
        destinations.push_back(Pk);

        //add output for 12500
        amounts.push_back(12500);
        amount_keys.push_back(hash_to_scalar(zero()));
        skpkGen(Sk, Pk);
        destinations.push_back(Pk);

        const rct::RCTConfig rct_config { RangeProofBorromean, 0 };

        //compute rct data with mixin 3
        rctSig s = genRctSimple(rct::zero(), sc, pc, destinations, inamounts, amounts, amount_keys, 1, 3, rct_config, hw::get_device("default"));

        //verify rct data
        ASSERT_TRUE(verRctSimple(s));

        //decode received amount
        decodeRctSimple(s, amount_keys[1], 1, mask, hw::get_device("default"));

        // Ring CT with failing MG sig part should not verify!
        // Since sum of inputs != outputs

        amounts[1] = 12501;
        skpkGen(Sk, Pk);
        destinations[1] = Pk;


        //compute rct data with mixin 3
        s = genRctSimple(rct::zero(), sc, pc, destinations, inamounts, amounts, amount_keys, 500, 3, rct_config, hw::get_device("default"));

        //verify rct data
        ASSERT_FALSE(verRctSimple(s));

        //decode received amount
        decodeRctSimple(s, amount_keys[1], 1, mask, hw::get_device("default"));
}

TEST(ringct, simple)
{
        ctkeyV sc, pc;
        ctkey sctmp, pctmp;
        //this vector corresponds to output amounts
        vector<xmr_amount>outamounts;
       //this vector corresponds to input amounts
        vector<xmr_amount>inamounts;
        //this keyV corresponds to destination pubkeys
        keyV destinations;
        keyV amount_keys;
        key mask;

        //add fake input 3000
        //the sc is secret data
        //pc is public data
        tie(sctmp, pctmp) = ctskpkGen(3000);
        sc.push_back(sctmp);
        pc.push_back(pctmp);
        inamounts.push_back(3000);

        //add fake input 3000
        //the sc is secret data
        //pc is public data
        tie(sctmp, pctmp) = ctskpkGen(3000);
        sc.push_back(sctmp);
        pc.push_back(pctmp);
        inamounts.push_back(3000);

        //add output 5000
        outamounts.push_back(5000);
        amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
        //add the corresponding destination pubkey
        key Sk, Pk;
        skpkGen(Sk, Pk);
        destinations.push_back(Pk);

        //add output 999
        outamounts.push_back(999);
        amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
        //add the corresponding destination pubkey
        skpkGen(Sk, Pk);
        destinations.push_back(Pk);

        key message = skGen(); //real message later (hash of txn..)

        //compute sig with mixin 2
        xmr_amount txnfee = 1;

        const rct::RCTConfig rct_config { RangeProofBorromean, 0 };
        rctSig s = genRctSimple(message, sc, pc, destinations,inamounts, outamounts, amount_keys, txnfee, 2, rct_config, hw::get_device("default"));

        //verify ring ct signature
        ASSERT_TRUE(verRctSimple(s));

        //decode received amount corresponding to output pubkey index 1
        decodeRctSimple(s, amount_keys[1], 1, mask,  hw::get_device("default"));
}

static rct::rctSig make_sample_rct_sig(int n_inputs, const uint64_t input_amounts[], int n_outputs, const uint64_t output_amounts[], bool last_is_fee)
{
    ctkeyV sc, pc;
    ctkey sctmp, pctmp;
    vector<xmr_amount >amounts;
    keyV destinations;
    keyV amount_keys;
    key Sk, Pk;

    for (int n = 0; n < n_inputs; ++n) {
        tie(sctmp, pctmp) = ctskpkGen(input_amounts[n]);
        sc.push_back(sctmp);
        pc.push_back(pctmp);
    }

    for (int n = 0; n < n_outputs; ++n) {
        amounts.push_back(output_amounts[n]);
        skpkGen(Sk, Pk);
        if (n < n_outputs - 1 || !last_is_fee)
        {
          destinations.push_back(Pk);
          amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
        }
    }

    const rct::RCTConfig rct_config { RangeProofBorromean, 0 };
    return genRct(rct::zero(), sc, pc, destinations, amounts, amount_keys, 3, rct_config, hw::get_device("default"));
}

static rct::rctSig make_sample_simple_rct_sig(int n_inputs, const uint64_t input_amounts[], int n_outputs, const uint64_t output_amounts[], uint64_t fee)
{
    ctkeyV sc, pc;
    ctkey sctmp, pctmp;
    vector<xmr_amount> inamounts, outamounts;
    keyV destinations;
    keyV amount_keys;
    key Sk, Pk;

    for (int n = 0; n < n_inputs; ++n) {
        inamounts.push_back(input_amounts[n]);
        tie(sctmp, pctmp) = ctskpkGen(input_amounts[n]);
        sc.push_back(sctmp);
        pc.push_back(pctmp);
    }

    for (int n = 0; n < n_outputs; ++n) {
        outamounts.push_back(output_amounts[n]);
        amount_keys.push_back(hash_to_scalar(zero()));
        skpkGen(Sk, Pk);
        destinations.push_back(Pk);
    }

    const rct::RCTConfig rct_config { RangeProofBorromean, 0 };
    return genRctSimple(rct::zero(), sc, pc, destinations, inamounts, outamounts, amount_keys, fee, 3, rct_config, hw::get_device("default"));
}

static bool range_proof_test(bool expected_valid,
    int n_inputs, const uint64_t input_amounts[], int n_outputs, const uint64_t output_amounts[], bool last_is_fee, bool simple)
{
    //compute rct data
    bool valid;
    try {
        rctSig s;
        // simple takes fee as a parameter, non-simple takes it as an extra element to output amounts
        if (simple) {
          s = make_sample_simple_rct_sig(n_inputs, input_amounts, last_is_fee ? n_outputs - 1 : n_outputs, output_amounts, last_is_fee ? output_amounts[n_outputs - 1] : 0);
          valid = verRctSimple(s);
        }
        else {
          s = make_sample_rct_sig(n_inputs, input_amounts, n_outputs, output_amounts, last_is_fee);
          valid = verRct(s);
        }
    }
    catch (const std::exception &e) {
        valid = false;
    }

    if (valid == expected_valid) {
        return testing::AssertionSuccess();
    }
    else {
        return testing::AssertionFailure();
    }
}

#define NELTS(array) (sizeof(array)/sizeof(array[0]))

TEST(ringct, range_proofs_reject_empty_outs)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_empty_outs_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_empty_ins)
{
  const uint64_t inputs[] = {};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_empty_ins_simple)
{
  const uint64_t inputs[] = {};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_all_empty)
{
  const uint64_t inputs[] = {};
  const uint64_t outputs[] = {};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_all_empty_simple)
{
  const uint64_t inputs[] = {};
  const uint64_t outputs[] = {};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_zero_empty)
{
  const uint64_t inputs[] = {0};
  const uint64_t outputs[] = {};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_accept_zero_empty_simple)
{
  const uint64_t inputs[] = {0};
  const uint64_t outputs[] = {};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_empty_zero)
{
  const uint64_t inputs[] = {};
  const uint64_t outputs[] = {0};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_empty_zero_simple)
{
  const uint64_t inputs[] = {};
  const uint64_t outputs[] = {0};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_zero_zero)
{
  const uint64_t inputs[] = {0};
  const uint64_t outputs[] = {0};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_accept_zero_zero_simple)
{
  const uint64_t inputs[] = {0};
  const uint64_t outputs[] = {0};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_zero_out_first)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {0, 5000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_accept_zero_out_first_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {0, 5000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_zero_out_last)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {5000, 0};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_accept_zero_out_last_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {5000, 0};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_zero_out_middle)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {2500, 0, 2500};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_accept_zero_out_middle_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {2500, 0, 2500};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_zero)
{
  const uint64_t inputs[] = {0};
  const uint64_t outputs[] = {0};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_accept_zero_in_first_simple)
{
  const uint64_t inputs[] = {0, 5000};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_zero_in_last_simple)
{
  const uint64_t inputs[] = {5000, 0};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_zero_in_middle_simple)
{
  const uint64_t inputs[] = {2500, 0, 2500};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_single_lower)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {1};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_single_lower_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {1};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_single_higher)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {5001};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_single_higher_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {5001};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_single_out_negative)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {(uint64_t)-1000ll};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_single_out_negative_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {(uint64_t)-1000ll};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_out_negative_first)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {(uint64_t)-1000ll, 6000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_out_negative_first_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {(uint64_t)-1000ll, 6000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_out_negative_last)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {6000, (uint64_t)-1000ll};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_out_negative_last_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {6000, (uint64_t)-1000ll};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_out_negative_middle)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {3000, (uint64_t)-1000ll, 3000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_out_negative_middle_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {3000, (uint64_t)-1000ll, 3000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_single_in_negative)
{
  const uint64_t inputs[] = {(uint64_t)-1000ll};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_single_in_negative_simple)
{
  const uint64_t inputs[] = {(uint64_t)-1000ll};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_in_negative_first)
{
  const uint64_t inputs[] = {(uint64_t)-1000ll, 6000};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_in_negative_first_simple)
{
  const uint64_t inputs[] = {(uint64_t)-1000ll, 6000};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_in_negative_last)
{
  const uint64_t inputs[] = {6000, (uint64_t)-1000ll};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_in_negative_last_simple)
{
  const uint64_t inputs[] = {6000, (uint64_t)-1000ll};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_in_negative_middle)
{
  const uint64_t inputs[] = {3000, (uint64_t)-1000ll, 3000};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_in_negative_middle_simple)
{
  const uint64_t inputs[] = {3000, (uint64_t)-1000ll, 3000};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_reject_higher_list)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {1000, 1000, 1000, 1000, 1000, 1000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_reject_higher_list_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {1000, 1000, 1000, 1000, 1000, 1000};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_1_to_1)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_accept_1_to_1_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_1_to_N)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {1000, 1000, 1000, 1000, 1000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, false));
}

TEST(ringct, range_proofs_accept_1_to_N_simple)
{
  const uint64_t inputs[] = {5000};
  const uint64_t outputs[] = {1000, 1000, 1000, 1000, 1000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false,true));
}

TEST(ringct, range_proofs_accept_N_to_1_simple)
{
  const uint64_t inputs[] = {1000, 1000, 1000, 1000, 1000};
  const uint64_t outputs[] = {5000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_N_to_N_simple)
{
  const uint64_t inputs[] = {1000, 1000, 1000, 1000, 1000};
  const uint64_t outputs[] = {1000, 1000, 1000, 1000, 1000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, range_proofs_accept_very_long_simple)
{
  const size_t N=12;
  uint64_t inputs[N];
  uint64_t outputs[N];
  for (size_t n = 0; n < N; ++n) {
    inputs[n] = n;
    outputs[n] = n;
  }
  std::shuffle(inputs, inputs + N, crypto::random_device{});
  std::shuffle(outputs, outputs + N, crypto::random_device{});
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, false, true));
}

TEST(ringct, HPow2)
{
  key G = scalarmultBase(d2h(1));

  // Note that H is computed differently than standard hashing
  // This method is not guaranteed to return a curvepoint for all inputs
  // Don't use it elsewhere
  key H = cn_fast_hash(G);
  ge_p3 H_p3;
  int decode = ge_frombytes_vartime(&H_p3, H.bytes);
  ASSERT_EQ(decode, 0); // this is known to pass for the particular value G
  ge_p2 H_p2;
  ge_p3_to_p2(&H_p2, &H_p3);
  ge_p1p1 H8_p1p1;
  ge_mul8(&H8_p1p1, &H_p2);
  ge_p1p1_to_p3(&H_p3, &H8_p1p1);
  ge_p3_tobytes(H.bytes, &H_p3);

  for (int j = 0 ; j < ATOMS ; j++) {
    ASSERT_TRUE(equalKeys(H, H2[j]));
    addKeys(H, H, H);
  }
}

static const xmr_amount test_amounts[]={0, 1, 2, 3, 4, 5, 10000, 10000000000000000000ull, 10203040506070809000ull, 123456789123456789};

TEST(ringct, d2h)
{
  key k, P1;
  skpkGen(k, P1);
  for (auto amount: test_amounts) {
    d2h(k, amount);
    ASSERT_TRUE(amount == h2d(k));
  }
}

TEST(ringct, d2b)
{
  for (auto amount: test_amounts) {
    bits b;
    d2b(b, amount);
    ASSERT_TRUE(amount == b2d(b));
  }
}

TEST(ringct, prooveRange_is_non_deterministic)
{
  key C[2], mask[2];
  for (int n = 0; n < 2; ++n)
    proveRange(C[n], mask[n], 80);
  ASSERT_TRUE(memcmp(C[0].bytes, C[1].bytes, sizeof(C[0].bytes)));
  ASSERT_TRUE(memcmp(mask[0].bytes, mask[1].bytes, sizeof(mask[0].bytes)));
}

TEST(ringct, fee_0_valid)
{
  const uint64_t inputs[] = {2000};
  const uint64_t outputs[] = {2000, 0};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, true, false));
}

TEST(ringct, fee_0_valid_simple)
{
  const uint64_t inputs[] = {1000, 1000};
  const uint64_t outputs[] = {2000, 0};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, true, true));
}

TEST(ringct, fee_non_0_valid)
{
  const uint64_t inputs[] = {2000};
  const uint64_t outputs[] = {1900, 100};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, true, false));
}

TEST(ringct, fee_non_0_valid_simple)
{
  const uint64_t inputs[] = {1000, 1000};
  const uint64_t outputs[] = {1900, 100};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, true, true));
}

TEST(ringct, fee_non_0_invalid_higher)
{
  const uint64_t inputs[] = {1000, 1000};
  const uint64_t outputs[] = {1990, 100};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, true, false));
}

TEST(ringct, fee_non_0_invalid_higher_simple)
{
  const uint64_t inputs[] = {1000, 1000};
  const uint64_t outputs[] = {1990, 100};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, true, true));
}

TEST(ringct, fee_non_0_invalid_lower)
{
  const uint64_t inputs[] = {1000, 1000};
  const uint64_t outputs[] = {1000, 100};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, true, false));
}

TEST(ringct, fee_non_0_invalid_lower_simple)
{
  const uint64_t inputs[] = {1000, 1000};
  const uint64_t outputs[] = {1000, 100};
  EXPECT_TRUE(range_proof_test(false, NELTS(inputs), inputs, NELTS(outputs), outputs, true, true));
}

TEST(ringct, fee_burn_valid_one_out)
{
  const uint64_t inputs[] = {2000};
  const uint64_t outputs[] = {0, 2000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, true, false));
}

TEST(ringct, fee_burn_valid_one_out_simple)
{
  const uint64_t inputs[] = {1000, 1000};
  const uint64_t outputs[] = {0, 2000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, true, true));
}

TEST(ringct, fee_burn_valid_zero_out)
{
  const uint64_t inputs[] = {2000};
  const uint64_t outputs[] = {2000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, true, false));
}

TEST(ringct, fee_burn_valid_zero_out_simple)
{
  const uint64_t inputs[] = {1000, 1000};
  const uint64_t outputs[] = {2000};
  EXPECT_TRUE(range_proof_test(true, NELTS(inputs), inputs, NELTS(outputs), outputs, true, true));
}

static rctSig make_sig()
{
  static const uint64_t inputs[] = {2000};
  static const uint64_t outputs[] = {1000, 1000};
  static rct::rctSig sig = make_sample_rct_sig(NELTS(inputs), inputs, NELTS(outputs), outputs, true);
  return sig;
}

#define TEST_rctSig_elements(name, op) \
TEST(ringct, rctSig_##name) \
{ \
  rct::rctSig sig = make_sig(); \
  ASSERT_TRUE(rct::verRct(sig)); \
  op; \
  ASSERT_FALSE(rct::verRct(sig)); \
}

TEST_rctSig_elements(rangeSigs_empty, sig.p.rangeSigs.resize(0));
TEST_rctSig_elements(rangeSigs_too_many, sig.p.rangeSigs.push_back(sig.p.rangeSigs.back()));
TEST_rctSig_elements(rangeSigs_too_few, sig.p.rangeSigs.pop_back());
TEST_rctSig_elements(mgSig_MG_empty, sig.p.MGs.resize(0));
TEST_rctSig_elements(mgSig_ss_empty, sig.p.MGs[0].ss.resize(0));
TEST_rctSig_elements(mgSig_ss_too_many, sig.p.MGs[0].ss.push_back(sig.p.MGs[0].ss.back()));
TEST_rctSig_elements(mgSig_ss_too_few, sig.p.MGs[0].ss.pop_back());
TEST_rctSig_elements(mgSig_ss0_empty, sig.p.MGs[0].ss[0].resize(0));
TEST_rctSig_elements(mgSig_ss0_too_many, sig.p.MGs[0].ss[0].push_back(sig.p.MGs[0].ss[0].back()));
TEST_rctSig_elements(mgSig_ss0_too_few, sig.p.MGs[0].ss[0].pop_back());
TEST_rctSig_elements(mgSig_II_empty, sig.p.MGs[0].II.resize(0));
TEST_rctSig_elements(mgSig_II_too_many, sig.p.MGs[0].II.push_back(sig.p.MGs[0].II.back()));
TEST_rctSig_elements(mgSig_II_too_few, sig.p.MGs[0].II.pop_back());
TEST_rctSig_elements(mixRing_empty, sig.mixRing.resize(0));
TEST_rctSig_elements(mixRing_too_many, sig.mixRing.push_back(sig.mixRing.back()));
TEST_rctSig_elements(mixRing_too_few, sig.mixRing.pop_back());
TEST_rctSig_elements(mixRing0_empty, sig.mixRing[0].resize(0));
TEST_rctSig_elements(mixRing0_too_many, sig.mixRing[0].push_back(sig.mixRing[0].back()));
TEST_rctSig_elements(mixRing0_too_few, sig.mixRing[0].pop_back());
TEST_rctSig_elements(ecdhInfo_empty, sig.ecdhInfo.resize(0));
TEST_rctSig_elements(ecdhInfo_too_many, sig.ecdhInfo.push_back(sig.ecdhInfo.back()));
TEST_rctSig_elements(ecdhInfo_too_few, sig.ecdhInfo.pop_back());
TEST_rctSig_elements(outPk_empty, sig.outPk.resize(0));
TEST_rctSig_elements(outPk_too_many, sig.outPk.push_back(sig.outPk.back()));
TEST_rctSig_elements(outPk_too_few, sig.outPk.pop_back());

static rct::rctSig make_sig_simple()
{
  static const uint64_t inputs[] = {1000, 1000};
  static const uint64_t outputs[] = {1000};
  static rct::rctSig sig = make_sample_simple_rct_sig(NELTS(inputs), inputs, NELTS(outputs), outputs, 1000);
  return sig;
}

#define TEST_rctSig_elements_simple(name, op) \
TEST(ringct, rctSig_##name##_simple) \
{ \
  rct::rctSig sig = make_sig_simple(); \
  ASSERT_TRUE(rct::verRctSimple(sig)); \
  op; \
  ASSERT_FALSE(rct::verRctSimple(sig)); \
}

TEST_rctSig_elements_simple(rangeSigs_empty, sig.p.rangeSigs.resize(0));
TEST_rctSig_elements_simple(rangeSigs_too_many, sig.p.rangeSigs.push_back(sig.p.rangeSigs.back()));
TEST_rctSig_elements_simple(rangeSigs_too_few, sig.p.rangeSigs.pop_back());
TEST_rctSig_elements_simple(mgSig_empty, sig.p.MGs.resize(0));
TEST_rctSig_elements_simple(mgSig_too_many, sig.p.MGs.push_back(sig.p.MGs.back()));
TEST_rctSig_elements_simple(mgSig_too_few, sig.p.MGs.pop_back());
TEST_rctSig_elements_simple(mgSig0_ss_empty, sig.p.MGs[0].ss.resize(0));
TEST_rctSig_elements_simple(mgSig0_ss_too_many, sig.p.MGs[0].ss.push_back(sig.p.MGs[0].ss.back()));
TEST_rctSig_elements_simple(mgSig0_ss_too_few, sig.p.MGs[0].ss.pop_back());
TEST_rctSig_elements_simple(mgSig_ss0_empty, sig.p.MGs[0].ss[0].resize(0));
TEST_rctSig_elements_simple(mgSig_ss0_too_many, sig.p.MGs[0].ss[0].push_back(sig.p.MGs[0].ss[0].back()));
TEST_rctSig_elements_simple(mgSig_ss0_too_few, sig.p.MGs[0].ss[0].pop_back());
TEST_rctSig_elements_simple(mgSig0_II_empty, sig.p.MGs[0].II.resize(0));
TEST_rctSig_elements_simple(mgSig0_II_too_many, sig.p.MGs[0].II.push_back(sig.p.MGs[0].II.back()));
TEST_rctSig_elements_simple(mgSig0_II_too_few, sig.p.MGs[0].II.pop_back());
TEST_rctSig_elements_simple(mixRing_empty, sig.mixRing.resize(0));
TEST_rctSig_elements_simple(mixRing_too_many, sig.mixRing.push_back(sig.mixRing.back()));
TEST_rctSig_elements_simple(mixRing_too_few, sig.mixRing.pop_back());
TEST_rctSig_elements_simple(mixRing0_empty, sig.mixRing[0].resize(0));
TEST_rctSig_elements_simple(mixRing0_too_many, sig.mixRing[0].push_back(sig.mixRing[0].back()));
TEST_rctSig_elements_simple(mixRing0_too_few, sig.mixRing[0].pop_back());
TEST_rctSig_elements_simple(pseudoOuts_empty, sig.pseudoOuts.resize(0));
TEST_rctSig_elements_simple(pseudoOuts_too_many, sig.pseudoOuts.push_back(sig.pseudoOuts.back()));
TEST_rctSig_elements_simple(pseudoOuts_too_few, sig.pseudoOuts.pop_back());
TEST_rctSig_elements_simple(ecdhInfo_empty, sig.ecdhInfo.resize(0));
TEST_rctSig_elements_simple(ecdhInfo_too_many, sig.ecdhInfo.push_back(sig.ecdhInfo.back()));
TEST_rctSig_elements_simple(ecdhInfo_too_few, sig.ecdhInfo.pop_back());
TEST_rctSig_elements_simple(outPk_empty, sig.outPk.resize(0));
TEST_rctSig_elements_simple(outPk_too_many, sig.outPk.push_back(sig.outPk.back()));
TEST_rctSig_elements_simple(outPk_too_few, sig.outPk.pop_back());

TEST(ringct, reject_gen_simple_ver_non_simple)
{
  const uint64_t inputs[] = {1000, 1000};
  const uint64_t outputs[] = {1000};
  rct::rctSig sig = make_sample_simple_rct_sig(NELTS(inputs), inputs, NELTS(outputs), outputs, 1000);
  ASSERT_FALSE(rct::verRct(sig));
}

TEST(ringct, reject_gen_non_simple_ver_simple)
{
  const uint64_t inputs[] = {2000};
  const uint64_t outputs[] = {1000, 1000};
  rct::rctSig sig = make_sample_rct_sig(NELTS(inputs), inputs, NELTS(outputs), outputs, true);
  ASSERT_FALSE(rct::verRctSimple(sig));
}

TEST(ringct, key_ostream)
{
  std::stringstream out;
  out << "BEGIN" << rct::H << "END";
  EXPECT_EQ(
    std::string{"BEGIN<8b655970153799af2aeadc9ff1add0ea6c7251d54154cfa92c173a0dd39c1f94>END"},
    out.str()
  );
}

TEST(ringct, zeroCommmit)
{
  static const uint64_t amount = crypto::rand<uint64_t>();
  const rct::key z = rct::zeroCommitVartime(amount);
  const rct::key a = rct::scalarmultBase(rct::identity());
  const rct::key b = rct::scalarmultH(rct::d2h(amount));
  const rct::key manual = rct::addKeys(a, b);
  ASSERT_EQ(z, manual);
}

static rct::key uncachedZeroCommit(uint64_t amount)
{
  const rct::key am = rct::d2h(amount);
  const rct::key bH = rct::scalarmultH(am);
  return rct::addKeys(rct::G, bH);
}

TEST(ringct, zeroCommitCache)
{
  ASSERT_EQ(rct::zeroCommitVartime(0), uncachedZeroCommit(0));
  ASSERT_EQ(rct::zeroCommitVartime(1), uncachedZeroCommit(1));
  ASSERT_EQ(rct::zeroCommitVartime(2), uncachedZeroCommit(2));
  ASSERT_EQ(rct::zeroCommitVartime(10), uncachedZeroCommit(10));
  ASSERT_EQ(rct::zeroCommitVartime(200), uncachedZeroCommit(200));
  ASSERT_EQ(rct::zeroCommitVartime(1000000000), uncachedZeroCommit(1000000000));
  ASSERT_EQ(rct::zeroCommitVartime(3000000000000), uncachedZeroCommit(3000000000000));
  ASSERT_EQ(rct::zeroCommitVartime(900000000000000), uncachedZeroCommit(900000000000000));
}

TEST(ringct, H)
{
  ge_p3 p3;
  ASSERT_EQ(ge_frombytes_vartime(&p3, rct::H.bytes), 0);
  ASSERT_EQ(memcmp(&p3, &ge_p3_H, sizeof(ge_p3)), 0);
}

TEST(ringct, mul8)
{
  ge_p3 p3;
  rct::key key;
  ASSERT_EQ(rct::scalarmult8(rct::identity()), rct::identity());
  rct::scalarmult8(p3,rct::identity());
  ge_p3_tobytes(key.bytes, &p3);
  ASSERT_EQ(key, rct::identity());
  ASSERT_EQ(rct::scalarmult8(rct::H), rct::scalarmultKey(rct::H, rct::EIGHT));
  rct::scalarmult8(p3,rct::H);
  ge_p3_tobytes(key.bytes, &p3);
  ASSERT_EQ(key, rct::scalarmultKey(rct::H, rct::EIGHT));
  ASSERT_EQ(rct::scalarmultKey(rct::scalarmultKey(rct::H, rct::INV_EIGHT), rct::EIGHT), rct::H);
}

TEST(ringct, aggregated)
{
  static const size_t N_PROOFS = 16;
  std::vector<rctSig> s(N_PROOFS);
  std::vector<const rctSig*> sp(N_PROOFS);

  for (size_t n = 0; n < N_PROOFS; ++n)
  {
    static const uint64_t inputs[] = {1000, 1000};
    static const uint64_t outputs[] = {500, 1500};
    s[n] = make_sample_simple_rct_sig(NELTS(inputs), inputs, NELTS(outputs), outputs, 0);
    sp[n] = &s[n];
  }

  ASSERT_TRUE(verRctSemanticsSimple(sp));
}
