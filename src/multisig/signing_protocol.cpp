#include "signing_protocol.h"
#include "memwipe.h"
#include "device/device.hpp"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "ringct/bulletproofs.h"
#include "ringct/rctSigs.h"

namespace multisig {

namespace signing {

cached_CLSAG_Gen_t::cached_CLSAG_Gen_t(): initialized{false} {}

template<typename T, std::size_t N>
void encode_varint(T t, unsigned char (&out)[N]) {
  static_assert(std::is_integral<T>::value, "");
  static_assert(std::is_unsigned<T>::value, "");
  static_assert((sizeof(T) + 6) / 7 <= N, "");
  for (std::size_t i = 0; i < N && t; ++i) {
    if (t >= 0x80)
      out[i] = (static_cast<unsigned char>(t) & 0x7F) | 0x80;
    else
      out[i] = static_cast<unsigned char>(t);
    t >>= 7;
  }
}

template<std::size_t N>
rct::key string_to_key(const unsigned char (&str)[N]) {
  rct::key tmp{};
  static_assert(sizeof(tmp.bytes) >= N, "");
  std::memcpy(tmp.bytes, str, N);
  return tmp;
}

bool cached_CLSAG_Gen_t::init(
  const rct::keyV& P,
  const rct::keyV& C_nonzero,
  const rct::key& C_offset,
  const rct::key& message,
  const rct::key& I,
  const rct::key& D,
  const unsigned int l,
  const rct::keyV& s
)
{
  initialized = false;

  n = P.size();
  if (n <= 0)
    return false;
  if (C_nonzero.size() != n)
    return false;
  if (s.size() != n)
    return false;
  if (l >= n)
    return false;

  c_params.clear();
  c_params.reserve(n * 2 + 5);
  b_params.clear();
  b_params.reserve(n * 3 + 2 * multisig::signing::kAlphaComponents + 5);

  c_params.push_back(string_to_key(config::HASH_KEY_CLSAG_ROUND));
  b_params.push_back(string_to_key(config::HASH_KEY_CLSAG_ROUND_MULTISIG));
  c_params.insert(c_params.end(), P.begin(), P.end());
  b_params.insert(b_params.end(), P.begin(), P.end());
  c_params.insert(c_params.end(), C_nonzero.begin(), C_nonzero.end());
  b_params.insert(b_params.end(), C_nonzero.begin(), C_nonzero.end());
  c_params.push_back(C_offset);
  b_params.push_back(C_offset);
  c_params.push_back(message);
  b_params.push_back(message);
  c_params_L_offset = c_params.size();
  b_params_L_offset = b_params.size();
  c_params.resize(c_params.size() + 1);
  b_params.resize(b_params.size() + multisig::signing::kAlphaComponents);
  c_params_R_offset = c_params.size();
  b_params_R_offset = b_params.size();
  c_params.resize(c_params.size() + 1);
  b_params.resize(b_params.size() + multisig::signing::kAlphaComponents);
  b_params.push_back(I);
  b_params.push_back(D);
  b_params.insert(b_params.end(), s.begin(), s.begin() + l);
  b_params.insert(b_params.end(), s.begin() + l + 1, s.end());
  b_params.emplace_back();
  encode_varint(l, b_params.back().bytes);

  rct::keyV mu_P_params;
  rct::keyV mu_C_params;
  mu_P_params.reserve(n * 2 + 4);
  mu_C_params.reserve(n * 2 + 4);

  mu_P_params.push_back(string_to_key(config::HASH_KEY_CLSAG_AGG_0));
  mu_C_params.push_back(string_to_key(config::HASH_KEY_CLSAG_AGG_1));
  mu_P_params.insert(mu_P_params.end(), P.begin(), P.end());
  mu_C_params.insert(mu_C_params.end(), P.begin(), P.end());
  mu_P_params.insert(mu_P_params.end(), C_nonzero.begin(), C_nonzero.end());
  mu_C_params.insert(mu_C_params.end(), C_nonzero.begin(), C_nonzero.end());
  mu_P_params.push_back(I);
  mu_C_params.push_back(I);
  mu_P_params.push_back(scalarmultKey(D, rct::INV_EIGHT));
  mu_C_params.push_back(scalarmultKey(D, rct::INV_EIGHT));
  mu_P_params.push_back(C_offset);
  mu_C_params.push_back(C_offset);
  mu_P = rct::hash_to_scalar(mu_P_params);
  mu_C = rct::hash_to_scalar(mu_C_params);

  rct::geDsmp I_precomp;
  rct::geDsmp D_precomp;
  rct::precomp(I_precomp.k, I);
  rct::precomp(D_precomp.k, D);
  rct::key wH_l;
  rct::addKeys3(wH_l, mu_P, I_precomp.k, mu_C, D_precomp.k);
  rct::precomp(wH_l_precomp.k, wH_l);
  W_precomp.resize(n);
  H_precomp.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    rct::geDsmp P_precomp;
    rct::geDsmp C_precomp;
    rct::key C;
    rct::subKeys(C, C_nonzero[i], C_offset);
    rct::precomp(P_precomp.k, P[i]);
    rct::precomp(C_precomp.k, C);
    rct::key W;
    rct::addKeys3(W, mu_P, P_precomp.k, mu_C, C_precomp.k);
    rct::precomp(W_precomp[i].k, W);
    ge_p3 Hi_p3;
    rct::hash_to_p3(Hi_p3, P[i]);
    ge_dsm_precomp(H_precomp[i].k, &Hi_p3);
  }
  rct::precomp(G_precomp.k, rct::G);
  this->l = l;
  this->s = s;

