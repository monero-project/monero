#include "cryptonote_config.h"
#include "common/exp2.h"
#include "common/int-util.h"
#include <vector>

#include "service_node_rules.h"

namespace service_nodes {


	uint64_t get_staking_requirement(cryptonote::network_type m_nettype, uint64_t height)
	{
		if (m_nettype == cryptonote::TESTNET || m_nettype == cryptonote::FAKECHAIN)
			return COIN * 100;

		uint64_t hardfork_height = m_nettype == cryptonote::MAINNET ? 101250 : 96210 /* stagenet */;
		if (height < hardfork_height) height = hardfork_height;

		uint64_t height_adjusted = height - hardfork_height;
		uint64_t base = 10000 * COIN;
		uint64_t variable = (35000.0 * COIN) / triton::exp2(height_adjusted / 129600.0);
		uint64_t linear_up = (uint64_t)(5 * COIN * height / 2592) + 8000 * COIN;
		uint64_t flat = 15000 * COIN;
		return std::max(base + variable, height < 3628800 ? linear_up : flat);
	}

	uint64_t portions_to_amount(uint64_t portions, uint64_t staking_requirement)
	{
		uint64_t hi, lo, resulthi, resultlo;
		lo = mul128(staking_requirement, portions, &hi);
		div128_64(hi, lo, STAKING_PORTIONS, &resulthi, &resultlo);
		return resultlo;
	}

	bool check_service_node_portions(const std::vector<uint64_t>& portions)
	{
		uint64_t portions_left = STAKING_PORTIONS;

		for (const auto portion : portions) {
			const uint64_t min_portions = std::min(portions_left, MIN_PORTIONS);
			if (portion < min_portions || portion > portions_left) return false;
			portions_left -= portion;
		}

		return true;
	}

}