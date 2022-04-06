// Copyright (c) 2014-2022, The Monero Project
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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#if defined(__GNUC__)
#if defined(__sun) && defined(__SVR4)
#define INITIALIZER(name) __attribute__((constructor)) static void name(void)
#define FINALIZER(name) __attribute__((destructor)) static void name(void)
#else
#define INITIALIZER(name) __attribute__((constructor(101))) static void name(void)
#define FINALIZER(name) __attribute__((destructor(101))) static void name(void)
#endif
#define REGISTER_FINALIZER(name) ((void) 0)

#elif defined(_MSC_VER)
#include <assert.h>
#include <stdlib.h>
// https://stackoverflow.com/questions/1113409/attribute-constructor-equivalent-in-vc
// https://msdn.microsoft.com/en-us/library/bb918180.aspx
#pragma section(".CRT$XCT", read)
#define INITIALIZER(name) \
  static void __cdecl name(void); \
  __declspec(allocate(".CRT$XCT")) void (__cdecl *const _##name)(void) = &name; \
  static void __cdecl name(void)
#define FINALIZER(name) \
  static void __cdecl name(void)
#define REGISTER_FINALIZER(name) \
  do { \
    int _res = atexit(name); \
    assert(_res == 0); \
  } while (0);

#else
#error Unsupported compiler
#endif
