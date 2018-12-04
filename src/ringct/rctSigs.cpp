// Copyright (c) 2016, Monero Research Labs
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

#include "misc_log_ex.h"
#include "common/perf_timer.h"
#include "common/threadpool.h"
#include "common/util.h"
#include "rctSigs.h"
#include "bulletproofs.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

using namespace crypto;
using namespace std;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "ringct"

#define CHECK_AND_ASSERT_MES_L1(expr, ret, message) {if(!(expr)) {MCERROR("verify", message); return ret;}}

namespace
{
    rct::Bulletproof make_dummy_bulletproof(size_t n_outs)
    {
        const rct::key I = rct::identity();
        size_t nrl = 0;
        while ((1u << nrl) < n_outs)
          ++nrl;
        nrl += 6;
        return rct::Bulletproof{rct::keyV(n_outs, I), I, I, I, I, I, I, rct::keyV(nrl, I), rct::keyV(nrl, I), I, I, I};
    }
}

namespace rct {
    Bulletproof proveRangeBulletproof(keyV &C, keyV &masks, const std::vector<uint64_t> &amounts)
    {
        masks = rct::skvGen(amounts.size());
        Bulletproof proof = bulletproof_PROVE(amounts, masks);
        CHECK_AND_ASSERT_THROW_MES(proof.V.size() == amounts.size(), "V does not have the expected size");
        C = proof.V;
        return proof;
    }

    bool verBulletproof(const Bulletproof &proof)
    {
      try { return bulletproof_VERIFY(proof); }
      // we can get deep throws from ge_frombytes_vartime if input isn't valid
      catch (...) { return false; }
    }

    bool verBulletproof(const std::vector<const Bulletproof*> &proofs)
    {
      try { return bulletproof_VERIFY(proofs); }
      // we can get deep throws from ge_frombytes_vartime if input isn't valid
      catch (...) { return false; }
    }

    //Borromean (c.f. gmax/andytoshi's paper)
    boroSig genBorromean(const key64 x, const key64 P1, const key64 P2, const bits indices) {
        key64 L[2], alpha;
        key c;
        int naught = 0, prime = 0, ii = 0, jj=0;
        boroSig bb;
        for (ii = 0 ; ii < 64 ; ii++) {
            naught = indices[ii]; prime = (indices[ii] + 1) % 2;
            skGen(alpha[ii]);
            scalarmultBase(L[naught][ii], alpha[ii]);
            if (naught == 0) {
                skGen(bb.s1[ii]);
                c = hash_to_scalar(L[naught][ii]);
                addKeys2(L[prime][ii], bb.s1[ii], c, P2[ii]);
            }
        }
        bb.ee = hash_to_scalar(L[1]); //or L[1]..
        key LL, cc;
        for (jj = 0 ; jj < 64 ; jj++) {
            if (!indices[jj]) {
                sc_mulsub(bb.s0[jj].bytes, x[jj].bytes, bb.ee.bytes, alpha[jj].bytes);
            } else {
                skGen(bb.s0[jj]);
                addKeys2(LL, bb.s0[jj], bb.ee, P1[jj]); //different L0
                cc = hash_to_scalar(LL);
                sc_mulsub(bb.s1[jj].bytes, x[jj].bytes, cc.bytes, alpha[jj].bytes);
            }
        }
        return bb;
    }
    
    //see above.
    bool verifyBorromean(const boroSig &bb, const ge_p3 P1[64], const ge_p3 P2[64]) {
        key64 Lv1; key chash, LL;
        int ii = 0;
        ge_p2 p2;
        for (ii = 0 ; ii < 64 ; ii++) {
            // equivalent of: addKeys2(LL, bb.s0[ii], bb.ee, P1[ii]);
            ge_double_scalarmult_base_vartime(&p2, bb.ee.bytes, &P1[ii], bb.s0[ii].bytes);
            ge_tobytes(LL.bytes, &p2);
            chash = hash_to_scalar(LL);
            // equivalent of: addKeys2(Lv1[ii], bb.s1[ii], chash, P2[ii]);
            ge_double_scalarmult_base_vartime(&p2, chash.bytes, &P2[ii], bb.s1[ii].bytes);
            ge_tobytes(Lv1[ii].bytes, &p2);
        }
        key eeComputed = hash_to_scalar(Lv1); //hash function fine
        return equalKeys(eeComputed, bb.ee);
    }

    bool verifyBorromean(const boroSig &bb, const key64 P1, const key64 P2) {
      ge_p3 P1_p3[64], P2_p3[64];
      for (size_t i = 0 ; i < 64 ; ++i) {
        CHECK_AND_ASSERT_MES_L1(ge_frombytes_vartime(&P1_p3[i], P1[i].bytes) == 0, false, "point conv failed");
        CHECK_AND_ASSERT_MES_L1(ge_frombytes_vartime(&P2_p3[i], P2[i].bytes) == 0, false, "point conv failed");
      }
      return verifyBorromean(bb, P1_p3, P2_p3);
    }

