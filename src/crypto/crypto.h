#pragma once

#include <cstddef>
#include <cstring>

namespace crypto {

#pragma pack(push, 1)
  class hash {
    char data[32];
  };

  class ec_point {
    char data[32];
  };

  class ec_scalar {
    char data[32];
  };

  class public_key: ec_point {
    friend class crypto_ops;
  };

  class secret_key: ec_scalar {
    friend class crypto_ops;
  };

  class key_image: ec_point {
    friend class crypto_ops;
  };

  class signature {
    ec_scalar c, r;
    friend class crypto_ops;
  };
#pragma pack(pop)

  static_assert(sizeof(hash) == 32 && sizeof(ec_point) == 32 &&
    sizeof(ec_scalar) == 32 && sizeof(public_key) == 32 &&
    sizeof(secret_key) == 32 && sizeof(key_image) == 32 &&
    sizeof(signature) == 64, "Invalid structure size");

  extern "C" {
    void keccak(const void *data, std::size_t length, char *hash);
  }

  inline void keccak(const void *data, std::size_t length, hash &hash) {
    keccak(data, length, reinterpret_cast<char *>(&hash));
  }

  inline bool operator==(const hash &a, const hash &b) {
    return std::memcmp(&a, &b, sizeof(struct hash)) == 0;
  }

  inline bool operator==(const public_key &a, const public_key &b) {
    return std::memcmp(&a, &b, sizeof(struct public_key)) == 0;
  }

  inline bool operator==(const key_image &a, const key_image &b) {
    return std::memcmp(&a, &b, sizeof(struct key_image)) == 0;
  }

  class crypto_ops {
    crypto_ops();
    crypto_ops(const crypto_ops &);
    void operator=(const crypto_ops &);
    ~crypto_ops();

    static void generate_keys(public_key &, secret_key &);
    friend void generate_keys(public_key &, secret_key &);
    static bool check_key(const public_key &);
    friend bool check_key(const public_key &);
    static void generate_signature(const hash &, const public_key &, const secret_key &, signature &);
    friend void generate_signature(const hash &, const public_key &, const secret_key &, signature &);
    static bool check_signature(const hash &, const public_key &, const signature &);
    friend bool check_signature(const hash &, const public_key &, const signature &);
    static void generate_key_image(const public_key &, const secret_key &, key_image &);
    friend void generate_key_image(const public_key &, const secret_key &, key_image &);
    static void generate_ring_signature(const hash &, const key_image &,
      const public_key *const *, std::size_t, const secret_key &, std::size_t, signature *);
    friend void generate_ring_signature(const hash &, const key_image &,
      const public_key *const *, std::size_t, const secret_key &, std::size_t, signature *);
    static bool check_ring_signature(const hash &, const key_image &,
      const public_key *const *, std::size_t, const signature *);
    friend bool check_ring_signature(const hash &, const key_image &,
      const public_key *const *, std::size_t, const signature *);
  };

  /* Generate a new key pair.
   * pub: a newly generated public key.
   * sec: a newly generated secret key.
   */
  inline void generate_keys(public_key &pub, secret_key &sec) {
    crypto_ops::generate_keys(pub, sec);
  }

  /* Check a public key.
   * key: a key to check.
   * returns: true if the key is valid, false otherwise.
   */
  inline bool check_key(const public_key &key) {
    return crypto_ops::check_key(key);
  }

  /* Sign a message.
   * message_hash: hash of a message.
   * pub: public key used for signing. Assumed to be valid.
   * sec: secret key used for signing. Assumed to correspond to pub.
   * sig: the resulting signature.
   */
  inline void generate_signature(const hash &message_hash, const public_key &pub, const secret_key &sec, signature &sig) {
    crypto_ops::generate_signature(message_hash, pub, sec, sig);
  }

  /* Verify a signature.
   * message_hash: hash of a message.
   * pub: public key used for signing. Assumed to be valid, use check_key to check it first.
   * sig: a signature.
   * returns: true if the signature is valid, false otherwise.
   */
  inline bool check_signature(const hash &message_hash, const public_key &pub, const signature &sig) {
    return crypto_ops::check_signature(message_hash, pub, sig);
  }

  /* Generate the image of a key.
   * pub: public key used for signing. Assumed to be valid.
   * sec: secret key used for signing. Assumed to correspond to pub.
   * image: the resulting key image.
   */
  inline void generate_key_image(const public_key &pub, const secret_key &sec, key_image &image) {
    crypto_ops::generate_key_image(pub, sec, image);
  }

  /* Sign a message using linkable ring signature.
   * message_hash: hash of a message.
   * image: image of the key used for signing. Use generate_key_image to create it. Assumed to correspond to the key used for signing.
   * pubs: pointer to an array of pointers to public keys of a ring. All keys are assumed to be valid, use check_key to check them first.
   * pubs_count: number of keys in a ring.
   * sec: secret key used for signing.
   * sec_index: index of the key used for signing in pubs. It is assumed that 0 <= sec_index < pubs_count and that sec corresponds to *pubs[sec_index].
   * sig: the resulting signature (occupies pubs_count elements). To verify it, image of the key is also necessary.
   */
  inline void generate_ring_signature(const hash &message_hash, const key_image &image,
    const public_key *const *pubs, std::size_t pubs_count,
    const secret_key &sec, std::size_t sec_index,
    signature *sig) {
    crypto_ops::generate_ring_signature(message_hash, image, pubs, pubs_count, sec, sec_index, sig);
  }

  /* Verify a linkable ring signature.
   * message_hash: hash of a message.
   * image: image of the key used for signing.
   * pubs: pointer to an array of pointers to public keys of a ring. All keys are assumed to be valid, use check_key to check them first.
   * pubs_count: number of keys in a ring.
   * sig: a signature (occupies pubs_count elements).
   * returns: true if the signature is valid, false otherwise.
   */
  inline bool check_ring_signature(const hash &message_hash, const key_image &image,
    const public_key *const *pubs, std::size_t pubs_count,
    const signature *sig) {
    return crypto_ops::check_ring_signature(message_hash, image, pubs, pubs_count, sig);
  }

  /* To check whether two signatures are linked, compare their key images. */
}