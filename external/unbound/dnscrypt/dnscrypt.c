
#include "config.h"
#include <stdlib.h>
#include <fcntl.h>
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include "sldns/sbuffer.h"
#include "util/config_file.h"
#include "util/net_help.h"
#include "util/netevent.h"
#include "util/log.h"

#include "dnscrypt/cert.h"
#include "dnscrypt/dnscrypt.h"

#include <ctype.h>

/**
 * \file
 * dnscrypt functions for encrypting DNS packets.
 */

#define DNSCRYPT_QUERY_BOX_OFFSET \
    (DNSCRYPT_MAGIC_HEADER_LEN + crypto_box_PUBLICKEYBYTES + crypto_box_HALF_NONCEBYTES)

//  8 bytes: magic header (CERT_MAGIC_HEADER)
// 12 bytes: the client's nonce
// 12 bytes: server nonce extension
// 16 bytes: Poly1305 MAC (crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES)

#define DNSCRYPT_REPLY_BOX_OFFSET \
    (DNSCRYPT_MAGIC_HEADER_LEN + crypto_box_HALF_NONCEBYTES + crypto_box_HALF_NONCEBYTES)

/**
 * Decrypt a query using the keypair that was found using dnsc_find_keypair.
 * The client nonce will be extracted from the encrypted query and stored in
 * client_nonce, a shared secret will be computed and stored in nmkey and the
 * buffer will be decrypted inplace.
 * \param[in] keypair the keypair that matches this encrypted query.
 * \param[in] client_nonce where the client nonce will be stored.
 * \param[in] nmkey where the shared secret key will be written.
 * \param[in] buffer the encrypted buffer.
 * \return 0 on success.
 */
static int
dnscrypt_server_uncurve(const KeyPair *keypair,
                        uint8_t client_nonce[crypto_box_HALF_NONCEBYTES],
                        uint8_t nmkey[crypto_box_BEFORENMBYTES],
                        struct sldns_buffer* buffer)
{
    size_t len = sldns_buffer_limit(buffer);
    uint8_t *const buf = sldns_buffer_begin(buffer);
    uint8_t nonce[crypto_box_NONCEBYTES];
    struct dnscrypt_query_header *query_header;

    if (len <= DNSCRYPT_QUERY_HEADER_SIZE) {
        return -1;
    }

    query_header = (struct dnscrypt_query_header *)buf;
    memcpy(nmkey, query_header->publickey, crypto_box_PUBLICKEYBYTES);
    if (crypto_box_beforenm(nmkey, nmkey, keypair->crypt_secretkey) != 0) {
        return -1;
    }

    memcpy(nonce, query_header->nonce, crypto_box_HALF_NONCEBYTES);
    memset(nonce + crypto_box_HALF_NONCEBYTES, 0, crypto_box_HALF_NONCEBYTES);

    sldns_buffer_set_at(buffer,
                        DNSCRYPT_QUERY_BOX_OFFSET - crypto_box_BOXZEROBYTES,
                        0, crypto_box_BOXZEROBYTES);

    if (crypto_box_open_afternm
        (buf + DNSCRYPT_QUERY_BOX_OFFSET - crypto_box_BOXZEROBYTES,
         buf + DNSCRYPT_QUERY_BOX_OFFSET - crypto_box_BOXZEROBYTES,
         len - DNSCRYPT_QUERY_BOX_OFFSET + crypto_box_BOXZEROBYTES, nonce,
         nmkey) != 0) {
        return -1;
    }

    while (*sldns_buffer_at(buffer, --len) == 0)
	    ;

    if (*sldns_buffer_at(buffer, len) != 0x80) {
        return -1;
    }

    memcpy(client_nonce, nonce, crypto_box_HALF_NONCEBYTES);
    memmove(sldns_buffer_begin(buffer),
            sldns_buffer_at(buffer, DNSCRYPT_QUERY_HEADER_SIZE),
            len - DNSCRYPT_QUERY_HEADER_SIZE);

    sldns_buffer_set_position(buffer, 0);
    sldns_buffer_set_limit(buffer, len - DNSCRYPT_QUERY_HEADER_SIZE);

    return 0;
}


