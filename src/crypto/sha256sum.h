#pragma once

#include <string>

#include "crypto/hash.h"

/**
 * \file This file contains easy-to-use wrapper functions around OpenSSL's EVP API for SHA-256 digests.
 */

namespace tools
{
	/**
	 * \brief Creates a SHA-256 digest of a data buffer
	 *
	 * \param[in] data pointer to the buffer
	 * \param[in] len size of the buffer in bytes
	 * \param[out] hash where message digest will be written to
	 *
	 * \returns true if successful, false otherwise
	 */
	bool sha256sum(const uint8_t *data, size_t len, crypto::hash &hash) noexcept;

	/**
	 * \brief Creates a SHA-256 digest of a file's contents, equivalent to the sha256sum command in Linux
	 *
	 * \param[in] filename path to target file
	 * \param[out] hash where message digest will be written to
	 *
	 * \returns true if successful, false otherwise (OpenSSL backend failure, file I/O exceptions, etc)
	 */
	bool sha256sum(const std::string &filename, crypto::hash &hash) noexcept;
}
