// Copyright (c) 2017, The Monero Project
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
// Adapted from Java code by Sarang Noether

#pragma once

#ifndef BULLETPROOFS_H
#define BULLETPROOFS_H

#include "serialization/serialization.h"
#include "ringct/rctOps.h"

namespace rct
{

struct Bulletproof
{
  rct::key V, A, S, T1, T2;
  rct::key taux, mu;
  rct::keyV L, R;
  rct::key a, b, t;

  Bulletproof() {}
  Bulletproof(const rct::key &V, const rct::key &A, const rct::key &S, const rct::key &T1, const rct::key &T2, const rct::key &taux, const rct::key &mu, const rct::keyV &L, const rct::keyV &R, const rct::key &a, const rct::key &b, const rct::key &t):
    V(V), A(A), S(S), T1(T1), T2(T2), taux(taux), mu(mu), L(L), R(R), a(a), b(b), t(t) {}

  BEGIN_SERIALIZE_OBJECT()
    FIELD(V)
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

Bulletproof bulletproof_PROVE(const rct::key &v, const rct::key &gamma);
Bulletproof bulletproof_PROVE(uint64_t v, const rct::key &gamma);
bool bulletproof_VERIFY(const Bulletproof &proof);

}

#endif