/**
 * Add random padding to a buffer, according to a client nonce.
 * The length has to depend on the query in order to avoid reply attacks.
 *
 * @param buf a buffer
 * @param len the initial size of the buffer
 * @param max_len the maximum size
 * @param nonce a nonce, made of the client nonce repeated twice
 * @param secretkey
 * @return the new size, after padding
 */
size_t
dnscrypt_pad(uint8_t *buf, const size_t len, const size_t max_len,
             const uint8_t *nonce, const uint8_t *secretkey)
{
    uint8_t *buf_padding_area = buf + len;
    size_t padded_len;
    uint32_t rnd;

    // no padding
    if (max_len < len + DNSCRYPT_MIN_PAD_LEN)
        return len;

    assert(nonce[crypto_box_HALF_NONCEBYTES] == nonce[0]);

    crypto_stream((unsigned char *)&rnd, (unsigned long long)sizeof(rnd), nonce,
                  secretkey);
    padded_len =
        len + DNSCRYPT_MIN_PAD_LEN + rnd % (max_len - len -
                                            DNSCRYPT_MIN_PAD_LEN + 1);
    padded_len += DNSCRYPT_BLOCK_SIZE - padded_len % DNSCRYPT_BLOCK_SIZE;
    if (padded_len > max_len)
        padded_len = max_len;

    memset(buf_padding_area, 0, padded_len - len);
    *buf_padding_area = 0x80;

    return padded_len;
}

uint64_t
dnscrypt_hrtime(void)
{
    struct timeval tv;
    uint64_t ts = (uint64_t)0U;
    int ret;

    ret = gettimeofday(&tv, NULL);
    if (ret == 0) {
        ts = (uint64_t)tv.tv_sec * 1000000U + (uint64_t)tv.tv_usec;
    } else {
	log_err("gettimeofday: %s", strerror(errno));
    }
    return ts;
}

/**
 * Add the server nonce part to once.
 * The nonce is made half of client nonce and the seconf half of the server
 * nonce, both of them of size crypto_box_HALF_NONCEBYTES.
 * \param[in] nonce: a uint8_t* of size crypto_box_NONCEBYTES
 */
static void
add_server_nonce(uint8_t *nonce)
{
    uint64_t ts;
    uint64_t tsn;
    uint32_t suffix;
    ts = dnscrypt_hrtime();
    // TODO? dnscrypt-wrapper does some logic with context->nonce_ts_last
    // unclear if we really need it, so skipping it for now.
    tsn = (ts << 10) | (randombytes_random() & 0x3ff);
#if (BYTE_ORDER == LITTLE_ENDIAN)
    tsn =
        (((uint64_t)htonl((uint32_t)tsn)) << 32) | htonl((uint32_t)(tsn >> 32));
#endif
    memcpy(nonce + crypto_box_HALF_NONCEBYTES, &tsn, 8);
    suffix = randombytes_random();
    memcpy(nonce + crypto_box_HALF_NONCEBYTES + 8, &suffix, 4);
}

/**
 * Encrypt a reply using the keypair that was used with the query.
 * The client nonce will be extracted from the encrypted query and stored in
 * The buffer will be encrypted inplace.
 * \param[in] keypair the keypair that matches this encrypted query.
 * \param[in] client_nonce client nonce used during the query
 * \param[in] nmkey shared secret key used during the query.
 * \param[in] buffer the buffer where to encrypt the reply.
 * \param[in] udp if whether or not it is a UDP query.
 * \param[in] max_udp_size configured max udp size.
 * \return 0 on success.
 */