    //Multilayered Spontaneous Anonymous Group Signatures (MLSAG signatures)
    //This is a just slghtly more efficient version than the ones described below
    //(will be explained in more detail in Ring Multisig paper
    //These are aka MG signatutes in earlier drafts of the ring ct paper
    // c.f. https://eprint.iacr.org/2015/1098 section 2. 
    // Gen creates a signature which proves that for some column in the keymatrix "pk"
    //   the signer knows a secret key for each row in that column
    // Ver verifies that the MG sig was created correctly        
    mgSig MLSAG_Gen(const key &message, const keyM & pk, const keyV & xx, const multisig_kLRki *kLRki, key *mscout, const unsigned int index, size_t dsRows, hw::device &hwdev) {
        mgSig rv;
        size_t cols = pk.size();
        CHECK_AND_ASSERT_THROW_MES(cols >= 2, "Error! What is c if cols = 1!");
        CHECK_AND_ASSERT_THROW_MES(index < cols, "Index out of range");
        size_t rows = pk[0].size();
        CHECK_AND_ASSERT_THROW_MES(rows >= 1, "Empty pk");
        for (size_t i = 1; i < cols; ++i) {
          CHECK_AND_ASSERT_THROW_MES(pk[i].size() == rows, "pk is not rectangular");
        }
        CHECK_AND_ASSERT_THROW_MES(xx.size() == rows, "Bad xx size");
        CHECK_AND_ASSERT_THROW_MES(dsRows <= rows, "Bad dsRows size");
        CHECK_AND_ASSERT_THROW_MES((kLRki && mscout) || (!kLRki && !mscout), "Only one of kLRki/mscout is present");
        CHECK_AND_ASSERT_THROW_MES(!kLRki || dsRows == 1, "Multisig requires exactly 1 dsRows");

        size_t i = 0, j = 0, ii = 0;
        key c, c_old, L, R, Hi;
        sc_0(c_old.bytes);
        vector<geDsmp> Ip(dsRows);
        rv.II = keyV(dsRows);
        keyV alpha(rows);
        keyV aG(rows);
        rv.ss = keyM(cols, aG);
        keyV aHP(dsRows);
        keyV toHash(1 + 3 * dsRows + 2 * (rows - dsRows));
        toHash[0] = message;
        DP("here1");
        for (i = 0; i < dsRows; i++) {
            toHash[3 * i + 1] = pk[index][i];
            if (kLRki) {
              // multisig
              alpha[i] = kLRki->k;
              toHash[3 * i + 2] = kLRki->L;
              toHash[3 * i + 3] = kLRki->R;
              rv.II[i] = kLRki->ki;
            }
            else {
              Hi = hashToPoint(pk[index][i]);
              hwdev.mlsag_prepare(Hi, xx[i], alpha[i] , aG[i] , aHP[i] , rv.II[i]);
              toHash[3 * i + 2] = aG[i];
              toHash[3 * i + 3] = aHP[i];
            }
            precomp(Ip[i].k, rv.II[i]);
        }
        size_t ndsRows = 3 * dsRows; //non Double Spendable Rows (see identity chains paper)
        for (i = dsRows, ii = 0 ; i < rows ; i++, ii++) {
            skpkGen(alpha[i], aG[i]); //need to save alphas for later..
            toHash[ndsRows + 2 * ii + 1] = pk[index][i];
            toHash[ndsRows + 2 * ii + 2] = aG[i];
        }

        hwdev.mlsag_hash(toHash, c_old);

        
        i = (index + 1) % cols;
        if (i == 0) {
            copy(rv.cc, c_old);
        }
        while (i != index) {

            rv.ss[i] = skvGen(rows);            
            sc_0(c.bytes);
            for (j = 0; j < dsRows; j++) {
                addKeys2(L, rv.ss[i][j], c_old, pk[i][j]);
                hashToPoint(Hi, pk[i][j]);
                addKeys3(R, rv.ss[i][j], Hi, c_old, Ip[j].k);
                toHash[3 * j + 1] = pk[i][j];
                toHash[3 * j + 2] = L; 
                toHash[3 * j + 3] = R;
            }
            for (j = dsRows, ii = 0; j < rows; j++, ii++) {
                addKeys2(L, rv.ss[i][j], c_old, pk[i][j]);
                toHash[ndsRows + 2 * ii + 1] = pk[i][j];
                toHash[ndsRows + 2 * ii + 2] = L;
            }
            hwdev.mlsag_hash(toHash, c);
            copy(c_old, c);
            i = (i + 1) % cols;
            
            if (i == 0) { 
                copy(rv.cc, c_old);
            }   
        }
        hwdev.mlsag_sign(c, xx, alpha, rows, dsRows, rv.ss[index]);
        if (mscout)
          *mscout = c;
        return rv;
    }
    
    //Multilayered Spontaneous Anonymous Group Signatures (MLSAG signatures)
    //This is a just slghtly more efficient version than the ones described below
    //(will be explained in more detail in Ring Multisig paper
    //These are aka MG signatutes in earlier drafts of the ring ct paper
    // c.f. https://eprint.iacr.org/2015/1098 section 2. 
    // Gen creates a signature which proves that for some column in the keymatrix "pk"
    //   the signer knows a secret key for each row in that column
    // Ver verifies that the MG sig was created correctly            
    bool MLSAG_Ver(const key &message, const keyM & pk, const mgSig & rv, size_t dsRows) {

        size_t cols = pk.size();
        CHECK_AND_ASSERT_MES(cols >= 2, false, "Error! What is c if cols = 1!");
        size_t rows = pk[0].size();
        CHECK_AND_ASSERT_MES(rows >= 1, false, "Empty pk");
        for (size_t i = 1; i < cols; ++i) {
          CHECK_AND_ASSERT_MES(pk[i].size() == rows, false, "pk is not rectangular");
        }
        CHECK_AND_ASSERT_MES(rv.II.size() == dsRows, false, "Bad II size");
        CHECK_AND_ASSERT_MES(rv.ss.size() == cols, false, "Bad rv.ss size");
        for (size_t i = 0; i < cols; ++i) {
          CHECK_AND_ASSERT_MES(rv.ss[i].size() == rows, false, "rv.ss is not rectangular");
        }
        CHECK_AND_ASSERT_MES(dsRows <= rows, false, "Bad dsRows value");

        for (size_t i = 0; i < rv.ss.size(); ++i)
          for (size_t j = 0; j < rv.ss[i].size(); ++j)
            CHECK_AND_ASSERT_MES(sc_check(rv.ss[i][j].bytes) == 0, false, "Bad ss slot");
        CHECK_AND_ASSERT_MES(sc_check(rv.cc.bytes) == 0, false, "Bad cc");

        size_t i = 0, j = 0, ii = 0;
        key c,  L, R, Hi;
        key c_old = copy(rv.cc);
        vector<geDsmp> Ip(dsRows);
        for (i = 0 ; i < dsRows ; i++) {
            precomp(Ip[i].k, rv.II[i]);
        }
        size_t ndsRows = 3 * dsRows; //non Double Spendable Rows (see identity chains paper
        keyV toHash(1 + 3 * dsRows + 2 * (rows - dsRows));
        toHash[0] = message;
        i = 0;
        while (i < cols) {
            sc_0(c.bytes);
            for (j = 0; j < dsRows; j++) {
                addKeys2(L, rv.ss[i][j], c_old, pk[i][j]);
                hashToPoint(Hi, pk[i][j]);
                CHECK_AND_ASSERT_MES(!(Hi == rct::identity()), false, "Data hashed to point at infinity");
                addKeys3(R, rv.ss[i][j], Hi, c_old, Ip[j].k);
                toHash[3 * j + 1] = pk[i][j];
                toHash[3 * j + 2] = L; 
                toHash[3 * j + 3] = R;
            }
            for (j = dsRows, ii = 0 ; j < rows ; j++, ii++) {
                addKeys2(L, rv.ss[i][j], c_old, pk[i][j]);
                toHash[ndsRows + 2 * ii + 1] = pk[i][j];
                toHash[ndsRows + 2 * ii + 2] = L;
            }
            c = hash_to_scalar(toHash);
            copy(c_old, c);
            i = (i + 1);
        }
        sc_sub(c.bytes, c_old.bytes, rv.cc.bytes);
        return sc_isnonzero(c.bytes) == 0;  
    }
    


    //proveRange and verRange
    //proveRange gives C, and mask such that \sumCi = C
    //   c.f. https://eprint.iacr.org/2015/1098 section 5.1
    //   and Ci is a commitment to either 0 or 2^i, i=0,...,63
    //   thus this proves that "amount" is in [0, 2^64]
    //   mask is a such that C = aG + bH, and b = amount
    //verRange verifies that \sum Ci = C and that each Ci is a commitment to 0 or 2^i
    rangeSig proveRange(key & C, key & mask, const xmr_amount & amount) {
        sc_0(mask.bytes);
        identity(C);
        bits b;
        d2b(b, amount);
        rangeSig sig;
        key64 ai;
        key64 CiH;
        int i = 0;
        for (i = 0; i < ATOMS; i++) {
            skGen(ai[i]);
            if (b[i] == 0) {
                scalarmultBase(sig.Ci[i], ai[i]);
            }
            if (b[i] == 1) {
                addKeys1(sig.Ci[i], ai[i], H2[i]);
            }
            subKeys(CiH[i], sig.Ci[i], H2[i]);
            sc_add(mask.bytes, mask.bytes, ai[i].bytes);
            addKeys(C, C, sig.Ci[i]);
        }
        sig.asig = genBorromean(ai, sig.Ci, CiH, b);
        return sig;
    }

