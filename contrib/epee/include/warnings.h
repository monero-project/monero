// Copyright (c) 2014, The Monero Project
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
// Parts of this file are originally copyright (c) 2006-2013, Andrey N. Sabelnikov

#pragma once

#if defined(_MSC_VER)

#define PUSH_WARNINGS __pragma(warning(push))
#define POP_WARNINGS __pragma(warning(pop))
#define DISABLE_VS_WARNINGS(w) __pragma(warning(disable: w))
#define DISABLE_GCC_WARNING(w)
#define DISABLE_CLANG_WARNING(w)
#define DISABLE_GCC_AND_CLANG_WARNING(w)

#else

#include <boost/preprocessor/stringize.hpp>

#define PUSH_WARNINGS _Pragma("GCC diagnostic push")
#define POP_WARNINGS _Pragma("GCC diagnostic pop")
#define DISABLE_VS_WARNINGS(w)

#if defined(__clang__)
#define DISABLE_GCC_WARNING(w)
#define DISABLE_CLANG_WARNING DISABLE_GCC_AND_CLANG_WARNING
#else
#define DISABLE_GCC_WARNING DISABLE_GCC_AND_CLANG_WARNING
#define DISABLE_CLANG_WARNING(w)
#endif

#define DISABLE_GCC_AND_CLANG_WARNING(w) _Pragma(BOOST_PP_STRINGIZE(GCC diagnostic ignored BOOST_PP_STRINGIZE(-W##w)))

#endif