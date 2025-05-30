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

//! @file Exceptions thrown by Carrot

#pragma once

//local headers

//third party headers

//standard headers
#include <stdexcept>

//forward declarations

namespace carrot
{
#define CARROT_DEFINE_SIMPLE_ERROR_TYPE(e, b) class e: b { using b::b; };

class carrot_logic_error: std::logic_error { using std::logic_error::logic_error; };

CARROT_DEFINE_SIMPLE_ERROR_TYPE(bad_address_type,       carrot_logic_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(component_out_of_order, carrot_logic_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(invalid_point,          carrot_logic_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(missing_components,     carrot_logic_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(missing_randomness,     carrot_logic_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(too_few_inputs,         carrot_logic_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(too_few_outputs,        carrot_logic_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(too_many_outputs,       carrot_logic_error)

class carrot_runtime_error: std::runtime_error { using std::runtime_error::runtime_error; };

CARROT_DEFINE_SIMPLE_ERROR_TYPE(crypto_function_failed,    carrot_runtime_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(not_enough_money,          carrot_runtime_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(not_enough_usable_money,   carrot_runtime_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(unexpected_scan_failure,   carrot_runtime_error)
CARROT_DEFINE_SIMPLE_ERROR_TYPE(burnt_enote,               carrot_runtime_error)

/// one needs to include misc_log_ex.h to use the following macros
#define CARROT_THROW(errtype, message) {      \
        std::stringstream ss;                 \
        ss << message;                        \
        const std::string msg_str = ss.str(); \
        LOG_ERROR(msg_str);                   \
        throw errtype(msg_str);               \
    }
#define CARROT_CHECK_AND_THROW(expr, errtype, message) if (!(expr)) { CARROT_THROW(errtype, message) }

#undef CARROT_DEFINE_SIMPLE_ERROR_TYPE
} //namespace carrot
