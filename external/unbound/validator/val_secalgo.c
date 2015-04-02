/*
 * validator/val_secalgo.c - validator security algorithm functions.
 *
 * Copyright (c) 2012, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * This file contains helper functions for the validator module.
 * These functions take raw data buffers, formatted for crypto verification,
 * and do the library calls (for the crypto library in use).
 */
#include "config.h"
/* packed_rrset on top to define enum types (forced by c99 standard) */
#include "util/data/packed_rrset.h"
#include "validator/val_secalgo.h"
#include "util/log.h"
#include "sldns/rrdef.h"
#include "sldns/keyraw.h"
#include "sldns/sbuffer.h"

#if !defined(HAVE_SSL) && !defined(HAVE_NSS)
#error "Need crypto library to do digital signature cryptography"
#endif

/* OpenSSL implementation */
#ifdef HAVE_SSL
#ifdef HAVE_OPENSSL_ERR_H
#include <openssl/err.h>
#endif

#ifdef HAVE_OPENSSL_RAND_H
#include <openssl/rand.h>
#endif

#ifdef HAVE_OPENSSL_CONF_H
#include <openssl/conf.h>
#endif

#ifdef HAVE_OPENSSL_ENGINE_H
#include <openssl/engine.h>
#endif

/**
 * Return size of DS digest according to its hash algorithm.
 * @param algo: DS digest algo.
 * @return size in bytes of digest, or 0 if not supported. 
 */
size_t
ds_digest_size_supported(int algo)
{
	switch(algo) {
#ifdef HAVE_EVP_SHA1
		case LDNS_SHA1:
			return SHA_DIGEST_LENGTH;
#endif
#ifdef HAVE_EVP_SHA256
		case LDNS_SHA256:
			return SHA256_DIGEST_LENGTH;
#endif
#ifdef USE_GOST
		case LDNS_HASH_GOST:
			if(EVP_get_digestbyname("md_gost94"))
				return 32;
			else	return 0;
#endif
#ifdef USE_ECDSA
		case LDNS_SHA384:
			return SHA384_DIGEST_LENGTH;
#endif
		default: break;
	}
	return 0;
}

#ifdef USE_GOST
/** Perform GOST hash */
static int
do_gost94(unsigned char* data, size_t len, unsigned char* dest)
{
	const EVP_MD* md = EVP_get_digestbyname("md_gost94");
	if(!md) 
		return 0;
	return sldns_digest_evp(data, (unsigned int)len, dest, md);
}
#endif

int
secalgo_ds_digest(int algo, unsigned char* buf, size_t len,
	unsigned char* res)
{
	switch(algo) {
#ifdef HAVE_EVP_SHA1
		case LDNS_SHA1:
			(void)SHA1(buf, len, res);
			return 1;
#endif
#ifdef HAVE_EVP_SHA256
		case LDNS_SHA256:
			(void)SHA256(buf, len, res);
			return 1;
#endif
#ifdef USE_GOST
		case LDNS_HASH_GOST:
			if(do_gost94(buf, len, res))
				return 1;
			break;
#endif
#ifdef USE_ECDSA
		case LDNS_SHA384:
			(void)SHA384(buf, len, res);
			return 1;
#endif
		default: 
			verbose(VERB_QUERY, "unknown DS digest algorithm %d", 
				algo);
			break;
	}
	return 0;
}

/** return true if DNSKEY algorithm id is supported */
int
dnskey_algo_id_is_supported(int id)
{
	switch(id) {
	case LDNS_RSAMD5:
		/* RFC 6725 deprecates RSAMD5 */
		return 0;
	case LDNS_DSA:
	case LDNS_DSA_NSEC3:
	case LDNS_RSASHA1:
	case LDNS_RSASHA1_NSEC3:
#if defined(HAVE_EVP_SHA256) && defined(USE_SHA2)
	case LDNS_RSASHA256:
#endif
#if defined(HAVE_EVP_SHA512) && defined(USE_SHA2)
	case LDNS_RSASHA512:
#endif
#ifdef USE_ECDSA
	case LDNS_ECDSAP256SHA256:
	case LDNS_ECDSAP384SHA384:
#endif
		return 1;
#ifdef USE_GOST
	case LDNS_ECC_GOST:
		/* we support GOST if it can be loaded */
		return sldns_key_EVP_load_gost_id();
#endif
	default:
		return 0;
	}
}

/**
 * Output a libcrypto openssl error to the logfile.
 * @param str: string to add to it.
 * @param e: the error to output, error number from ERR_get_error().
 */
