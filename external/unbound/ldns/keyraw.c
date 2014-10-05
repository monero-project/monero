/*
 * keyraw.c - raw key operations and conversions
 *
 * (c) NLnet Labs, 2004-2008
 *
 * See the file LICENSE for the license
 */
/**
 * \file
 * Implementation of raw DNSKEY functions (work on wire rdata).
 */

#include "config.h"
#include "ldns/keyraw.h"
#include "ldns/rrdef.h"

#ifdef HAVE_SSL
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#ifdef HAVE_OPENSSL_ENGINE_H
#  include <openssl/engine.h>
#endif
#endif /* HAVE_SSL */

size_t
sldns_rr_dnskey_key_size_raw(const unsigned char* keydata,
	const size_t len, int alg)
{
	/* for DSA keys */
	uint8_t t;
	
	/* for RSA keys */
	uint16_t exp;
	uint16_t int16;
	
	switch ((sldns_algorithm)alg) {
	case LDNS_DSA:
	case LDNS_DSA_NSEC3:
		if (len > 0) {
			t = keydata[0];
			return (64 + t*8)*8;
		} else {
			return 0;
		}
		break;
	case LDNS_RSAMD5:
	case LDNS_RSASHA1:
	case LDNS_RSASHA1_NSEC3:
#ifdef USE_SHA2
	case LDNS_RSASHA256:
	case LDNS_RSASHA512:
#endif
		if (len > 0) {
			if (keydata[0] == 0) {
				/* big exponent */
				if (len > 3) {
					memmove(&int16, keydata + 1, 2);
					exp = ntohs(int16);
					return (len - exp - 3)*8;
				} else {
					return 0;
				}
			} else {
				exp = keydata[0];
				return (len-exp-1)*8;
			}
		} else {
			return 0;
		}
		break;
#ifdef USE_GOST
	case LDNS_ECC_GOST:
		return 512;
#endif
#ifdef USE_ECDSA
        case LDNS_ECDSAP256SHA256:
                return 256;
        case LDNS_ECDSAP384SHA384:
                return 384;
#endif
	default:
		return 0;
	}
}

uint16_t sldns_calc_keytag_raw(uint8_t* key, size_t keysize)
{
	if(keysize < 4) {
		return 0;
	}
	/* look at the algorithm field, copied from 2535bis */
	if (key[3] == LDNS_RSAMD5) {
		uint16_t ac16 = 0;
		if (keysize > 4) {
			memmove(&ac16, key + keysize - 3, 2);
		}
		ac16 = ntohs(ac16);
		return (uint16_t) ac16;
	} else {
		size_t i;
		uint32_t ac32 = 0;
		for (i = 0; i < keysize; ++i) {
			ac32 += (i & 1) ? key[i] : key[i] << 8;
		}
		ac32 += (ac32 >> 16) & 0xFFFF;
		return (uint16_t) (ac32 & 0xFFFF);
	}
}

#ifdef HAVE_SSL
#ifdef USE_GOST
/** store GOST engine reference loaded into OpenSSL library */
ENGINE* sldns_gost_engine = NULL;

int
sldns_key_EVP_load_gost_id(void)
{
	static int gost_id = 0;
	const EVP_PKEY_ASN1_METHOD* meth;
	ENGINE* e;

	if(gost_id) return gost_id;

	/* see if configuration loaded gost implementation from other engine*/
	meth = EVP_PKEY_asn1_find_str(NULL, "gost2001", -1);
	if(meth) {
		EVP_PKEY_asn1_get0_info(&gost_id, NULL, NULL, NULL, NULL, meth);
		return gost_id;
	}

	/* see if engine can be loaded already */
	e = ENGINE_by_id("gost");
	if(!e) {
		/* load it ourself, in case statically linked */
		ENGINE_load_builtin_engines();
		ENGINE_load_dynamic();
		e = ENGINE_by_id("gost");
	}
	if(!e) {
		/* no gost engine in openssl */
		return 0;
	}
	if(!ENGINE_set_default(e, ENGINE_METHOD_ALL)) {
		ENGINE_finish(e);
		ENGINE_free(e);
		return 0;
	}

	meth = EVP_PKEY_asn1_find_str(&e, "gost2001", -1);
	if(!meth) {
		/* algo not found */
		ENGINE_finish(e);
		ENGINE_free(e);
		return 0;
	}
        /* Note: do not ENGINE_finish and ENGINE_free the acquired engine
         * on some platforms this frees up the meth and unloads gost stuff */
        sldns_gost_engine = e;
	
	EVP_PKEY_asn1_get0_info(&gost_id, NULL, NULL, NULL, NULL, meth);
	return gost_id;
} 

void sldns_key_EVP_unload_gost(void)
{
        if(sldns_gost_engine) {
                ENGINE_finish(sldns_gost_engine);
                ENGINE_free(sldns_gost_engine);
                sldns_gost_engine = NULL;
        }
}
#endif /* USE_GOST */

