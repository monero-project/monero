// Copyright (c) 2017-2022, The Monero Project
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

#ifdef __APPLE__

#define PLATFORM_MACOS 1

#include <mach/vm_statistics.h>
#include <TargetConditionals.h>
#include <AvailabilityMacros.h>
#include <sys/utsname.h>

#ifdef __cplusplus
#include <stdexcept>
extern "C" {
#else
#include <stdio.h>
#endif
int32_t __isOSVersionAtLeast(int32_t major, int32_t minor, int32_t subminor);
int32_t __isPlatformVersionAtLeast(uint32_t platform, uint32_t major, uint32_t minor, uint32_t subminor);
#ifdef __cplusplus
}
#endif

#if defined(MAC_OS_VERSION_11_0) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_11_0
static int MacOSchecked, MacOSver;

/* This function is used implicitly by clang's __builtin_available() checker.
 * When cross-compiling, the library containing this function doesn't exist,
 * and linking will fail because the symbol is unresolved. The function here
 * is a quick and dirty hack to get close enough to identify MacOSX 11.0.
 */
int32_t __isOSVersionAtLeast(int32_t major, int32_t minor, int32_t subminor) {
  if (!MacOSchecked) {
    struct utsname ut;
    int mmaj, mmin;
    uname(&ut);
    sscanf(ut.release, "%d.%d", &mmaj, &mmin);
    // The utsname release version is 9 greater than the canonical OS version
    mmaj -= 9;
    MacOSver = (mmaj << 8) | mmin;
    MacOSchecked = 1;
  }
  return MacOSver >= ((major << 8) | minor);
}

int32_t __isPlatformVersionAtLeast(uint32_t platform, uint32_t major, uint32_t minor, uint32_t subminor) {
  if (platform != PLATFORM_MACOS)
      return 0;

  return __isOSVersionAtLeast((int32_t)major, (int32_t)minor, (int32_t)subminor);
}

#endif
#endif
