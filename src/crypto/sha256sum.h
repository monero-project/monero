#pragma once

#include <string>

#include "crypto/hash.h"

namespace tools
{
	bool sha256sum(const uint8_t *data, size_t len, crypto::hash &hash);
	bool sha256sum(const std::string &filename, crypto::hash &hash);
}
