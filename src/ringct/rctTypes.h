// Copyright (c) 2016-2024, Monero Research Labs
//
// Author: Shen Noether <shen.noether@gmx.com>
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

#pragma once
#ifndef RCT_TYPES_H
#define RCT_TYPES_H

#include <cstddef>
#include <vector>
#include <iostream>
#include <cinttypes>
#include <sodium/crypto_verify_32.h>

extern "C" {
#include "crypto/crypto-ops.h"
#include "crypto/random.h"
#include "crypto/keccak.h"
}
#include "crypto/generic-ops.h"
#include "crypto/crypto.h"
#include "fcmp_pp/fcmp_pp_types.h"
#include "fcmp_pp/prove.h"
#include "hex.h"
#include "span.h"
#include "memwipe.h"
#include "serialization/containers.h"
#include "serialization/debug_archive.h"
#include "serialization/binary_archive.h"
#include "serialization/json_archive.h"


//Define this flag when debugging to get additional info on the console
#ifdef DBG
#define DP(x) dp(x)
#else
#define DP(x)
#endif

//atomic units of moneros
#define ATOMS 64

//for printing large ints

//Namespace specifically for ring ct code
namespace rct {
    //basic ops containers
    typedef unsigned char * Bytes;

    // Can contain a secret or public key
    //  similar to secret_key / public_key of crypto-ops,
    //  but uses unsigned chars,
    //  also includes an operator for accessing the i'th byte.
    struct key {
        unsigned char & operator[](int i) {
            return bytes[i];
        }
        unsigned char operator[](int i) const {
            return bytes[i];
        }
        bool operator==(const key &k) const { return !crypto_verify_32(bytes, k.bytes); }
        bool operator!=(const key &k) const { return crypto_verify_32(bytes, k.bytes); }
        unsigned char bytes[32];
    };
    typedef std::vector<key> keyV; //vector of keys
    typedef std::vector<keyV> keyM; //matrix of keys (indexed by column first)

    //containers For CT operations
    //if it's  representing a private ctkey then "dest" contains the secret key of the address
    // while "mask" contains a where C = aG + bH is CT pedersen commitment and b is the amount
    // (store b, the amount, separately
    //if it's representing a public ctkey, then "dest" = P the address, mask = C the commitment
    struct ctkey {
        key dest;
        key mask; //C here if public

        bool operator==(const ctkey &other) const {
          return (dest == other.dest) && (mask == other.mask);
        }

        bool operator!=(const ctkey &other) const {
          return !(*this == other);
        }
    };
    typedef std::vector<ctkey> ctkeyV;
    typedef std::vector<ctkeyV> ctkeyM;

    //used for multisig data
    struct multisig_kLRki {
        key k;
        key L;
        key R;
        key ki;

        ~multisig_kLRki() { memwipe(&k, sizeof(k)); }
    };

    struct multisig_out {
        std::vector<key> c; // for all inputs
        std::vector<key> mu_p; // for all inputs
        std::vector<key> c0; // for all inputs

        BEGIN_SERIALIZE_OBJECT()
          FIELD(c)
          FIELD(mu_p)
          if (!mu_p.empty() && mu_p.size() != c.size())
            return false;
        END_SERIALIZE()
    };

    //data for passing the amount to the receiver secretly
    // If the pedersen commitment to an amount is C = aG + bH,
    // "mask" contains a 32 byte key a
    // "amount" contains a hex representation (in 32 bytes) of a 64 bit number
    // the purpose of the ECDH exchange
    struct ecdhTuple {
        key mask;
        key amount;

        BEGIN_SERIALIZE_OBJECT()
          FIELD(mask) // not saved from v2 BPs
          FIELD(amount)
        END_SERIALIZE()
    };

    //containers for representing amounts
    typedef uint64_t xmr_amount;
    typedef unsigned int bits[ATOMS];
    typedef key key64[64];

    struct boroSig {
        key64 s0;
        key64 s1;
        key ee;
    };
  
    //Container for precomp
    struct geDsmp {
        ge_dsmp k;
    };
    
    //just contains the necessary keys to represent MLSAG sigs
    //c.f. https://eprint.iacr.org/2015/1098
    struct mgSig {
        keyM ss;
        key cc;
        keyV II;

        BEGIN_SERIALIZE_OBJECT()
            FIELD(ss)
            FIELD(cc)
            // FIELD(II) - not serialized, it can be reconstructed
        END_SERIALIZE()
    };

    // CLSAG signature
    struct clsag {
        keyV s; // scalars
        key c1;

        key I; // signing key image
        key D; // commitment key image

        BEGIN_SERIALIZE_OBJECT()
            FIELD(s)
            FIELD(c1)
            // FIELD(I) - not serialized, it can be reconstructed
            FIELD(D)
        END_SERIALIZE()
    };

    //contains the data for an Borromean sig
    // also contains the "Ci" values such that
    // \sum Ci = C
    // and the signature proves that each Ci is either
    // a Pedersen commitment to 0 or to 2^i
    //thus proving that C is in the range of [0, 2^64]
    struct rangeSig {
        boroSig asig;
        key64 Ci;

        BEGIN_SERIALIZE_OBJECT()
            FIELD(asig)
            FIELD(Ci)
        END_SERIALIZE()
    };

    struct Bulletproof
    {
      rct::keyV V;
      rct::key A, S, T1, T2;
      rct::key taux, mu;
      rct::keyV L, R;
      rct::key a, b, t;

      Bulletproof():
        A({}), S({}), T1({}), T2({}), taux({}), mu({}), a({}), b({}), t({}), V({}), L({}), R({}) {}
      Bulletproof(const rct::key &V, const rct::key &A, const rct::key &S, const rct::key &T1, const rct::key &T2, const rct::key &taux, const rct::key &mu, const rct::keyV &L, const rct::keyV &R, const rct::key &a, const rct::key &b, const rct::key &t):
        V({V}), A(A), S(S), T1(T1), T2(T2), taux(taux), mu(mu), L(L), R(R), a(a), b(b), t(t) {}
      Bulletproof(const rct::keyV &V, const rct::key &A, const rct::key &S, const rct::key &T1, const rct::key &T2, const rct::key &taux, const rct::key &mu, const rct::keyV &L, const rct::keyV &R, const rct::key &a, const rct::key &b, const rct::key &t):
        V(V), A(A), S(S), T1(T1), T2(T2), taux(taux), mu(mu), L(L), R(R), a(a), b(b), t(t) {}

      bool operator==(const Bulletproof &other) const { return V == other.V && A == other.A && S == other.S && T1 == other.T1 && T2 == other.T2 && taux == other.taux && mu == other.mu && L == other.L && R == other.R && a == other.a && b == other.b && t == other.t; }

      BEGIN_SERIALIZE_OBJECT()
        // Commitments aren't saved, they're restored via outPk
        // FIELD(V)
        FIELD(A)
        FIELD(S)
        FIELD(T1)
        FIELD(T2)
        FIELD(taux)
        FIELD(mu)
        FIELD(L)
        FIELD(R)
        FIELD(a)
        FIELD(b)
        FIELD(t)

        if (L.empty() || L.size() != R.size())
          return false;
      END_SERIALIZE()
    };

    struct BulletproofPlus
    {
      rct::keyV V;
      rct::key A, A1, B;
      rct::key r1, s1, d1;
      rct::keyV L, R;

      BulletproofPlus(): V(), A(), A1(), B(), r1(), s1(), d1(), L(), R() {}
      BulletproofPlus(const rct::key &V, const rct::key &A, const rct::key &A1, const rct::key &B, const rct::key &r1, const rct::key &s1, const rct::key &d1, const rct::keyV &L, const rct::keyV &R):
        V({V}), A(A), A1(A1), B(B), r1(r1), s1(s1), d1(d1), L(L), R(R) {}
      BulletproofPlus(const rct::keyV &V, const rct::key &A, const rct::key &A1, const rct::key &B, const rct::key &r1, const rct::key &s1, const rct::key &d1, const rct::keyV &L, const rct::keyV &R):
        V(V), A(A), A1(A1), B(B), r1(r1), s1(s1), d1(d1), L(L), R(R) {}