static void
log_crypto_error(const char* str, unsigned long e)
{
	char buf[128];
	/* or use ERR_error_string if ERR_error_string_n is not avail TODO */
	ERR_error_string_n(e, buf, sizeof(buf));
	/* buf now contains */
	/* error:[error code]:[library name]:[function name]:[reason string] */
	log_err("%s crypto %s", str, buf);
}

/**
 * Setup DSA key digest in DER encoding ... 
 * @param sig: input is signature output alloced ptr (unless failure).
 * 	caller must free alloced ptr if this routine returns true.
 * @param len: input is initial siglen, output is output len.
 * @return false on failure.
 */
static int
setup_dsa_sig(unsigned char** sig, unsigned int* len)
{
	unsigned char* orig = *sig;
	unsigned int origlen = *len;
	int newlen;
	BIGNUM *R, *S;
	DSA_SIG *dsasig;

	/* extract the R and S field from the sig buffer */
	if(origlen < 1 + 2*SHA_DIGEST_LENGTH)
		return 0;
	R = BN_new();
	if(!R) return 0;
	(void) BN_bin2bn(orig + 1, SHA_DIGEST_LENGTH, R);
	S = BN_new();
	if(!S) return 0;
	(void) BN_bin2bn(orig + 21, SHA_DIGEST_LENGTH, S);
	dsasig = DSA_SIG_new();
	if(!dsasig) return 0;

	dsasig->r = R;
	dsasig->s = S;
	*sig = NULL;
	newlen = i2d_DSA_SIG(dsasig, sig);
	if(newlen < 0) {
		DSA_SIG_free(dsasig);
		free(*sig);
		return 0;
	}
	*len = (unsigned int)newlen;
	DSA_SIG_free(dsasig);
	return 1;
}

#ifdef USE_ECDSA
/**
 * Setup the ECDSA signature in its encoding that the library wants.
 * Converts from plain numbers to ASN formatted.
 * @param sig: input is signature, output alloced ptr (unless failure).
 * 	caller must free alloced ptr if this routine returns true.
 * @param len: input is initial siglen, output is output len.
 * @return false on failure.
 */
static int
setup_ecdsa_sig(unsigned char** sig, unsigned int* len)
{
	ECDSA_SIG* ecdsa_sig;
	int newlen;
	int bnsize = (int)((*len)/2);
	/* if too short or not even length, fails */
	if(*len < 16 || bnsize*2 != (int)*len)
		return 0;
	/* use the raw data to parse two evenly long BIGNUMs, "r | s". */
	ecdsa_sig = ECDSA_SIG_new();
	if(!ecdsa_sig) return 0;
	ecdsa_sig->r = BN_bin2bn(*sig, bnsize, ecdsa_sig->r);
	ecdsa_sig->s = BN_bin2bn(*sig+bnsize, bnsize, ecdsa_sig->s);
	if(!ecdsa_sig->r || !ecdsa_sig->s) {
		ECDSA_SIG_free(ecdsa_sig);
		return 0;
	}

	/* spool it into ASN format */
	*sig = NULL;
	newlen = i2d_ECDSA_SIG(ecdsa_sig, sig);
	if(newlen <= 0) {
		ECDSA_SIG_free(ecdsa_sig);
		free(*sig);
		return 0;
	}
	*len = (unsigned int)newlen;
	ECDSA_SIG_free(ecdsa_sig);
	return 1;
}
#endif /* USE_ECDSA */

/**
 * Setup key and digest for verification. Adjust sig if necessary.
 *
 * @param algo: key algorithm
 * @param evp_key: EVP PKEY public key to create.
 * @param digest_type: digest type to use
 * @param key: key to setup for.
 * @param keylen: length of key.
 * @return false on failure.
 */
