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

/*
 * $Id: md5global.h,v 1.1.1.1 2005/03/18 20:17:28 zautrix Exp $
 */

/* GLOBAL.H - RSAREF types and constants
 */

#ifndef MD5GLOBAL_H
#define MD5GLOBAL_H

namespace md5
{


	/* PROTOTYPES should be set to one if and only if the compiler supports
	function argument prototyping.
	The following makes PROTOTYPES default to 0 if it has not already
	been defined with C compiler flags.
	*/
#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

	/* POINTER defines a generic pointer type */
	typedef unsigned char *POINTER;

	/* UINT2 defines a two byte word */
	typedef unsigned short int UINT2;

	/* UINT4 defines a four byte word */
	//typedef unsigned long int UINT4;
	typedef unsigned int UINT4;

	/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
	If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
	returns an empty list.
	*/
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

}

#endif
