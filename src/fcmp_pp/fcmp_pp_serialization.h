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

#pragma once

#include "fcmp_pp_types.h"
#include "serialization/serialization.h"
#include "serialization/binary_archive.h"
#include "serialization/debug_archive.h"
#include "serialization/json_archive.h"
#include "serialization/variant.h"


BEGIN_SERIALIZE_OBJECT_FN(fcmp_pp::LegacyOutputPair)
    FIELD_F(output_pubkey)
    FIELD_F(commitment)
END_SERIALIZE()

BEGIN_SERIALIZE_OBJECT_FN(fcmp_pp::CarrotOutputPairV1)
    FIELD_F(output_pubkey)
    FIELD_F(commitment)
END_SERIALIZE()

VARIANT_TAG(binary_archive, fcmp_pp::LegacyOutputPair, 0x70);
VARIANT_TAG(binary_archive, fcmp_pp::CarrotOutputPairV1, 0x71);

VARIANT_TAG(json_archive, fcmp_pp::LegacyOutputPair, "legacy_out_pair");
VARIANT_TAG(json_archive, fcmp_pp::CarrotOutputPairV1, "carrot_out_pair_v1");

VARIANT_TAG(debug_archive, fcmp_pp::LegacyOutputPair, "legacy_out_pair");
VARIANT_TAG(debug_archive, fcmp_pp::CarrotOutputPairV1, "carrot_out_pair_v1");

BEGIN_SERIALIZE_OBJECT_FN(fcmp_pp::UnifiedOutput)
    FIELD_F(unified_id)
    FIELD_F(output_pair)
END_SERIALIZE()
