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

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
bool mul8_is_identity(const ge_p3 &point) {
    ge_p2 point_ge_p2;
    ge_p3_to_p2(&point_ge_p2, &point);
    ge_p1p1 point_mul8;
    ge_mul8(&point_mul8, &point_ge_p2);
    ge_p3 point_mul8_p3;
    ge_p1p1_to_p3(&point_mul8_p3, &point_mul8);
    return ge_p3_is_point_at_infinity_vartime(&point_mul8_p3);
}
//----------------------------------------------------------------------------------------------------------------------
crypto::ec_point clear_torsion(const ge_p3 &point) {
    // mul by inv 8, then mul by 8
    ge_p2 point_inv_8;
    ge_scalarmult(&point_inv_8, to_bytes(crypto::EC_INV_EIGHT), &point);
    ge_p1p1 point_inv_8_mul_8;
    ge_mul8(&point_inv_8_mul_8, &point_inv_8);
    ge_p3 torsion_cleared_point;
    ge_p1p1_to_p3(&torsion_cleared_point, &point_inv_8_mul_8);
    crypto::ec_point k_out;
    ge_p3_tobytes(to_bytes(k_out), &torsion_cleared_point);
    return k_out;
}
//----------------------------------------------------------------------------------------------------------------------
bool get_valid_torsion_cleared_point(const crypto::ec_point &point, crypto::ec_point &torsion_cleared_out) {
    ge_p3 p3;
    if (ge_frombytes_vartime(&p3, to_bytes(point)) != 0)
        return false;
    if (mul8_is_identity(p3))
        return false;
    torsion_cleared_out = fcmp_pp::clear_torsion(p3);
    if (torsion_cleared_out == crypto::EC_I)
        return false;
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
