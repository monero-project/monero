// SPDX-License-Identifier: 0BSD

#include "chacha.h"
#include "warnings.h"

DISABLE_GCC_AND_CLANG_WARNING(undef)

#include <cryptopp/chacha.h>
#include <cryptopp/algparam.h>

extern "C" void chacha8(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* cipher) {
  CryptoPP::ChaCha::Encryption chacha;
  chacha.SetKey(key, CHACHA_KEY_SIZE, CryptoPP::MakeParameters
    (CryptoPP::Name::IV(), CryptoPP::ConstByteArrayParameter(iv, CHACHA_IV_SIZE))
    (CryptoPP::Name::Rounds(), 8));
  chacha.ProcessData(reinterpret_cast<uint8_t *>(cipher), reinterpret_cast<const uint8_t *>(data), length);
}

extern "C" void chacha20(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* cipher) {
  CryptoPP::ChaCha::Encryption chacha;
  chacha.SetKeyWithIV(key, CHACHA_KEY_SIZE, iv);
  chacha.ProcessData(reinterpret_cast<uint8_t *>(cipher), reinterpret_cast<const uint8_t *>(data), length);
}
