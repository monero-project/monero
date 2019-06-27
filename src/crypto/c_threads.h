// Copyright (c) 2019, The Monero Project
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

/* Brain-dead simple portability wrapper over thread APIs for C */
#pragma once

#ifdef _WIN32
#include <windows.h>
#define CTHR_MUTEX_TYPE	HANDLE
#define CTHR_MUTEX_INIT	NULL
#define CTHR_MUTEX_LOCK(x)	do { if (x == NULL) { \
    HANDLE p = CreateMutex(NULL, FALSE, NULL); \
    if (InterlockedCompareExchangePointer((PVOID*)&x, (PVOID)p, NULL) != NULL) \
      CloseHandle(p); \
  } WaitForSingleObject(x, INFINITE); } while(0)
#define CTHR_MUTEX_UNLOCK(x)	ReleaseMutex(x)
#define CTHR_THREAD_TYPE	HANDLE
#define CTHR_THREAD_RTYPE	void
#define CTHR_THREAD_RETURN	return
#define CTHR_THREAD_CREATE(thr, func, arg)	thr = (HANDLE)_beginthread(func, 0, arg)
#define CTHR_THREAD_JOIN(thr)			WaitForSingleObject(thr, INFINITE)
#else
#include <pthread.h>
#define CTHR_MUTEX_TYPE pthread_mutex_t
#define CTHR_MUTEX_INIT	PTHREAD_MUTEX_INITIALIZER
#define CTHR_MUTEX_LOCK(x)	pthread_mutex_lock(&x)
#define CTHR_MUTEX_UNLOCK(x)	pthread_mutex_unlock(&x)
#define CTHR_THREAD_TYPE pthread_t
#define CTHR_THREAD_RTYPE	void *
#define CTHR_THREAD_RETURN	return NULL
#define CTHR_THREAD_CREATE(thr, func, arg)	pthread_create(&thr, NULL, func, arg)
#define CTHR_THREAD_JOIN(thr)			pthread_join(thr, NULL)
#endif
