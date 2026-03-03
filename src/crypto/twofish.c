// SPDX-License-Identifier: 0BSD

#include "twofish.h"
#include "twofish_impl.h"

void   twofish_encrypt(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* cipher) {
	Twofish_context ctx = {};
	Twofish_setup(&ctx, (uint8_t *)key, (uint8_t *)iv, Twofish_options_default);
	Twofish_encrypt(&ctx, (Twofish_Byte *)data, length, (Twofish_Byte *)cipher, twofish_encrypted_size(length));
}

size_t twofish_decrypt(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* plain) {
	Twofish_UInt64 plainlen = length;
	Twofish_context ctx = {};
	Twofish_setup(&ctx, (uint8_t *)key, (uint8_t *)iv, Twofish_options_default);
	Twofish_decrypt(&ctx, (Twofish_Byte *)data, length, (Twofish_Byte *)plain, &plainlen);
	return plainlen;
}