static int
dnscrypt_server_curve(const KeyPair *keypair,
                      uint8_t client_nonce[crypto_box_HALF_NONCEBYTES],
                      uint8_t nmkey[crypto_box_BEFORENMBYTES],
                      struct sldns_buffer* buffer,
                      uint8_t udp,
                      size_t max_udp_size)
{
    size_t dns_reply_len = sldns_buffer_limit(buffer);
    size_t max_len = dns_reply_len + DNSCRYPT_MAX_PADDING + DNSCRYPT_REPLY_HEADER_SIZE;
    size_t max_reply_size = max_udp_size - 20U - 8U;
    uint8_t nonce[crypto_box_NONCEBYTES];
    uint8_t *boxed;
    uint8_t *const buf = sldns_buffer_begin(buffer);
    size_t len = sldns_buffer_limit(buffer);

    if(udp){
        if (max_len > max_reply_size)
            max_len = max_reply_size;
    }


    memcpy(nonce, client_nonce, crypto_box_HALF_NONCEBYTES);
    memcpy(nonce + crypto_box_HALF_NONCEBYTES, client_nonce,
           crypto_box_HALF_NONCEBYTES);

    boxed = buf + DNSCRYPT_REPLY_BOX_OFFSET;
    memmove(boxed + crypto_box_MACBYTES, buf, len);
    len = dnscrypt_pad(boxed + crypto_box_MACBYTES, len,
                       max_len - DNSCRYPT_REPLY_HEADER_SIZE, nonce,
                       keypair->crypt_secretkey);
    sldns_buffer_set_at(buffer,
                        DNSCRYPT_REPLY_BOX_OFFSET - crypto_box_BOXZEROBYTES,
                        0, crypto_box_ZEROBYTES);

    // add server nonce extension
    add_server_nonce(nonce);

    if (crypto_box_afternm
        (boxed - crypto_box_BOXZEROBYTES, boxed - crypto_box_BOXZEROBYTES,
         len + crypto_box_ZEROBYTES, nonce, nmkey) != 0) {
        return -1;
    }

    sldns_buffer_write_at(buffer, 0, DNSCRYPT_MAGIC_RESPONSE, DNSCRYPT_MAGIC_HEADER_LEN);
    sldns_buffer_write_at(buffer, DNSCRYPT_MAGIC_HEADER_LEN, nonce, crypto_box_NONCEBYTES);
    sldns_buffer_set_limit(buffer, len + DNSCRYPT_REPLY_HEADER_SIZE);
    return 0;
}

/**
 * Read the content of fname into buf.
 * \param[in] fname name of the file to read.
 * \param[in] buf the buffer in which to read the content of the file.
 * \param[in] count number of bytes to read.
 * \return 0 on success.
 */
static int
dnsc_read_from_file(char *fname, char *buf, size_t count)
{
	int fd;
	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		return -1;
	}
	if (read(fd, buf, count) != (ssize_t)count) {
		close(fd);
		return -2;
	}
	close(fd);
	return 0;
}

/**
 * Parse certificates files provided by the configuration and load them into
 * dnsc_env.
 * \param[in] env the dnsc_env structure to load the certs into.
 * \param[in] cfg the configuration.
 * \return the number of certificates loaded.
 */
static int
dnsc_parse_certs(struct dnsc_env *env, struct config_file *cfg)
{
	struct config_strlist *head;
	size_t signed_cert_id;

	env->signed_certs_count = 0U;
	for (head = cfg->dnscrypt_provider_cert; head; head = head->next) {
		env->signed_certs_count++;
	}
	env->signed_certs = sodium_allocarray(env->signed_certs_count,
										  sizeof *env->signed_certs);

	signed_cert_id = 0U;
	for(head = cfg->dnscrypt_provider_cert; head; head = head->next, signed_cert_id++) {
		if(dnsc_read_from_file(
				head->str,
				(char *)(env->signed_certs + signed_cert_id),
				sizeof(struct SignedCert)) != 0) {
			fatal_exit("dnsc_parse_certs: failed to load %s: %s", head->str, strerror(errno));
		}
		verbose(VERB_OPS, "Loaded cert %s", head->str);
	}
	return signed_cert_id;
}

/**
 * Helper function to convert a binary key into a printable fingerprint.
 * \param[in] fingerprint the buffer in which to write the printable key.
 * \param[in] key the key to convert.
 */
void
dnsc_key_to_fingerprint(char fingerprint[80U], const uint8_t * const key)
{
    const size_t fingerprint_size = 80U;
    size_t       fingerprint_pos = (size_t) 0U;
    size_t       key_pos = (size_t) 0U;

    for (;;) {
        assert(fingerprint_size > fingerprint_pos);
        snprintf(&fingerprint[fingerprint_pos],
                        fingerprint_size - fingerprint_pos, "%02X%02X",
                        key[key_pos], key[key_pos + 1U]);
        key_pos += 2U;
        if (key_pos >= crypto_box_PUBLICKEYBYTES) {
            break;
        }
        fingerprint[fingerprint_pos + 4U] = ':';
        fingerprint_pos += 5U;
    }
}