static int
setup_key_digest(int algo, EVP_PKEY** evp_key, const EVP_MD** digest_type, 
	unsigned char* key, size_t keylen)
{
	DSA* dsa;
	RSA* rsa;

	switch(algo) {
		case LDNS_DSA:
		case LDNS_DSA_NSEC3:
			*evp_key = EVP_PKEY_new();
			if(!*evp_key) {
				log_err("verify: malloc failure in crypto");
				return 0;
			}
			dsa = sldns_key_buf2dsa_raw(key, keylen);
			if(!dsa) {
				verbose(VERB_QUERY, "verify: "
					"sldns_key_buf2dsa_raw failed");
				return 0;
			}
			if(EVP_PKEY_assign_DSA(*evp_key, dsa) == 0) {
				verbose(VERB_QUERY, "verify: "
					"EVP_PKEY_assign_DSA failed");
				return 0;
			}
			*digest_type = EVP_dss1();

			break;
		case LDNS_RSASHA1:
		case LDNS_RSASHA1_NSEC3:
#if defined(HAVE_EVP_SHA256) && defined(USE_SHA2)
		case LDNS_RSASHA256:
#endif
#if defined(HAVE_EVP_SHA512) && defined(USE_SHA2)
		case LDNS_RSASHA512:
#endif
			*evp_key = EVP_PKEY_new();
			if(!*evp_key) {
				log_err("verify: malloc failure in crypto");
				return 0;
			}
			rsa = sldns_key_buf2rsa_raw(key, keylen);
			if(!rsa) {
				verbose(VERB_QUERY, "verify: "
					"sldns_key_buf2rsa_raw SHA failed");
				return 0;
			}
			if(EVP_PKEY_assign_RSA(*evp_key, rsa) == 0) {
				verbose(VERB_QUERY, "verify: "
					"EVP_PKEY_assign_RSA SHA failed");
				return 0;
			}

			/* select SHA version */
#if defined(HAVE_EVP_SHA256) && defined(USE_SHA2)
			if(algo == LDNS_RSASHA256)
				*digest_type = EVP_sha256();
			else
#endif
#if defined(HAVE_EVP_SHA512) && defined(USE_SHA2)
				if(algo == LDNS_RSASHA512)
				*digest_type = EVP_sha512();
			else
#endif
				*digest_type = EVP_sha1();

			break;
		case LDNS_RSAMD5:
			*evp_key = EVP_PKEY_new();
			if(!*evp_key) {
				log_err("verify: malloc failure in crypto");
				return 0;
			}
			rsa = sldns_key_buf2rsa_raw(key, keylen);
			if(!rsa) {
				verbose(VERB_QUERY, "verify: "
					"sldns_key_buf2rsa_raw MD5 failed");
				return 0;
			}
			if(EVP_PKEY_assign_RSA(*evp_key, rsa) == 0) {
				verbose(VERB_QUERY, "verify: "
					"EVP_PKEY_assign_RSA MD5 failed");
				return 0;
			}
			*digest_type = EVP_md5();

			break;
#ifdef USE_GOST
		case LDNS_ECC_GOST:
			*evp_key = sldns_gost2pkey_raw(key, keylen);
			if(!*evp_key) {
				verbose(VERB_QUERY, "verify: "
					"sldns_gost2pkey_raw failed");
				return 0;
			}
			*digest_type = EVP_get_digestbyname("md_gost94");
			if(!*digest_type) {
				verbose(VERB_QUERY, "verify: "
					"EVP_getdigest md_gost94 failed");
				return 0;
			}
			break;
#endif
#ifdef USE_ECDSA
		case LDNS_ECDSAP256SHA256:
			*evp_key = sldns_ecdsa2pkey_raw(key, keylen,
				LDNS_ECDSAP256SHA256);
			if(!*evp_key) {
				verbose(VERB_QUERY, "verify: "
					"sldns_ecdsa2pkey_raw failed");
				return 0;
			}
#ifdef USE_ECDSA_EVP_WORKAROUND
			/* openssl before 1.0.0 fixes RSA with the SHA256
			 * hash in EVP.  We create one for ecdsa_sha256 */
			{
				static int md_ecdsa_256_done = 0;
				static EVP_MD md;
				if(!md_ecdsa_256_done) {
					EVP_MD m = *EVP_sha256();
					md_ecdsa_256_done = 1;
					m.required_pkey_type[0] = (*evp_key)->type;
					m.verify = (void*)ECDSA_verify;
					md = m;
				}
				*digest_type = &md;
			}
#else
			*digest_type = EVP_sha256();
#endif
			break;
		case LDNS_ECDSAP384SHA384:
			*evp_key = sldns_ecdsa2pkey_raw(key, keylen,
				LDNS_ECDSAP384SHA384);
			if(!*evp_key) {
				verbose(VERB_QUERY, "verify: "
					"sldns_ecdsa2pkey_raw failed");
				return 0;
			}
#ifdef USE_ECDSA_EVP_WORKAROUND
			/* openssl before 1.0.0 fixes RSA with the SHA384
			 * hash in EVP.  We create one for ecdsa_sha384 */
			{
				static int md_ecdsa_384_done = 0;
				static EVP_MD md;
				if(!md_ecdsa_384_done) {
					EVP_MD m = *EVP_sha384();
					md_ecdsa_384_done = 1;
					m.required_pkey_type[0] = (*evp_key)->type;
					m.verify = (void*)ECDSA_verify;
					md = m;
				}
				*digest_type = &md;
			}
#else
			*digest_type = EVP_sha384();
#endif
			break;
#endif /* USE_ECDSA */
		default:
			verbose(VERB_QUERY, "verify: unknown algorithm %d", 
				algo);
			return 0;
	}
	return 1;
}

