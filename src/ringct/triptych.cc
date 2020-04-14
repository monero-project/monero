// Copyright (c) 2014-2020, The Monero Project
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

#include <stdlib.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "rctOps.h"
#include "rctTypes.h"
#include "multiexp.h"
#include "triptych.h"
#include "cryptonote_config.h"
#include "misc_log_ex.h"

namespace rct
{
    // Maximum size parameter
    static const size_t max_mn = 128;

    // Global data
    static ge_p3 Hi_p3[max_mn];
    static ge_p3 H_p3;
    static rct::key U;
    static ge_p3 U_p3;
    static boost::mutex init_mutex;

    // Useful scalar constants
    static const rct::key ZERO = rct::zero();
    static const rct::key ONE = rct::identity();
    static const rct::key TWO = { {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };
    static const rct::key MINUS_ONE = { {0xec, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10} };

    // Initialize transcript
    static void transcript_init(rct::key &transcript)
    {
        std::string salt(config::HASH_KEY_TRIPTYCH_TRANSCRIPT);
        rct::hash_to_scalar(transcript,salt.data(),salt.size());
    }

    // Update transcript: transcript, message, M, P, J, K, A, B, C, D
    static void transcript_update_1(rct::key &transcript, const rct::key &message, const rct::keyV &M, const rct::keyV &P, const rct::key &J, const rct::key &K, const rct::key &A, const rct::key &B, const rct::key &C, const rct::key &D)
    {
        std::string hash;
        hash.reserve((2*M.size() + 8)*sizeof(key));
        hash = std::string((const char*) transcript.bytes, sizeof(transcript));
        hash += std::string((const char*) message.bytes, sizeof(message));
        for (size_t k = 0; k < M.size(); k++)
        {
            hash += std::string((const char*) M[k].bytes, sizeof(M[k]));
            hash += std::string((const char*) P[k].bytes, sizeof(P[k]));
        }
        hash += std::string((const char*) J.bytes, sizeof(J));
        hash += std::string((const char*) K.bytes, sizeof(K));
        hash += std::string((const char*) A.bytes, sizeof(A));
        hash += std::string((const char*) B.bytes, sizeof(B));
        hash += std::string((const char*) C.bytes, sizeof(C));
        hash += std::string((const char*) D.bytes, sizeof(D));
        rct::hash_to_scalar(transcript,hash.data(),hash.size());

        CHECK_AND_ASSERT_THROW_MES(!(transcript == ZERO), "Transcript challenge must be nonzero!");
    }

    // Update transcript: transcript, X, Y
    static void transcript_update_2(rct::key &transcript, const rct::keyV &X, const rct::keyV &Y)
    {
        std::string hash;
        hash.reserve((2*X.size() + 1)*sizeof(key));
        hash = std::string((const char*) transcript.bytes, sizeof(transcript));
        for (size_t j = 0; j < X.size(); j++)
        {
            hash += std::string((const char*) X[j].bytes, sizeof(X[j]));
            hash += std::string((const char*) Y[j].bytes, sizeof(Y[j]));
        }
        rct::hash_to_scalar(transcript,hash.data(),hash.size());

        CHECK_AND_ASSERT_THROW_MES(!(transcript == ZERO), "Transcript challenge must be nonzero!");
    }
    
    // Helper function for scalar inversion
    static rct::key sm(rct::key y, int n, const rct::key &x)
    {
        while (n--)
            sc_mul(y.bytes, y.bytes, y.bytes);
        sc_mul(y.bytes, y.bytes, x.bytes);
        return y;
    }

    // Invert a nonzero scalar
    // NOTE: scalar must be nonzero!
    static rct::key invert(const rct::key &x)
    {
        CHECK_AND_ASSERT_THROW_MES(!(x == ZERO), "Cannot invert zero!");

        rct::key _1, _10, _100, _11, _101, _111, _1001, _1011, _1111;

        _1 = x;
        sc_mul(_10.bytes, _1.bytes, _1.bytes);
        sc_mul(_100.bytes, _10.bytes, _10.bytes);
        sc_mul(_11.bytes, _10.bytes, _1.bytes);
        sc_mul(_101.bytes, _10.bytes, _11.bytes);
        sc_mul(_111.bytes, _10.bytes, _101.bytes);
        sc_mul(_1001.bytes, _10.bytes, _111.bytes);
        sc_mul(_1011.bytes, _10.bytes, _1001.bytes);
        sc_mul(_1111.bytes, _100.bytes, _1011.bytes);

        rct::key inv;
        sc_mul(inv.bytes, _1111.bytes, _1.bytes);

        inv = sm(inv, 123 + 3, _101);
        inv = sm(inv, 2 + 2, _11);
        inv = sm(inv, 1 + 4, _1111);
        inv = sm(inv, 1 + 4, _1111);
        inv = sm(inv, 4, _1001);
        inv = sm(inv, 2, _11);
        inv = sm(inv, 1 + 4, _1111);
        inv = sm(inv, 1 + 3, _101);
        inv = sm(inv, 3 + 3, _101);
        inv = sm(inv, 3, _111);
        inv = sm(inv, 1 + 4, _1111);
        inv = sm(inv, 2 + 3, _111);
        inv = sm(inv, 2 + 2, _11);
        inv = sm(inv, 1 + 4, _1011);
        inv = sm(inv, 2 + 4, _1011);
        inv = sm(inv, 6 + 4, _1001);
        inv = sm(inv, 2 + 2, _11);
        inv = sm(inv, 3 + 2, _11);
        inv = sm(inv, 3 + 2, _11);
        inv = sm(inv, 1 + 4, _1001);
        inv = sm(inv, 1 + 3, _111);
        inv = sm(inv, 2 + 4, _1111);
        inv = sm(inv, 1 + 4, _1011);
        inv = sm(inv, 3, _101);
        inv = sm(inv, 2 + 4, _1111);
        inv = sm(inv, 3, _101);
        inv = sm(inv, 1 + 2, _11);

        return inv;
    }

    // Make generators, but only once
    static void init_gens()
    {
        boost::lock_guard<boost::mutex> lock(init_mutex);
        static const std::string H_salt(config::HASH_KEY_TRIPTYCH_H);

        static bool init_done = false;
        if (init_done) return;

        // Build Hi generators
        for (size_t i = 0; i < max_mn; i++)
        {
            std::string hash = H_salt + tools::get_varint_data(i);
            rct::hash_to_p3(Hi_p3[i], rct::hash2rct(crypto::cn_fast_hash(hash.data(),hash.size())));
        }

        // Build U
        // U = keccak("triptych U")
        static const std::string U_salt(config::HASH_KEY_TRIPTYCH_U);
        rct::hash_to_p3(U_p3, rct::hash2rct(crypto::cn_fast_hash(U_salt.data(),U_salt.size())));
        ge_p3_tobytes(U.bytes, &U_p3);

        // Build H
        ge_frombytes_vartime(&H_p3, rct::H.bytes);

        init_done = true;
    }

    // Decompose an integer with a fixed base and size
    static void decompose(std::vector<size_t> &r, const size_t val, const size_t base, const size_t size)
    {
        size_t temp = val;

        for (size_t i = 0; i < size; i++)
        {
            size_t slot = pow(base,size-i-1);
            r[size-i-1] = temp/slot;
            temp -= slot*r[size-i-1];
        }
    }

    // Commit to a scalar matrix
    static rct::key com_matrix(std::vector<MultiexpData> &data, const rct::keyM &M, const rct::key &r)
    {
        const size_t m = M.size();
        const size_t n = M[0].size();

        for (size_t j = 0; j < m; j++)
        {
            for (size_t i = 0; i < n; i++)
            {
                data[j*n + i] = {M[j][i], Hi_p3[j*n + i]};
            }
        }
        data[m*n] = {r, H_p3}; // mask

        return straus(data);
    }

    // Kronecker delta
    static rct::key delta(const size_t x, const size_t y)
    {
        if (x == y)
            return ONE;
        else
            return ZERO;
    }

    // Compute a convolution with a degree-one polynomial
    static rct::keyV convolve(const keyV &x, const keyV &y, const size_t m)
    {
        rct::keyV r;
        r.reserve(m+1);

        for (size_t i = 0; i < m+1; i++)
        {
            r[i] = ZERO;
        }

        for (size_t i = 0; i < m; i++)
        {
            for (size_t j = 0; j < 2; j++)
            {
                rct::key temp;
                sc_mul(temp.bytes,x[i].bytes,y[j].bytes);
                sc_add(r[i+j].bytes,r[i+j].bytes,temp.bytes);
            }
        }

        return r;
    }

    // Generate a Triptych proof
    TriptychProof triptych_prove(const rct::keyV &M, const rct::keyV &P, const size_t l, const key &r, const key &s, const size_t n, const size_t m, const rct::key &message)
    {
        CHECK_AND_ASSERT_THROW_MES(n > 1, "Must have n > 1!");
        CHECK_AND_ASSERT_THROW_MES(m > 1, "Must have m > 1!");
        CHECK_AND_ASSERT_THROW_MES(m*n <= max_mn, "Size parameters are too large!");
        CHECK_AND_ASSERT_THROW_MES(M.size() == pow(n,m), "Public key vector is wrong size!");
        CHECK_AND_ASSERT_THROW_MES(P.size() == pow(n,m), "Commitment vector is wrong size!");
        CHECK_AND_ASSERT_THROW_MES(l < M.size(), "Signing index out of bounds!");
        CHECK_AND_ASSERT_THROW_MES(rct::scalarmultBase(r) == M[l], "Bad signing key!");
        CHECK_AND_ASSERT_THROW_MES(rct::scalarmultBase(s) == P[l], "Bad commitment key!");

        init_gens();
        const size_t N = pow(n,m);

        TriptychProof proof;
        std::vector<MultiexpData> data;
        data.reserve(m*n + 1);

        // Begin transcript
        rct::key tr;
        transcript_init(tr);

        // Compute key images
        // J = (1/r)*U
        // K = s*J
        proof.J = rct::scalarmultKey(U,invert(r));
        proof.K = rct::scalarmultKey(proof.J,s);

        // Matrix masks
        rct::key rA = rct::skGen();
        rct::key rB = rct::skGen();
        rct::key rC = rct::skGen();
        rct::key rD = rct::skGen();

        // Commit to zero-sum values
        rct::keyM a = rct::keyMInit(n,m);
        for (size_t j = 0; j < m; j++)
        {
            a[j][0] = ZERO;
            for (size_t i = 1; i < n; i++)
            {
                a[j][i] = rct::skGen();
                sc_sub(a[j][0].bytes,a[j][0].bytes,a[j][i].bytes);
            }
        }
        com_matrix(data,a,rA);
        proof.A = straus(data);

        // Commit to decomposition bits
        std::vector<size_t> decomp_l;
        decomp_l.reserve(m);
        decompose(decomp_l,l,n,m);

        rct::keyM sigma = rct::keyMInit(n,m);
        for (size_t j = 0; j < m; j++)
        {
            for (size_t i = 0; i < n; i++)
            {
                sigma[j][i] = delta(decomp_l[j],i);
            }
        }
        com_matrix(data,sigma,rB);
        proof.B = straus(data);

        // Commit to a/sigma relationships
        rct::keyM a_sigma = rct::keyMInit(n,m);
        for (size_t j = 0; j < m; j++)
        {
            for (size_t i = 0; i < n; i++)
            {
                // a_sigma[j][i] = a[j][i]*(ONE - TWO*sigma[j][i])
                sc_mulsub(a_sigma[j][i].bytes, TWO.bytes, sigma[j][i].bytes, ONE.bytes);
                sc_mul(a_sigma[j][i].bytes, a_sigma[j][i].bytes, a[j][i].bytes);
            }
        }
        com_matrix(data,a_sigma,rC);
        proof.C = straus(data);

        // Commit to squared a-values
        rct::keyM a_sq = rct::keyMInit(n,m);
        for (size_t j = 0; j < m; j++)
        {
            for (size_t i = 0; i < n; i++)
            {
                sc_mul(a_sq[j][i].bytes,a[j][i].bytes,a[j][i].bytes);
                sc_mul(a_sq[j][i].bytes,MINUS_ONE.bytes,a_sq[j][i].bytes);
            }
        }
        com_matrix(data,a_sq,rD);
        proof.D = straus(data);

        // Compute p coefficients
        rct::keyM p = rct::keyMInit(m+1,N);
        for (size_t k = 0; k < N; k++)
        {
            std::vector<size_t> decomp_k;
            decomp_k.reserve(m);
            decompose(decomp_k,k,n,m);

            for (size_t j = 0; j < m+1; j++)
            {
                p[k][j] = ZERO;
            }
            p[k][0] = a[0][decomp_k[0]];
            p[k][1] = delta(decomp_l[0],decomp_k[0]);

            for (size_t j = 1; j < m; j++)
            {
                rct::keyV temp;
                temp.reserve(2);
                temp[0] = a[j][decomp_k[j]];
                temp[1] = delta(decomp_l[j],decomp_k[j]);

                p[k] = convolve(p[k],temp,m);
            }
        }

        // Generate initial proof values
        proof.X = rct::keyV(m);
        proof.Y = rct::keyV(m);

        rct::keyV rho;
        rho.reserve(m);
        for (size_t j = 0; j < m; j++)
        {
            rho[j] = rct::skGen();
        }

        // Challenge
        transcript_update_1(tr,message,M,P,proof.J,proof.K,proof.A,proof.B,proof.C,proof.D);
        const rct::key mu = copy(tr);

        for (size_t j = 0; j < m; j++)
        {
            std::vector<MultiexpData> data_X;
            data_X.reserve(2*N);
            
            rct::key U_scalars = ZERO;

            for (size_t k = 0; k < N; k++)
            {
                // X[j] += p[k][j]*(M[k] + mu*P[k])
                // Y[j] += p[k][j]*U
                rct::key temp;
                ge_p3 temp_p3;

                ge_frombytes_vartime(&temp_p3,M[k].bytes);
                data_X.push_back({p[k][j], temp_p3});

                ge_frombytes_vartime(&temp_p3,P[k].bytes);
                sc_mul(temp.bytes,mu.bytes,p[k][j].bytes);
                data_X.push_back({temp, temp_p3});

                sc_add(U_scalars.bytes,U_scalars.bytes,p[k][j].bytes);
            }
            // X[j] += rho[j]*G
            // Y[j] += rho[j]*J
            rct::addKeys1(proof.X[j], rho[j], straus(data_X));

            proof.Y[j] = rct::scalarmultKey(U,U_scalars);
            rct::key rho_J = rct::scalarmultKey(proof.J,rho[j]);
            rct::addKeys(proof.Y[j],proof.Y[j],rho_J);
        }

        // Challenge and powers
        transcript_update_2(tr,proof.X,proof.Y);
        rct::keyV x_pow;
        const rct::key x = copy(tr);
        x_pow.reserve(m+1);
        x_pow[0] = ONE;
        for (size_t j = 1; j < m+1; j++)
        {
            sc_mul(x_pow[j].bytes,x_pow[j-1].bytes,x.bytes);
        }

        // Build the f-matrix
        proof.f = rct::keyMInit(n,m);
        for (size_t j = 0; j < m; j++)
        {
            proof.f[j][0] = ZERO;
            for (size_t i = 1; i < n; i++)
            {
                sc_muladd(proof.f[j][i].bytes,sigma[j][i].bytes,x.bytes,a[j][i].bytes);
            }
        }

        // Build the z-terms
        // zA = rB*x + rA
        // zC = rC*x + rD
        // z = (r + mu*s)*x**m - rho[0]*x**0 - ... - rho[m-1]*x**(m-1)

        sc_muladd(proof.zA.bytes,rB.bytes,x.bytes,rA.bytes);
        sc_muladd(proof.zC.bytes,rC.bytes,x.bytes,rD.bytes);

        sc_muladd(proof.z.bytes,mu.bytes,s.bytes,r.bytes);
        sc_mul(proof.z.bytes,proof.z.bytes,x_pow[m].bytes);

        for (size_t j = 0; j < m; j++)
        {
            sc_mulsub(proof.z.bytes,rho[j].bytes,x_pow[j].bytes,proof.z.bytes);
        }

        // Clear secret prover data
        memwipe(&rA,sizeof(key));
        memwipe(&rB,sizeof(key));
        memwipe(&rC,sizeof(key));
        memwipe(&rD,sizeof(key));
        for (size_t j = 0; j < m; j++)
        {
            memwipe(a[j].data(),a[j].size()*sizeof(key));
        }
        memwipe(rho.data(),rho.size()*sizeof(key));

        return proof;
    }

    // Verify a Triptych proof
    bool triptych_verify(const rct::keyV &M, const rct::keyV &P, TriptychProof proof, const size_t n, const size_t m, const rct::key &message)
    {
        CHECK_AND_ASSERT_THROW_MES(n > 1, "Must have n > 1!");
        CHECK_AND_ASSERT_THROW_MES(m > 1, "Must have m > 1!");
        CHECK_AND_ASSERT_THROW_MES(m*n <= max_mn, "Size parameters are too large!");
        CHECK_AND_ASSERT_THROW_MES(M.size() == pow(n,m), "Public key vector is wrong size!");
        CHECK_AND_ASSERT_THROW_MES(P.size() == pow(n,m), "Commitment vector is wrong size!");

        init_gens();
        const size_t N = pow(n,m);

        rct::key L,R;
        std::vector<MultiexpData> data;
        data.reserve((m*n + 1) + (2*N + 2) + (2*m + 2));

        // Transcript
        rct::key tr;
        transcript_init(tr);
        transcript_update_1(tr,message,M,P,proof.J,proof.K,proof.A,proof.B,proof.C,proof.D);
        const rct::key mu = copy(tr);
        transcript_update_2(tr,proof.X,proof.Y);
        const rct::key x = copy(tr);

        // Challenge powers (negated)
        rct::keyV minus_x;
        minus_x.reserve(m);
        minus_x[0] = MINUS_ONE;
        for (size_t j = 1; j < m; j++)
        {
            sc_mul(minus_x[j].bytes,minus_x[j-1].bytes,x.bytes);
        }

        // Random weights for multiscalar multiplication evaluation
        rct::key w_abcd = ZERO;
        rct::key w_x = ZERO;
        rct::key w_y = ZERO;
        while (w_abcd == ZERO || w_x == ZERO || w_y == ZERO)
        {
            w_abcd = rct::skGen();
            w_x = rct::skGen();
            w_y = rct::skGen();
        }

        // Reconstruct the f-matrix
        for (size_t j = 0; j < m; j++)
        {
            proof.f[j][0] = x;
            for (size_t i = 1; i < n; i++)
            {
                sc_sub(proof.f[j][0].bytes,proof.f[j][0].bytes,proof.f[j][i].bytes);
            }
        }

        // Build the weighted matrix
        rct::keyM abcd = rct::keyMInit(n,m);
        for (size_t j = 0; j < m; j++)
        {
            for (size_t i = 0; i < n; i++)
            {
                rct::key temp;
                sc_sub(temp.bytes,x.bytes,proof.f[j][i].bytes);
                sc_mul(temp.bytes,proof.f[j][i].bytes,temp.bytes);
                sc_muladd(abcd[j][i].bytes,temp.bytes,w_abcd.bytes,proof.f[j][i].bytes);
            }
        }

        // Weighted mask and commitment
        // Com(abcd,zA + w*zC) - A - x*B - w*(x*C + D)
        rct::key temp;
        sc_muladd(temp.bytes,proof.zC.bytes,w_abcd.bytes,proof.zA.bytes);
        com_matrix(data,abcd,temp);

        ge_p3 temp_p3;
        ge_frombytes_vartime(&temp_p3,proof.A.bytes);
        data.push_back({MINUS_ONE,temp_p3});

        ge_frombytes_vartime(&temp_p3,proof.B.bytes);
        data.push_back({minus_x[1],temp_p3});

        ge_frombytes_vartime(&temp_p3,proof.C.bytes);
        sc_mul(temp.bytes,w_abcd.bytes,minus_x[1].bytes);
        data.push_back({temp,temp_p3});

        ge_frombytes_vartime(&temp_p3,proof.D.bytes);
        sc_mul(temp.bytes,w_abcd.bytes,MINUS_ONE.bytes);
        data.push_back({temp,temp_p3});

        // Weighted keys
        rct::key sum_t = ZERO;
        for (size_t k = 0; k < N; k++)
        {
            rct::key t = ONE;
            std::vector<size_t> decomp_k;
            decomp_k.reserve(m);
            decompose(decomp_k,k,n,m);

            for (size_t j = 0; j < m; j++)
            {
                sc_mul(t.bytes,t.bytes,proof.f[j][decomp_k[j]].bytes);
            }

            ge_p3 temp_p3;

            // t*w_x*M[k]
            sc_mul(temp.bytes,t.bytes,w_x.bytes);
            ge_frombytes_vartime(&temp_p3,M[k].bytes);
            data.push_back({temp, temp_p3});

            // mu*t*w_x*P[k]
            sc_mul(temp.bytes,temp.bytes,mu.bytes);
            ge_frombytes_vartime(&temp_p3,P[k].bytes);
            data.push_back({temp, temp_p3});

            sc_add(sum_t.bytes,sum_t.bytes,t.bytes);
        }

        // sum_t*w_y*U
        sc_mul(temp.bytes,sum_t.bytes,w_y.bytes);
        data.push_back({temp,U_p3});

        // mu*sum_t*w_y*K
        sc_mul(temp.bytes,temp.bytes,mu.bytes);
        ge_p3 K_p3;
        ge_frombytes_vartime(&K_p3,proof.K.bytes);
        data.push_back({temp,K_p3});

        for (size_t j = 0; j < m; j++)
        {
            ge_p3 temp_p3;

            // -x^j*w_x*X[j]
            sc_mul(temp.bytes,minus_x[j].bytes,w_x.bytes);
            ge_frombytes_vartime(&temp_p3,proof.X[j].bytes);
            data.push_back({temp,temp_p3});

            // -x^j*w_y*Y[j]
            sc_mul(temp.bytes,minus_x[j].bytes,w_y.bytes);
            ge_frombytes_vartime(&temp_p3,proof.Y[j].bytes);
            data.push_back({temp,temp_p3});
        }

        // -z*w_x*G
        sc_mul(temp.bytes,MINUS_ONE.bytes,proof.z.bytes);
        sc_mul(temp.bytes,temp.bytes,w_x.bytes);
        data.push_back({temp,rct::G});

        // -z*w_y*J
        sc_mul(temp.bytes,MINUS_ONE.bytes,proof.z.bytes);
        sc_mul(temp.bytes,temp.bytes,w_y.bytes);
        ge_frombytes_vartime(&temp_p3,proof.J.bytes);
        data.push_back({temp,temp_p3});

        // Final check
        if (!(straus(data) == rct::identity()))
        {
            MERROR("Triptych verification failed!");
            return false;
        }

        return true;
    }
}
