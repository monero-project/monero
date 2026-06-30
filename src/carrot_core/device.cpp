// Copyright (c) 2025, The Monero Project
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

//paired header
#include "device.h"
#include "crypto/crypto.h"

//local headers
extern "C"
{
#include "crypto/crypto-ops.h"
}

//third party headers

//standard headers

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "carrot.device"

namespace carrot
{
//-------------------------------------------------------------------------------------------------------------------
bool view_incoming_key_device::view_key_scalar_mult8_ed25519(const crypto::public_key &P,
    crypto::public_key &kv8P) const
{
    // Is slow b/c it does 2 compressions, override if you want speed

    if (!this->view_key_scalar_mult_ed25519(P, kv8P))
        return false;

    // decompress k_v * P
    ge_p3 tmp1;
    ge_p2 tmp2;
    if (0 != ge_frombytes_vartime(&tmp1, to_bytes(kv8P)))
        return false;
    ge_p3_to_p2(&tmp2, &tmp1);

    // mul 8
    ge_p1p1 tmp3;
    for (int i = 0; i < 3; ++i)
    {
        ge_p2_dbl(&tmp3, &tmp2);
        ge_p1p1_to_p2(&tmp2, &tmp3);
    }

    // re-compress
    ge_tobytes(to_bytes(kv8P), &tmp2);

    return true;
}
//-------------------------------------------------------------------------------------------------------------------
} //namespace carrot
