// Copyright (c) 2024, The Monero Project
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

#include "fcmp_pp_crypto.h"

#include "ringct/rctOps.h"

static void print_bytes(const fe f)
{
    unsigned char bytes[32];
    fe_tobytes(bytes, f);
    for (int i = 0; i < 32; ++i)
    {
        printf("%d, ", bytes[i]);
    }
    printf("\n");
}

static void print_fe(const fe f)
{
    for (int i = 0; i < 10; ++i)
    {
        printf("%d, ", f[i]);
    }
    printf("\n");
}

static void inv_iso(fe u_out, fe w_out, const fe u, const fe w)
{
    // 4u
    fe_add(u_out, u, u);
    fe_add(u_out, u_out, u_out);
    // 2w
    fe_add(w_out, w, w);
};

// z = x^y
// FIXME: this doesn't yield same result as rust impl
// TODO: pre-compute table + use windowing
static void fe_pow(fe z, const fe x, const fe y)
{
    fe_copy(z, fe_one);

    fe v;
    fe_copy(v, x);

    char unsigned y_bytes[32];
    fe_tobytes(y_bytes, y);

    for (int i = 0; i < 256; ++i) {
        int y_idx = i / 8;
        int y_mask = i % 8;
        int bit_set = y_bytes[y_idx] & (1UL << y_mask);
        if (bit_set > 0) {
            fe_mul(z, z, v); /* z *= v */
        }
        fe_sq(v, v); /* v = v^2 */
    }
}