    //proveRange and verRange
    //proveRange gives C, and mask such that \sumCi = C
    //   c.f. https://eprint.iacr.org/2015/1098 section 5.1
    //   and Ci is a commitment to either 0 or 2^i, i=0,...,63
    //   thus this proves that "amount" is in [0, 2^64]
    //   mask is a such that C = aG + bH, and b = amount
    //verRange verifies that \sum Ci = C and that each Ci is a commitment to 0 or 2^i
    bool verRange(const key & C, const rangeSig & as) {
      try
      {
        PERF_TIMER(verRange);
        ge_p3 CiH[64], asCi[64];
        int i = 0;
        ge_p3 Ctmp_p3 = ge_p3_identity;
        for (i = 0; i < 64; i++) {
            // faster equivalent of:
            // subKeys(CiH[i], as.Ci[i], H2[i]);
            // addKeys(Ctmp, Ctmp, as.Ci[i]);
            ge_cached cached;
            ge_p3 p3;
            ge_p1p1 p1;
            CHECK_AND_ASSERT_MES_L1(ge_frombytes_vartime(&p3, H2[i].bytes) == 0, false, "point conv failed");
            ge_p3_to_cached(&cached, &p3);
            CHECK_AND_ASSERT_MES_L1(ge_frombytes_vartime(&asCi[i], as.Ci[i].bytes) == 0, false, "point conv failed");
            ge_sub(&p1, &asCi[i], &cached);
            ge_p3_to_cached(&cached, &asCi[i]);
            ge_p1p1_to_p3(&CiH[i], &p1);
            ge_add(&p1, &Ctmp_p3, &cached);
            ge_p1p1_to_p3(&Ctmp_p3, &p1);
        }
        key Ctmp;
        ge_p3_tobytes(Ctmp.bytes, &Ctmp_p3);
        if (!equalKeys(C, Ctmp))
          return false;
        if (!verifyBorromean(as.asig, asCi, CiH))
          return false;
        return true;
      }
      // we can get deep throws from ge_frombytes_vartime if input isn't valid
      catch (...) { return false; }
    }

    key get_pre_mlsag_hash(const rctSig &rv, hw::device &hwdev)
    {
      keyV hashes;
      hashes.reserve(3);
      hashes.push_back(rv.message);
      crypto::hash h;

      std::stringstream ss;
      binary_archive<true> ba(ss);
      CHECK_AND_ASSERT_THROW_MES(!rv.mixRing.empty(), "Empty mixRing");
      const size_t inputs = is_rct_simple(rv.type) ? rv.mixRing.size() : rv.mixRing[0].size();
      const size_t outputs = rv.ecdhInfo.size();
      key prehash;
      CHECK_AND_ASSERT_THROW_MES(const_cast<rctSig&>(rv).serialize_rctsig_base(ba, inputs, outputs),
          "Failed to serialize rctSigBase");
      cryptonote::get_blob_hash(ss.str(), h);
      hashes.push_back(hash2rct(h));

      keyV kv;
      if (rv.type == RCTTypeBulletproof)
      {
        kv.reserve((6*2+9) * rv.p.bulletproofs.size());
        for (const auto &p: rv.p.bulletproofs)
        {
          // V are not hashed as they're expanded from outPk.mask
          // (and thus hashed as part of rctSigBase above)
          kv.push_back(p.A);
          kv.push_back(p.S);
          kv.push_back(p.T1);
          kv.push_back(p.T2);
          kv.push_back(p.taux);
          kv.push_back(p.mu);
          for (size_t n = 0; n < p.L.size(); ++n)
            kv.push_back(p.L[n]);
          for (size_t n = 0; n < p.R.size(); ++n)
            kv.push_back(p.R[n]);
          kv.push_back(p.a);
          kv.push_back(p.b);
          kv.push_back(p.t);
        }
      }
      else
      {
        kv.reserve((64*3+1) * rv.p.rangeSigs.size());
        for (const auto &r: rv.p.rangeSigs)
        {
          for (size_t n = 0; n < 64; ++n)
            kv.push_back(r.asig.s0[n]);
          for (size_t n = 0; n < 64; ++n)
            kv.push_back(r.asig.s1[n]);
          kv.push_back(r.asig.ee);
          for (size_t n = 0; n < 64; ++n)
            kv.push_back(r.Ci[n]);
        }
      }
      hashes.push_back(cn_fast_hash(kv));
      hwdev.mlsag_prehash(ss.str(), inputs, outputs, hashes, rv.outPk, prehash);
      return  prehash;
    }

    //Ring-ct MG sigs
    //Prove: 
    //   c.f. https://eprint.iacr.org/2015/1098 section 4. definition 10. 
    //   This does the MG sig on the "dest" part of the given key matrix, and 
    //   the last row is the sum of input commitments from that column - sum output commitments
    //   this shows that sum inputs = sum outputs
    //Ver:    
    //   verifies the above sig is created corretly
    mgSig proveRctMG(const key &message, const ctkeyM & pubs, const ctkeyV & inSk, const ctkeyV &outSk, const ctkeyV & outPk, const multisig_kLRki *kLRki, key *mscout, unsigned int index, const key &txnFeeKey, hw::device &hwdev) {
        mgSig mg;
        //setup vars
        size_t cols = pubs.size();
        CHECK_AND_ASSERT_THROW_MES(cols >= 1, "Empty pubs");
        size_t rows = pubs[0].size();
        CHECK_AND_ASSERT_THROW_MES(rows >= 1, "Empty pubs");
        for (size_t i = 1; i < cols; ++i) {
          CHECK_AND_ASSERT_THROW_MES(pubs[i].size() == rows, "pubs is not rectangular");
        }
        CHECK_AND_ASSERT_THROW_MES(inSk.size() == rows, "Bad inSk size");
        CHECK_AND_ASSERT_THROW_MES(outSk.size() == outPk.size(), "Bad outSk/outPk size");
        CHECK_AND_ASSERT_THROW_MES((kLRki && mscout) || (!kLRki && !mscout), "Only one of kLRki/mscout is present");

        keyV sk(rows + 1);
        keyV tmp(rows + 1);
        size_t i = 0, j = 0;
        for (i = 0; i < rows + 1; i++) {
            sc_0(sk[i].bytes);
            identity(tmp[i]);
        }
        keyM M(cols, tmp);
        //create the matrix to mg sig
        for (i = 0; i < cols; i++) {
            M[i][rows] = identity();
            for (j = 0; j < rows; j++) {
                M[i][j] = pubs[i][j].dest;
                addKeys(M[i][rows], M[i][rows], pubs[i][j].mask); //add input commitments in last row
            }
        }
        sc_0(sk[rows].bytes);
        for (j = 0; j < rows; j++) {
            sk[j] = copy(inSk[j].dest);
            sc_add(sk[rows].bytes, sk[rows].bytes, inSk[j].mask.bytes); //add masks in last row
        }
        for (i = 0; i < cols; i++) {
            for (size_t j = 0; j < outPk.size(); j++) {
                subKeys(M[i][rows], M[i][rows], outPk[j].mask); //subtract output Ci's in last row
            }
            //subtract txn fee output in last row
            subKeys(M[i][rows], M[i][rows], txnFeeKey);
        }
        for (size_t j = 0; j < outPk.size(); j++) {
            sc_sub(sk[rows].bytes, sk[rows].bytes, outSk[j].mask.bytes); //subtract output masks in last row..
        }
        mgSig result = MLSAG_Gen(message, M, sk, kLRki, mscout, index, rows, hwdev);
        memwipe(sk.data(), sk.size() * sizeof(key));
        return result;
    }