/**
 * Check a canonical sig+rrset and signature against a dnskey
 * @param buf: buffer with data to verify, the first rrsig part and the
 *	canonicalized rrset.
 * @param algo: DNSKEY algorithm.
 * @param sigblock: signature rdata field from RRSIG
 * @param sigblock_len: length of sigblock data.
 * @param key: public key data from DNSKEY RR.
 * @param keylen: length of keydata.
 * @param reason: bogus reason in more detail.
 * @return secure if verification succeeded, bogus on crypto failure,
 *	unchecked on format errors and alloc failures.
 */
enum sec_status
verify_canonrrset(sldns_buffer* buf, int algo, unsigned char* sigblock, 
	unsigned int sigblock_len, unsigned char* key, unsigned int keylen,
	char** reason)
{
	const EVP_MD *digest_type;
	EVP_MD_CTX ctx;
	int res, dofree = 0;
	EVP_PKEY *evp_key = NULL;
	
	if(!setup_key_digest(algo, &evp_key, &digest_type, key, keylen)) {
		verbose(VERB_QUERY, "verify: failed to setup key");
		*reason = "use of key for crypto failed";
		EVP_PKEY_free(evp_key);
		return sec_status_bogus;
	}
	/* if it is a DSA signature in bind format, convert to DER format */
	if((algo == LDNS_DSA || algo == LDNS_DSA_NSEC3) && 
		sigblock_len == 1+2*SHA_DIGEST_LENGTH) {
		if(!setup_dsa_sig(&sigblock, &sigblock_len)) {
			verbose(VERB_QUERY, "verify: failed to setup DSA sig");
			*reason = "use of key for DSA crypto failed";
			EVP_PKEY_free(evp_key);
			return sec_status_bogus;
		}
		dofree = 1;
	}
#ifdef USE_ECDSA
	else if(algo == LDNS_ECDSAP256SHA256 || algo == LDNS_ECDSAP384SHA384) {
		/* EVP uses ASN prefix on sig, which is not in the wire data */
		if(!setup_ecdsa_sig(&sigblock, &sigblock_len)) {
			verbose(VERB_QUERY, "verify: failed to setup ECDSA sig");
			*reason = "use of signature for ECDSA crypto failed";
			EVP_PKEY_free(evp_key);
			return sec_status_bogus;
		}
		dofree = 1;
	}
#endif /* USE_ECDSA */

	/* do the signature cryptography work */
	EVP_MD_CTX_init(&ctx);
	if(EVP_VerifyInit(&ctx, digest_type) == 0) {
		verbose(VERB_QUERY, "verify: EVP_VerifyInit failed");
		EVP_PKEY_free(evp_key);
		if(dofree) free(sigblock);
		return sec_status_unchecked;
	}
	if(EVP_VerifyUpdate(&ctx, (unsigned char*)sldns_buffer_begin(buf), 
		(unsigned int)sldns_buffer_limit(buf)) == 0) {
		verbose(VERB_QUERY, "verify: EVP_VerifyUpdate failed");
		EVP_PKEY_free(evp_key);
		if(dofree) free(sigblock);
		return sec_status_unchecked;
	}

	res = EVP_VerifyFinal(&ctx, sigblock, sigblock_len, evp_key);
	if(EVP_MD_CTX_cleanup(&ctx) == 0) {
		verbose(VERB_QUERY, "verify: EVP_MD_CTX_cleanup failed");
		EVP_PKEY_free(evp_key);
		if(dofree) free(sigblock);
		return sec_status_unchecked;
	}
	EVP_PKEY_free(evp_key);

	if(dofree)
		free(sigblock);

	if(res == 1) {
		return sec_status_secure;
	} else if(res == 0) {
		verbose(VERB_QUERY, "verify: signature mismatch");
		*reason = "signature crypto failed";
		return sec_status_bogus;
	}

	log_crypto_error("verify:", ERR_get_error());
	return sec_status_unchecked;
}