/**
 * Find the keypair matching a DNSCrypt query.
 * \param[in] dnscenv The DNSCrypt enviroment, which contains the list of keys
 * supported by the server.
 * \param[in] buffer The encrypted DNS query.
 * \return a KeyPair * if we found a key pair matching the query, NULL otherwise.
 */
static const KeyPair *
dnsc_find_keypair(struct dnsc_env* dnscenv, struct sldns_buffer* buffer)
{
	const KeyPair *keypairs = dnscenv->keypairs;
	struct dnscrypt_query_header *dnscrypt_header;
	size_t i;

	if (sldns_buffer_limit(buffer) < DNSCRYPT_QUERY_HEADER_SIZE) {
		return NULL;
	}
	dnscrypt_header = (struct dnscrypt_query_header *)sldns_buffer_begin(buffer);
	for (i = 0U; i < dnscenv->keypairs_count; i++) {
		if (memcmp(keypairs[i].crypt_publickey, dnscrypt_header->magic_query,
                   DNSCRYPT_MAGIC_HEADER_LEN) == 0) {
			return &keypairs[i];
		}
	}
	return NULL;
}

/**
 * Insert local-zone and local-data into configuration.
 * In order to be able to serve certs over TXT, we can reuse the local-zone and
 * local-data config option. The zone and qname are infered from the
 * provider_name and the content of the TXT record from the certificate content.
 * returns the number of certtificate TXT record that were loaded.
 * < 0 in case of error.
 */
static int
dnsc_load_local_data(struct dnsc_env* dnscenv, struct config_file *cfg)
{
    size_t i, j;
	// Insert 'local-zone: "2.dnscrypt-cert.example.com" deny'
    if(!cfg_str2list_insert(&cfg->local_zones,
                            strdup(dnscenv->provider_name),
                            strdup("deny"))) {
        log_err("Could not load dnscrypt local-zone: %s deny",
                dnscenv->provider_name);
        return -1;
    }

    // Add local data entry of type:
    // 2.dnscrypt-cert.example.com 86400 IN TXT "DNSC......"
    for(i=0; i<dnscenv->signed_certs_count; i++) {
        const char *ttl_class_type = " 86400 IN TXT \"";
        struct SignedCert *cert = dnscenv->signed_certs + i;
        uint16_t rrlen = strlen(dnscenv->provider_name) +
                         strlen(ttl_class_type) +
                         4 * sizeof(struct SignedCert) + // worst case scenario
                         1 + // trailing double quote
                         1;
        char *rr = malloc(rrlen);
        if(!rr) {
            log_err("Could not allocate memory");
            return -2;
        }
        snprintf(rr, rrlen - 1, "%s 86400 IN TXT \"", dnscenv->provider_name);
        for(j=0; j<sizeof(struct SignedCert); j++) {
       	    int c = (int)*((const uint8_t *) cert + j);
            if (isprint(c) && c != '"' && c != '\\') {
                snprintf(rr + strlen(rr), rrlen - 1 - strlen(rr), "%c", c);
            } else {
                snprintf(rr + strlen(rr), rrlen - 1 - strlen(rr), "\\%03d", c);
            }
        }
        snprintf(rr + strlen(rr), rrlen - 1 - strlen(rr), "\"");
        cfg_strlist_insert(&cfg->local_data, strdup(rr));
        free(rr);
    }
    return dnscenv->signed_certs_count;
}

/**
 * Parse the secret key files from `dnscrypt-secret-key` config and populates
 * a list of secret/public keys supported by dnscrypt listener.
 * \param[in] env The dnsc_env structure which will hold the keypairs.
 * \param[in] cfg The config with the secret key file paths.
 */
