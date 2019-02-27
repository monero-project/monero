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

#ifndef USE_DEVICE_TREZOR
#define USE_DEVICE_TREZOR 1
#endif

// HAVE_TREZOR_READY set by cmake after protobuf messages
// were generated successfully and all minimal dependencies are met.
#ifndef DEVICE_TREZOR_READY
#undef  USE_DEVICE_TREZOR
#define USE_DEVICE_TREZOR 0
#endif

#if USE_DEVICE_TREZOR
#define WITH_DEVICE_TREZOR 1
#endif

#ifndef WITH_DEVICE_TREZOR
#undef WITH_DEVICE_TREZOR_LITE
#endif

#if defined(HAVE_TREZOR_LIBUSB) && USE_DEVICE_TREZOR
#define WITH_DEVICE_TREZOR_WEBUSB 1
#endif

// Enable / disable UDP in the enumeration
#ifndef USE_DEVICE_TREZOR_UDP
#define USE_DEVICE_TREZOR_UDP 1
#endif

// Enable / disable UDP in the enumeration for release
#ifndef USE_DEVICE_TREZOR_UDP_RELEASE
#define USE_DEVICE_TREZOR_UDP_RELEASE 0
#endif

#if USE_DEVICE_TREZOR_UDP && (USE_DEVICE_TREZOR_UDP_RELEASE || defined(TREZOR_DEBUG))
#define WITH_DEVICE_TREZOR_UDP 1
#endif

// Avoids protobuf undefined macro warning
#ifndef PROTOBUF_INLINE_NOT_IN_HEADERS
#define PROTOBUF_INLINE_NOT_IN_HEADERS 0
#endif

// Fixes gcc7 problem with minor macro defined clashing with minor() field.
#ifdef minor
#undef minor
#endif