static bool sqrt_ext(fe y, const fe x)
{
    fe x2;
    fe_add(x2, x, x);

    fe b;
    // FIXME: this doesn't yield rust impl's ^(q-5)/8... so trying to use fe_pow instead
    // fe_pow22523(b, x2);

    // FIXME: this doesn't yield same result as rust pow impl
    fe_pow(b, x2, fe_mod_5_8);

    fe b_sq;
    fe_sq(b_sq, b);

    fe c;
    fe_mul(c, x2, b_sq);

    if (memcmp(c, fe_one, sizeof(fe)) == 0 || memcmp(c, fe_m1, sizeof(fe)) == 0)
    {
        static const fe fe_3 = {3, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        fe_copy(c, fe_3);
    }

    fe c_sub_1;
    fe_sub(c_sub_1, c, fe_one);

    fe_mul(y, x, b);
    fe_mul(y, y, c_sub_1);

    // TODO: double check this
    if (fe_isnegative(y))
        fe_neg(y, y);

    fe y_sq;
    fe_sq(y_sq, y);
    return memcmp(x, y_sq, sizeof(fe)) == 0;
};

// TODO: impl faster sqrt
static bool sqrt(fe y, const fe x)
{
    return sqrt_ext(y, x);
};

static void inv_psi1(fe e_out, fe u_out, fe w_out, const fe e, const fe u, const fe w, const fe msqrt2b)
{
    fe tt;
    bool cc = sqrt_ext(tt, u);
    memcpy(w_out, tt, sizeof(fe));
    fe w_;
    memcpy(w_, w, sizeof(fe));
    memcpy(e_out, e, sizeof(fe));

    if (!cc)
    {
        fe tt_sq;
        fe_sq(tt_sq, tt);
        fe neg_u_dbl;
        fe_add(neg_u_dbl, u, u);
        fe_neg(neg_u_dbl, neg_u_dbl);
        if (memcmp(tt_sq, neg_u_dbl, sizeof(fe)) == 0)
            fe_mul(tt, tt, fe_sqrtm1);

        fe_mul(w_, w_, tt);

        fe e_sq;
        fe_sq(e_sq, e);

        fe_mul(w_out, msqrt2b, e_sq);
        fe_mul(e_out, e, tt);
    }

    fe w_out_sq;
    fe_sq(w_out_sq, w_out);

    fe e_out_sq;
    fe_sq(e_out_sq, e_out);

    fe A_e_sq;
    fe_mul(A_e_sq, fe_a0, e_out_sq);

    fe w_out_w;
    fe_mul(w_out_w, w_out, w_);

    fe_sub(u_out, w_out_sq, A_e_sq);
    fe_sub(u_out, u_out, w_out_w);
    fe_mul(u_out, u_out, fe_inv2);
};

static bool inv_psi2(fe u_out, fe w_out, const fe e, const fe u, const fe w)
{
    if (!sqrt(w_out, u))
        return false;
    fe e_sq;
    fe_sq(e_sq, e);
    fe Ap_e_sq;
    fe_mul(Ap_e_sq, fe_ap, e_sq);

    fe w_out_w;
    fe_mul(w_out_w, w_out, w);

    fe_sub(u_out, u, Ap_e_sq);
    fe_sub(u_out, u_out, w_out_w);
    fe_mul(u_out, u_out, fe_inv2);

    return true;
};

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
// https://github.com/kayabaNerve/fcmp-plus-plus/blob/94744c5324e869a9483bbbd93a864e108304bf76/crypto/divisors/src/tests/torsion_check.rs
bool torsion_check(const rct::key &k) {
    // TODO: remove and just use static consts
    static fe a;
    fe_1(a);
    fe_neg(a, a);
    static fe D;
    fe_copy(D, fe_d);
    static fe a_minus_D;
    fe_sub(a_minus_D, a, D);
    static fe A;
    fe_add(A, a, D);
    fe_add(A, A, A);
    static fe B;
    fe_sq(B, a_minus_D);
    static fe Ap;
    fe_add(Ap, A, A);
    fe_neg(Ap, Ap);
    static fe Asq;
    fe_sq(Asq, A);
    fe B_mul4;
    fe_add(B_mul4, B, B);
    fe_add(B_mul4, B_mul4, B_mul4);
    static fe Bp;
    fe_sub(Bp, Asq, B_mul4);
    static fe msqrt2b;
    fe_add(msqrt2b, Bp, Bp);
    if (!sqrt(msqrt2b, msqrt2b))
        return false;
    fe_neg(msqrt2b, msqrt2b);

    // fe mod_5_8;
    // fe fe_5;
    // fe_0(fe_5);
    // fe_5[0] = 5;
    // fe neg_fe_5;
    // fe_neg(neg_fe_5, fe_5);
    // fe fe_8;
    // fe_0(fe_8);
    // fe_8[0] = 8;
    // fe fe_inv8;
    // fe_invert(fe_inv8, fe_8);
    // fe_mul(mod_5_8, neg_fe_5, fe_inv8);

    // De-compress the point
    ge_p3 point;
    if (ge_frombytes_vartime(&point, k.bytes) != 0)
        return false;

    // Make sure point not equal to identity
    {
        ge_p2 point_ge_p2;
        ge_p3_to_p2(&point_ge_p2, &point);
        ge_p1p1 point_mul8;
        ge_mul8(&point_mul8, &point_ge_p2);

        ge_p2 point_mul8_p2;
        ge_p1p1_to_p2(&point_mul8_p2, &point_mul8);
        rct::key tmp;
        ge_tobytes(tmp.bytes, &point_mul8_p2);
        if (tmp == rct::I)
            return false;
    }

    // ed to wei
    fe e, u, w;
    {
        fe z_plus_ed_y, z_minus_ed_y;
        fe_add(z_plus_ed_y, fe_one, point.Y);
        fe_sub(z_minus_ed_y, fe_one, point.Y);

        // e
        fe_mul(e, z_minus_ed_y, point.X);
        // u
        fe_mul(u, fe_a_sub_d, z_plus_ed_y);
        fe_mul(u, u, point.X);
        fe_mul(u, u, e);
        // w
        fe_add(w, z_minus_ed_y, z_minus_ed_y);
    }

    // FIXME: only run this in debug mode
    {
        // Make sure e, u, w are correct here
        fe w_sq, u_w_sq;
        fe_sq(w_sq, w);
        fe_mul(u_w_sq, u, w_sq);

        fe u_sq, A_u_mul_e_sq, e_sq, e_sq_sq, B_mul_e_sq_sq, sum;
        fe_sq(u_sq, u);
        fe_mul(A_u_mul_e_sq, A, u);
        fe_sq(e_sq, e);
        fe_mul(A_u_mul_e_sq, A_u_mul_e_sq, e_sq);
        fe_sq(e_sq_sq, e_sq);
        fe_mul(B_mul_e_sq_sq, B, e_sq_sq);

        fe_add(sum, u_sq, A_u_mul_e_sq);
        fe_add(sum, sum, B_mul_e_sq_sq);

        if (memcmp(u_w_sq, sum, sizeof(fe)) != 0)
        {
            printf("wei mapping is wrong\n");
            return false;
        }
    }

    // Torsion check
    inv_iso(u, w, u, w);
    if (!inv_psi2(u, w, e, u, w))
        return false;
    inv_psi1(e, u, w, e, u, w, msqrt2b);

    inv_iso(u, w, u, w);
    if (!inv_psi2(u, w, e, u, w))
        return false;
    inv_psi1(e, u, w, e, u, w, msqrt2b);

    inv_iso(u, w, u, w);

    // TODO: check if u has a square root via legendre symbol
    // e.g. https://github.com/signalapp/curve25519-java/blob/70fae57d6dccff7e78a46203c534314b07dfdd98/android/jni/ed25519/additions/elligator.c#L8-L25
    if (!sqrt(u, u))
        return false;

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
bool clear_torsion(const rct::key &k, rct::key &k_out) {
    ge_p3 point;
    if (ge_frombytes_vartime(&point, k.bytes) != 0)
        return false;
    // mul by inv 8, then mul by 8
    ge_p2 point_inv_8;
    ge_scalarmult(&point_inv_8, rct::INV_EIGHT.bytes, &point);
    ge_p1p1 point_inv_8_mul_8;
    ge_mul8(&point_inv_8_mul_8, &point_inv_8);
    ge_p3 torsion_cleared_point;
    ge_p1p1_to_p3(&torsion_cleared_point, &point_inv_8_mul_8);
    ge_p3_tobytes(k_out.bytes, &torsion_cleared_point);
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
bool point_to_ed_y_derivatives(const rct::key &pub, EdYDerivatives &ed_y_derivatives) {
    if (pub == rct::I)
        return false;
    fe y;
    if (fe_frombytes_vartime(y, pub.bytes) != 0)
        return false;
    fe one;
    fe_1(one);
    // (1+y),(1-y)
    fe_add(ed_y_derivatives.one_plus_y, one, y);
    fe_sub(ed_y_derivatives.one_minus_y, one, y);
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
void ed_y_derivatives_to_wei_x(const EdYDerivatives &pre_wei_x, rct::key &wei_x) {
    fe inv_one_minus_y;
    fe_invert(inv_one_minus_y, pre_wei_x.one_minus_y);
    fe_ed_y_derivatives_to_wei_x(wei_x.bytes, inv_one_minus_y, pre_wei_x.one_plus_y);
}
//----------------------------------------------------------------------------------------------------------------------
bool point_to_wei_x(const rct::key &pub, rct::key &wei_x) {
    EdYDerivatives ed_y_derivatives;
    if (!point_to_ed_y_derivatives(pub, ed_y_derivatives))
        return false;
    ed_y_derivatives_to_wei_x(ed_y_derivatives, wei_x);
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