static int
dnsc_parse_keys(struct dnsc_env *env, struct config_file *cfg)
{
	struct config_strlist *head;
	size_t keypair_id;

	env->keypairs_count = 0U;
	for (head = cfg->dnscrypt_secret_key; head; head = head->next) {
		env->keypairs_count++;
	}
	env->keypairs = sodium_allocarray(env->keypairs_count,
										  sizeof *env->keypairs);

	keypair_id = 0U;
	for(head = cfg->dnscrypt_secret_key; head; head = head->next, keypair_id++) {
		char fingerprint[80];
		if(dnsc_read_from_file(
				head->str,
				(char *)(env->keypairs[keypair_id].crypt_secretkey),
				crypto_box_SECRETKEYBYTES) != 0) {
			fatal_exit("dnsc_parse_keys: failed to load %s: %s", head->str, strerror(errno));
		}
		verbose(VERB_OPS, "Loaded key %s", head->str);
		if (crypto_scalarmult_base(env->keypairs[keypair_id].crypt_publickey,
								   env->keypairs[keypair_id].crypt_secretkey) != 0) {
			fatal_exit("dnsc_parse_keys: could not generate public key from %s", head->str);
		}
		dnsc_key_to_fingerprint(fingerprint, env->keypairs[keypair_id].crypt_publickey);
		verbose(VERB_OPS, "Crypt public key fingerprint for %s: %s", head->str, fingerprint);
	}
	return keypair_id;
}


/**
 * #########################################################
 * ############# Publicly accessible functions #############
 * #########################################################
 */

int
dnsc_handle_curved_request(struct dnsc_env* dnscenv,
                           struct comm_reply* repinfo)
{
    struct comm_point* c = repinfo->c;

    repinfo->is_dnscrypted = 0;
    if( !c->dnscrypt ) {
        return 1;
    }
    // Attempt to decrypt the query. If it is not crypted, we may still need
    // to serve the certificate.
    verbose(VERB_ALGO, "handle request called on DNSCrypt socket");
    if ((repinfo->keypair = dnsc_find_keypair(dnscenv, c->buffer)) != NULL) {
        if(dnscrypt_server_uncurve(repinfo->keypair,
                                   repinfo->client_nonce,
                                   repinfo->nmkey,
                                   c->buffer) != 0){
            verbose(VERB_ALGO, "dnscrypt: Failed to uncurve");
            comm_point_drop_reply(repinfo);
            return 0;
        }
        repinfo->is_dnscrypted = 1;
        sldns_buffer_rewind(c->buffer);
    }
    return 1;
}

int
dnsc_handle_uncurved_request(struct comm_reply *repinfo)
{
    if(!repinfo->c->dnscrypt) {
        return 1;
    }
    sldns_buffer_copy(repinfo->c->dnscrypt_buffer, repinfo->c->buffer);
    if(!repinfo->is_dnscrypted) {
        return 1;
    }
	if(dnscrypt_server_curve(repinfo->keypair,
                             repinfo->client_nonce,
                             repinfo->nmkey,
                             repinfo->c->dnscrypt_buffer,
                             repinfo->c->type == comm_udp,
                             repinfo->max_udp_size) != 0){
		verbose(VERB_ALGO, "dnscrypt: Failed to curve cached missed answer");
		comm_point_drop_reply(repinfo);
		return 0;
	}
    return 1;
}

struct dnsc_env *
dnsc_create(void)
{
	struct dnsc_env *env;
	if (sodium_init() == -1) {
		fatal_exit("dnsc_create: could not initialize libsodium.");
	}
	env = (struct dnsc_env *) calloc(1, sizeof(struct dnsc_env));
	return env;
}

int
dnsc_apply_cfg(struct dnsc_env *env, struct config_file *cfg)
{
	if(dnsc_parse_certs(env, cfg) <= 0) {
		fatal_exit("dnsc_apply_cfg: no cert file loaded");
	}
	if(dnsc_parse_keys(env, cfg) <= 0) {
		fatal_exit("dnsc_apply_cfg: no key file loaded");
	}
	randombytes_buf(env->hash_key, sizeof env->hash_key);
	env->provider_name = cfg->dnscrypt_provider;

	if(dnsc_load_local_data(env, cfg) <= 0) {
		fatal_exit("dnsc_apply_cfg: could not load local data");
	}
	return 0;
}
