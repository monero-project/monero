// Copyright (c) 2019, Ryo Currency Project
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
// Parts of this file are originally copyright (c) 2014-2017, SUMOKOIN
// Parts of this file are originally copyright (c) 2014-2017, The Monero Project
// Parts of this file are originally copyright (c) 2012-2013, The Cryptonote developers

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <intrin.h>
#include <malloc.h>
#define HAS_WIN_INTRIN_API
#endif

// Note HAS_INTEL_HW and HAS_ARM_HW only mean we can emit the AES instructions
// check CPU support for the hardware AES encryption has to be done at runtime
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X86) || defined(_M_X64)
#ifdef __GNUC__
#include <x86intrin.h>
#if !defined(HAS_WIN_INTRIN_API)
#include <cpuid.h>
#endif // !defined(HAS_WIN_INTRIN_API)
#endif // __GNUC__
#define HAS_INTEL_HW
#endif

#if defined(__arm__) || defined(__aarch64__)
#define HAS_ARM
#endif

#if defined(__aarch64__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#define HAS_ARM_HW
#endif

#if !defined(_LP64) && !defined(_WIN64)
#define BUILD32
#endif

#if defined(CN_ADD_TARGETS_AND_HEADERS)
#if defined(__aarch64__)
#ifndef __ARM_FEATURE_CRYPTO
#define __ARM_FEATURE_CRYPTO 1
#endif
#ifndef __clang__
#pragma GCC target("+crypto")
#endif
#include "arm8_neon.hpp"
#elif defined(__arm__)
#ifndef __clang__
#pragma GCC target("fpu=vfpv4")
#endif
#include "arm_vfp.hpp"
#elif defined(HAS_INTEL_HW) && defined(INTEL_AVX2)
#ifndef __clang__
#pragma GCC target("aes,avx2")
#endif
#elif defined(HAS_INTEL_HW)
#ifndef __clang__
#pragma GCC target("aes,sse2")
#endif
#endif
#endif