    //Ring-ct MG sigs Simple
    //   Simple version for when we assume only
    //       post rct inputs
    //       here pubs is a vector of (P, C) length mixin
    //   inSk is x, a_in corresponding to signing index
    //       a_out, Cout is for the output commitment
    //       index is the signing index..
    mgSig proveRctMGSimple(const key &message, const ctkeyV & pubs, const ctkey & inSk, const key &a , const key &Cout, const multisig_kLRki *kLRki, key *mscout, unsigned int index, hw::device &hwdev) {
        mgSig mg;
        //setup vars
        size_t rows = 1;
        size_t cols = pubs.size();
        CHECK_AND_ASSERT_THROW_MES(cols >= 1, "Empty pubs");
        CHECK_AND_ASSERT_THROW_MES((kLRki && mscout) || (!kLRki && !mscout), "Only one of kLRki/mscout is present");
        keyV tmp(rows + 1);
        keyV sk(rows + 1);
        size_t i;
        keyM M(cols, tmp);

        sk[0] = copy(inSk.dest);
        sc_sub(sk[1].bytes, inSk.mask.bytes, a.bytes);
        for (i = 0; i < cols; i++) {
            M[i][0] = pubs[i].dest;
            subKeys(M[i][1], pubs[i].mask, Cout);
        }
        mgSig result = MLSAG_Gen(message, M, sk, kLRki, mscout, index, rows, hwdev);
        memwipe(&sk[0], sizeof(key));
        return result;
    }


    //Ring-ct MG sigs
    //Prove: 
    //   c.f. https://eprint.iacr.org/2015/1098 section 4. definition 10. 
    //   This does the MG sig on the "dest" part of the given key matrix, and 
    //   the last row is the sum of input commitments from that column - sum output commitments
    //   this shows that sum inputs = sum outputs
    //Ver:    
    //   verifies the above sig is created corretly
    bool verRctMG(const mgSig &mg, const ctkeyM & pubs, const ctkeyV & outPk, const key &txnFeeKey, const key &message) {
        PERF_TIMER(verRctMG);
        //setup vars
        size_t cols = pubs.size();
        CHECK_AND_ASSERT_MES(cols >= 1, false, "Empty pubs");
        size_t rows = pubs[0].size();
        CHECK_AND_ASSERT_MES(rows >= 1, false, "Empty pubs");
        for (size_t i = 1; i < cols; ++i) {
          CHECK_AND_ASSERT_MES(pubs[i].size() == rows, false, "pubs is not rectangular");
        }

        keyV tmp(rows + 1);
        size_t i = 0, j = 0;
        for (i = 0; i < rows + 1; i++) {
            identity(tmp[i]);
        }
        keyM M(cols, tmp);

        //create the matrix to mg sig
        for (j = 0; j < rows; j++) {
            for (i = 0; i < cols; i++) {
                M[i][j] = pubs[i][j].dest;
                addKeys(M[i][rows], M[i][rows], pubs[i][j].mask); //add Ci in last row
            }
        }
        for (i = 0; i < cols; i++) {
            for (j = 0; j < outPk.size(); j++) {
                subKeys(M[i][rows], M[i][rows], outPk[j].mask); //subtract output Ci's in last row
            }
            //subtract txn fee output in last row
            subKeys(M[i][rows], M[i][rows], txnFeeKey);
        }
        return MLSAG_Ver(message, M, mg, rows);
    }

    //Ring-ct Simple MG sigs
    //Ver: 
    //This does a simplified version, assuming only post Rct
    //inputs
    bool verRctMGSimple(const key &message, const mgSig &mg, const ctkeyV & pubs, const key & C) {
        try
        {
            PERF_TIMER(verRctMGSimple);
            //setup vars
            size_t rows = 1;
            size_t cols = pubs.size();
            CHECK_AND_ASSERT_MES(cols >= 1, false, "Empty pubs");
            keyV tmp(rows + 1);
            size_t i;
            keyM M(cols, tmp);
            //create the matrix to mg sig
            for (i = 0; i < cols; i++) {
                    M[i][0] = pubs[i].dest;
                    subKeys(M[i][1], pubs[i].mask, C);
            }
            //DP(C);
            return MLSAG_Ver(message, M, mg, rows);
        }
        catch (...) { return false; }
    }


    //These functions get keys from blockchain
    //replace these when connecting blockchain
    //getKeyFromBlockchain grabs a key from the blockchain at "reference_index" to mix with
    //populateFromBlockchain creates a keymatrix with "mixin" columns and one of the columns is inPk
    //   the return value are the key matrix, and the index where inPk was put (random).    
    void getKeyFromBlockchain(ctkey & a, size_t reference_index) {
        a.mask = pkGen();
        a.dest = pkGen();
    }

    //These functions get keys from blockchain
    //replace these when connecting blockchain
    //getKeyFromBlockchain grabs a key from the blockchain at "reference_index" to mix with
    //populateFromBlockchain creates a keymatrix with "mixin" + 1 columns and one of the columns is inPk
    //   the return value are the key matrix, and the index where inPk was put (random).     
    tuple<ctkeyM, xmr_amount> populateFromBlockchain(ctkeyV inPk, int mixin) {
        int rows = inPk.size();
        ctkeyM rv(mixin + 1, inPk);
        int index = randXmrAmount(mixin);
        int i = 0, j = 0;
        for (i = 0; i <= mixin; i++) {
            if (i != index) {
                for (j = 0; j < rows; j++) {
                    getKeyFromBlockchain(rv[i][j], (size_t)randXmrAmount);
                }
            }
        }
        return make_tuple(rv, index);
    }

    //These functions get keys from blockchain
    //replace these when connecting blockchain
    //getKeyFromBlockchain grabs a key from the blockchain at "reference_index" to mix with
    //populateFromBlockchain creates a keymatrix with "mixin" columns and one of the columns is inPk
    //   the return value are the key matrix, and the index where inPk was put (random).     
    xmr_amount populateFromBlockchainSimple(ctkeyV & mixRing, const ctkey & inPk, int mixin) {
        int index = randXmrAmount(mixin);
        int i = 0;
        for (i = 0; i <= mixin; i++) {
            if (i != index) {
                getKeyFromBlockchain(mixRing[i], (size_t)randXmrAmount(1000));
            } else {
                mixRing[i] = inPk;
            }
        }
        return index;
    }

