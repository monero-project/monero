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

#include "fcmp_pp_rust/fcmp++.h"
#include "misc_log_ex.h"

namespace fcmp_pp
{
namespace ffi
{
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

// FFI types instantiated on the Rust side must be free'd back on the Rust side. We wrap them in a unique ptr with a
// custom deleter that calls the respective Rust free fn.
#define DEFINE_FCMP_FFI_TYPE(raw_t, new_fn, free_fn)                                              \
    struct raw_t##Deleter { void operator()(raw_t##Unsafe *p) const noexcept { ::free_fn(p); } }; \
    using raw_t = std::unique_ptr<raw_t##Unsafe, raw_t##Deleter>;                                 \
    raw_t raw_t##New()                                                                            \
    {                                                                                             \
        ::raw_t##Unsafe* raw_ptr;                                                                 \
        CHECK_AND_ASSERT_THROW_MES(::new_fn(&raw_ptr) == 0, "Failed to construct " << #raw_t);    \
        return raw_t(raw_ptr);                                                                    \
    };

DEFINE_FCMP_FFI_TYPE(HeliosBranchBlind, new_helios_branch_blind, free_helios_branch_blind);

//----------------------------------------------------------------------------------------------------------------------
}//namespace ffi
}//namespace fcmp_pp
