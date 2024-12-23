/* 
 * ---------------------------------------------------------------------------
 * OpenAES License
 * ---------------------------------------------------------------------------
 * Copyright (c) 2012, Nabil S. Al Ramli, www.nalramli.com
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ---------------------------------------------------------------------------
 */

#ifndef _OAES_LIB_H
#define _OAES_LIB_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus 
extern "C" {
#endif

#ifdef _WIN32
#	ifdef OAES_SHARED
#		ifdef oaes_lib_EXPORTS
#			define OAES_API __declspec(dllexport)
#		else
#			define OAES_API __declspec(dllimport)
#		endif
#	else
#		define OAES_API
#	endif
#else
#	define OAES_API
#endif // WIN32

#define OAES_VERSION "0.8.1"
#define OAES_BLOCK_SIZE 16

typedef void OAES_CTX;

typedef enum
{
	OAES_RET_FIRST = 0,
	OAES_RET_SUCCESS = 0,
	OAES_RET_UNKNOWN,
	OAES_RET_ARG1,
	OAES_RET_ARG2,
	OAES_RET_ARG3,
	OAES_RET_ARG4,
	OAES_RET_ARG5,
	OAES_RET_NOKEY,
	OAES_RET_MEM,
	OAES_RET_BUF,
	OAES_RET_HEADER,
	OAES_RET_COUNT
} OAES_RET;

typedef uint16_t OAES_OPTION;

typedef struct _oaes_key
{
  size_t data_len;
  uint8_t *data;
  size_t exp_data_len;
  uint8_t *exp_data;
  size_t num_keys;
  size_t key_base;
} oaes_key;

typedef struct _oaes_ctx
{
  oaes_key * key;
} oaes_ctx;
/*
 * // usage:
 * 
 * OAES_CTX * ctx = oaes_alloc();
 * .
 * .
 * .
 * {
 *   oaes_gen_key_xxx( ctx );
 *   {
 *     oaes_key_export( ctx, _buf, &_buf_len );
 *     // or
 *     oaes_key_export_data( ctx, _buf, &_buf_len );\
 *   }
 * }
 * // or
 * {
 *   oaes_key_import( ctx, _buf, _buf_len );
 *   // or
 *   oaes_key_import_data( ctx, _buf, _buf_len );
 * }
 * .
 * .
 * .
 * oaes_encrypt( ctx, m, m_len, c, &c_len );
 * .
 * .
 * .
 * oaes_decrypt( ctx, c, c_len, m, &m_len );
 * .
 * .
 * .
 * oaes_free( &ctx );
 */

OAES_API OAES_CTX * oaes_alloc(void);

OAES_API OAES_RET oaes_free( OAES_CTX ** ctx );

// directly import data into key
OAES_API OAES_RET oaes_key_import_data( OAES_CTX * ctx,
		const uint8_t * data, size_t data_len );

#ifdef __cplusplus 
}
#endif

#endif // _OAES_LIB_H
