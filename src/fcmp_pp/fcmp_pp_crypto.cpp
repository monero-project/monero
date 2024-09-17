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

namespace fcmp_pp
{
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
bool point_to_pre_wei_x(const rct::key &pub, PreWeiX &pre_wei_x) {
    if (pub == rct::I)
        return false;
    fe y;
    if (fe_frombytes_vartime(y, pub.bytes) != 0)
        return false;
    fe one;
    fe_1(one);
    // (1+y),(1-y)
    fe_add(pre_wei_x.one_plus_y, one, y);
    fe_sub(pre_wei_x.one_minus_y, one, y);
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
void pre_wei_x_to_wei_x(const PreWeiX pre_wei_x, rct::key &wei_x) {
    fe inv_one_minus_y;
    fe_invert(inv_one_minus_y, pre_wei_x.one_minus_y);
    fe_to_wei_x(wei_x.bytes, inv_one_minus_y, pre_wei_x.one_plus_y);
}
//----------------------------------------------------------------------------------------------------------------------
bool point_to_wei_x(const rct::key &pub, rct::key &wei_x) {
    PreWeiX pre_wei_x;
    if (!point_to_pre_wei_x(pub, pre_wei_x))
        return false;
    pre_wei_x_to_wei_x(pre_wei_x, wei_x);
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