/**************************************************/
#elif defined(HAVE_NSS)
/* libnss implementation */
/* nss3 */
#include "sechash.h"
#include "pk11pub.h"
#include "keyhi.h"
#include "secerr.h"
#include "cryptohi.h"
/* nspr4 */
#include "prerror.h"

size_t
ds_digest_size_supported(int algo)
{
	/* uses libNSS */
	switch(algo) {
		case LDNS_SHA1:
			return SHA1_LENGTH;
#ifdef USE_SHA2
		case LDNS_SHA256:
			return SHA256_LENGTH;
#endif
#ifdef USE_ECDSA
		case LDNS_SHA384:
			return SHA384_LENGTH;
#endif
		/* GOST not supported in NSS */
		case LDNS_HASH_GOST:
		default: break;
	}
	return 0;
}

int
secalgo_ds_digest(int algo, unsigned char* buf, size_t len,
	unsigned char* res)
{
	/* uses libNSS */
	switch(algo) {
		case LDNS_SHA1:
			return HASH_HashBuf(HASH_AlgSHA1, res, buf, len)
				== SECSuccess;
#if defined(USE_SHA2)
		case LDNS_SHA256:
			return HASH_HashBuf(HASH_AlgSHA256, res, buf, len)
				== SECSuccess;
#endif
#ifdef USE_ECDSA
		case LDNS_SHA384:
			return HASH_HashBuf(HASH_AlgSHA384, res, buf, len)
				== SECSuccess;
#endif
		case LDNS_HASH_GOST:
		default: 
			verbose(VERB_QUERY, "unknown DS digest algorithm %d", 
				algo);
			break;
	}
	return 0;
}

int
dnskey_algo_id_is_supported(int id)
{
	/* uses libNSS */
	switch(id) {
	case LDNS_RSAMD5:
		/* RFC 6725 deprecates RSAMD5 */
		return 0;
	case LDNS_DSA:
	case LDNS_DSA_NSEC3:
	case LDNS_RSASHA1:
	case LDNS_RSASHA1_NSEC3:
#ifdef USE_SHA2
	case LDNS_RSASHA256:
#endif
#ifdef USE_SHA2
	case LDNS_RSASHA512:
#endif
		return 1;
#ifdef USE_ECDSA
	case LDNS_ECDSAP256SHA256:
	case LDNS_ECDSAP384SHA384:
		return PK11_TokenExists(CKM_ECDSA);
#endif
	case LDNS_ECC_GOST:
	default:
		return 0;
	}
}

/* return a new public key for NSS */
static SECKEYPublicKey* nss_key_create(KeyType ktype)
{
	SECKEYPublicKey* key;
	PLArenaPool* arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if(!arena) {
		log_err("out of memory, PORT_NewArena failed");
		return NULL;
	}
	key = PORT_ArenaZNew(arena, SECKEYPublicKey);
	if(!key) {
		log_err("out of memory, PORT_ArenaZNew failed");
		PORT_FreeArena(arena, PR_FALSE);
		return NULL;
	}
	key->arena = arena;
	key->keyType = ktype;
	key->pkcs11Slot = NULL;
	key->pkcs11ID = CK_INVALID_HANDLE;
	return key;
}

static SECKEYPublicKey* nss_buf2ecdsa(unsigned char* key, size_t len, int algo)
{
	SECKEYPublicKey* pk;
	SECItem pub = {siBuffer, NULL, 0};
	SECItem params = {siBuffer, NULL, 0};
	static unsigned char param256[] = {
		/* OBJECTIDENTIFIER 1.2.840.10045.3.1.7 (P-256)
		 * {iso(1) member-body(2) us(840) ansi-x962(10045) curves(3) prime(1) prime256v1(7)} */
		0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07
	};
	static unsigned char param384[] = {
		/* OBJECTIDENTIFIER 1.3.132.0.34 (P-384)
		 * {iso(1) identified-organization(3) certicom(132) curve(0) ansip384r1(34)} */
		0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x22
	};
	unsigned char buf[256+2]; /* sufficient for 2*384/8+1 */

	/* check length, which uncompressed must be 2 bignums */
	if(algo == LDNS_ECDSAP256SHA256) {
		if(len != 2*256/8) return NULL;
		/* ECCurve_X9_62_PRIME_256V1 */
	} else if(algo == LDNS_ECDSAP384SHA384) {
		if(len != 2*384/8) return NULL;
		/* ECCurve_X9_62_PRIME_384R1 */
	} else    return NULL;

	buf[0] = 0x04; /* POINT_FORM_UNCOMPRESSED */
	memmove(buf+1, key, len);
	pub.data = buf;
	pub.len = len+1;
	if(algo == LDNS_ECDSAP256SHA256) {
		params.data = param256;
		params.len = sizeof(param256);
	} else {
		params.data = param384;
		params.len = sizeof(param384);
	}

	pk = nss_key_create(ecKey);
	if(!pk)
		return NULL;
	pk->u.ec.size = (len/2)*8;
	if(SECITEM_CopyItem(pk->arena, &pk->u.ec.publicValue, &pub)) {
		SECKEY_DestroyPublicKey(pk);
		return NULL;
	}
	if(SECITEM_CopyItem(pk->arena, &pk->u.ec.DEREncodedParams, &params)) {
		SECKEY_DestroyPublicKey(pk);
		return NULL;
	}

	return pk;
}