      bool operator==(const BulletproofPlus &other) const { return V == other.V && A == other.A && A1 == other.A1 && B == other.B && r1 == other.r1 && s1 == other.s1 && d1 == other.d1 && L == other.L && R == other.R; }

      BEGIN_SERIALIZE_OBJECT()
        // Commitments aren't saved, they're restored via outPk
        // FIELD(V)
        FIELD(A)
        FIELD(A1)
        FIELD(B)
        FIELD(r1)
        FIELD(s1)
        FIELD(d1)
        FIELD(L)
        FIELD(R)

        if (L.empty() || L.size() != R.size())
          return false;
      END_SERIALIZE()
    };

    size_t n_bulletproof_amounts(const Bulletproof &proof);
    size_t n_bulletproof_max_amounts(const Bulletproof &proof);
    size_t n_bulletproof_amounts(const std::vector<Bulletproof> &proofs);
    size_t n_bulletproof_max_amounts(const std::vector<Bulletproof> &proofs);

    size_t n_bulletproof_plus_amounts(const BulletproofPlus &proof);
    size_t n_bulletproof_plus_max_amounts(const BulletproofPlus &proof);
    size_t n_bulletproof_plus_amounts(const std::vector<BulletproofPlus> &proofs);
    size_t n_bulletproof_plus_max_amounts(const std::vector<BulletproofPlus> &proofs);

    //A container to hold all signatures necessary for RingCT
    // rangeSigs holds all the rangeproof data of a transaction
    // MG holds the MLSAG signature of a transaction
    // mixRing holds all the public keypairs (P, C) for a transaction
    // ecdhInfo holds an encoded mask / amount to be passed to each receiver
    // outPk contains public keypairs which are destinations (P, C),
    //  P = address, C = commitment to amount
    enum {
      RCTTypeNull = 0,
      RCTTypeFull = 1,
      RCTTypeSimple = 2,
      RCTTypeBulletproof = 3,
      RCTTypeBulletproof2 = 4,
      RCTTypeCLSAG = 5,
      RCTTypeBulletproofPlus = 6,
      RCTTypeFcmpPlusPlus = 7,
    };
    enum RangeProofType { RangeProofBorromean, RangeProofPaddedBulletproof };
    struct RCTConfig {
      RangeProofType range_proof_type;
      int bp_version;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        VARINT_FIELD(range_proof_type)
        VARINT_FIELD(bp_version)
      END_SERIALIZE()
    };
    struct rctSigBase {
        uint8_t type;
        key message;
        ctkeyM mixRing; //the set of all pubkeys / copy
        //pairs that you mix with
        keyV pseudoOuts; //C - for simple rct
        std::vector<ecdhTuple> ecdhInfo;
        ctkeyV outPk;
        xmr_amount txnFee; // contains b

        rctSigBase() :
          type(RCTTypeNull), message{}, mixRing{}, pseudoOuts{}, ecdhInfo{}, outPk{}, txnFee(0)
        {}

        template<bool W, template <bool> class Archive>
        bool serialize_rctsig_base(Archive<W> &ar, size_t inputs, size_t outputs)
        {
          FIELD(type)
          if (type == RCTTypeNull)
            return ar.good();
          if (type != RCTTypeFull && type != RCTTypeSimple && type != RCTTypeBulletproof && type != RCTTypeBulletproof2 && type != RCTTypeCLSAG && type != RCTTypeBulletproofPlus && type != RCTTypeFcmpPlusPlus)
            return false;
          VARINT_FIELD(txnFee)
          // inputs/outputs not saved, only here for serialization help
          // FIELD(message) - not serialized, it can be reconstructed
          // FIELD(mixRing) - not serialized, it can be reconstructed
          if (type == RCTTypeSimple) // moved to prunable with bulletproofs
          {
            ar.tag("pseudoOuts");
            ar.begin_array();
            PREPARE_CUSTOM_VECTOR_SERIALIZATION(inputs, pseudoOuts);
            if (pseudoOuts.size() != inputs)
              return false;
            for (size_t i = 0; i < inputs; ++i)
            {
              FIELDS(pseudoOuts[i])
              if (inputs - i > 1)
                ar.delimit_array();
            }
            ar.end_array();
          }

          ar.tag("ecdhInfo");
          ar.begin_array();
          PREPARE_CUSTOM_VECTOR_SERIALIZATION(outputs, ecdhInfo);
          if (ecdhInfo.size() != outputs)
            return false;
          for (size_t i = 0; i < outputs; ++i)
          {
            if (type == RCTTypeBulletproof2 || type == RCTTypeCLSAG || type == RCTTypeBulletproofPlus || type == RCTTypeFcmpPlusPlus)
            {
              // Since RCTTypeBulletproof2 enote types, we don't serialize the blinding factor, and only serialize the
              // first 8 bytes of ecdhInfo[i].amount
              ar.begin_object();
              crypto::hash8 trunc_amount; // placeholder variable needed to maintain "strict aliasing"
              if (!typename Archive<W>::is_saving()) // loading
                memset(ecdhInfo[i].amount.bytes, 0, sizeof(ecdhInfo[i].amount.bytes));
              else // saving
                memcpy(trunc_amount.data, ecdhInfo[i].amount.bytes, sizeof(trunc_amount));
              FIELD_N("amount", trunc_amount);
              if (!typename Archive<W>::is_saving()) // loading
                memcpy(ecdhInfo[i].amount.bytes, trunc_amount.data, sizeof(trunc_amount));
              ar.end_object();
            }
            else
            {
              FIELDS(ecdhInfo[i])
            }
            if (outputs - i > 1)
              ar.delimit_array();
          }
          ar.end_array();

          ar.tag("outPk");
          ar.begin_array();
          PREPARE_CUSTOM_VECTOR_SERIALIZATION(outputs, outPk);
          if (outPk.size() != outputs)
            return false;
          for (size_t i = 0; i < outputs; ++i)
          {
            FIELDS(outPk[i].mask)
            if (outputs - i > 1)
              ar.delimit_array();
          }
          ar.end_array();
          return ar.good();
        }

        BEGIN_SERIALIZE_OBJECT()
          FIELD(type)
          FIELD(message)
          FIELD(mixRing)
          FIELD(pseudoOuts)
          FIELD(ecdhInfo)
          FIELD(outPk)
          VARINT_FIELD(txnFee)
        END_SERIALIZE()
    };
    struct rctSigPrunable {
        std::vector<rangeSig> rangeSigs;
        std::vector<Bulletproof> bulletproofs;
        std::vector<BulletproofPlus> bulletproofs_plus;
        std::vector<mgSig> MGs; // simple rct has N, full has 1
        std::vector<clsag> CLSAGs;
        keyV pseudoOuts; //C - for simple rct
        // FCMP data
        uint64_t reference_block{0}; // used to get the tree root as of when this reference block index enters the chain
        uint8_t n_tree_layers{0}; // number of layers in the tree as of the block when the reference block index enters the chain
        fcmp_pp::FcmpPpProof fcmp_pp; // FCMP++ SAL and membership proof
        fcmp_pp::FcmpVerifyHelperData fcmp_ver_helper_data; // used to verify FCMP proofs (not serialized, reconstructed)

