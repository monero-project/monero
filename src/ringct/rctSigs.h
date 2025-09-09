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

//#define DBG

#ifndef RCTSIGS_H
#define RCTSIGS_H

#include <cstddef>
#include <vector>
#include <tuple>

#include "crypto/generic-ops.h"

extern "C" {
#include "crypto/random.h"
#include "crypto/keccak.h"
}
#include "crypto/crypto.h"

#include "fcmp_pp/curve_trees.h"
#include "rctTypes.h"
#include "rctOps.h"

//Define this flag when debugging to get additional info on the console
#ifdef DBG
#define DP(x) dp(x)
#else
#define DP(x)
#endif

namespace hw {
    class device;
}


namespace rct {
    // helpers for mock txs
    Bulletproof make_dummy_bulletproof(const std::vector<uint64_t> &outamounts, keyV &C, keyV &masks);
    BulletproofPlus make_dummy_bulletproof_plus(const std::vector<uint64_t> &outamounts, keyV &C, keyV &masks);
    clsag make_dummy_clsag(size_t ring_size);

    boroSig genBorromean(const key64 x, const key64 P1, const key64 P2, const bits indices);
    bool verifyBorromean(const boroSig &bb, const key64 P1, const key64 P2);

    //Multilayered Spontaneous Anonymous Group Signatures (MLSAG signatures)
    //These are aka MG signatutes in earlier drafts of the ring ct paper
    // c.f. https://eprint.iacr.org/2015/1098 section 2.
    // Gen creates a signature which proves that for some column in the keymatrix "pk"
    //   the signer knows a secret key for each row in that column
    // Ver verifies that the MG sig was created correctly
    mgSig MLSAG_Gen(const key &message, const keyM & pk, const keyV & xx, const unsigned int index, size_t dsRows, hw::device &hwdev);
    bool MLSAG_Ver(const key &message, const keyM &pk, const mgSig &sig, size_t dsRows);

    clsag CLSAG_Gen(const key &message, const keyV & P, const key & p, const keyV & C, const key & z, const keyV & C_nonzero, const key & C_offset, const unsigned int l, hw::device &hwdev);
    clsag CLSAG_Gen(const key &message, const keyV & P, const key & p, const keyV & C, const key & z, const keyV & C_nonzero, const key & C_offset, const unsigned int l);
    clsag proveRctCLSAGSimple(const key &, const ctkeyV &, const ctkey &, const key &, const key &, unsigned int, hw::device &);
    bool verRctCLSAGSimple(const key &, const clsag &, const ctkeyV &, const key &);

    //proveRange and verRange
    //proveRange gives C, and mask such that \sumCi = C
    //   c.f. https://eprint.iacr.org/2015/1098 section 5.1
    //   and Ci is a commitment to either 0 or 2^i, i=0,...,63
    //   thus this proves that "amount" is in [0, 2^64]
    //   mask is a such that C = aG + bH, and b = amount
    //verRange verifies that \sum Ci = C and that each Ci is a commitment to 0 or 2^i
    rangeSig proveRange(key & C, key & mask, const xmr_amount & amount);
    bool verRange(const key & C, const rangeSig & as);

    //Ring-ct MG sigs
    //Prove:
    //   c.f. https://eprint.iacr.org/2015/1098 section 4. definition 10.
    //   This does the MG sig on the "dest" part of the given key matrix, and
    //   the last row is the sum of input commitments from that column - sum output commitments
    //   this shows that sum inputs = sum outputs
    //Ver:
    //   verifies the above sig is created corretly
    mgSig proveRctMG(const ctkeyM & pubs, const ctkeyV & inSk, const keyV &outMasks, const ctkeyV & outPk, unsigned int index, const key &txnFee, const key &message, hw::device &hwdev);
    mgSig proveRctMGSimple(const key & message, const ctkeyV & pubs, const ctkey & inSk, const key &a , const key &Cout, unsigned int index, hw::device &hwdev);
    bool verRctMG(const mgSig &mg, const ctkeyM & pubs, const ctkeyV & outPk, const key &txnFee, const key &message);
    bool verRctMGSimple(const key &message, const mgSig &mg, const ctkeyV & pubs, const key & C);

    //These functions get keys from blockchain
    //replace these when connecting blockchain
    //getKeyFromBlockchain grabs a key from the blockchain at "reference_index" to mix with
    //populateFromBlockchain creates a keymatrix with "mixin" columns and one of the columns is inPk
    //   the return value are the key matrix, and the index where inPk was put (random).
    void getKeyFromBlockchain(ctkey & a, size_t reference_index);
    std::tuple<ctkeyM, xmr_amount> populateFromBlockchain(ctkeyV inPk, int mixin);

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
    rctSig genRct(const key &message, const ctkeyV & inSk, const keyV & destinations, const std::vector<xmr_amount> & amounts, const ctkeyM &mixRing, const keyV &amount_keys, unsigned int index, ctkeyV &outSk, const RCTConfig &rct_config, hw::device &hwdev);
    rctSig genRct(const key &message, const ctkeyV & inSk, const ctkeyV  & inPk, const keyV & destinations, const std::vector<xmr_amount> & amounts, const keyV &amount_keys, const int mixin, const RCTConfig &rct_config, hw::device &hwdev);
    rctSig genRctSimple(const key & message, const ctkeyV & inSk, const ctkeyV & inPk, const keyV & destinations, const std::vector<xmr_amount> & inamounts, const std::vector<xmr_amount> & outamounts, const keyV &amount_keys, xmr_amount txnFee, unsigned int mixin, const RCTConfig &rct_config, hw::device &hwdev);
    rctSig genRctSimple(const key & message, const ctkeyV & inSk, const keyV & destinations, const std::vector<xmr_amount> & inamounts, const std::vector<xmr_amount> & outamounts, xmr_amount txnFee, const ctkeyM & mixRing, const keyV &amount_keys, const std::vector<unsigned int> & index, ctkeyV &outSk, const RCTConfig &rct_config, hw::device &hwdev);
    bool verRct(const rctSig & rv, bool semantics);
    static inline bool verRct(const rctSig & rv) { return verRct(rv, true) && verRct(rv, false); }
    bool verRctSemanticsSimple(const rctSig & rv);
    bool verRctSemanticsSimple(const std::vector<const rctSig*> & rv);
    bool verRctNonSemanticsSimple(const rctSig & rv);
    static inline bool verRctSimple(const rctSig & rv) { return verRctSemanticsSimple(rv) && verRctNonSemanticsSimple(rv); }
    xmr_amount decodeRct(const rctSig & rv, const key & sk, unsigned int i, key & mask, hw::device &hwdev);
    xmr_amount decodeRct(const rctSig & rv, const key & sk, unsigned int i, hw::device &hwdev);
    xmr_amount decodeRctSimple(const rctSig & rv, const key & sk, unsigned int i, key & mask, hw::device &hwdev);
    xmr_amount decodeRctSimple(const rctSig & rv, const key & sk, unsigned int i, hw::device &hwdev);
    key get_pre_mlsag_hash(const rctSig &rv, hw::device &hwdev);

    // Make sure points are valid points, don't have torsion, and are not equal to identity
    bool verPointsForTorsion(const std::vector<key> & pts);
}
#endif  /* RCTSIGS_H */

