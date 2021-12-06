#pragma once

#include "ringct/rctTypes.h"

namespace cryptonote {

class transaction;
class tx_source_entry;
class tx_destination_entry;
class account_keys;

}

namespace multisig {

namespace signing {

constexpr std::size_t kAlphaComponents = 2;

struct cached_CLSAG_Gen_t {
private:
  bool initialized;
  rct::keyV c_params;
  std::size_t c_params_L_offset;
  std::size_t c_params_R_offset;
  rct::keyV b_params;
  std::size_t b_params_L_offset;
  std::size_t b_params_R_offset;
  rct::key mu_P;
  rct::key mu_C;
  std::size_t n;
  rct::geDsmp wH_l_precomp;
  std::vector<rct::geDsmp> W_precomp;
  std::vector<rct::geDsmp> H_precomp;
  rct::geDsmp G_precomp;
  std::size_t l;
  rct::keyV s;
public:
  cached_CLSAG_Gen_t();
  bool init(
    const rct::keyV& P,
    const rct::keyV& C_nonzero,
    const rct::key& C_offset,
    const rct::key& message,
    const rct::key& I,
    const rct::key& D,
    const unsigned int l,
    const rct::keyV& s
  );
  bool combine_alpha_and_compute_challenge(
    const rct::keyV& total_alpha_G,
    const rct::keyV& total_alpha_H,
    const rct::keyV& alpha,
    rct::key& alpha_combined,
    rct::key& c_0,
    rct::key& c
  );
  bool get_mu(
    rct::key& mu_P,
    rct::key& mu_C
  ) const;
};

struct tx_builder_t {
private:
  bool initialized;
  bool reconstruction;
  rct::keyV cached_w;
  std::vector<cached_CLSAG_Gen_t> cached_CLSAG;
public:
  tx_builder_t();
  ~tx_builder_t();
  bool init(
    const cryptonote::account_keys& account_keys,
    const std::vector<std::uint8_t>& extra,
    const std::uint64_t unlock_time,
    const std::uint32_t subaddr_account,
    const std::set<std::uint32_t>& subaddr_minor_indices,
    std::vector<cryptonote::tx_source_entry>& sources,
    std::vector<cryptonote::tx_destination_entry>& destinations,
    const cryptonote::tx_destination_entry& change,
    const rct::RCTConfig& rct_config,
    const bool use_rct,
    const bool reconstruction,
    crypto::secret_key& tx_secret_key,
    std::vector<crypto::secret_key>& tx_aux_secret_keys,
    cryptonote::transaction& unsigned_tx
  );
  bool first_partial_sign(
    const std::size_t source,
    const rct::keyV& total_alpha_G,
    const rct::keyV& total_alpha_H,
    const rct::keyV& alpha,
    rct::key& c_0,
    rct::key& s
  );
  bool next_partial_sign(
    const rct::keyM& total_alpha_G,
    const rct::keyM& total_alpha_H,
    const rct::keyM& alpha,
    const rct::key& x,
    rct::keyV& c_0,
    rct::keyV& s
  );
  static bool construct_tx(
    const std::vector<cryptonote::tx_source_entry>& sources,
    const rct::keyV& c_0,
    const rct::keyV& s,
    cryptonote::transaction& unsigned_tx
  );
};

}

}