        // when changing this function, update cryptonote::get_pruned_transaction_weight
        template<bool W, template <bool> class Archive>
        bool serialize_rctsig_prunable(Archive<W> &ar, uint8_t type, size_t inputs, size_t outputs, size_t mixin)
        {
          if (inputs >= 0xffffffff)
            return false;
          if (outputs >= 0xffffffff)
            return false;
          if (mixin >= 0xffffffff)
            return false;
          if (type == RCTTypeNull)
            return ar.good();
          if (type != RCTTypeFull && type != RCTTypeSimple && type != RCTTypeBulletproof && type != RCTTypeBulletproof2 && type != RCTTypeCLSAG && type != RCTTypeBulletproofPlus && type != RCTTypeFcmpPlusPlus)
            return false;
          if (type == RCTTypeBulletproofPlus || type == RCTTypeFcmpPlusPlus)
          {
            uint32_t nbp = bulletproofs_plus.size();
            VARINT_FIELD(nbp)
            ar.tag("bpp");
            ar.begin_array();
            if (nbp > outputs)
              return false;
            PREPARE_CUSTOM_VECTOR_SERIALIZATION(nbp, bulletproofs_plus);
            for (size_t i = 0; i < nbp; ++i)
            {
              FIELDS(bulletproofs_plus[i])
              if (nbp - i > 1)
                ar.delimit_array();
            }
            if (n_bulletproof_plus_max_amounts(bulletproofs_plus) < outputs)
              return false;
            ar.end_array();
          }
          else if (type == RCTTypeBulletproof || type == RCTTypeBulletproof2 || type == RCTTypeCLSAG)
          {
            uint32_t nbp = bulletproofs.size();
            if (type == RCTTypeBulletproof2 || type == RCTTypeCLSAG)
              VARINT_FIELD(nbp)
            else
              FIELD(nbp)
            ar.tag("bp");
            ar.begin_array();
            if (nbp > outputs)
              return false;
            PREPARE_CUSTOM_VECTOR_SERIALIZATION(nbp, bulletproofs);
            for (size_t i = 0; i < nbp; ++i)
            {
              FIELDS(bulletproofs[i])
              if (nbp - i > 1)
                ar.delimit_array();
            }
            if (n_bulletproof_max_amounts(bulletproofs) < outputs)
              return false;
            ar.end_array();
          }
          else
          {
            ar.tag("rangeSigs");
            ar.begin_array();
            PREPARE_CUSTOM_VECTOR_SERIALIZATION(outputs, rangeSigs);
            if (rangeSigs.size() != outputs)
              return false;
            for (size_t i = 0; i < outputs; ++i)
            {
              FIELDS(rangeSigs[i])
              if (outputs - i > 1)
                ar.delimit_array();
            }
            ar.end_array();
          }

          if (type == RCTTypeFcmpPlusPlus)
          {
            VARINT_FIELD(reference_block)
            // n_tree_layers can be inferred from the reference_block, however, if we didn't save n_tree_layers on the
            // tx, we would need a db read (for n_tree_layers as of the block) in order to de-serialize the FCMP++ proof
            VARINT_FIELD(n_tree_layers)
            ar.tag("fcmp_pp");
            const std::size_t proof_len = fcmp_pp::proof_len(inputs, n_tree_layers);
            if (!typename Archive<W>::is_saving())
              fcmp_pp.resize(proof_len);
            if (fcmp_pp.size() != proof_len)
              return false;
            ar.serialize_blob(fcmp_pp.data(), proof_len);
            if (!ar.good())
              return false;
          }
          else if (type == RCTTypeCLSAG || type == RCTTypeBulletproofPlus)
          {
            ar.tag("CLSAGs");
            ar.begin_array();
            PREPARE_CUSTOM_VECTOR_SERIALIZATION(inputs, CLSAGs);
            if (CLSAGs.size() != inputs)
              return false;
            for (size_t i = 0; i < inputs; ++i)
            {
              // we save the CLSAGs contents directly, because we want it to save its
              // arrays without the size prefixes, and the load can't know what size
              // to expect if it's not in the data
              ar.begin_object();
              ar.tag("s");
              ar.begin_array();
              PREPARE_CUSTOM_VECTOR_SERIALIZATION(mixin + 1, CLSAGs[i].s);
              if (CLSAGs[i].s.size() != mixin + 1)
                return false;
              for (size_t j = 0; j <= mixin; ++j)
              {
                FIELDS(CLSAGs[i].s[j])
                if (mixin + 1 - j > 1)
                  ar.delimit_array();
              }
              ar.end_array();

              ar.tag("c1");
              FIELDS(CLSAGs[i].c1)

              // CLSAGs[i].I not saved, it can be reconstructed
              ar.tag("D");
              FIELDS(CLSAGs[i].D)
              ar.end_object();

              if (inputs - i > 1)
                 ar.delimit_array();
            }

            ar.end_array();
          }
          else
          {
            ar.tag("MGs");
            ar.begin_array();
            // we keep a byte for size of MGs, because we don't know whether this is
            // a simple or full rct signature, and it's starting to annoy the hell out of me
            size_t mg_elements = (type == RCTTypeSimple || type == RCTTypeBulletproof || type == RCTTypeBulletproof2) ? inputs : 1;
            PREPARE_CUSTOM_VECTOR_SERIALIZATION(mg_elements, MGs);
            if (MGs.size() != mg_elements)
              return false;
            for (size_t i = 0; i < mg_elements; ++i)
            {
              // we save the MGs contents directly, because we want it to save its
              // arrays and matrices without the size prefixes, and the load can't
              // know what size to expect if it's not in the data
              ar.begin_object();
              ar.tag("ss");
              ar.begin_array();
              PREPARE_CUSTOM_VECTOR_SERIALIZATION(mixin + 1, MGs[i].ss);
              if (MGs[i].ss.size() != mixin + 1)
                return false;
              for (size_t j = 0; j < mixin + 1; ++j)
              {
                ar.begin_array();
                size_t mg_ss2_elements = ((type == RCTTypeSimple || type == RCTTypeBulletproof || type == RCTTypeBulletproof2) ? 1 : inputs) + 1;
                PREPARE_CUSTOM_VECTOR_SERIALIZATION(mg_ss2_elements, MGs[i].ss[j]);
                if (MGs[i].ss[j].size() != mg_ss2_elements)
                  return false;
                for (size_t k = 0; k < mg_ss2_elements; ++k)
                {
                  FIELDS(MGs[i].ss[j][k])
                  if (mg_ss2_elements - k > 1)
                    ar.delimit_array();
                }
                ar.end_array();
  
                if (mixin + 1 - j > 1)
                  ar.delimit_array();
              }
              ar.end_array();

              ar.tag("cc");
              FIELDS(MGs[i].cc)
              // MGs[i].II not saved, it can be reconstructed
              ar.end_object();

              if (mg_elements - i > 1)
                 ar.delimit_array();
            }
            ar.end_array();
          }
          if (type == RCTTypeBulletproof || type == RCTTypeBulletproof2 || type == RCTTypeCLSAG || type == RCTTypeBulletproofPlus || type == RCTTypeFcmpPlusPlus)
          {
            ar.tag("pseudoOuts");
            ar.begin_array();
            PREPARE_CUSTOM_VECTOR_SERIALIZATION(inputs, pseudoOuts);
            if (pseudoOuts.size() != inputs)
              return false;
            for (size_t i = 0; i < inputs; ++i)
            {
              FIELDS(pseudoOuts[i])
              if (inputs - i > 1)
                ar.delimit_array();
            }
            ar.end_array();
          }
          return ar.good();
        }

        BEGIN_SERIALIZE_OBJECT()
          FIELD(rangeSigs)
          FIELD(bulletproofs)
          FIELD(bulletproofs_plus)
          FIELD(MGs)
          FIELD(CLSAGs)
          VARINT_FIELD(reference_block)
          VARINT_FIELD(n_tree_layers)
          FIELD(fcmp_pp)
          FIELD(pseudoOuts)
        END_SERIALIZE()
    };
    struct rctSig: public rctSigBase {
        rctSigPrunable p;

