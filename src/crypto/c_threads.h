// Copyright (c) 2019-2024, The Monero Project
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

#define CTHR_RWLOCK_TYPE	SRWLOCK
#define CTHR_RWLOCK_INIT	SRWLOCK_INIT
#define CTHR_RWLOCK_LOCK_WRITE(x)	AcquireSRWLockExclusive(&x)
#define CTHR_RWLOCK_UNLOCK_WRITE(x)	ReleaseSRWLockExclusive(&x)
#define CTHR_RWLOCK_LOCK_READ(x)	AcquireSRWLockShared(&x)
#define CTHR_RWLOCK_UNLOCK_READ(x)	ReleaseSRWLockShared(&x)
#define CTHR_RWLOCK_TRYLOCK_READ(x)	TryAcquireSRWLockShared(&x)

#define CTHR_THREAD_TYPE	HANDLE
#define CTHR_THREAD_RTYPE	unsigned __stdcall
#define CTHR_THREAD_RETURN	_endthreadex(0); return 0;
#define CTHR_THREAD_CREATE(thr, func, arg)	((thr = (HANDLE)_beginthreadex(0, 0, func, arg, 0, 0)) != 0L)
#define CTHR_THREAD_JOIN(thr)			do { WaitForSingleObject(thr, INFINITE); CloseHandle(thr); } while(0)
#define CTHR_THREAD_CLOSE(thr)			CloseHandle((HANDLE)thr);

#else

#include <pthread.h>

#define CTHR_RWLOCK_TYPE	pthread_rwlock_t
#define CTHR_RWLOCK_INIT	PTHREAD_RWLOCK_INITIALIZER
#define CTHR_RWLOCK_LOCK_WRITE(x)	pthread_rwlock_wrlock(&x)
#define CTHR_RWLOCK_UNLOCK_WRITE(x)	pthread_rwlock_unlock(&x)
#define CTHR_RWLOCK_LOCK_READ(x)	pthread_rwlock_rdlock(&x)
#define CTHR_RWLOCK_UNLOCK_READ(x)	pthread_rwlock_unlock(&x)
#define CTHR_RWLOCK_TRYLOCK_READ(x)	(pthread_rwlock_tryrdlock(&x) == 0)

#define CTHR_THREAD_TYPE pthread_t
#define CTHR_THREAD_RTYPE	void *
#define CTHR_THREAD_RETURN	return NULL
#define CTHR_THREAD_CREATE(thr, func, arg)	(pthread_create(&thr, NULL, func, arg) == 0)
#define CTHR_THREAD_JOIN(thr)			pthread_join(thr, NULL)
#define CTHR_THREAD_CLOSE(thr)

#endif
