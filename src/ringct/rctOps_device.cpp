// Copyright (c) 2017-2018, The Monero Project
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

#include "misc_log_ex.h"
#include "rctOps.h"
#include "device/device.hpp"
using namespace crypto;
using namespace std;


namespace rct
{
    void scalarmultKey(key & aP, const key &P, const key &a, hw::device &hwdev) {
        hwdev.scalarmultKey(aP, P, a);
    }

    key scalarmultKey(const key & P, const key & a, hw::device &hwdev) {
        key aP;
        hwdev.scalarmultKey(aP, P, a);
        return aP;
    }

    void scalarmultBase(key &aG, const key &a, hw::device &hwdev) {
        hwdev.scalarmultBase(aG, a);
    }

    key scalarmultBase(const key & a, hw::device &hwdev) {
        key aG;
        hwdev.scalarmultBase(aG, a);
        return aG;
    }

    void ecdhDecode(ecdhTuple & masked, const key & sharedSec, hw::device &hwdev) {
        hwdev.ecdhDecode(masked, sharedSec);
    }

    void ecdhEncode(ecdhTuple & unmasked, const key & sharedSec, hw::device &hwdev) {
        hwdev.ecdhEncode(unmasked, sharedSec);
    }
}