        keyV& get_pseudo_outs()
        {
          return type == RCTTypeBulletproof || type == RCTTypeBulletproof2 || type == RCTTypeCLSAG || type == RCTTypeBulletproofPlus || type == RCTTypeFcmpPlusPlus ? p.pseudoOuts : pseudoOuts;
        }

        keyV const& get_pseudo_outs() const
        {
          return type == RCTTypeBulletproof || type == RCTTypeBulletproof2 || type == RCTTypeCLSAG || type == RCTTypeBulletproofPlus || type == RCTTypeFcmpPlusPlus ? p.pseudoOuts : pseudoOuts;
        }

        BEGIN_SERIALIZE_OBJECT()
          FIELDS((rctSigBase&)*this)
          FIELD(p)
        END_SERIALIZE()
    };

    //other basepoint H = toPoint(cn_fast_hash(G)), G the basepoint
    static const key H = { {0x8b, 0x65, 0x59, 0x70, 0x15, 0x37, 0x99, 0xaf, 0x2a, 0xea, 0xdc, 0x9f, 0xf1, 0xad, 0xd0, 0xea, 0x6c, 0x72, 0x51, 0xd5, 0x41, 0x54, 0xcf, 0xa9, 0x2c, 0x17, 0x3a, 0x0d, 0xd3, 0x9c, 0x1f, 0x94} };

