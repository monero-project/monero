// Copyright (c) 2019 The Circle Foundation
// Copyright (c) 2019 fireice-uk
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cn_conceal.hpp"

namespace crypto {

void cn_conceal_slow_hash_v0(const void *data, size_t length, char *hash) {
	if(hw_check_aes())
		cryptonight_hash<true, CRYPTONIGHT_CONCEAL>(data, length, hash);
	else
		cryptonight_hash<false, CRYPTONIGHT_CONCEAL>(data, length, hash);
}
}