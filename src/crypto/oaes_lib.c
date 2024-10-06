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
#include <stddef.h>
#include <time.h> 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// OS X, FreeBSD, OpenBSD and NetBSD don't need malloc.h
#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__OpenBSD__) \
  && !defined(__DragonFly__) && !defined(__NetBSD__)
 #include <malloc.h>
#endif

// ANDROID, FreeBSD, OpenBSD and NetBSD also don't need timeb.h
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__ANDROID__) \
  && !defined(__NetBSD__)
 #include <sys/timeb.h>
#else
 #include <sys/time.h>
#endif

#ifdef WIN32
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include "oaes_lib.h"

#define OAES_RKEY_LEN 4
#define OAES_COL_LEN 4
#define OAES_ROUND_BASE 7

static uint8_t oaes_gf_8[] = {
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };

static uint8_t oaes_sub_byte_value[16][16] = {
	// 		0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    a,    b,    c,    d,    e,    f,
	/*0*/	{ 0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76 },
	/*1*/	{ 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0 },
	/*2*/	{ 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15 },
	/*3*/	{ 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75 },
	/*4*/	{ 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84 },
	/*5*/	{ 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf },
	/*6*/	{ 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8 },
	/*7*/	{ 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2 },
	/*8*/	{ 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73 },
	/*9*/	{ 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb },
	/*a*/	{ 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79 },
	/*b*/	{ 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08 },
	/*c*/	{ 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a },
	/*d*/	{ 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e },
	/*e*/	{ 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf },
	/*f*/	{ 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 },
};

static OAES_RET oaes_sub_byte( uint8_t * byte )
{
	size_t _x, _y;
	
	if( NULL == byte )
		return OAES_RET_ARG1;

	_x = _y = *byte;
	_x &= 0x0f;
	_y &= 0xf0;
	_y >>= 4;
	*byte = oaes_sub_byte_value[_y][_x];
	
	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_word_rot_left( uint8_t word[OAES_COL_LEN] )
{
	uint8_t _temp[OAES_COL_LEN];
	
	if( NULL == word )
		return OAES_RET_ARG1;

	memcpy( _temp, word + 1, OAES_COL_LEN - 1 );
	_temp[OAES_COL_LEN - 1] = word[0];
	memcpy( word, _temp, OAES_COL_LEN );
	
	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_key_destroy( oaes_key ** key )
{
	if( NULL == *key )
		return OAES_RET_SUCCESS;
	
	if( (*key)->data )
	{
		free( (*key)->data );
		(*key)->data = NULL;
	}
	
	if( (*key)->exp_data )
	{
		free( (*key)->exp_data );
		(*key)->exp_data = NULL;
	}
	
	(*key)->data_len = 0;
	(*key)->exp_data_len = 0;
	(*key)->num_keys = 0;
	(*key)->key_base = 0;
	free( *key );
	*key = NULL;
	
	return OAES_RET_SUCCESS;
}

static OAES_RET oaes_key_expand( OAES_CTX * ctx )
{
	size_t _i, _j;
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == _ctx->key )
		return OAES_RET_NOKEY;
	
	_ctx->key->key_base = _ctx->key->data_len / OAES_RKEY_LEN;
	_ctx->key->num_keys =  _ctx->key->key_base + OAES_ROUND_BASE;
					
	_ctx->key->exp_data_len = _ctx->key->num_keys * OAES_RKEY_LEN * OAES_COL_LEN;
	_ctx->key->exp_data = (uint8_t *)
			calloc( _ctx->key->exp_data_len, sizeof( uint8_t ));
	
	if( NULL == _ctx->key->exp_data )
		return OAES_RET_MEM;
	
	// the first _ctx->key->data_len are a direct copy
	memcpy( _ctx->key->exp_data, _ctx->key->data, _ctx->key->data_len );

	// apply ExpandKey algorithm for remainder
	for( _i = _ctx->key->key_base; _i < _ctx->key->num_keys * OAES_RKEY_LEN; _i++ )
	{
		uint8_t _temp[OAES_COL_LEN];
		
		memcpy( _temp,
				_ctx->key->exp_data + ( _i - 1 ) * OAES_RKEY_LEN, OAES_COL_LEN );
		
		// transform key column
		if( 0 == _i % _ctx->key->key_base )
		{
			oaes_word_rot_left( _temp );

			for( _j = 0; _j < OAES_COL_LEN; _j++ )
				oaes_sub_byte( _temp + _j );

			_temp[0] = _temp[0] ^ oaes_gf_8[ _i / _ctx->key->key_base - 1 ];
		}
		else if( _ctx->key->key_base > 6 && 4 == _i % _ctx->key->key_base )
		{
			for( _j = 0; _j < OAES_COL_LEN; _j++ )
				oaes_sub_byte( _temp + _j );
		}
		
		for( _j = 0; _j < OAES_COL_LEN; _j++ )
		{
			_ctx->key->exp_data[ _i * OAES_RKEY_LEN + _j ] =
					_ctx->key->exp_data[ ( _i - _ctx->key->key_base ) *
					OAES_RKEY_LEN + _j ] ^ _temp[_j];
		}
	}
	
	return OAES_RET_SUCCESS;
}

OAES_RET oaes_key_import_data( OAES_CTX * ctx,
		const uint8_t * data, size_t data_len )
{
	oaes_ctx * _ctx = (oaes_ctx *) ctx;
	OAES_RET _rc = OAES_RET_SUCCESS;
	
	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == data )
		return OAES_RET_ARG2;
	
	switch( data_len )
	{
		case 16:
		case 24:
		case 32:
			break;
		default:
			return OAES_RET_ARG3;
	}
	
	if( _ctx->key )
		oaes_key_destroy( &(_ctx->key) );
	
	_ctx->key = (oaes_key *) calloc( sizeof( oaes_key ), 1 );
	
	if( NULL == _ctx->key )
		return OAES_RET_MEM;
	
	_ctx->key->data_len = data_len;
	_ctx->key->data = (uint8_t *)
			calloc( data_len, sizeof( uint8_t ));
	
	if( NULL == _ctx->key->data )
	{
		oaes_key_destroy( &(_ctx->key) );
		return OAES_RET_MEM;
	}

	memcpy( _ctx->key->data, data, data_len );
	_rc = _rc || oaes_key_expand( ctx );
	
	if( _rc != OAES_RET_SUCCESS )
	{
		oaes_key_destroy( &(_ctx->key) );
		return _rc;
	}
	
	return OAES_RET_SUCCESS;
}

OAES_CTX * oaes_alloc(void)
{
	oaes_ctx * _ctx = (oaes_ctx *) calloc( sizeof( oaes_ctx ), 1 );
	
	if( NULL == _ctx )
		return NULL;

	_ctx->key = NULL;

	return (OAES_CTX *) _ctx;
}

OAES_RET oaes_free( OAES_CTX ** ctx )
{
	oaes_ctx ** _ctx = (oaes_ctx **) ctx;

	if( NULL == _ctx )
		return OAES_RET_ARG1;
	
	if( NULL == *_ctx )
		return OAES_RET_SUCCESS;
	
	if( (*_ctx)->key )
		oaes_key_destroy( &((*_ctx)->key) );

	free( *_ctx );
	*_ctx = NULL;

	return OAES_RET_SUCCESS;
}