    //H2 contains 2^i H in each index, i.e. H, 2H, 4H, 8H, ...
    //This is used for the range proofG
    //You can regenerate this by running python2 Test.py HPow2 in the MiniNero repo
    static const key64 H2 = {{{0x8b, 0x65, 0x59, 0x70, 0x15, 0x37, 0x99, 0xaf, 0x2a, 0xea, 0xdc, 0x9f, 0xf1, 0xad, 0xd0, 0xea, 0x6c, 0x72, 0x51, 0xd5, 0x41, 0x54, 0xcf, 0xa9, 0x2c, 0x17, 0x3a, 0x0d, 0xd3, 0x9c, 0x1f, 0x94}},
    {{0x8f, 0xaa, 0x44, 0x8a, 0xe4, 0xb3, 0xe2, 0xbb, 0x3d, 0x4d, 0x13, 0x09, 0x09, 0xf5, 0x5f, 0xcd, 0x79, 0x71, 0x1c, 0x1c, 0x83, 0xcd, 0xbc, 0xca, 0xdd, 0x42, 0xcb, 0xe1, 0x51, 0x5e, 0x87, 0x12}},
    {{0x12, 0xa7, 0xd6, 0x2c, 0x77, 0x91, 0x65, 0x4a, 0x57, 0xf3, 0xe6, 0x76, 0x94, 0xed, 0x50, 0xb4, 0x9a, 0x7d, 0x9e, 0x3f, 0xc1, 0xe4, 0xc7, 0xa0, 0xbd, 0xe2, 0x9d, 0x18, 0x7e, 0x9c, 0xc7, 0x1d}},
    {{0x78, 0x9a, 0xb9, 0x93, 0x4b, 0x49, 0xc4, 0xf9, 0xe6, 0x78, 0x5c, 0x6d, 0x57, 0xa4, 0x98, 0xb3, 0xea, 0xd4, 0x43, 0xf0, 0x4f, 0x13, 0xdf, 0x11, 0x0c, 0x54, 0x27, 0xb4, 0xf2, 0x14, 0xc7, 0x39}},
    {{0x77, 0x1e, 0x92, 0x99, 0xd9, 0x4f, 0x02, 0xac, 0x72, 0xe3, 0x8e, 0x44, 0xde, 0x56, 0x8a, 0xc1, 0xdc, 0xb2, 0xed, 0xc6, 0xed, 0xb6, 0x1f, 0x83, 0xca, 0x41, 0x8e, 0x10, 0x77, 0xce, 0x3d, 0xe8}},
    {{0x73, 0xb9, 0x6d, 0xb4, 0x30, 0x39, 0x81, 0x9b, 0xda, 0xf5, 0x68, 0x0e, 0x5c, 0x32, 0xd7, 0x41, 0x48, 0x88, 0x84, 0xd1, 0x8d, 0x93, 0x86, 0x6d, 0x40, 0x74, 0xa8, 0x49, 0x18, 0x2a, 0x8a, 0x64}},
    {{0x8d, 0x45, 0x8e, 0x1c, 0x2f, 0x68, 0xeb, 0xeb, 0xcc, 0xd2, 0xfd, 0x5d, 0x37, 0x9f, 0x5e, 0x58, 0xf8, 0x13, 0x4d, 0xf3, 0xe0, 0xe8, 0x8c, 0xad, 0x3d, 0x46, 0x70, 0x10, 0x63, 0xa8, 0xd4, 0x12}},
    {{0x09, 0x55, 0x1e, 0xdb, 0xe4, 0x94, 0x41, 0x8e, 0x81, 0x28, 0x44, 0x55, 0xd6, 0x4b, 0x35, 0xee, 0x8a, 0xc0, 0x93, 0x06, 0x8a, 0x5f, 0x16, 0x1f, 0xa6, 0x63, 0x75, 0x59, 0x17, 0x7e, 0xf4, 0x04}},
    {{0xd0, 0x5a, 0x88, 0x66, 0xf4, 0xdf, 0x8c, 0xee, 0x1e, 0x26, 0x8b, 0x1d, 0x23, 0xa4, 0xc5, 0x8c, 0x92, 0xe7, 0x60, 0x30, 0x97, 0x86, 0xcd, 0xac, 0x0f, 0xed, 0xa1, 0xd2, 0x47, 0xa9, 0xc9, 0xa7}},
    {{0x55, 0xcd, 0xaa, 0xd5, 0x18, 0xbd, 0x87, 0x1d, 0xd1, 0xeb, 0x7b, 0xc7, 0x02, 0x3e, 0x1d, 0xc0, 0xfd, 0xf3, 0x33, 0x98, 0x64, 0xf8, 0x8f, 0xdd, 0x2d, 0xe2, 0x69, 0xfe, 0x9e, 0xe1, 0x83, 0x2d}},
    {{0xe7, 0x69, 0x7e, 0x95, 0x1a, 0x98, 0xcf, 0xd5, 0x71, 0x2b, 0x84, 0xbb, 0xe5, 0xf3, 0x4e, 0xd7, 0x33, 0xe9, 0x47, 0x3f, 0xcb, 0x68, 0xed, 0xa6, 0x6e, 0x37, 0x88, 0xdf, 0x19, 0x58, 0xc3, 0x06}},
    {{0xf9, 0x2a, 0x97, 0x0b, 0xae, 0x72, 0x78, 0x29, 0x89, 0xbf, 0xc8, 0x3a, 0xdf, 0xaa, 0x92, 0xa4, 0xf4, 0x9c, 0x7e, 0x95, 0x91, 0x8b, 0x3b, 0xba, 0x3c, 0xdc, 0x7f, 0xe8, 0x8a, 0xcc, 0x8d, 0x47}},
    {{0x1f, 0x66, 0xc2, 0xd4, 0x91, 0xd7, 0x5a, 0xf9, 0x15, 0xc8, 0xdb, 0x6a, 0x6d, 0x1c, 0xb0, 0xcd, 0x4f, 0x7d, 0xdc, 0xd5, 0xe6, 0x3d, 0x3b, 0xa9, 0xb8, 0x3c, 0x86, 0x6c, 0x39, 0xef, 0x3a, 0x2b}},
    {{0x3e, 0xec, 0x98, 0x84, 0xb4, 0x3f, 0x58, 0xe9, 0x3e, 0xf8, 0xde, 0xea, 0x26, 0x00, 0x04, 0xef, 0xea, 0x2a, 0x46, 0x34, 0x4f, 0xc5, 0x96, 0x5b, 0x1a, 0x7d, 0xd5, 0xd1, 0x89, 0x97, 0xef, 0xa7}},
    {{0xb2, 0x9f, 0x8f, 0x0c, 0xcb, 0x96, 0x97, 0x7f, 0xe7, 0x77, 0xd4, 0x89, 0xd6, 0xbe, 0x9e, 0x7e, 0xbc, 0x19, 0xc4, 0x09, 0xb5, 0x10, 0x35, 0x68, 0xf2, 0x77, 0x61, 0x1d, 0x7e, 0xa8, 0x48, 0x94}},
    {{0x56, 0xb1, 0xf5, 0x12, 0x65, 0xb9, 0x55, 0x98, 0x76, 0xd5, 0x8d, 0x24, 0x9d, 0x0c, 0x14, 0x6d, 0x69, 0xa1, 0x03, 0x63, 0x66, 0x99, 0x87, 0x4d, 0x3f, 0x90, 0x47, 0x35, 0x50, 0xfe, 0x3f, 0x2c}},
    {{0x1d, 0x7a, 0x36, 0x57, 0x5e, 0x22, 0xf5, 0xd1, 0x39, 0xff, 0x9c, 0xc5, 0x10, 0xfa, 0x13, 0x85, 0x05, 0x57, 0x6b, 0x63, 0x81, 0x5a, 0x94, 0xe4, 0xb0, 0x12, 0xbf, 0xd4, 0x57, 0xca, 0xaa, 0xda}},
    {{0xd0, 0xac, 0x50, 0x7a, 0x86, 0x4e, 0xcd, 0x05, 0x93, 0xfa, 0x67, 0xbe, 0x7d, 0x23, 0x13, 0x43, 0x92, 0xd0, 0x0e, 0x40, 0x07, 0xe2, 0x53, 0x48, 0x78, 0xd9, 0xb2, 0x42, 0xe1, 0x0d, 0x76, 0x20}},
    {{0xf6, 0xc6, 0x84, 0x0b, 0x9c, 0xf1, 0x45, 0xbb, 0x2d, 0xcc, 0xf8, 0x6e, 0x94, 0x0b, 0xe0, 0xfc, 0x09, 0x8e, 0x32, 0xe3, 0x10, 0x99, 0xd5, 0x6f, 0x7f, 0xe0, 0x87, 0xbd, 0x5d, 0xeb, 0x50, 0x94}},
    {{0x28, 0x83, 0x1a, 0x33, 0x40, 0x07, 0x0e, 0xb1, 0xdb, 0x87, 0xc1, 0x2e, 0x05, 0x98, 0x0d, 0x5f, 0x33, 0xe9, 0xef, 0x90, 0xf8, 0x3a, 0x48, 0x17, 0xc9, 0xf4, 0xa0, 0xa3, 0x32, 0x27, 0xe1, 0x97}},
    {{0x87, 0x63, 0x22, 0x73, 0xd6, 0x29, 0xcc, 0xb7, 0xe1, 0xed, 0x1a, 0x76, 0x8f, 0xa2, 0xeb, 0xd5, 0x17, 0x60, 0xf3, 0x2e, 0x1c, 0x0b, 0x86, 0x7a, 0x5d, 0x36, 0x8d, 0x52, 0x71, 0x05, 0x5c, 0x6e}},
    {{0x5c, 0x7b, 0x29, 0x42, 0x43, 0x47, 0x96, 0x4d, 0x04, 0x27, 0x55, 0x17, 0xc5, 0xae, 0x14, 0xb6, 0xb5, 0xea, 0x27, 0x98, 0xb5, 0x73, 0xfc, 0x94, 0xe6, 0xe4, 0x4a, 0x53, 0x21, 0x60, 0x0c, 0xfb}},
    {{0xe6, 0x94, 0x50, 0x42, 0xd7, 0x8b, 0xc2, 0xc3, 0xbd, 0x6e, 0xc5, 0x8c, 0x51, 0x1a, 0x9f, 0xe8, 0x59, 0xc0, 0xad, 0x63, 0xfd, 0xe4, 0x94, 0xf5, 0x03, 0x9e, 0x0e, 0x82, 0x32, 0x61, 0x2b, 0xd5}},
    {{0x36, 0xd5, 0x69, 0x07, 0xe2, 0xec, 0x74, 0x5d, 0xb6, 0xe5, 0x4f, 0x0b, 0x2e, 0x1b, 0x23, 0x00, 0xab, 0xcb, 0x42, 0x2e, 0x71, 0x2d, 0xa5, 0x88, 0xa4, 0x0d, 0x3f, 0x1e, 0xbb, 0xbe, 0x02, 0xf6}},
    {{0x34, 0xdb, 0x6e, 0xe4, 0xd0, 0x60, 0x8e, 0x5f, 0x78, 0x36, 0x50, 0x49, 0x5a, 0x3b, 0x2f, 0x52, 0x73, 0xc5, 0x13, 0x4e, 0x52, 0x84, 0xe4, 0xfd, 0xf9, 0x66, 0x27, 0xbb, 0x16, 0xe3, 0x1e, 0x6b}},
    {{0x8e, 0x76, 0x59, 0xfb, 0x45, 0xa3, 0x78, 0x7d, 0x67, 0x4a, 0xe8, 0x67, 0x31, 0xfa, 0xa2, 0x53, 0x8e, 0xc0, 0xfd, 0xf4, 0x42, 0xab, 0x26, 0xe9, 0xc7, 0x91, 0xfa, 0xda, 0x08, 0x94, 0x67, 0xe9}},
    {{0x30, 0x06, 0xcf, 0x19, 0x8b, 0x24, 0xf3, 0x1b, 0xb4, 0xc7, 0xe6, 0x34, 0x60, 0x00, 0xab, 0xc7, 0x01, 0xe8, 0x27, 0xcf, 0xbb, 0x5d, 0xf5, 0x2d, 0xcf, 0xa4, 0x2e, 0x9c, 0xa9, 0xff, 0x08, 0x02}},
    {{0xf5, 0xfd, 0x40, 0x3c, 0xb6, 0xe8, 0xbe, 0x21, 0x47, 0x2e, 0x37, 0x7f, 0xfd, 0x80, 0x5a, 0x8c, 0x60, 0x83, 0xea, 0x48, 0x03, 0xb8, 0x48, 0x53, 0x89, 0xcc, 0x3e, 0xbc, 0x21, 0x5f, 0x00, 0x2a}},
    {{0x37, 0x31, 0xb2, 0x60, 0xeb, 0x3f, 0x94, 0x82, 0xe4, 0x5f, 0x1c, 0x3f, 0x3b, 0x9d, 0xcf, 0x83, 0x4b, 0x75, 0xe6, 0xee, 0xf8, 0xc4, 0x0f, 0x46, 0x1e, 0xa2, 0x7e, 0x8b, 0x6e, 0xd9, 0x47, 0x3d}},
    {{0x9f, 0x9d, 0xab, 0x09, 0xc3, 0xf5, 0xe4, 0x28, 0x55, 0xc2, 0xde, 0x97, 0x1b, 0x65, 0x93, 0x28, 0xa2, 0xdb, 0xc4, 0x54, 0x84, 0x5f, 0x39, 0x6f, 0xfc, 0x05, 0x3f, 0x0b, 0xb1, 0x92, 0xf8, 0xc3}},
    {{0x5e, 0x05, 0x5d, 0x25, 0xf8, 0x5f, 0xdb, 0x98, 0xf2, 0x73, 0xe4, 0xaf, 0xe0, 0x84, 0x64, 0xc0, 0x03, 0xb7, 0x0f, 0x1e, 0xf0, 0x67, 0x7b, 0xb5, 0xe2, 0x57, 0x06, 0x40, 0x0b, 0xe6, 0x20, 0xa5}},
    {{0x86, 0x8b, 0xcf, 0x36, 0x79, 0xcb, 0x6b, 0x50, 0x0b, 0x94, 0x41, 0x8c, 0x0b, 0x89, 0x25, 0xf9, 0x86, 0x55, 0x30, 0x30, 0x3a, 0xe4, 0xe4, 0xb2, 0x62, 0x59, 0x18, 0x65, 0x66, 0x6a, 0x45, 0x90}},
    {{0xb3, 0xdb, 0x6b, 0xd3, 0x89, 0x7a, 0xfb, 0xd1, 0xdf, 0x3f, 0x96, 0x44, 0xab, 0x21, 0xc8, 0x05, 0x0e, 0x1f, 0x00, 0x38, 0xa5, 0x2f, 0x7c, 0xa9, 0x5a, 0xc0, 0xc3, 0xde, 0x75, 0x58, 0xcb, 0x7a}},
    {{0x81, 0x19, 0xb3, 0xa0, 0x59, 0xff, 0x2c, 0xac, 0x48, 0x3e, 0x69, 0xbc, 0xd4, 0x1d, 0x6d, 0x27, 0x14, 0x94, 0x47, 0x91, 0x42, 0x88, 0xbb, 0xea, 0xee, 0x34, 0x13, 0xe6, 0xdc, 0xc6, 0xd1, 0xeb}},
    {{0x10, 0xfc, 0x58, 0xf3, 0x5f, 0xc7, 0xfe, 0x7a, 0xe8, 0x75, 0x52, 0x4b, 0xb5, 0x85, 0x00, 0x03, 0x00, 0x5b, 0x7f, 0x97, 0x8c, 0x0c, 0x65, 0xe2, 0xa9, 0x65, 0x46, 0x4b, 0x6d, 0x00, 0x81, 0x9c}},
    {{0x5a, 0xcd, 0x94, 0xeb, 0x3c, 0x57, 0x83, 0x79, 0xc1, 0xea, 0x58, 0xa3, 0x43, 0xec, 0x4f, 0xcf, 0xf9, 0x62, 0x77, 0x6f, 0xe3, 0x55, 0x21, 0xe4, 0x75, 0xa0, 0xe0, 0x6d, 0x88, 0x7b, 0x2d, 0xb9}},
    {{0x33, 0xda, 0xf3, 0xa2, 0x14, 0xd6, 0xe0, 0xd4, 0x2d, 0x23, 0x00, 0xa7, 0xb4, 0x4b, 0x39, 0x29, 0x0d, 0xb8, 0x98, 0x9b, 0x42, 0x79, 0x74, 0xcd, 0x86, 0x5d, 0xb0, 0x11, 0x05, 0x5a, 0x29, 0x01}},
    {{0xcf, 0xc6, 0x57, 0x2f, 0x29, 0xaf, 0xd1, 0x64, 0xa4, 0x94, 0xe6, 0x4e, 0x6f, 0x1a, 0xeb, 0x82, 0x0c, 0x3e, 0x7d, 0xa3, 0x55, 0x14, 0x4e, 0x51, 0x24, 0xa3, 0x91, 0xd0, 0x6e, 0x9f, 0x95, 0xea}},
    {{0xd5, 0x31, 0x2a, 0x4b, 0x0e, 0xf6, 0x15, 0xa3, 0x31, 0xf6, 0x35, 0x2c, 0x2e, 0xd2, 0x1d, 0xac, 0x9e, 0x7c, 0x36, 0x39, 0x8b, 0x93, 0x9a, 0xec, 0x90, 0x1c, 0x25, 0x7f, 0x6c, 0xbc, 0x9e, 0x8e}},
    {{0x55, 0x1d, 0x67, 0xfe, 0xfc, 0x7b, 0x5b, 0x9f, 0x9f, 0xdb, 0xf6, 0xaf, 0x57, 0xc9, 0x6c, 0x8a, 0x74, 0xd7, 0xe4, 0x5a, 0x00, 0x20, 0x78, 0xa7, 0xb5, 0xba, 0x45, 0xc6, 0xfd, 0xe9, 0x3e, 0x33}},
    {{0xd5, 0x0a, 0xc7, 0xbd, 0x5c, 0xa5, 0x93, 0xc6, 0x56, 0x92, 0x8f, 0x38, 0x42, 0x80, 0x17, 0xfc, 0x7b, 0xa5, 0x02, 0x85, 0x4c, 0x43, 0xd8, 0x41, 0x49, 0x50, 0xe9, 0x6e, 0xcb, 0x40, 0x5d, 0xc3}},
    {{0x07, 0x73, 0xe1, 0x8e, 0xa1, 0xbe, 0x44, 0xfe, 0x1a, 0x97, 0xe2, 0x39, 0x57, 0x3c, 0xfa, 0xe3, 0xe4, 0xe9, 0x5e, 0xf9, 0xaa, 0x9f, 0xaa, 0xbe, 0xac, 0x12, 0x74, 0xd3, 0xad, 0x26, 0x16, 0x04}},
    {{0xe9, 0xaf, 0x0e, 0x7c, 0xa8, 0x93, 0x30, 0xd2, 0xb8, 0x61, 0x5d, 0x1b, 0x41, 0x37, 0xca, 0x61, 0x7e, 0x21, 0x29, 0x7f, 0x2f, 0x0d, 0xed, 0x8e, 0x31, 0xb7, 0xd2, 0xea, 0xd8, 0x71, 0x46, 0x60}},
    {{0x7b, 0x12, 0x45, 0x83, 0x09, 0x7f, 0x10, 0x29, 0xa0, 0xc7, 0x41, 0x91, 0xfe, 0x73, 0x78, 0xc9, 0x10, 0x5a, 0xcc, 0x70, 0x66, 0x95, 0xed, 0x14, 0x93, 0xbb, 0x76, 0x03, 0x42, 0x26, 0xa5, 0x7b}},
    {{0xec, 0x40, 0x05, 0x7b, 0x99, 0x54, 0x76, 0x65, 0x0b, 0x3d, 0xb9, 0x8e, 0x9d, 0xb7, 0x57, 0x38, 0xa8, 0xcd, 0x2f, 0x94, 0xd8, 0x63, 0xb9, 0x06, 0x15, 0x0c, 0x56, 0xaa, 0xc1, 0x9c, 0xaa, 0x6b}},
    {{0x01, 0xd9, 0xff, 0x72, 0x9e, 0xfd, 0x39, 0xd8, 0x37, 0x84, 0xc0, 0xfe, 0x59, 0xc4, 0xae, 0x81, 0xa6, 0x70, 0x34, 0xcb, 0x53, 0xc9, 0x43, 0xfb, 0x81, 0x8b, 0x9d, 0x8a, 0xe7, 0xfc, 0x33, 0xe5}},
    {{0x00, 0xdf, 0xb3, 0xc6, 0x96, 0x32, 0x8c, 0x76, 0x42, 0x45, 0x19, 0xa7, 0xbe, 0xfe, 0x8e, 0x0f, 0x6c, 0x76, 0xf9, 0x47, 0xb5, 0x27, 0x67, 0x91, 0x6d, 0x24, 0x82, 0x3f, 0x73, 0x5b, 0xaf, 0x2e}},
    {{0x46, 0x1b, 0x79, 0x9b, 0x4d, 0x9c, 0xee, 0xa8, 0xd5, 0x80, 0xdc, 0xb7, 0x6d, 0x11, 0x15, 0x0d, 0x53, 0x5e, 0x16, 0x39, 0xd1, 0x60, 0x03, 0xc3, 0xfb, 0x7e, 0x9d, 0x1f, 0xd1, 0x30, 0x83, 0xa8}},
    {{0xee, 0x03, 0x03, 0x94, 0x79, 0xe5, 0x22, 0x8f, 0xdc, 0x55, 0x1c, 0xbd, 0xe7, 0x07, 0x9d, 0x34, 0x12, 0xea, 0x18, 0x6a, 0x51, 0x7c, 0xcc, 0x63, 0xe4, 0x6e, 0x9f, 0xcc, 0xe4, 0xfe, 0x3a, 0x6c}},
    {{0xa8, 0xcf, 0xb5, 0x43, 0x52, 0x4e, 0x7f, 0x02, 0xb9, 0xf0, 0x45, 0xac, 0xd5, 0x43, 0xc2, 0x1c, 0x37, 0x3b, 0x4c, 0x9b, 0x98, 0xac, 0x20, 0xce, 0xc4, 0x17, 0xa6, 0xdd, 0xb5, 0x74, 0x4e, 0x94}},
    {{0x93, 0x2b, 0x79, 0x4b, 0xf8, 0x9c, 0x6e, 0xda, 0xf5, 0xd0, 0x65, 0x0c, 0x7c, 0x4b, 0xad, 0x92, 0x42, 0xb2, 0x56, 0x26, 0xe3, 0x7e, 0xad, 0x5a, 0xa7, 0x5e, 0xc8, 0xc6, 0x4e, 0x09, 0xdd, 0x4f}},
    {{0x16, 0xb1, 0x0c, 0x77, 0x9c, 0xe5, 0xcf, 0xef, 0x59, 0xc7, 0x71, 0x0d, 0x2e, 0x68, 0x44, 0x1e, 0xa6, 0xfa, 0xcb, 0x68, 0xe9, 0xb5, 0xf7, 0xd5, 0x33, 0xae, 0x0b, 0xb7, 0x8e, 0x28, 0xbf, 0x57}},
    {{0x0f, 0x77, 0xc7, 0x67, 0x43, 0xe7, 0x39, 0x6f, 0x99, 0x10, 0x13, 0x9f, 0x49, 0x37, 0xd8, 0x37, 0xae, 0x54, 0xe2, 0x10, 0x38, 0xac, 0x5c, 0x0b, 0x3f, 0xd6, 0xef, 0x17, 0x1a, 0x28, 0xa7, 0xe4}},
    {{0xd7, 0xe5, 0x74, 0xb7, 0xb9, 0x52, 0xf2, 0x93, 0xe8, 0x0d, 0xde, 0x90, 0x5e, 0xb5, 0x09, 0x37, 0x3f, 0x3f, 0x6c, 0xd1, 0x09, 0xa0, 0x22, 0x08, 0xb3, 0xc1, 0xe9, 0x24, 0x08, 0x0a, 0x20, 0xca}},
    {{0x45, 0x66, 0x6f, 0x8c, 0x38, 0x1e, 0x3d, 0xa6, 0x75, 0x56, 0x3f, 0xf8, 0xba, 0x23, 0xf8, 0x3b, 0xfa, 0xc3, 0x0c, 0x34, 0xab, 0xdd, 0xe6, 0xe5, 0xc0, 0x97, 0x5e, 0xf9, 0xfd, 0x70, 0x0c, 0xb9}},
    {{0xb2, 0x46, 0x12, 0xe4, 0x54, 0x60, 0x7e, 0xb1, 0xab, 0xa4, 0x47, 0xf8, 0x16, 0xd1, 0xa4, 0x55, 0x1e, 0xf9, 0x5f, 0xa7, 0x24, 0x7f, 0xb7, 0xc1, 0xf5, 0x03, 0x02, 0x0a, 0x71, 0x77, 0xf0, 0xdd}},
    {{0x7e, 0x20, 0x88, 0x61, 0x85, 0x6d, 0xa4, 0x2c, 0x8b, 0xb4, 0x6a, 0x75, 0x67, 0xf8, 0x12, 0x13, 0x62, 0xd9, 0xfb, 0x24, 0x96, 0xf1, 0x31, 0xa4, 0xaa, 0x90, 0x17, 0xcf, 0x36, 0x6c, 0xdf, 0xce}},
    {{0x5b, 0x64, 0x6b, 0xff, 0x6a, 0xd1, 0x10, 0x01, 0x65, 0x03, 0x7a, 0x05, 0x56, 0x01, 0xea, 0x02, 0x35, 0x8c, 0x0f, 0x41, 0x05, 0x0f, 0x9d, 0xfe, 0x3c, 0x95, 0xdc, 0xcb, 0xd3, 0x08, 0x7b, 0xe0}},
    {{0x74, 0x6d, 0x1d, 0xcc, 0xfe, 0xd2, 0xf0, 0xff, 0x1e, 0x13, 0xc5, 0x1e, 0x2d, 0x50, 0xd5, 0x32, 0x43, 0x75, 0xfb, 0xd5, 0xbf, 0x7c, 0xa8, 0x2a, 0x89, 0x31, 0x82, 0x8d, 0x80, 0x1d, 0x43, 0xab}},
    {{0xcb, 0x98, 0x11, 0x0d, 0x4a, 0x6b, 0xb9, 0x7d, 0x22, 0xfe, 0xad, 0xbc, 0x6c, 0x0d, 0x89, 0x30, 0xc5, 0xf8, 0xfc, 0x50, 0x8b, 0x2f, 0xc5, 0xb3, 0x53, 0x28, 0xd2, 0x6b, 0x88, 0xdb, 0x19, 0xae}},
    {{0x60, 0xb6, 0x26, 0xa0, 0x33, 0xb5, 0x5f, 0x27, 0xd7, 0x67, 0x6c, 0x40, 0x95, 0xea, 0xba, 0xbc, 0x7a, 0x2c, 0x7e, 0xde, 0x26, 0x24, 0xb4, 0x72, 0xe9, 0x7f, 0x64, 0xf9, 0x6b, 0x8c, 0xfc, 0x0e}},
    {{0xe5, 0xb5, 0x2b, 0xc9, 0x27, 0x46, 0x8d, 0xf7, 0x18, 0x93, 0xeb, 0x81, 0x97, 0xef, 0x82, 0x0c, 0xf7, 0x6c, 0xb0, 0xaa, 0xf6, 0xe8, 0xe4, 0xfe, 0x93, 0xad, 0x62, 0xd8, 0x03, 0x98, 0x31, 0x04}},
    {{0x05, 0x65, 0x41, 0xae, 0x5d, 0xa9, 0x96, 0x1b, 0xe2, 0xb0, 0xa5, 0xe8, 0x95, 0xe5, 0xc5, 0xba, 0x15, 0x3c, 0xbb, 0x62, 0xdd, 0x56, 0x1a, 0x42, 0x7b, 0xad, 0x0f, 0xfd, 0x41, 0x92, 0x31, 0x99}},
    {{0xf8, 0xfe, 0xf0, 0x5a, 0x3f, 0xa5, 0xc9, 0xf3, 0xeb, 0xa4, 0x16, 0x38, 0xb2, 0x47, 0xb7, 0x11, 0xa9, 0x9f, 0x96, 0x0f, 0xe7, 0x3a, 0xa2, 0xf9, 0x01, 0x36, 0xae, 0xb2, 0x03, 0x29, 0xb8, 0x88}}};