    //RingCT protocol
    //genRct: 
    //   creates an rctSig with all data necessary to verify the rangeProofs and that the signer owns one of the
    //   columns that are claimed as inputs, and that the sum of inputs  = sum of outputs.
    //   Also contains masked "amount" and "mask" so the receiver can see how much they received
    //verRct:
    //   verifies that all signatures (rangeProogs, MG sig, sum inputs = outputs) are correct
    //decodeRct: (c.f. https://eprint.iacr.org/2015/1098 section 5.1.1)
    //   uses the attached ecdh info to find the amounts represented by each output commitment 
    //   must know the destination private key to find the correct amount, else will return a random number
    //   Note: For txn fees, the last index in the amounts vector should contain that
    //   Thus the amounts vector will be "one" longer than the destinations vectort
    rctSig genRct(const key &message, const ctkeyV & inSk, const keyV & destinations, const vector<xmr_amount> & amounts, const ctkeyM &mixRing, const keyV &amount_keys, const multisig_kLRki *kLRki, multisig_out *msout, unsigned int index, ctkeyV &outSk, hw::device &hwdev) {
        CHECK_AND_ASSERT_THROW_MES(amounts.size() == destinations.size() || amounts.size() == destinations.size() + 1, "Different number of amounts/destinations");
        CHECK_AND_ASSERT_THROW_MES(amount_keys.size() == destinations.size(), "Different number of amount_keys/destinations");
        CHECK_AND_ASSERT_THROW_MES(index < mixRing.size(), "Bad index into mixRing");
        for (size_t n = 0; n < mixRing.size(); ++n) {
          CHECK_AND_ASSERT_THROW_MES(mixRing[n].size() == inSk.size(), "Bad mixRing size");
        }
        CHECK_AND_ASSERT_THROW_MES((kLRki && msout) || (!kLRki && !msout), "Only one of kLRki/msout is present");

        rctSig rv;
        rv.type = RCTTypeFull;
        rv.message = message;
        rv.outPk.resize(destinations.size());
        rv.p.rangeSigs.resize(destinations.size());
        rv.ecdhInfo.resize(destinations.size());

        size_t i = 0;
        keyV masks(destinations.size()); //sk mask..
        outSk.resize(destinations.size());
        for (i = 0; i < destinations.size(); i++) {
            //add destination to sig
            rv.outPk[i].dest = copy(destinations[i]);
            //compute range proof
            rv.p.rangeSigs[i] = proveRange(rv.outPk[i].mask, outSk[i].mask, amounts[i]);
            #ifdef DBG
            CHECK_AND_ASSERT_THROW_MES(verRange(rv.outPk[i].mask, rv.p.rangeSigs[i]), "verRange failed on newly created proof");
            #endif
            //mask amount and mask
            rv.ecdhInfo[i].mask = copy(outSk[i].mask);
            rv.ecdhInfo[i].amount = d2h(amounts[i]);
            hwdev.ecdhEncode(rv.ecdhInfo[i], amount_keys[i]);
        }

        //set txn fee
        if (amounts.size() > destinations.size())
        {
          rv.txnFee = amounts[destinations.size()];
        }
        else
        {
          rv.txnFee = 0;
        }
        key txnFeeKey = scalarmultH(d2h(rv.txnFee));

        rv.mixRing = mixRing;
        if (msout)
          msout->c.resize(1);
        rv.p.MGs.push_back(proveRctMG(get_pre_mlsag_hash(rv, hwdev), rv.mixRing, inSk, outSk, rv.outPk, kLRki, msout ? &msout->c[0] : NULL, index, txnFeeKey,hwdev));
        return rv;
    }

    rctSig genRct(const key &message, const ctkeyV & inSk, const ctkeyV  & inPk, const keyV & destinations, const vector<xmr_amount> & amounts, const keyV &amount_keys, const multisig_kLRki *kLRki, multisig_out *msout, const int mixin, hw::device &hwdev) {
        unsigned int index;
        ctkeyM mixRing;
        ctkeyV outSk;
        tie(mixRing, index) = populateFromBlockchain(inPk, mixin);
        return genRct(message, inSk, destinations, amounts, mixRing, amount_keys, kLRki, msout, index, outSk, hwdev);
    }
    
