#include "cryptonote_config.h"
#include "common/loki.h"
#include "int-util.h"
#include <vector>
#include <boost/lexical_cast.hpp>

#include "service_node_rules.h"

namespace service_nodes {


uint64_t get_staking_requirement(cryptonote::network_type m_nettype, uint64_t height, int hf_version)
{
  if (m_nettype == cryptonote::TESTNET || m_nettype == cryptonote::FAKECHAIN)
      return COIN * 100;

  uint64_t hardfork_height = m_nettype == cryptonote::MAINNET ? 101250 : 96210 /* stagenet */;
  if (height < hardfork_height) height = hardfork_height;

  uint64_t height_adjusted = height - hardfork_height;
  uint64_t base = 0, variable = 0;
  if (hf_version >= cryptonote::network_version_11_infinite_staking)
  {
    base     = 15000 * COIN;
    variable = (25007.0 * COIN) / loki::exp2(height_adjusted/129600.0);
  }
  else
  {
    base      = 10000 * COIN;
    variable  = (35000.0 * COIN) / loki::exp2(height_adjusted/129600.0);
  }

  uint64_t result = base + variable;
  return result;
}

uint64_t portions_to_amount(uint64_t portions, uint64_t staking_requirement)
{
  uint64_t hi, lo, resulthi, resultlo;
  lo = mul128(staking_requirement, portions, &hi);
  div128_64(hi, lo, STAKING_PORTIONS, &resulthi, &resultlo);
  return resultlo;
}

bool check_service_node_portions(uint8_t hf_version, const std::vector<uint64_t>& portions)
{
  if (portions.size() > MAX_NUMBER_OF_CONTRIBUTORS) return false;

  uint64_t reserved = 0;
  for (auto i = 0u; i < portions.size(); ++i)
  {
    const uint64_t min_portions = get_min_node_contribution(hf_version, STAKING_PORTIONS, reserved, i);
    if (portions[i] < min_portions) return false;
    reserved += portions[i];
  }

  return reserved <= STAKING_PORTIONS;
}

crypto::hash generate_request_stake_unlock_hash(uint32_t nonce)
{
  crypto::hash result   = {};
  char const *nonce_ptr = (char *)&nonce;
  char *hash_ptr        = result.data;
  static_assert(sizeof(result) % sizeof(nonce) == 0, "The nonce should be evenly divisible into the hash");
  for (size_t i = 0; i < sizeof(result) / sizeof(nonce); ++i)
  {
    memcpy(hash_ptr, nonce_ptr, sizeof(nonce));
    hash_ptr += sizeof(nonce);
  }

  assert(hash_ptr == (char *)result.data + sizeof(result));
  return result;
}

uint64_t get_locked_key_image_unlock_height(cryptonote::network_type nettype, uint64_t node_register_height, uint64_t curr_height)
{
  uint64_t blocks_to_lock = staking_num_lock_blocks(nettype);
  uint64_t result         = curr_height + (blocks_to_lock / 2);
  return result;
}

static uint64_t get_min_node_contribution_pre_v11(uint64_t staking_requirement, uint64_t total_reserved)
{
  return std::min(staking_requirement - total_reserved, staking_requirement / MAX_NUMBER_OF_CONTRIBUTORS);
}

uint64_t get_min_node_contribution(uint8_t version, uint64_t staking_requirement, uint64_t total_reserved, size_t num_contributions)
{
  if (version < cryptonote::network_version_11_infinite_staking)
    return get_min_node_contribution_pre_v11(staking_requirement, total_reserved);

  const uint64_t needed                 = staking_requirement - total_reserved;
  const size_t max_num_of_contributions = MAX_NUMBER_OF_CONTRIBUTORS * MAX_KEY_IMAGES_PER_CONTRIBUTOR;
  assert(max_num_of_contributions > num_contributions);
  if (max_num_of_contributions <= num_contributions) return UINT64_MAX;

  const size_t num_contributions_remaining_avail = max_num_of_contributions - num_contributions;
  return needed / num_contributions_remaining_avail;
}

uint64_t get_min_node_contribution_in_portions(uint8_t version, uint64_t staking_requirement, uint64_t total_reserved, size_t num_contributions)
{
  uint64_t atomic_amount = get_min_node_contribution(version, staking_requirement, total_reserved, num_contributions);
  uint64_t result        = (atomic_amount == UINT64_MAX) ? UINT64_MAX : (get_portions_to_make_amount(staking_requirement, atomic_amount));
  return result;
}

uint64_t get_portions_to_make_amount(uint64_t staking_requirement, uint64_t amount)
{
  uint64_t lo, hi, resulthi, resultlo;
  lo = mul128(amount, STAKING_PORTIONS, &hi);
  if (lo > UINT64_MAX - (staking_requirement - 1))
    hi++;
  lo += staking_requirement-1;
  div128_64(hi, lo, staking_requirement, &resulthi, &resultlo);
  return resultlo;
}

static bool get_portions_from_percent(double cur_percent, uint64_t& portions) {
  if(cur_percent < 0.0 || cur_percent > 100.0) return false;

  // Fix for truncation issue when operator cut = 100 for a pool Service Node.
  if (cur_percent == 100.0)
  {
    portions = STAKING_PORTIONS;
  }
  else
  {
    portions = (cur_percent / 100.0) * STAKING_PORTIONS;
  }

  return true;
}

bool get_portions_from_percent_str(std::string cut_str, uint64_t& portions) {

  if(!cut_str.empty() && cut_str.back() == '%')
  {
    cut_str.pop_back();
  }

  double cut_percent;
  try
  {
    cut_percent = boost::lexical_cast<double>(cut_str);
  }
  catch(...)
  {
    return false;
  }

  return get_portions_from_percent(cut_percent, portions);
}

uint64_t uniform_distribution_portable(std::mt19937_64& mersenne_twister, uint64_t n)
{
  assert(n > 0);
  uint64_t secureMax = mersenne_twister.max() - mersenne_twister.max() % n;
  uint64_t x;
  do x = mersenne_twister(); while (x >= secureMax);
  return  x / (secureMax / n);
}

} // namespace service_nodes
