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

#include "misc_log_ex.h"
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

static bool sqrt_ext(fe y, const fe x)
{
    fe y_res;

    fe x2;
    fe_dbl(x2, x);

    fe b;
    fe_pow22523(b, x2);

    fe b_sq;
    fe_sq(b_sq, b);

    fe c;
    fe_mul(c, x2, b_sq);

    if (memcmp(c, fe_one, sizeof(fe)) == 0 || memcmp(c, fe_m1, sizeof(fe)) == 0)
    {
        fe_0(c);
        c[0] = 3;
    }

    fe c_sub_1;
    fe_sub(c_sub_1, c, fe_one);

    fe_mul(y_res, x, b);
    fe_mul(y_res, y_res, c_sub_1);

    // TODO: double check this
    if (fe_isnegative(y_res)) {
        fe_neg(y_res, y_res);
    }

    fe y_sq;
    fe_sq(y_sq, y_res);
    bool r = memcmp(x, y_sq, sizeof(fe)) == 0;

    fe_copy(y, y_res);
    return r;
};

// TODO: impl faster sqrt
static bool sqrt(fe y, const fe x)
{
    return sqrt_ext(y, x);
};

static void inv_iso(fe u_out, fe w_out, const fe u, const fe w)
{
    // 4u
    fe_dbl(u_out, u);
    fe_dbl(u_out, u_out);
    // 2w
    fe_dbl(w_out, w);
};

static void inv_psi1(fe e_out, fe u_out, fe w_out, const fe e, const fe u, const fe w)
{
    fe e_res, u_res, w_res;

    fe tt;
    bool cc = sqrt_ext(tt, u);
    fe_copy(w_res, tt);
    fe w_;
    fe_copy(w_, w);
    fe_copy(e_res, e);

    if (!cc)
    {
        fe tt_sq;
        fe_sq(tt_sq, tt);
        fe neg_u_dbl;
        fe_dbl(neg_u_dbl, u);
        fe_neg(neg_u_dbl, neg_u_dbl);
        if (memcmp(tt_sq, neg_u_dbl, sizeof(fe)) == 0) {
            fe_mul(tt, tt, fe_sqrtm1);
        }

        fe_mul(w_, w, tt);

        fe e_sq;
        fe_sq(e_sq, e);
        fe_mul(w_res, fe_msqrt2b, e_sq);

        fe_mul(e_res, e_res, tt);
    }

    fe w_res_sq;
    fe_sq(w_res_sq, w_res);

    fe e_res_sq;
    fe_sq(e_res_sq, e_res);

    fe A_e_sq;
    fe_mul(A_e_sq, fe_a0, e_res_sq);

    fe w_res_w;
    fe_mul(w_res_w, w_res, w_);

    fe_sub(u_res, w_res_sq, A_e_sq);
    fe_reduce(u_res, u_res);
    fe_sub(u_res, u_res, w_res_w);
    fe_mul(u_res, u_res, fe_inv2);

    fe_copy(e_out, e_res);
    fe_copy(u_out, u_res);
    fe_copy(w_out, w_res);
};

static bool inv_psi2(fe u_out, fe w_out, const fe e, const fe u, const fe w)
{
    fe u_res, w_res;

    if (!sqrt(w_res, u))
        return false;
    fe e_sq;
    fe_sq(e_sq, e);
    fe Ap_e_sq;
    fe_mul(Ap_e_sq, fe_ap, e_sq);

    fe w_res_w;
    fe_mul(w_res_w, w_res, w);

    fe_sub(u_res, u, Ap_e_sq);
    fe_reduce(u_res, u_res);
    fe_sub(u_res, u_res, w_res_w);
    fe_mul(u_res, u_res, fe_inv2);

    fe_copy(u_out, u_res);
    fe_copy(w_out, w_res);

    return true;
};