    //RCT simple    
    //for post-rct only
    rctSig genRctSimple(const key &message, const ctkeyV & inSk, const keyV & destinations, const vector<xmr_amount> &inamounts, const vector<xmr_amount> &outamounts, xmr_amount txnFee, const ctkeyM & mixRing, const keyV &amount_keys, const std::vector<multisig_kLRki> *kLRki, multisig_out *msout, const std::vector<unsigned int> & index, ctkeyV &outSk, RangeProofType range_proof_type, hw::device &hwdev) {
        const bool bulletproof = range_proof_type != RangeProofBorromean;
        CHECK_AND_ASSERT_THROW_MES(inamounts.size() > 0, "Empty inamounts");
        CHECK_AND_ASSERT_THROW_MES(inamounts.size() == inSk.size(), "Different number of inamounts/inSk");
        CHECK_AND_ASSERT_THROW_MES(outamounts.size() == destinations.size(), "Different number of amounts/destinations");
        CHECK_AND_ASSERT_THROW_MES(amount_keys.size() == destinations.size(), "Different number of amount_keys/destinations");
        CHECK_AND_ASSERT_THROW_MES(index.size() == inSk.size(), "Different number of index/inSk");
        CHECK_AND_ASSERT_THROW_MES(mixRing.size() == inSk.size(), "Different number of mixRing/inSk");
        for (size_t n = 0; n < mixRing.size(); ++n) {
          CHECK_AND_ASSERT_THROW_MES(index[n] < mixRing[n].size(), "Bad index into mixRing");
        }
        CHECK_AND_ASSERT_THROW_MES((kLRki && msout) || (!kLRki && !msout), "Only one of kLRki/msout is present");
        if (kLRki && msout) {
          CHECK_AND_ASSERT_THROW_MES(kLRki->size() == inamounts.size(), "Mismatched kLRki/inamounts sizes");
        }

        rctSig rv;
        rv.type = bulletproof ? RCTTypeBulletproof : RCTTypeSimple;
        rv.message = message;
        rv.outPk.resize(destinations.size());
        if (!bulletproof)
          rv.p.rangeSigs.resize(destinations.size());
        rv.ecdhInfo.resize(destinations.size());

        size_t i;
        keyV masks(destinations.size()); //sk mask..
        outSk.resize(destinations.size());
        for (i = 0; i < destinations.size(); i++) {

            //add destination to sig
            rv.outPk[i].dest = copy(destinations[i]);
            //compute range proof
            if (!bulletproof)
              rv.p.rangeSigs[i] = proveRange(rv.outPk[i].mask, outSk[i].mask, outamounts[i]);
            #ifdef DBG
            if (!bulletproof)
                CHECK_AND_ASSERT_THROW_MES(verRange(rv.outPk[i].mask, rv.p.rangeSigs[i]), "verRange failed on newly created proof");
            #endif
        }

        rv.p.bulletproofs.clear();
        if (bulletproof)
        {
            std::vector<uint64_t> proof_amounts;
            size_t n_amounts = outamounts.size();
            size_t amounts_proved = 0;
            if (range_proof_type == RangeProofPaddedBulletproof)
            {
                rct::keyV C, masks;
                if (hwdev.get_mode() == hw::device::TRANSACTION_CREATE_FAKE)
                {
                    // use a fake bulletproof for speed
                    rv.p.bulletproofs.push_back(make_dummy_bulletproof(outamounts.size()));
                    C = rct::keyV(outamounts.size(), I);
                    masks = rct::keyV(outamounts.size(), I);
                }
                else
                {
                    rv.p.bulletproofs.push_back(proveRangeBulletproof(C, masks, outamounts));
                    #ifdef DBG
                    CHECK_AND_ASSERT_THROW_MES(verBulletproof(rv.p.bulletproofs.back()), "verBulletproof failed on newly created proof");
                    #endif
                }
                for (i = 0; i < outamounts.size(); ++i)
                {
                    rv.outPk[i].mask = rct::scalarmult8(C[i]);
                    outSk[i].mask = masks[i];
                }
            }
            else while (amounts_proved < n_amounts)
            {
                size_t batch_size = 1;
                if (range_proof_type == RangeProofMultiOutputBulletproof)
                  while (batch_size * 2 + amounts_proved <= n_amounts && batch_size * 2 <= BULLETPROOF_MAX_OUTPUTS)
                    batch_size *= 2;
                rct::keyV C, masks;
                std::vector<uint64_t> batch_amounts(batch_size);
                for (i = 0; i < batch_size; ++i)
                  batch_amounts[i] = outamounts[i + amounts_proved];
                if (hwdev.get_mode() == hw::device::TRANSACTION_CREATE_FAKE)
                {
                    // use a fake bulletproof for speed
                    rv.p.bulletproofs.push_back(make_dummy_bulletproof(batch_amounts.size()));
                    C = rct::keyV(batch_amounts.size(), I);
                    masks = rct::keyV(batch_amounts.size(), I);
                }
                else
                {
                    rv.p.bulletproofs.push_back(proveRangeBulletproof(C, masks, batch_amounts));
                #ifdef DBG
                    CHECK_AND_ASSERT_THROW_MES(verBulletproof(rv.p.bulletproofs.back()), "verBulletproof failed on newly created proof");
                #endif
                }
                for (i = 0; i < batch_size; ++i)
                {
                  rv.outPk[i + amounts_proved].mask = rct::scalarmult8(C[i]);
                  outSk[i + amounts_proved].mask = masks[i];
                }
                amounts_proved += batch_size;
            }
        }

        key sumout = zero();
        for (i = 0; i < outSk.size(); ++i)
        {
            sc_add(sumout.bytes, outSk[i].mask.bytes, sumout.bytes);

            //mask amount and mask
            rv.ecdhInfo[i].mask = copy(outSk[i].mask);
            rv.ecdhInfo[i].amount = d2h(outamounts[i]);
            hwdev.ecdhEncode(rv.ecdhInfo[i], amount_keys[i]);
        }
            
        //set txn fee
        rv.txnFee = txnFee;
//        TODO: unused ??
//        key txnFeeKey = scalarmultH(d2h(rv.txnFee));
        rv.mixRing = mixRing;
        keyV &pseudoOuts = bulletproof ? rv.p.pseudoOuts : rv.pseudoOuts;
        pseudoOuts.resize(inamounts.size());
        rv.p.MGs.resize(inamounts.size());
        key sumpouts = zero(); //sum pseudoOut masks
        keyV a(inamounts.size());
        for (i = 0 ; i < inamounts.size() - 1; i++) {
            skGen(a[i]);
            sc_add(sumpouts.bytes, a[i].bytes, sumpouts.bytes);
            genC(pseudoOuts[i], a[i], inamounts[i]);
        }
        rv.mixRing = mixRing;
        sc_sub(a[i].bytes, sumout.bytes, sumpouts.bytes);
        genC(pseudoOuts[i], a[i], inamounts[i]);
        DP(pseudoOuts[i]);

        key full_message = get_pre_mlsag_hash(rv,hwdev);
        if (msout)
          msout->c.resize(inamounts.size());
        for (i = 0 ; i < inamounts.size(); i++) {
            rv.p.MGs[i] = proveRctMGSimple(full_message, rv.mixRing[i], inSk[i], a[i], pseudoOuts[i], kLRki ? &(*kLRki)[i]: NULL, msout ? &msout->c[i] : NULL, index[i], hwdev);
        }
        return rv;
    }

    rctSig genRctSimple(const key &message, const ctkeyV & inSk, const ctkeyV & inPk, const keyV & destinations, const vector<xmr_amount> &inamounts, const vector<xmr_amount> &outamounts, const keyV &amount_keys, const std::vector<multisig_kLRki> *kLRki, multisig_out *msout, xmr_amount txnFee, unsigned int mixin, hw::device &hwdev) {
        std::vector<unsigned int> index;
        index.resize(inPk.size());
        ctkeyM mixRing;
        ctkeyV outSk;
        mixRing.resize(inPk.size());
        for (size_t i = 0; i < inPk.size(); ++i) {
          mixRing[i].resize(mixin+1);
          index[i] = populateFromBlockchainSimple(mixRing[i], inPk[i], mixin);
        }
        return genRctSimple(message, inSk, destinations, inamounts, outamounts, txnFee, mixRing, amount_keys, kLRki, msout, index, outSk, RangeProofBorromean, hwdev);
    }