static SECKEYPublicKey* nss_buf2dsa(unsigned char* key, size_t len)
{
	SECKEYPublicKey* pk;
	uint8_t T;
	uint16_t length;
	uint16_t offset;
	SECItem Q = {siBuffer, NULL, 0};
	SECItem P = {siBuffer, NULL, 0};
	SECItem G = {siBuffer, NULL, 0};
	SECItem Y = {siBuffer, NULL, 0};

	if(len == 0)
		return NULL;
	T = (uint8_t)key[0];
	length = (64 + T * 8);
	offset = 1;

	if (T > 8) {
		return NULL;
	}
	if(len < (size_t)1 + SHA1_LENGTH + 3*length)
		return NULL;

	Q.data = key+offset;
	Q.len = SHA1_LENGTH;
	offset += SHA1_LENGTH;

	P.data = key+offset;
	P.len = length;
	offset += length;

	G.data = key+offset;
	G.len = length;
	offset += length;

	Y.data = key+offset;
	Y.len = length;
	offset += length;

	pk = nss_key_create(dsaKey);
	if(!pk)
		return NULL;
	if(SECITEM_CopyItem(pk->arena, &pk->u.dsa.params.prime, &P)) {
		SECKEY_DestroyPublicKey(pk);
		return NULL;
	}
	if(SECITEM_CopyItem(pk->arena, &pk->u.dsa.params.subPrime, &Q)) {
		SECKEY_DestroyPublicKey(pk);
		return NULL;
	}
	if(SECITEM_CopyItem(pk->arena, &pk->u.dsa.params.base, &G)) {
		SECKEY_DestroyPublicKey(pk);
		return NULL;
	}
	if(SECITEM_CopyItem(pk->arena, &pk->u.dsa.publicValue, &Y)) {
		SECKEY_DestroyPublicKey(pk);
		return NULL;
	}
	return pk;
}

static SECKEYPublicKey* nss_buf2rsa(unsigned char* key, size_t len)
{
	SECKEYPublicKey* pk;
	uint16_t exp;
	uint16_t offset;
	uint16_t int16;
	SECItem modulus = {siBuffer, NULL, 0};
	SECItem exponent = {siBuffer, NULL, 0};
	if(len == 0)
		return NULL;
	if(key[0] == 0) {
		if(len < 3)
			return NULL;
		/* the exponent is too large so it's places further */
		memmove(&int16, key+1, 2);
		exp = ntohs(int16);
		offset = 3;
	} else {
		exp = key[0];
		offset = 1;
	}

	/* key length at least one */
	if(len < (size_t)offset + exp + 1)
		return NULL;
	
	exponent.data = key+offset;
	exponent.len = exp;
	offset += exp;
	modulus.data = key+offset;
	modulus.len = (len - offset);

	pk = nss_key_create(rsaKey);
	if(!pk)
		return NULL;
	if(SECITEM_CopyItem(pk->arena, &pk->u.rsa.modulus, &modulus)) {
		SECKEY_DestroyPublicKey(pk);
		return NULL;
	}
	if(SECITEM_CopyItem(pk->arena, &pk->u.rsa.publicExponent, &exponent)) {
		SECKEY_DestroyPublicKey(pk);
		return NULL;
	}
	return pk;
}

/**
 * Setup key and digest for verification. Adjust sig if necessary.
 *
 * @param algo: key algorithm
 * @param evp_key: EVP PKEY public key to create.
 * @param digest_type: digest type to use
 * @param key: key to setup for.
 * @param keylen: length of key.
 * @param prefix: if returned, the ASN prefix for the hashblob.
 * @param prefixlen: length of the prefix.
 * @return false on failure.
 */
