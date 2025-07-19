// SPDX-License-Identifier: 0BSD

#include "twofish.h"
#include "warnings.h"

DISABLE_GCC_AND_CLANG_WARNING(undef)

#include <cryptopp/twofish.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>

extern "C" void   twofish_encrypt(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* cipher) {
  CryptoPP::CBC_Mode<CryptoPP::Twofish>::Encryption e;
  e.SetKeyWithIV(key, TWOFISH_KEY_SIZE, iv, TWOFISH_IV_SIZE);

  CryptoPP::ArraySource(reinterpret_cast<const uint8_t *>(data), length, true, new CryptoPP::StreamTransformationFilter(e,
    new CryptoPP::ArraySink(reinterpret_cast<uint8_t *>(cipher), crypto::twofish_encrypted_size(length))));
}

extern "C" size_t twofish_decrypt(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* plain) {
  CryptoPP::CBC_Mode<CryptoPP::Twofish>::Decryption d;
  d.SetKeyWithIV(key, TWOFISH_KEY_SIZE, iv, TWOFISH_IV_SIZE);

  auto sink = new CryptoPP::ArraySink(reinterpret_cast<uint8_t *>(plain), length);
  CryptoPP::ArraySource(reinterpret_cast<const uint8_t *>(data), length, true, new CryptoPP::StreamTransformationFilter(d, sink));
  return sink->TotalPutLength();
}