    //Debug printing for the above types
    //Actually use DP(value) and #define DBG
    void dp(key a);
    void dp(bool a);
    void dp(const char * a, int l);
    void dp(keyV a);
    void dp(keyM a);
    void dp(xmr_amount vali);
    void dp(int vali);
    void dp(bits amountb);
    void dp(const char * st);

    //various conversions

    //uint long long to 32 byte key
    void d2h(key & amounth, xmr_amount val);
    key d2h(xmr_amount val);
    //uint long long to int[64]
    void d2b(bits  amountb, xmr_amount val);
    //32 byte key to uint long long
    // if the key holds a value > 2^64
    // then the value in the first 8 bytes is returned
    xmr_amount h2d(const key &test);
    //32 byte key to int[64]
    void h2b(bits  amountb2, const key & test);
    //int[64] to 32 byte key
    void b2h(key  & amountdh, bits amountb2);
    //int[64] to uint long long
    xmr_amount b2d(bits amountb);

    bool is_rct_simple(int type);
    bool is_rct_bulletproof(int type);
    bool is_rct_bulletproof_plus(int type);
    bool is_rct_borromean(int type);
    bool is_rct_clsag(int type);
    bool is_rct_short_amount(int type);
    bool is_rct_fcmp(int type);

    static inline const rct::key &pk2rct(const crypto::public_key &pk) { return (const rct::key&)pk; }
    static inline const rct::key &sk2rct(const crypto::secret_key &sk) { return (const rct::key&)sk; }
    static inline const rct::key &ki2rct(const crypto::key_image &ki) { return (const rct::key&)ki; }
    static inline const rct::key &hash2rct(const crypto::hash &h) { return (const rct::key&)h; }
    static inline const rct::key &pt2rct(const crypto::ec_point &pt) { return (const rct::key&)pt; }
    static inline const crypto::public_key &rct2pk(const rct::key &k) { return (const crypto::public_key&)k; }
    static inline const crypto::secret_key &rct2sk(const rct::key &k) { return (const crypto::secret_key&)k; }
    static inline const crypto::key_image &rct2ki(const rct::key &k) { return (const crypto::key_image&)k; }
    static inline const crypto::hash &rct2hash(const rct::key &k) { return (const crypto::hash&)k; }
    static inline const crypto::ec_point &rct2pt(const rct::key &k) { return (const crypto::ec_point&)k; }
    static inline bool operator==(const rct::key &k0, const crypto::public_key &k1) { return !crypto_verify_32(k0.bytes, (const unsigned char*)&k1); }
    static inline bool operator!=(const rct::key &k0, const crypto::public_key &k1) { return crypto_verify_32(k0.bytes, (const unsigned char*)&k1); }
}