    //RingCT protocol
    //genRct: 
    //   creates an rctSig with all data necessary to verify the rangeProofs and that the signer owns one of the
    //   columns that are claimed as inputs, and that the sum of inputs  = sum of outputs.
    //   Also contains masked "amount" and "mask" so the receiver can see how much they received
    //verRct:
    //   verifies that all signatures (rangeProogs, MG sig, sum inputs = outputs) are correct
    //decodeRct: (c.f. https://eprint.iacr.org/2015/1098 section 5.1.1)
    //   uses the attached ecdh info to find the amounts represented by each output commitment 
    //   must know the destination private key to find the correct amount, else will return a random number    
    bool verRct(const rctSig & rv, bool semantics) {
        PERF_TIMER(verRct);
        CHECK_AND_ASSERT_MES(rv.type == RCTTypeFull, false, "verRct called on non-full rctSig");
        if (semantics)
        {
          CHECK_AND_ASSERT_MES(rv.outPk.size() == rv.p.rangeSigs.size(), false, "Mismatched sizes of outPk and rv.p.rangeSigs");
          CHECK_AND_ASSERT_MES(rv.outPk.size() == rv.ecdhInfo.size(), false, "Mismatched sizes of outPk and rv.ecdhInfo");
          CHECK_AND_ASSERT_MES(rv.p.MGs.size() == 1, false, "full rctSig has not one MG");
        }
        else
        {
          // semantics check is early, we don't have the MGs resolved yet
        }

        // some rct ops can throw
        try
        {
          if (semantics) {
            tools::threadpool& tpool = tools::threadpool::getInstance();
            tools::threadpool::waiter waiter;
            std::deque<bool> results(rv.outPk.size(), false);
            DP("range proofs verified?");
            for (size_t i = 0; i < rv.outPk.size(); i++)
              tpool.submit(&waiter, [&, i] { results[i] = verRange(rv.outPk[i].mask, rv.p.rangeSigs[i]); });
            waiter.wait(&tpool);

            for (size_t i = 0; i < results.size(); ++i) {
              if (!results[i]) {
                LOG_PRINT_L1("Range proof verified failed for proof " << i);
                return false;
              }
            }
          }

          if (!semantics) {
            //compute txn fee
            key txnFeeKey = scalarmultH(d2h(rv.txnFee));
            bool mgVerd = verRctMG(rv.p.MGs[0], rv.mixRing, rv.outPk, txnFeeKey, get_pre_mlsag_hash(rv, hw::get_device("default")));
            DP("mg sig verified?");
            DP(mgVerd);
            if (!mgVerd) {
              LOG_PRINT_L1("MG signature verification failed");
              return false;
            }
          }

          return true;
        }
        catch (const std::exception &e)
        {
          LOG_PRINT_L1("Error in verRct: " << e.what());
          return false;
        }
        catch (...)
        {
          LOG_PRINT_L1("Error in verRct, but not an actual exception");
          return false;
        }
    }

    //ver RingCT simple
    //assumes only post-rct style inputs (at least for max anonymity)
    bool verRctSemanticsSimple(const std::vector<const rctSig*> & rvv) {
      try
      {
        PERF_TIMER(verRctSemanticsSimple);

        tools::threadpool& tpool = tools::threadpool::getInstance();
        tools::threadpool::waiter waiter;
        std::deque<bool> results;
        std::vector<const Bulletproof*> proofs;
        size_t max_non_bp_proofs = 0, offset = 0;

        for (const rctSig *rvp: rvv)
        {
          CHECK_AND_ASSERT_MES(rvp, false, "rctSig pointer is NULL");
          const rctSig &rv = *rvp;
          CHECK_AND_ASSERT_MES(rv.type == RCTTypeSimple || rv.type == RCTTypeBulletproof, false, "verRctSemanticsSimple called on non simple rctSig");
          const bool bulletproof = is_rct_bulletproof(rv.type);
          if (bulletproof)
          {
            CHECK_AND_ASSERT_MES(rv.outPk.size() == n_bulletproof_amounts(rv.p.bulletproofs), false, "Mismatched sizes of outPk and bulletproofs");
            CHECK_AND_ASSERT_MES(rv.p.pseudoOuts.size() == rv.p.MGs.size(), false, "Mismatched sizes of rv.p.pseudoOuts and rv.p.MGs");
            CHECK_AND_ASSERT_MES(rv.pseudoOuts.empty(), false, "rv.pseudoOuts is not empty");
          }
          else
          {
            CHECK_AND_ASSERT_MES(rv.outPk.size() == rv.p.rangeSigs.size(), false, "Mismatched sizes of outPk and rv.p.rangeSigs");
            CHECK_AND_ASSERT_MES(rv.pseudoOuts.size() == rv.p.MGs.size(), false, "Mismatched sizes of rv.pseudoOuts and rv.p.MGs");
            CHECK_AND_ASSERT_MES(rv.p.pseudoOuts.empty(), false, "rv.p.pseudoOuts is not empty");
          }
          CHECK_AND_ASSERT_MES(rv.outPk.size() == rv.ecdhInfo.size(), false, "Mismatched sizes of outPk and rv.ecdhInfo");

          if (!bulletproof)
            max_non_bp_proofs += rv.p.rangeSigs.size();
        }

        results.resize(max_non_bp_proofs);
        for (const rctSig *rvp: rvv)
        {
          const rctSig &rv = *rvp;

          const bool bulletproof = is_rct_bulletproof(rv.type);
          const keyV &pseudoOuts = bulletproof ? rv.p.pseudoOuts : rv.pseudoOuts;

          rct::keyV masks(rv.outPk.size());
          for (size_t i = 0; i < rv.outPk.size(); i++) {
            masks[i] = rv.outPk[i].mask;
          }
          key sumOutpks = addKeys(masks);
          DP(sumOutpks);
          const key txnFeeKey = scalarmultH(d2h(rv.txnFee));
          addKeys(sumOutpks, txnFeeKey, sumOutpks);

          key sumPseudoOuts = addKeys(pseudoOuts);
          DP(sumPseudoOuts);

          //check pseudoOuts vs Outs..
          if (!equalKeys(sumPseudoOuts, sumOutpks)) {
            LOG_PRINT_L1("Sum check failed");
            return false;
          }

          if (bulletproof)
          {
            for (size_t i = 0; i < rv.p.bulletproofs.size(); i++)
              proofs.push_back(&rv.p.bulletproofs[i]);
          }
          else
          {
            for (size_t i = 0; i < rv.p.rangeSigs.size(); i++)
              tpool.submit(&waiter, [&, i, offset] { results[i+offset] = verRange(rv.outPk[i].mask, rv.p.rangeSigs[i]); });
            offset += rv.p.rangeSigs.size();
          }
        }
        if (!proofs.empty() && !verBulletproof(proofs))
        {
          LOG_PRINT_L1("Aggregate range proof verified failed");
          return false;
        }

        waiter.wait(&tpool);
        for (size_t i = 0; i < results.size(); ++i) {
          if (!results[i]) {
            LOG_PRINT_L1("Range proof verified failed for proof " << i);
            return false;
          }
        }

        return true;
      }
      // we can get deep throws from ge_frombytes_vartime if input isn't valid
      catch (const std::exception &e)
      {
        LOG_PRINT_L1("Error in verRctSemanticsSimple: " << e.what());
        return false;
      }
      catch (...)
      {
        LOG_PRINT_L1("Error in verRctSemanticsSimple, but not an actual exception");
        return false;
      }
    }

    bool verRctSemanticsSimple(const rctSig & rv)
    {
      return verRctSemanticsSimple(std::vector<const rctSig*>(1, &rv));
    }