static int
nss_setup_key_digest(int algo, SECKEYPublicKey** pubkey, HASH_HashType* htype,
	unsigned char* key, size_t keylen, unsigned char** prefix,
	size_t* prefixlen)
{
	/* uses libNSS */

	/* hash prefix for md5, RFC2537 */
	static unsigned char p_md5[] = {0x30, 0x20, 0x30, 0x0c, 0x06, 0x08, 0x2a,
	0x86, 0x48, 0x86, 0xf7, 0x0d, 0x02, 0x05, 0x05, 0x00, 0x04, 0x10};
	/* hash prefix to prepend to hash output, from RFC3110 */
	static unsigned char p_sha1[] = {0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2B,
		0x0E, 0x03, 0x02, 0x1A, 0x05, 0x00, 0x04, 0x14};
	/* from RFC5702 */
	static unsigned char p_sha256[] = {0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60,
	0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20};
	static unsigned char p_sha512[] = {0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60,
	0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40};
	/* from RFC6234 */
	/* for future RSASHA384 .. 
	static unsigned char p_sha384[] = {0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60,
	0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, 0x05, 0x00, 0x04, 0x30};
	*/

	switch(algo) {
		case LDNS_DSA:
		case LDNS_DSA_NSEC3:
			*pubkey = nss_buf2dsa(key, keylen);
			if(!*pubkey) {
				log_err("verify: malloc failure in crypto");
				return 0;
			}
			*htype = HASH_AlgSHA1;
			/* no prefix for DSA verification */
			break;
		case LDNS_RSASHA1:
		case LDNS_RSASHA1_NSEC3:
#ifdef USE_SHA2
		case LDNS_RSASHA256:
#endif
#ifdef USE_SHA2
		case LDNS_RSASHA512:
#endif
			*pubkey = nss_buf2rsa(key, keylen);
			if(!*pubkey) {
				log_err("verify: malloc failure in crypto");
				return 0;
			}
			/* select SHA version */
#ifdef USE_SHA2
			if(algo == LDNS_RSASHA256) {
				*htype = HASH_AlgSHA256;
				*prefix = p_sha256;
				*prefixlen = sizeof(p_sha256);
			} else
#endif
#ifdef USE_SHA2
				if(algo == LDNS_RSASHA512) {
				*htype = HASH_AlgSHA512;
				*prefix = p_sha512;
				*prefixlen = sizeof(p_sha512);
			} else
#endif
			{
				*htype = HASH_AlgSHA1;
				*prefix = p_sha1;
				*prefixlen = sizeof(p_sha1);
			}

			break;
		case LDNS_RSAMD5:
			*pubkey = nss_buf2rsa(key, keylen);
			if(!*pubkey) {
				log_err("verify: malloc failure in crypto");
				return 0;
			}
			*htype = HASH_AlgMD5;
			*prefix = p_md5;
			*prefixlen = sizeof(p_md5);

			break;
#ifdef USE_ECDSA
		case LDNS_ECDSAP256SHA256:
			*pubkey = nss_buf2ecdsa(key, keylen,
				LDNS_ECDSAP256SHA256);
			if(!*pubkey) {
				log_err("verify: malloc failure in crypto");
				return 0;
			}
			*htype = HASH_AlgSHA256;
			/* no prefix for DSA verification */
			break;
		case LDNS_ECDSAP384SHA384:
			*pubkey = nss_buf2ecdsa(key, keylen,
				LDNS_ECDSAP384SHA384);
			if(!*pubkey) {
				log_err("verify: malloc failure in crypto");
				return 0;
			}
			*htype = HASH_AlgSHA384;
			/* no prefix for DSA verification */
			break;
#endif /* USE_ECDSA */
		case LDNS_ECC_GOST:
		default:
			verbose(VERB_QUERY, "verify: unknown algorithm %d", 
				algo);
			return 0;
	}
	return 1;
}

/**
 * Check a canonical sig+rrset and signature against a dnskey
 * @param buf: buffer with data to verify, the first rrsig part and the
 *	canonicalized rrset.
 * @param algo: DNSKEY algorithm.
 * @param sigblock: signature rdata field from RRSIG
 * @param sigblock_len: length of sigblock data.
 * @param key: public key data from DNSKEY RR.
 * @param keylen: length of keydata.
 * @param reason: bogus reason in more detail.
 * @return secure if verification succeeded, bogus on crypto failure,
 *	unchecked on format errors and alloc failures.
 */
