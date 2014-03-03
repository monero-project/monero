/*
 * libEtPan! -- a mail stuff library
 *
 * Copyright (C) 2001, 2005 - DINH Viet Hoa
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the libEtPan! project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* hmac-md5.h -- HMAC_MD5 functions
 */

/*
 * $Id: hmac-md5.h,v 1.1.1.1 2005/03/18 20:17:28 zautrix Exp $
 */

#ifndef HMAC_MD5_H
#define HMAC_MD5_H 1

namespace md5
{



#define HMAC_MD5_SIZE 16

	/* intermediate MD5 context */
	typedef struct HMAC_MD5_CTX_s {
		MD5_CTX ictx, octx;
	} HMAC_MD5_CTX;

	/* intermediate HMAC state
	*  values stored in network byte order (Big Endian)
	*/
	typedef struct HMAC_MD5_STATE_s {
		UINT4 istate[4];
		UINT4 ostate[4];
	} HMAC_MD5_STATE;

	/* One step hmac computation
	*
	* digest may be same as text or key
	*/
	void hmac_md5(const unsigned char *text, int text_len,
		const unsigned char *key, int key_len,
		unsigned char digest[HMAC_MD5_SIZE]);

	/* create context from key
	*/
	void hmac_md5_init(HMAC_MD5_CTX *hmac,
		const unsigned char *key, int key_len);

	/* precalculate intermediate state from key
	*/
	void hmac_md5_precalc(HMAC_MD5_STATE *hmac,
		const unsigned char *key, int key_len);

	/* initialize context from intermediate state
	*/
	void hmac_md5_import(HMAC_MD5_CTX *hmac, HMAC_MD5_STATE *state);

#define hmac_md5_update(hmac, text, text_len) MD5Update(&(hmac)->ictx, (text), (text_len))

	/* finish hmac from intermediate result.  Intermediate result is zeroed.
	*/
	void hmac_md5_final(unsigned char digest[HMAC_MD5_SIZE],
		HMAC_MD5_CTX *hmac);

}

#endif /* HMAC_MD5_H */