static bool check_e_u_w(const fe e, const fe u, const fe w)
{
    static fe a;
    fe_1(a);
    fe_neg(a, a);
    fe A;
    fe_add(A, a, fe_d);
    fe_dbl(A, A);
    fe B;
    fe_sq(B, fe_a_sub_d);

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

    fe_reduce(u_sq, u_sq);
    fe_reduce(A_u_mul_e_sq, A_u_mul_e_sq);
    fe_add(sum, u_sq, A_u_mul_e_sq);

    fe_reduce(sum, sum);
    fe_reduce(B_mul_e_sq_sq, B_mul_e_sq_sq);
    fe_add(sum, sum, B_mul_e_sq_sq);

    fe_reduce(sum, sum);

    if (memcmp(u_w_sq, sum, sizeof(fe)) != 0) {
        return false;
    }

    return true;
}

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
// https://github.com/kayabaNerve/fcmp-plus-plus/blob/94744c5324e869a9483bbbd93a864e108304bf76/crypto/divisors/src/tests/torsion_check.rs
bool torsion_check(const rct::key &k) {
    // {
    //     static fe D;
    //     {
    //         fe fe_numer{121665, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //         fe fe_denom{121666, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //         fe_neg(fe_numer, fe_numer);
    //         fe_invert(fe_denom, fe_denom);
    //         fe_mul(D, fe_numer, fe_denom);
    //     }
    //     static fe one;
    //     fe_1(one);
    //     static fe a;
    //     fe_neg(a, one);
    //     static fe a_minus_D;
    //     fe_sub(a_minus_D, a, D);
    //     fe_reduce(a_minus_D, a_minus_D);
    //     static fe A;
    //     fe_add(A, a, D);
    //     fe_reduce(A, A);
    //     fe_dbl(A, A);
    //     static fe B;
    //     fe_sq(B, a_minus_D);
    //     static fe Ap;
    //     fe_neg(Ap, A);
    //     fe_dbl(Ap, Ap);
    //     static fe Asq;
    //     fe_sq(Asq, A);
    //     fe B_mul_4;
    //     fe_dbl(B_mul_4, B);
    //     fe_dbl(B_mul_4, B_mul_4);
    //     static fe Bp;
    //     fe_sub(Bp, Asq, B_mul_4);
    //     static const fe fe_5{5, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //     static const fe fe_8{8, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //     static fe fe_neg_5;
    //     fe_neg(fe_neg_5, fe_5);
    //     static fe inv_8;
    //     fe_invert(inv_8, fe_8);
    //     static fe Bp2;
    //     fe_dbl(Bp2, Bp);
    //     static fe neg_sqrt_2b;
    //     if (!sqrt(neg_sqrt_2b, Bp2))
    //         return false;
    //     // needs sqrt implemented to match rust const usage
    //     // fe_neg(neg_sqrt_2b, neg_sqrt_2b);
    //     static fe inv_2;
    //     static const fe fe_2{2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //     fe_invert(inv_2, fe_2);

    //     static fe sqrtm1;
    //     if (!sqrt(sqrtm1, a))
    //         return false;

    //     printf("fe_d: ");
    //     print_fe(D);

    //     printf("fe_a_sub_d: ");
    //     print_fe(a_minus_D);

    //     printf("fe_a0: ");
    //     print_fe(A);

    //     printf("fe_ap: ");
    //     print_fe(Ap);

    //     printf("neg_sqrt_2b: ");
    //     print_fe(neg_sqrt_2b);

    //     printf("inv_2: ");
    //     print_fe(inv_2);

    //     printf("fe_sqrtm1: ");
    //     print_fe(sqrtm1);

    //     assert(memcmp(fe_d,       D,           sizeof(fe)) == 0);
    //     assert(memcmp(fe_a_sub_d, a_minus_D,   sizeof(fe)) == 0);
    //     assert(memcmp(fe_a0,      A,           sizeof(fe)) == 0);
    //     assert(memcmp(fe_ap,      Ap,          sizeof(fe)) == 0);
    //     assert(memcmp(fe_msqrt2b, neg_sqrt_2b, sizeof(fe)) == 0);
    //     assert(memcmp(fe_inv2,    inv_2,       sizeof(fe)) == 0);
    //     assert(memcmp(fe_sqrtm1,  sqrtm1,      sizeof(fe)) == 0);

    //     CHECK_AND_ASSERT_THROW_MES(memcmp(fe_d,       D,           sizeof(fe)) == 0, "fe_d");
    //     CHECK_AND_ASSERT_THROW_MES(memcmp(fe_a_sub_d, a_minus_D,   sizeof(fe)) == 0, "fe_a_sub_d");
    //     CHECK_AND_ASSERT_THROW_MES(memcmp(fe_a0,      A,           sizeof(fe)) == 0, "fe_a0");
    //     CHECK_AND_ASSERT_THROW_MES(memcmp(fe_ap,      Ap,          sizeof(fe)) == 0, "fe_ap");
    //     CHECK_AND_ASSERT_THROW_MES(memcmp(fe_msqrt2b, neg_sqrt_2b, sizeof(fe)) == 0, "fe_msqrt2b");
    //     CHECK_AND_ASSERT_THROW_MES(memcmp(fe_inv2,    inv_2,       sizeof(fe)) == 0, "fe_ivn2");
    //     CHECK_AND_ASSERT_THROW_MES(memcmp(fe_sqrtm1,  sqrtm1,      sizeof(fe)) == 0, "fe_sqrtm1");
    // }

    // De-compress the point
    ge_p3 point;
    if (ge_frombytes_vartime(&point, k.bytes) != 0) {
        return false;
    }

    // Make sure point is not equal to identity
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
        fe_dbl(w, z_minus_ed_y);
    }

    assert(check_e_u_w(e, u, w));

    // Torsion check
    for (int i = 0; i < 2; ++i) {
        inv_iso(u, w, u, w);
        if (!inv_psi2(u, w, e, u, w)) {
            return false;
        }
        inv_psi1(e, u, w, e, u, w);
        assert(check_e_u_w(e, u, w));
    }

    fe _;
    inv_iso(u, _, u, w);

    if (!sqrt(u, u)) {
        return false;
    }

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
