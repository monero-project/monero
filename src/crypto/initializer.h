// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__GNUC__)
#define INITIALIZER(name) __attribute__((constructor(101))) static void name(void)
#define FINALIZER(name) __attribute__((destructor(101))) static void name(void)
#define REGISTER_FINALIZER(name) ((void) 0)

#elif defined(_MSC_VER)
#include <assert.h>
#include <stdlib.h>
// http://stackoverflow.com/questions/1113409/attribute-constructor-equivalent-in-vc
// http://msdn.microsoft.com/en-us/library/bb918180.aspx
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