namespace cryptonote {
    static inline bool operator==(const crypto::public_key &k0, const rct::key &k1) { return !crypto_verify_32((const unsigned char*)&k0, k1.bytes); }
    static inline bool operator!=(const crypto::public_key &k0, const rct::key &k1) { return crypto_verify_32((const unsigned char*)&k0, k1.bytes); }
    static inline bool operator==(const crypto::secret_key &k0, const rct::key &k1) { return !crypto_verify_32((const unsigned char*)&k0, k1.bytes); }
    static inline bool operator!=(const crypto::secret_key &k0, const rct::key &k1) { return crypto_verify_32((const unsigned char*)&k0, k1.bytes); }
}

namespace rct {
inline std::ostream &operator <<(std::ostream &o, const rct::key &v) {
  epee::to_hex::formatted(o, epee::as_byte_span(v)); return o;
}
}


namespace std
{
  template<> struct hash<rct::key> { std::size_t operator()(const rct::key &k) const { return reinterpret_cast<const std::size_t&>(k); } };
}

BLOB_SERIALIZER(rct::key);
BLOB_SERIALIZER(rct::key64);
BLOB_SERIALIZER(rct::ctkey);
BLOB_SERIALIZER_FORCED(rct::multisig_kLRki);
BLOB_SERIALIZER(rct::boroSig);