enum sec_status
verify_canonrrset(sldns_buffer* buf, int algo, unsigned char* sigblock, 
	unsigned int sigblock_len, unsigned char* key, unsigned int keylen,
	char** reason)
{
	/* uses libNSS */
	/* large enough for the different hashes */
	unsigned char hash[HASH_LENGTH_MAX];
	unsigned char hash2[HASH_LENGTH_MAX*2];
	HASH_HashType htype = 0;
	SECKEYPublicKey* pubkey = NULL;
	SECItem secsig = {siBuffer, sigblock, sigblock_len};
	SECItem sechash = {siBuffer, hash, 0};
	SECStatus res;
	unsigned char* prefix = NULL; /* prefix for hash, RFC3110, RFC5702 */
	size_t prefixlen = 0;
	int err;

	if(!nss_setup_key_digest(algo, &pubkey, &htype, key, keylen,
		&prefix, &prefixlen)) {
		verbose(VERB_QUERY, "verify: failed to setup key");
		*reason = "use of key for crypto failed";
		SECKEY_DestroyPublicKey(pubkey);
		return sec_status_bogus;
	}

	/* need to convert DSA, ECDSA signatures? */
	if((algo == LDNS_DSA || algo == LDNS_DSA_NSEC3)) {
		if(sigblock_len == 1+2*SHA1_LENGTH) {
			secsig.data ++;
			secsig.len --;
		} else {
			SECItem* p = DSAU_DecodeDerSig(&secsig);
			if(!p) {
				verbose(VERB_QUERY, "verify: failed DER decode");
				*reason = "signature DER decode failed";
				SECKEY_DestroyPublicKey(pubkey);
				return sec_status_bogus;
			}
			if(SECITEM_CopyItem(pubkey->arena, &secsig, p)) {
				log_err("alloc failure in DER decode");
				SECKEY_DestroyPublicKey(pubkey);
				SECITEM_FreeItem(p, PR_TRUE);
				return sec_status_unchecked;
			}
			SECITEM_FreeItem(p, PR_TRUE);
		}
	}

	/* do the signature cryptography work */
	/* hash the data */
	sechash.len = HASH_ResultLen(htype);
	if(sechash.len > sizeof(hash)) {
		verbose(VERB_QUERY, "verify: hash too large for buffer");
		SECKEY_DestroyPublicKey(pubkey);
		return sec_status_unchecked;
	}
	if(HASH_HashBuf(htype, hash, (unsigned char*)sldns_buffer_begin(buf),
		(unsigned int)sldns_buffer_limit(buf)) != SECSuccess) {
		verbose(VERB_QUERY, "verify: HASH_HashBuf failed");
		SECKEY_DestroyPublicKey(pubkey);
		return sec_status_unchecked;
	}
	if(prefix) {
		int hashlen = sechash.len;
		if(prefixlen+hashlen > sizeof(hash2)) {
			verbose(VERB_QUERY, "verify: hashprefix too large");
			SECKEY_DestroyPublicKey(pubkey);
			return sec_status_unchecked;
		}
		sechash.data = hash2;
		sechash.len = prefixlen+hashlen;
		memcpy(sechash.data, prefix, prefixlen);
		memmove(sechash.data+prefixlen, hash, hashlen);
	}

	/* verify the signature */
	res = PK11_Verify(pubkey, &secsig, &sechash, NULL /*wincx*/);
	SECKEY_DestroyPublicKey(pubkey);

	if(res == SECSuccess) {
		return sec_status_secure;
	}
	err = PORT_GetError();
	if(err != SEC_ERROR_BAD_SIGNATURE) {
		/* failed to verify */
		verbose(VERB_QUERY, "verify: PK11_Verify failed: %s",
			PORT_ErrorToString(err));
		/* if it is not supported, like ECC is removed, we get,
		 * SEC_ERROR_NO_MODULE */
		if(err == SEC_ERROR_NO_MODULE)
			return sec_status_unchecked;
		/* but other errors are commonly returned
		 * for a bad signature from NSS.  Thus we return bogus,
		 * not unchecked */
		*reason = "signature crypto failed";
		return sec_status_bogus;
	}
	verbose(VERB_QUERY, "verify: signature mismatch: %s",
		PORT_ErrorToString(err));
	*reason = "signature crypto failed";
	return sec_status_bogus;
}


#endif /* HAVE_SSL or HAVE_NSS */