    //ver RingCT simple
    //assumes only post-rct style inputs (at least for max anonymity)
    bool verRctNonSemanticsSimple(const rctSig & rv) {
      try
      {
        PERF_TIMER(verRctNonSemanticsSimple);

        CHECK_AND_ASSERT_MES(rv.type == RCTTypeSimple || rv.type == RCTTypeBulletproof, false, "verRctNonSemanticsSimple called on non simple rctSig");
        const bool bulletproof = is_rct_bulletproof(rv.type);
        // semantics check is early, and mixRing/MGs aren't resolved yet
        if (bulletproof)
          CHECK_AND_ASSERT_MES(rv.p.pseudoOuts.size() == rv.mixRing.size(), false, "Mismatched sizes of rv.p.pseudoOuts and mixRing");
        else
          CHECK_AND_ASSERT_MES(rv.pseudoOuts.size() == rv.mixRing.size(), false, "Mismatched sizes of rv.pseudoOuts and mixRing");

        const size_t threads = std::max(rv.outPk.size(), rv.mixRing.size());

        std::deque<bool> results(threads);
        tools::threadpool& tpool = tools::threadpool::getInstance();
        tools::threadpool::waiter waiter;

        const keyV &pseudoOuts = bulletproof ? rv.p.pseudoOuts : rv.pseudoOuts;

        const key message = get_pre_mlsag_hash(rv, hw::get_device("default"));

        results.clear();
        results.resize(rv.mixRing.size());
        for (size_t i = 0 ; i < rv.mixRing.size() ; i++) {
          tpool.submit(&waiter, [&, i] {
              results[i] = verRctMGSimple(message, rv.p.MGs[i], rv.mixRing[i], pseudoOuts[i]);
          });
        }
        waiter.wait(&tpool);

        for (size_t i = 0; i < results.size(); ++i) {
          if (!results[i]) {
            LOG_PRINT_L1("verRctMGSimple failed for input " << i);
            return false;
          }
        }

        return true;
      }
      // we can get deep throws from ge_frombytes_vartime if input isn't valid
      catch (const std::exception &e)
      {
        LOG_PRINT_L1("Error in verRctNonSemanticsSimple: " << e.what());
        return false;
      }
      catch (...)
      {
        LOG_PRINT_L1("Error in verRctNonSemanticsSimple, but not an actual exception");
        return false;
      }
    }

    //RingCT protocol
    //genRct: 
    //   creates an rctSig with all data necessary to verify the rangeProofs and that the signer owns one of the
    //   columns that are claimed as inputs, and that the sum of inputs  = sum of outputs.
    //   Also contains masked "amount" and "mask" so the receiver can see how much they received
    //verRct:
    //   verifies that all signatures (rangeProogs, MG sig, sum inputs = outputs) are correct
    //decodeRct: (c.f. https://eprint.iacr.org/2015/1098 section 5.1.1)
    //   uses the attached ecdh info to find the amounts represented by each output commitment 
    //   must know the destination private key to find the correct amount, else will return a random number    
    xmr_amount decodeRct(const rctSig & rv, const key & sk, unsigned int i, key & mask, hw::device &hwdev) {
        CHECK_AND_ASSERT_MES(rv.type == RCTTypeFull, false, "decodeRct called on non-full rctSig");
        CHECK_AND_ASSERT_THROW_MES(i < rv.ecdhInfo.size(), "Bad index");
        CHECK_AND_ASSERT_THROW_MES(rv.outPk.size() == rv.ecdhInfo.size(), "Mismatched sizes of rv.outPk and rv.ecdhInfo");

        //mask amount and mask
        ecdhTuple ecdh_info = rv.ecdhInfo[i];
        hwdev.ecdhDecode(ecdh_info, sk);
        mask = ecdh_info.mask;
        key amount = ecdh_info.amount;
        key C = rv.outPk[i].mask;
        DP("C");
        DP(C);
        key Ctmp;
        CHECK_AND_ASSERT_THROW_MES(sc_check(mask.bytes) == 0, "warning, bad ECDH mask");
        CHECK_AND_ASSERT_THROW_MES(sc_check(amount.bytes) == 0, "warning, bad ECDH amount");
        addKeys2(Ctmp, mask, amount, H);
        DP("Ctmp");
        DP(Ctmp);
        if (equalKeys(C, Ctmp) == false) {
            CHECK_AND_ASSERT_THROW_MES(false, "warning, amount decoded incorrectly, will be unable to spend");
        }
        return h2d(amount);
    }

    xmr_amount decodeRct(const rctSig & rv, const key & sk, unsigned int i, hw::device &hwdev) {
      key mask;
      return decodeRct(rv, sk, i, mask, hwdev);
    }

    xmr_amount decodeRctSimple(const rctSig & rv, const key & sk, unsigned int i, key &mask, hw::device &hwdev) {
        CHECK_AND_ASSERT_MES(rv.type == RCTTypeSimple || rv.type == RCTTypeBulletproof, false, "decodeRct called on non simple rctSig");
        CHECK_AND_ASSERT_THROW_MES(i < rv.ecdhInfo.size(), "Bad index");
        CHECK_AND_ASSERT_THROW_MES(rv.outPk.size() == rv.ecdhInfo.size(), "Mismatched sizes of rv.outPk and rv.ecdhInfo");

        //mask amount and mask
        ecdhTuple ecdh_info = rv.ecdhInfo[i];
        hwdev.ecdhDecode(ecdh_info, sk);
        mask = ecdh_info.mask;
        key amount = ecdh_info.amount;
        key C = rv.outPk[i].mask;
        DP("C");
        DP(C);
        key Ctmp;
        CHECK_AND_ASSERT_THROW_MES(sc_check(mask.bytes) == 0, "warning, bad ECDH mask");
        CHECK_AND_ASSERT_THROW_MES(sc_check(amount.bytes) == 0, "warning, bad ECDH amount");
        addKeys2(Ctmp, mask, amount, H);
        DP("Ctmp");
        DP(Ctmp);
        if (equalKeys(C, Ctmp) == false) {
            CHECK_AND_ASSERT_THROW_MES(false, "warning, amount decoded incorrectly, will be unable to spend");
        }
        return h2d(amount);
    }

    xmr_amount decodeRctSimple(const rctSig & rv, const key & sk, unsigned int i, hw::device &hwdev) {
      key mask;
      return decodeRctSimple(rv, sk, i, mask, hwdev);
    }

    bool signMultisig(rctSig &rv, const std::vector<unsigned int> &indices, const keyV &k, const multisig_out &msout, const key &secret_key) {
        CHECK_AND_ASSERT_MES(rv.type == RCTTypeFull || rv.type == RCTTypeSimple || rv.type == RCTTypeBulletproof,
            false, "unsupported rct type");
        CHECK_AND_ASSERT_MES(indices.size() == k.size(), false, "Mismatched k/indices sizes");
        CHECK_AND_ASSERT_MES(k.size() == rv.p.MGs.size(), false, "Mismatched k/MGs size");
        CHECK_AND_ASSERT_MES(k.size() == msout.c.size(), false, "Mismatched k/msout.c size");
        if (rv.type == RCTTypeFull)
        {
          CHECK_AND_ASSERT_MES(rv.p.MGs.size() == 1, false, "MGs not a single element");
        }
        for (size_t n = 0; n < indices.size(); ++n) {
            CHECK_AND_ASSERT_MES(indices[n] < rv.p.MGs[n].ss.size(), false, "Index out of range");
            CHECK_AND_ASSERT_MES(!rv.p.MGs[n].ss[indices[n]].empty(), false, "empty ss line");
        }

        for (size_t n = 0; n < indices.size(); ++n) {
            rct::key diff;
            sc_mulsub(diff.bytes, msout.c[n].bytes, secret_key.bytes, k[n].bytes);
            sc_add(rv.p.MGs[n].ss[indices[n]][0].bytes, rv.p.MGs[n].ss[indices[n]][0].bytes, diff.bytes);
        }
        return true;
    }
}