  initialized = true;
  return true;
}

bool cached_CLSAG_Gen_t::combine_alpha_and_compute_challenge(
  const rct::keyV& total_alpha_G,
  const rct::keyV& total_alpha_H,
  const rct::keyV& alpha,
  rct::key& alpha_combined,
  rct::key& c_0,
  rct::key& c
)
{
  if (not initialized)
    return false;
  {
    const std::size_t dim_alpha_components = multisig::signing::kAlphaComponents;
    if (dim_alpha_components != total_alpha_G.size())
      return false;
    if (dim_alpha_components != total_alpha_H.size())
      return false;
    if (dim_alpha_components != alpha.size())
      return false;
    for (std::size_t i = 0; i < dim_alpha_components; ++i) {
      b_params[b_params_L_offset + i] = total_alpha_G[i];
      b_params[b_params_R_offset + i] = total_alpha_H[i];
    }
    rct::key b = rct::hash_to_scalar(b_params);
    rct::key& L_l = c_params[c_params_L_offset];
    rct::key& R_l = c_params[c_params_R_offset];
    rct::key b_i = rct::identity();
    L_l = rct::identity();
    R_l = rct::identity();
    alpha_combined = rct::zero();
    for (std::size_t i = 0; i < dim_alpha_components; ++i) {
      rct::addKeys(L_l, L_l, rct::scalarmultKey(total_alpha_G[i], b_i));
      rct::addKeys(R_l, R_l, rct::scalarmultKey(total_alpha_H[i], b_i));
      sc_muladd(alpha_combined.bytes, alpha[i].bytes, b_i.bytes, alpha_combined.bytes);
      sc_mul(b_i.bytes, b_i.bytes, b.bytes);
    }
  }
  c = rct::hash_to_scalar(c_params);
  for (std::size_t i = (l + 1) % n; i != l; i = (i + 1) % n) {
    if (i == 0)
      c_0 = c;
    rct::addKeys3(c_params[c_params_L_offset], s[i], G_precomp.k, c, W_precomp[i].k);
    rct::addKeys3(c_params[c_params_R_offset], s[i], H_precomp[i].k, c, wH_l_precomp.k);
    c = rct::hash_to_scalar(c_params);
  }
  if (l == 0)
    c_0 = c;
  return true;
}

bool cached_CLSAG_Gen_t::get_mu(rct::key& mu_P, rct::key& mu_C) const
{
  if (not initialized)
    return false;
  mu_P = this->mu_P;
  mu_C = this->mu_C;
  return true;
}

tx_builder_t::tx_builder_t(): initialized(false) {}

tx_builder_t::~tx_builder_t()
{
  memwipe(static_cast<rct::key *>(cached_w.data()), cached_w.size() * sizeof(rct::key));
}

static void sort_sources(
  std::vector<cryptonote::tx_source_entry>& sources
)
{
  std::sort(sources.begin(), sources.end(), [](const auto& lhs, const auto& rhs){
    const rct::key& ki0 = lhs.multisig_kLRki.ki;
    const rct::key& ki1 = rhs.multisig_kLRki.ki;
    return memcmp(&ki0, &ki1, sizeof(rct::key)) > 0;
  });
}

static bool compute_keys_for_sources(
  const cryptonote::account_keys& account_keys,
  const std::vector<cryptonote::tx_source_entry>& sources,
  const std::uint32_t subaddr_account,
  const std::set<std::uint32_t>& subaddr_minor_indices,
  rct::keyV& input_secret_keys
)
{
  const std::size_t dim_sources = sources.size();
  hw::device& hwdev = account_keys.get_device();
  std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
  for (const std::uint32_t& minor_index: subaddr_minor_indices) {
    subaddresses[hwdev.get_subaddress_spend_public_key(
      account_keys,
      {subaddr_account, minor_index}
    )] = {subaddr_account, minor_index};
  }
  input_secret_keys.resize(dim_sources);
  for (std::size_t i = 0; i < dim_sources; ++i) {
    const auto& src = sources[i];
    crypto::key_image tmp_image;
    cryptonote::keypair tmp_key;
    if (src.real_output >= src.outputs.size())
      return false;
    if (not cryptonote::generate_key_image_helper(
      account_keys,
      subaddresses,
      rct::rct2pk(src.outputs[src.real_output].second.dest),
      src.real_out_tx_key,
      src.real_out_additional_tx_keys,
      src.real_output_in_tx_index,
      tmp_key,
      tmp_image,
      hwdev
    )) {
      return false;
    }
    input_secret_keys[i] = rct::sk2rct(tmp_key.sec);
  }
  return true;
}

static void shuffle_destinations(
  std::vector<cryptonote::tx_destination_entry>& destinations
)
{
  std::shuffle(destinations.begin(), destinations.end(), crypto::random_device{});
}

static bool set_tx_extra(
  const cryptonote::account_keys& account_keys,
  const std::vector<cryptonote::tx_destination_entry>& destinations,
  const cryptonote::tx_destination_entry& change,
  const crypto::secret_key& tx_secret_key,
  const crypto::public_key& tx_public_key,
  const std::vector<crypto::public_key>& tx_aux_public_keys,
  const std::vector<std::uint8_t>& extra,
  cryptonote::transaction& tx
)
{
  hw::device &hwdev = account_keys.get_device();
  tx.extra = extra;
  // if we have a stealth payment id, find it and encrypt it with the tx key now
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  if (cryptonote::parse_tx_extra(tx.extra, tx_extra_fields))
  {
    bool add_dummy_payment_id = true;
    cryptonote::tx_extra_nonce extra_nonce;
    if (cryptonote::find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
    {
      crypto::hash payment_id = crypto::null_hash;
      crypto::hash8 payment_id8 = crypto::null_hash8;
      if (cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
      {
        LOG_PRINT_L2("Encrypting payment id " << payment_id8);
        crypto::public_key view_key_pub = cryptonote::get_destination_view_key_pub(destinations, change.addr);
        if (view_key_pub == crypto::null_pkey)
        {
          LOG_ERROR("Destinations have to have exactly one output to support encrypted payment ids");
          return false;
        }

        if (!hwdev.encrypt_payment_id(payment_id8, view_key_pub, tx_secret_key))
        {
          LOG_ERROR("Failed to encrypt payment id");
          return false;
        }

        std::string extra_nonce;
        cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id8);
        cryptonote::remove_field_from_tx_extra(tx.extra, typeid(cryptonote::tx_extra_nonce));
        if (!cryptonote::add_extra_nonce_to_tx_extra(tx.extra, extra_nonce))
        {
          LOG_ERROR("Failed to add encrypted payment id to tx extra");
          return false;
        }
        LOG_PRINT_L1("Encrypted payment ID: " << payment_id8);
        add_dummy_payment_id = false;
      }
      else if (cryptonote::get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
      {
        add_dummy_payment_id = false;
      }
    }

    // we don't add one if we've got more than the usual 1 destination plus change
    if (destinations.size() > 2)
      add_dummy_payment_id = false;

    if (add_dummy_payment_id)
    {
      // if we have neither long nor short payment id, add a dummy short one,
      // this should end up being the vast majority of txes as time goes on
      std::string extra_nonce;
      crypto::hash8 payment_id8 = crypto::null_hash8;
      crypto::public_key view_key_pub = cryptonote::get_destination_view_key_pub(destinations, change.addr);
      if (view_key_pub == crypto::null_pkey)
      {
        LOG_ERROR("Failed to get key to encrypt dummy payment id with");
      }
      else
      {
        hwdev.encrypt_payment_id(payment_id8, view_key_pub, tx_secret_key);
        cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id8);
        if (!cryptonote::add_extra_nonce_to_tx_extra(tx.extra, extra_nonce))
        {
          LOG_ERROR("Failed to add dummy encrypted payment id to tx extra");
          // continue anyway
        }
      }
    }
  }
  else
  {
    MWARNING("Failed to parse tx extra");
    tx_extra_fields.clear();
  }

  cryptonote::remove_field_from_tx_extra(tx.extra, typeid(cryptonote::tx_extra_pub_key));
  cryptonote::add_tx_pub_key_to_extra(tx.extra, tx_public_key);
  cryptonote::remove_field_from_tx_extra(tx.extra, typeid(cryptonote::tx_extra_additional_pub_keys));
  LOG_PRINT_L2("tx pubkey: " << tx_public_key);
  if (tx_aux_public_keys.size())
  {
    LOG_PRINT_L2("additional tx pubkeys: ");
    for (size_t i = 0; i < tx_aux_public_keys.size(); ++i)
      LOG_PRINT_L2(tx_aux_public_keys[i]);
    cryptonote::add_additional_tx_pub_keys_to_extra(tx.extra, tx_aux_public_keys);
  }
  if (not cryptonote::sort_tx_extra(tx.extra, tx.extra))
    return false;
  return true;
}

static bool compute_keys_for_destinations(
  const cryptonote::account_keys& account_keys,
  const std::uint32_t subaddr_account,
  const std::vector<cryptonote::tx_destination_entry>& destinations,
  const cryptonote::tx_destination_entry& change,
  const std::vector<std::uint8_t>& extra,
  const bool reconstruction,
  crypto::secret_key& tx_secret_key,
  std::vector<crypto::secret_key>& tx_aux_secret_keys,
  rct::keyV& output_public_keys,
  rct::keyV& output_amount_secret_keys,
  cryptonote::transaction& unsigned_tx
)
{
  const std::size_t dim_destinations = destinations.size();
  crypto::public_key tx_public_key;
  std::vector<crypto::public_key> tx_aux_public_keys;
  hw::device &hwdev = account_keys.get_device();
  if (change.amount and change.addr != hwdev.get_subaddress(account_keys, {subaddr_account}))
      return false;
  std::unordered_set<cryptonote::account_public_address> unique_sub;
  std::unordered_set<cryptonote::account_public_address> unique_std;
  for(const auto& dst_entr: destinations) {
    if (dst_entr.addr == change.addr)
      continue;
    if (dst_entr.is_subaddress)
      unique_sub.insert(dst_entr.addr);
    else
      unique_std.insert(dst_entr.addr);
  }
  if (not reconstruction) {
    tx_secret_key = rct::rct2sk(rct::skGen());
  }
  tx_public_key = rct::rct2pk(
    unique_std.empty() && unique_sub.size() == 1 ?
    hwdev.scalarmultKey(
      rct::pk2rct(unique_sub.begin()->m_spend_public_key),
      rct::sk2rct(tx_secret_key)
    ) :
    hwdev.scalarmultBase(rct::sk2rct(tx_secret_key))
  );
  const bool need_tx_aux_keys = unique_sub.size() + bool(unique_std.size()) > 1;
  if (not reconstruction and need_tx_aux_keys) {
    for(std::size_t i = 0; i < dim_destinations; ++i)
      tx_aux_secret_keys.push_back(rct::rct2sk(rct::skGen()));
  }
  output_public_keys.resize(dim_destinations);
  for (std::size_t i = 0; i < dim_destinations; ++i) {
    if (not hwdev.generate_output_ephemeral_keys(
      unsigned_tx.version,
      account_keys,
      tx_public_key,
      tx_secret_key,
      destinations[i],
      change.addr,
      i,
      need_tx_aux_keys,
      tx_aux_secret_keys,
      tx_aux_public_keys,
      output_amount_secret_keys,
      reinterpret_cast<crypto::public_key &>(output_public_keys[i])
    )) {
      return false;
    }
  }
  if (dim_destinations != output_amount_secret_keys.size())
    return false;
  CHECK_AND_ASSERT_MES(
    tx_aux_public_keys.size() == tx_aux_secret_keys.size(),
    false,
    "Internal error creating additional public keys"
  );
  if (not set_tx_extra(account_keys, destinations, change, tx_secret_key, tx_public_key, tx_aux_public_keys, extra, unsigned_tx))
    return false;
  return true;
}

static void set_tx_inputs(
  const std::vector<cryptonote::tx_source_entry>& sources,
  cryptonote::transaction& unsigned_tx
)
{
  const std::size_t dim_sources = sources.size();
  unsigned_tx.vin.resize(dim_sources);
  for (std::size_t i = 0; i < dim_sources; ++i) {
    std::vector<std::uint64_t> offsets;
    for (const auto& e: sources[i].outputs)
      offsets.push_back(e.first);
    unsigned_tx.vin[i] = cryptonote::txin_to_key{
      .amount = 0,
      .key_offsets = cryptonote::absolute_output_offsets_to_relative(
        offsets
      ),
      .k_image = rct::rct2ki(sources[i].multisig_kLRki.ki),
    };
  }
}

static void set_tx_outputs(
  const rct::keyV& output_public_keys,
  cryptonote::transaction& unsigned_tx
)
{
  const std::size_t dim_destinations = output_public_keys.size();
  unsigned_tx.vout.resize(dim_destinations);
  for (std::size_t i = 0; i < dim_destinations; ++i) {
    unsigned_tx.vout[i] = cryptonote::tx_out{
      .amount = 0,
      .target = cryptonote::txout_to_key(rct::rct2pk(output_public_keys[i])),
    };
  }
}

static bool set_tx_rct_signatures(
  const std::uint64_t fee,
  const std::vector<cryptonote::tx_source_entry>& sources,
  const std::vector<cryptonote::tx_destination_entry>& destinations,
  const rct::keyV& input_secret_keys,
  const rct::keyV& output_public_keys,
  const rct::keyV& output_amount_secret_keys,
  const rct::RCTConfig& rct_config,
  const bool reconstruction,
  cryptonote::transaction& unsigned_tx,
  std::vector<cached_CLSAG_Gen_t>& cached_CLSAG,
  rct::keyV& cached_w
)
{
  const std::size_t dim_destinations = destinations.size();
  const std::size_t dim_sources = sources.size();
  if (rct_config.bp_version != 3)
    return false;
  if (rct_config.range_proof_type != rct::RangeProofPaddedBulletproof)
    return false;
  rct::rctSig rv{};
  rv.type = rct::RCTTypeCLSAG;
  rv.txnFee = fee;
  rv.message = rct::hash2rct(cryptonote::get_transaction_prefix_hash(unsigned_tx));

  std::vector<std::uint64_t> output_amounts(dim_destinations);
  rct::keyV output_amount_masks(dim_destinations);
  rv.ecdhInfo.resize(dim_destinations);
  rv.outPk.resize(dim_destinations);
  for (std::size_t i = 0; i < dim_destinations; ++i) {
    rv.outPk[i].dest = output_public_keys[i];
    output_amounts[i] = destinations[i].amount;
    output_amount_masks[i] = genCommitmentMask(output_amount_secret_keys[i]);
    rv.ecdhInfo[i].amount = rct::d2h(output_amounts[i]);
    rct::addKeys2(
      rv.outPk[i].mask,
      output_amount_masks[i],
      rv.ecdhInfo[i].amount,
      rct::H
    );
    rct::ecdhEncode(rv.ecdhInfo[i], output_amount_secret_keys[i], true);
  }

  if (not reconstruction) {
    rv.p.bulletproofs.push_back(rct::bulletproof_PROVE(output_amounts, output_amount_masks));
  }
  else {
    rv.p.bulletproofs = unsigned_tx.rct_signatures.p.bulletproofs;
    if (rv.p.bulletproofs.size() != 1)
      return false;
    rv.p.bulletproofs[0].V.resize(dim_destinations);
    for (std::size_t i = 0; i < dim_destinations; ++i) {
      rv.p.bulletproofs[0].V[i] = rct::scalarmultKey(rv.outPk[i].mask, rct::INV_EIGHT);
    }
    if (not bulletproof_VERIFY(rv.p.bulletproofs))
      return false;
  }

  rv.mixRing.resize(dim_sources);
  for (std::size_t i = 0; i < dim_sources; ++i) {
    const std::size_t dim_ring = sources[i].outputs.size();
    rv.mixRing[i].resize(dim_ring);
    for (std::size_t j = 0; j < dim_ring; ++j) {
      rv.mixRing[i][j].dest = sources[i].outputs[j].second.dest;
      rv.mixRing[i][j].mask = sources[i].outputs[j].second.mask;
    }
  }

  const rct::key message = get_pre_mlsag_hash(rv, hw::get_device("default"));

  rct::keyV a;
  auto a_wiper = epee::misc_utils::create_scope_leave_handler([&]{
    memwipe(static_cast<rct::key *>(a.data()), a.size() * sizeof(rct::key));
  });
  if (not reconstruction) {
    a.resize(dim_sources);
    rv.p.pseudoOuts.resize(dim_sources);
    a[dim_sources - 1] = rct::zero();
    for (std::size_t i = 0; i < dim_destinations; ++i) {
      sc_add(
        a[dim_sources - 1].bytes,
        a[dim_sources - 1].bytes,
        output_amount_masks[i].bytes
      );
    }
    for (std::size_t i = 0; i < dim_sources - 1; ++i) {
      rct::skGen(a[i]);
      sc_sub(
        a[dim_sources - 1].bytes,
        a[dim_sources - 1].bytes,
        a[i].bytes
      );
      rct::genC(rv.p.pseudoOuts[i], a[i], sources[i].amount);
    }
    rct::genC(
      rv.p.pseudoOuts[dim_sources - 1],
      a[dim_sources - 1],
      sources[dim_sources - 1].amount
    );
  }
  else {
    rv.p.pseudoOuts = unsigned_tx.rct_signatures.p.pseudoOuts;
    if (dim_sources != rv.p.pseudoOuts.size())
      return false;
    rct::key tmp = rct::scalarmultH(rct::d2h(fee));
    for (const auto& e: rv.outPk)
      rct::addKeys(tmp, tmp, e.mask);
    for (std::size_t i = 0; i < dim_sources; ++i)
      rct::subKeys(tmp, tmp, rv.p.pseudoOuts[i]);
    if (not (tmp == rct::identity()))
      return false;
  }


  rv.p.CLSAGs.resize(dim_sources);
  if (reconstruction) {
    if (dim_sources != unsigned_tx.rct_signatures.p.CLSAGs.size())
      return false;
  }

  cached_CLSAG.resize(dim_sources);
  if (not reconstruction)
    cached_w.resize(dim_sources);

  for (std::size_t i = 0; i < dim_sources; ++i) {
    const std::size_t dim_ring = rv.mixRing[i].size();
    const rct::key& I = sources[i].multisig_kLRki.ki;
    const std::size_t& l = sources[i].real_output;
    if (l >= dim_ring)
      return false;
    rct::keyV& s = rv.p.CLSAGs[i].s;
    const rct::key& C_offset = rv.p.pseudoOuts[i];
    rct::keyV P(dim_ring);
    rct::keyV C_nonzero(dim_ring);

    if (not reconstruction) {
      s.resize(dim_ring);
      for (std::size_t j = 0; j < dim_ring; ++j) {
        if (j != l)
          s[j] = rct::skGen();
      }
    }
    else {
      if (dim_ring != unsigned_tx.rct_signatures.p.CLSAGs[i].s.size())
        return false;
      s = unsigned_tx.rct_signatures.p.CLSAGs[i].s;
    }

    for (std::size_t j = 0; j < dim_ring; ++j) {
      P[j] = rv.mixRing[i][j].dest;
      C_nonzero[j] = rv.mixRing[i][j].mask;
    }

    rct::key D;
    rct::key z;
    auto z_wiper = epee::misc_utils::create_scope_leave_handler([&]{
      memwipe(static_cast<rct::key *>(&z), sizeof(rct::key));
    });
    if (not reconstruction) {
      sc_sub(z.bytes, sources[i].mask.bytes, a[i].bytes);
      ge_p3 H_p3;
      rct::hash_to_p3(H_p3, rv.mixRing[i][l].dest);
      rct::key H_l;
      ge_p3_tobytes(H_l.bytes, &H_p3);
      D = rct::scalarmultKey(H_l, z);
      rv.p.CLSAGs[i].D = rct::scalarmultKey(D, rct::INV_EIGHT);
      rv.p.CLSAGs[i].I = I;
    }
    else {
      rv.p.CLSAGs[i].D = unsigned_tx.rct_signatures.p.CLSAGs[i].D;
      rv.p.CLSAGs[i].I = I;
      D = rct::scalarmultKey(rv.p.CLSAGs[i].D, rct::EIGHT);
    }

    if (not cached_CLSAG[i].init(P, C_nonzero, C_offset, message, I, D, l, s))
      return false;

    if (not reconstruction) {
      rct::key mu_P;
      rct::key mu_C;
      if (not cached_CLSAG[i].get_mu(mu_P, mu_C))
        return false;
      sc_mul(cached_w[i].bytes, mu_P.bytes, input_secret_keys[i].bytes);
      sc_muladd(cached_w[i].bytes, mu_C.bytes, z.bytes, cached_w[i].bytes);
    }
  }
  unsigned_tx.rct_signatures = rv;
  return true;
}

static bool compute_tx_fee(
  const std::vector<cryptonote::tx_source_entry>& sources,
  const std::vector<cryptonote::tx_destination_entry>& destinations,
  std::uint64_t& fee
)
{
  std::uint64_t in_amount = 0;
  for (const auto& src: sources)
    in_amount += src.amount;

  std::uint64_t out_amount = 0;
  for (const auto& dst: destinations)
    out_amount += dst.amount;

  if(out_amount > in_amount)
    return false;
  fee = in_amount - out_amount;
  return true;
}

bool tx_builder_t::init(
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
)
{
  initialized = false;
  this->reconstruction = reconstruction;
  if (not use_rct)
    return false;
  if (sources.empty())
    return false;
  if (not reconstruction)
    unsigned_tx.set_null();
  std::uint64_t fee;
  if (not compute_tx_fee(sources, destinations, fee))
    return false;
  unsigned_tx.version = 2;
  unsigned_tx.unlock_time = unlock_time;
  sort_sources(sources);
  rct::keyV input_secret_keys;
  auto input_secret_keys_wiper = epee::misc_utils::create_scope_leave_handler([&]{
    memwipe(static_cast<rct::key *>(input_secret_keys.data()), input_secret_keys.size() * sizeof(rct::key));
  });
  if (not compute_keys_for_sources(account_keys, sources, subaddr_account, subaddr_minor_indices, input_secret_keys))
    return false;
  if (not reconstruction)
    shuffle_destinations(destinations);
  rct::keyV output_public_keys;
  rct::keyV output_amount_secret_keys;
  auto output_amount_secret_keys_wiper = epee::misc_utils::create_scope_leave_handler([&]{
    memwipe(static_cast<rct::key *>(output_amount_secret_keys.data()), output_amount_secret_keys.size() * sizeof(rct::key));
  });
  if (not compute_keys_for_destinations(account_keys, subaddr_account, destinations, change, extra, reconstruction, tx_secret_key, tx_aux_secret_keys, output_public_keys, output_amount_secret_keys, unsigned_tx))
    return false;
  set_tx_inputs(sources, unsigned_tx);
  set_tx_outputs(output_public_keys, unsigned_tx);
  if (not set_tx_rct_signatures(fee, sources, destinations, input_secret_keys, output_public_keys, output_amount_secret_keys, rct_config, reconstruction, unsigned_tx, cached_CLSAG, cached_w))
    return false;
  initialized = true;
  return true;
}

bool tx_builder_t::first_partial_sign(
  const std::size_t source,
  const rct::keyV& total_alpha_G,
  const rct::keyV& total_alpha_H,
  const rct::keyV& alpha,
  rct::key& c_0,
  rct::key& s
)
{
  if (not initialized or reconstruction)
    return false;
  const std::size_t dim_sources = cached_CLSAG.size();
  if (source >= dim_sources)
    return false;
  rct::key c;
  rct::key alpha_combined;
  auto alpha_combined_wiper = epee::misc_utils::create_scope_leave_handler([&]{
    memwipe(static_cast<rct::key *>(&alpha_combined), sizeof(rct::key));
  });
  if (not cached_CLSAG[source].combine_alpha_and_compute_challenge(
    total_alpha_G,
    total_alpha_H,
    alpha,
    alpha_combined,
    c_0,
    c
  )) {
    return false;
  }
  sc_mulsub(s.bytes, c.bytes, cached_w[source].bytes, alpha_combined.bytes);
  return true;
}

bool tx_builder_t::next_partial_sign(
  const rct::keyM& total_alpha_G,
  const rct::keyM& total_alpha_H,
  const rct::keyM& alpha,
  const rct::key& x,
  rct::keyV& c_0,
  rct::keyV& s
)
{
  if (not initialized or not reconstruction)
    return false;
  const std::size_t dim_sources = cached_CLSAG.size();
  if (dim_sources != total_alpha_G.size())
    return false;
  if (dim_sources != total_alpha_H.size())
    return false;
  if (dim_sources != alpha.size())
    return false;
  if (dim_sources != c_0.size())
    return false;
  if (dim_sources != s.size())
    return false;
  for (std::size_t i = 0; i < dim_sources; ++i) {
    rct::key c;
    rct::key alpha_combined;
    auto alpha_combined_wiper = epee::misc_utils::create_scope_leave_handler([&]{
      memwipe(static_cast<rct::key *>(&alpha_combined), sizeof(rct::key));
    });
    if (not cached_CLSAG[i].combine_alpha_and_compute_challenge(
      total_alpha_G[i],
      total_alpha_H[i],
      alpha[i],
      alpha_combined,
      c_0[i],
      c
    )) {
      return false;
    }
    rct::key mu_P;
    rct::key mu_C;
    if (not cached_CLSAG[i].get_mu(mu_P, mu_C))
      return false;
    rct::key w;
    auto w_wiper = epee::misc_utils::create_scope_leave_handler([&]{
      memwipe(static_cast<rct::key *>(&w), sizeof(rct::key));
    });
    sc_mul(w.bytes, mu_P.bytes, x.bytes);
    sc_mulsub(s[i].bytes, c.bytes, w.bytes, s[i].bytes);
    sc_add(s[i].bytes, s[i].bytes, alpha_combined.bytes);
  }
  return true;
}

bool tx_builder_t::construct_tx(
  const std::vector<cryptonote::tx_source_entry>& sources,
  const rct::keyV& c_0,
  const rct::keyV& s,
  cryptonote::transaction& unsigned_tx
)
{
  const std::size_t dim_sources = sources.size();
  if (dim_sources != unsigned_tx.rct_signatures.p.CLSAGs.size())
    return false;
  if (dim_sources != c_0.size())
    return false;
  if (dim_sources != s.size())
    return false;
  for (std::size_t i = 0; i < dim_sources; ++i) {
    const std::size_t dim_ring = unsigned_tx.rct_signatures.p.CLSAGs[i].s.size();
    if (sources[i].real_output >= dim_ring)
      return false;
    unsigned_tx.rct_signatures.p.CLSAGs[i].s[sources[i].real_output] = s[i];
    unsigned_tx.rct_signatures.p.CLSAGs[i].c1 = c_0[i];
  }
  return true;
}

}

}