DSA *
sldns_key_buf2dsa_raw(unsigned char* key, size_t len)
{
	uint8_t T;
	uint16_t length;
	uint16_t offset;
	DSA *dsa;
	BIGNUM *Q; BIGNUM *P;
	BIGNUM *G; BIGNUM *Y;

	if(len == 0)
		return NULL;
	T = (uint8_t)key[0];
	length = (64 + T * 8);
	offset = 1;

	if (T > 8) {
		return NULL;
	}
	if(len < (size_t)1 + SHA_DIGEST_LENGTH + 3*length)
		return NULL;

	Q = BN_bin2bn(key+offset, SHA_DIGEST_LENGTH, NULL);
	offset += SHA_DIGEST_LENGTH;

	P = BN_bin2bn(key+offset, (int)length, NULL);
	offset += length;

	G = BN_bin2bn(key+offset, (int)length, NULL);
	offset += length;

	Y = BN_bin2bn(key+offset, (int)length, NULL);
	offset += length;

	/* create the key and set its properties */
	if(!Q || !P || !G || !Y || !(dsa = DSA_new())) {
		BN_free(Q);
		BN_free(P);
		BN_free(G);
		BN_free(Y);
		return NULL;
	}
#ifndef S_SPLINT_S
	dsa->p = P;
	dsa->q = Q;
	dsa->g = G;
	dsa->pub_key = Y;
#endif /* splint */

	return dsa;
}

RSA *
sldns_key_buf2rsa_raw(unsigned char* key, size_t len)
{
	uint16_t offset;
	uint16_t exp;
	uint16_t int16;
	RSA *rsa;
	BIGNUM *modulus;
	BIGNUM *exponent;

	if (len == 0)
		return NULL;
	if (key[0] == 0) {
		if(len < 3)
			return NULL;
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

	/* Exponent */
	exponent = BN_new();
	if(!exponent) return NULL;
	(void) BN_bin2bn(key+offset, (int)exp, exponent);
	offset += exp;

	/* Modulus */
	modulus = BN_new();
	if(!modulus) {
		BN_free(exponent);
		return NULL;
	}
	/* length of the buffer must match the key length! */
	(void) BN_bin2bn(key+offset, (int)(len - offset), modulus);

	rsa = RSA_new();
	if(!rsa) {
		BN_free(exponent);
		BN_free(modulus);
		return NULL;
	}
#ifndef S_SPLINT_S
	rsa->n = modulus;
	rsa->e = exponent;
#endif /* splint */

	return rsa;
}

#ifdef USE_GOST
EVP_PKEY*
sldns_gost2pkey_raw(unsigned char* key, size_t keylen)
{
	/* prefix header for X509 encoding */
	uint8_t asn[37] = { 0x30, 0x63, 0x30, 0x1c, 0x06, 0x06, 0x2a, 0x85, 
		0x03, 0x02, 0x02, 0x13, 0x30, 0x12, 0x06, 0x07, 0x2a, 0x85, 
		0x03, 0x02, 0x02, 0x23, 0x01, 0x06, 0x07, 0x2a, 0x85, 0x03, 
		0x02, 0x02, 0x1e, 0x01, 0x03, 0x43, 0x00, 0x04, 0x40};
	unsigned char encoded[37+64];
	const unsigned char* pp;
	if(keylen != 64) {
		/* key wrong size */
		return NULL;
	}

	/* create evp_key */
	memmove(encoded, asn, 37);
	memmove(encoded+37, key, 64);
	pp = (unsigned char*)&encoded[0];

	return d2i_PUBKEY(NULL, &pp, (int)sizeof(encoded));
}
#endif /* USE_GOST */

#ifdef USE_ECDSA
EVP_PKEY*
sldns_ecdsa2pkey_raw(unsigned char* key, size_t keylen, uint8_t algo)
{
	unsigned char buf[256+2]; /* sufficient for 2*384/8+1 */
        const unsigned char* pp = buf;
        EVP_PKEY *evp_key;
        EC_KEY *ec;
	/* check length, which uncompressed must be 2 bignums */
        if(algo == LDNS_ECDSAP256SHA256) {
		if(keylen != 2*256/8) return NULL;
                ec = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        } else if(algo == LDNS_ECDSAP384SHA384) {
		if(keylen != 2*384/8) return NULL;
                ec = EC_KEY_new_by_curve_name(NID_secp384r1);
        } else    ec = NULL;
        if(!ec) return NULL;
	if(keylen+1 > sizeof(buf))
		return NULL; /* sanity check */
	/* prepend the 0x02 (from docs) (or actually 0x04 from implementation
	 * of openssl) for uncompressed data */
	buf[0] = POINT_CONVERSION_UNCOMPRESSED;
	memmove(buf+1, key, keylen);
        if(!o2i_ECPublicKey(&ec, &pp, (int)keylen+1)) {
                EC_KEY_free(ec);
                return NULL;
        }
        evp_key = EVP_PKEY_new();
        if(!evp_key) {
                EC_KEY_free(ec);
                return NULL;
        }
        if (!EVP_PKEY_assign_EC_KEY(evp_key, ec)) {
		EVP_PKEY_free(evp_key);
		EC_KEY_free(ec);
		return NULL;
	}
        return evp_key;
}
#endif /* USE_ECDSA */

int
sldns_digest_evp(unsigned char* data, unsigned int len, unsigned char* dest,
	const EVP_MD* md)
{
	EVP_MD_CTX* ctx;
	ctx = EVP_MD_CTX_create();
	if(!ctx)
		return 0;
	if(!EVP_DigestInit_ex(ctx, md, NULL) ||
		!EVP_DigestUpdate(ctx, data, len) ||
		!EVP_DigestFinal_ex(ctx, dest, NULL)) {
		EVP_MD_CTX_destroy(ctx);
		return 0;
	}
	EVP_MD_CTX_destroy(ctx);
	return 1;
}
#endif /* HAVE_SSL */
