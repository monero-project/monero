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

#pragma once

extern "C"
{
#include "crypto/crypto-ops.h"
}
#include "ringct/rctTypes.h"

namespace fcmp_pp
{
//----------------------------------------------------------------------------------------------------------------------
// Field elems needed to get wei x and y coords
struct EdDerivatives final
{
    fe one_plus_y;
    fe one_minus_y;
    fe one_minus_y_mul_x;
};
//----------------------------------------------------------------------------------------------------------------------
// TODO: tests for these functions
bool sqrt(fe y, const fe x);
bool mul8_is_identity(const ge_p3 &point);
bool torsion_check_vartime(const ge_p3 &point);
rct::key clear_torsion(const ge_p3 &point);
bool get_valid_torsion_cleared_point(const rct::key &point, rct::key &torsion_cleared_out);
bool get_valid_torsion_cleared_point_fast(const rct::key &point, rct::key &torsion_cleared_out);
bool point_to_ed_derivatives(const rct::key &pub, EdDerivatives &ed_derivatives);
bool ed_derivatives_to_wei_x_y(const EdDerivatives &ed_derivatives, rct::key &wei_x, rct::key &wei_y);
bool point_to_wei_x(const rct::key &pub, rct::key &wei_x);
/**
 * brief - scalarmult_and_add - Q = P + a * A
 */
void scalarmult_and_add(unsigned char *Q, const ge_p3 &P, const unsigned char *a, const ge_p3 &A);
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
}//namespace fcmp_pp