VARIANT_TAG(debug_archive, rct::key, "rct::key");
VARIANT_TAG(debug_archive, rct::key64, "rct::key64");
VARIANT_TAG(debug_archive, rct::keyV, "rct::keyV");
VARIANT_TAG(debug_archive, rct::keyM, "rct::keyM");
VARIANT_TAG(debug_archive, rct::ctkey, "rct::ctkey");
VARIANT_TAG(debug_archive, rct::ctkeyV, "rct::ctkeyV");
VARIANT_TAG(debug_archive, rct::ctkeyM, "rct::ctkeyM");
VARIANT_TAG(debug_archive, rct::ecdhTuple, "rct::ecdhTuple");
VARIANT_TAG(debug_archive, rct::mgSig, "rct::mgSig");
VARIANT_TAG(debug_archive, rct::rangeSig, "rct::rangeSig");
VARIANT_TAG(debug_archive, rct::boroSig, "rct::boroSig");
VARIANT_TAG(debug_archive, rct::rctSig, "rct::rctSig");
VARIANT_TAG(debug_archive, rct::Bulletproof, "rct::bulletproof");
VARIANT_TAG(debug_archive, rct::multisig_kLRki, "rct::multisig_kLRki");
VARIANT_TAG(debug_archive, rct::multisig_out, "rct::multisig_out");
VARIANT_TAG(debug_archive, rct::clsag, "rct::clsag");
VARIANT_TAG(debug_archive, rct::BulletproofPlus, "rct::bulletproof_plus");

VARIANT_TAG(binary_archive, rct::key, 0x90);
VARIANT_TAG(binary_archive, rct::key64, 0x91);
VARIANT_TAG(binary_archive, rct::keyV, 0x92);
VARIANT_TAG(binary_archive, rct::keyM, 0x93);
VARIANT_TAG(binary_archive, rct::ctkey, 0x94);
VARIANT_TAG(binary_archive, rct::ctkeyV, 0x95);
VARIANT_TAG(binary_archive, rct::ctkeyM, 0x96);
VARIANT_TAG(binary_archive, rct::ecdhTuple, 0x97);
VARIANT_TAG(binary_archive, rct::mgSig, 0x98);
VARIANT_TAG(binary_archive, rct::rangeSig, 0x99);
VARIANT_TAG(binary_archive, rct::boroSig, 0x9a);
VARIANT_TAG(binary_archive, rct::rctSig, 0x9b);
VARIANT_TAG(binary_archive, rct::Bulletproof, 0x9c);
VARIANT_TAG(binary_archive, rct::multisig_kLRki, 0x9d);
VARIANT_TAG(binary_archive, rct::multisig_out, 0x9e);
VARIANT_TAG(binary_archive, rct::clsag, 0x9f);
VARIANT_TAG(binary_archive, rct::BulletproofPlus, 0xa0);

VARIANT_TAG(json_archive, rct::key, "rct_key");
VARIANT_TAG(json_archive, rct::key64, "rct_key64");
VARIANT_TAG(json_archive, rct::keyV, "rct_keyV");
VARIANT_TAG(json_archive, rct::keyM, "rct_keyM");
VARIANT_TAG(json_archive, rct::ctkey, "rct_ctkey");
VARIANT_TAG(json_archive, rct::ctkeyV, "rct_ctkeyV");
VARIANT_TAG(json_archive, rct::ctkeyM, "rct_ctkeyM");
VARIANT_TAG(json_archive, rct::ecdhTuple, "rct_ecdhTuple");
VARIANT_TAG(json_archive, rct::mgSig, "rct_mgSig");
VARIANT_TAG(json_archive, rct::rangeSig, "rct_rangeSig");
VARIANT_TAG(json_archive, rct::boroSig, "rct_boroSig");
VARIANT_TAG(json_archive, rct::rctSig, "rct_rctSig");
VARIANT_TAG(json_archive, rct::Bulletproof, "rct_bulletproof");
VARIANT_TAG(json_archive, rct::multisig_kLRki, "rct_multisig_kLR");
VARIANT_TAG(json_archive, rct::multisig_out, "rct_multisig_out");
VARIANT_TAG(json_archive, rct::clsag, "rct_clsag");
VARIANT_TAG(json_archive, rct::BulletproofPlus, "rct_bulletproof_plus");

#endif  /* RCTTYPES_H */
