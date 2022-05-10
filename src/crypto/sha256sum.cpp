#include "crypto/sha256sum.h"

#include <algorithm>       // min
#include <cstdio>          // BUFSIZ
#include <fstream>         // ifstream
#include <ios>             // ios_base::{binary, in, ate}
#include <openssl/evp.h>

#include "misc_log_ex.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "util"

namespace {
	constexpr size_t SHA256_MD_SIZE = 32;
	static_assert(sizeof(crypto::hash::data) >= SHA256_MD_SIZE, "crypto::hash::data is too small for SHA-256");
}

namespace tools
{
	bool sha256sum(const uint8_t *data, size_t len, crypto::hash &hash) noexcept
	{
		if (!EVP_Digest(data, len, (unsigned char*) hash.data, NULL, EVP_sha256(), NULL)) {
			MERROR("Failed to digest with OpenSSL SHA-256 EVP");
			return false;
		}

		return true;
	}

	bool sha256sum(const std::string &filename, crypto::hash &hash) noexcept
	{
		// Open file for reading in binary mode, immediately putting the cursor at the end
		std::ifstream f(filename, std::ios_base::binary | std::ios_base::in | std::ios::ate);
		if (!f)
		{
			MERROR("sha256sum: Could not open file '" << filename << "' for reading");
			return false;
		}

		// Retreive file size and go back to the beginning of the stream
		size_t size_left = f.tellg();
		f.seekg(0, std::ios::beg);

		// Initialize digest context
		EVP_MD_CTX* ctx;
		if (NULL == (ctx = EVP_MD_CTX_new()))
		{
			MERROR("Failed to create new OpenSSL EVP_MD_CTX");
			return false;
		}
		else if (!EVP_DigestInit_ex(ctx, EVP_sha256(), NULL))
		{
			MERROR("Failed to initialize OpenSSL SHA-256 EVP context");
			return false;
		}

		while (size_left)
		{
			// Read N bytes from f into buf, where N is min(size_left, BUFSIZ)
			char buf[BUFSIZ];
			const std::ifstream::pos_type read_size = std::min(size_left, sizeof(buf));
			f.read(buf, read_size);
			if (!f || !f.good())
			{
				MERROR("sha256sum: I/O error while reading file '" << filename << "'");
				return false;
			}

			// Update digest
			if (!EVP_DigestUpdate(ctx, buf, read_size)) {
				MERROR("Failed to update OpenSSL SHA-256 EVP digest");
				return false;
			}

			size_left -= read_size;
		} // while (size_left)

		// Finalize digest
		if (!EVP_DigestFinal_ex(ctx, (unsigned char*) hash.data, NULL)) {
			MERROR("Failed to finalize OpenSSL SHA-256 EVP digest");
			return false;
		}

		// Free EVP context
		EVP_MD_CTX_free(ctx);

		return true;
	}
} // namespace tools
