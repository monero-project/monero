// Copyright (c) 2017-2024, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "version.h"
#include "protocol.hpp"
#include <unordered_map>
#include <set>
#include <utility>
#include <boost/endian/conversion.hpp>
#include <common/apply_permutation.h>
#include <common/json_util.h>
#include <crypto/hmac-keccak.h>
#include <ringct/rctSigs.h>
#include <ringct/bulletproofs.h>
#include <ringct/bulletproofs_plus.h>
#include "cryptonote_config.h"
#include <sodium.h>
#include <sodium/crypto_verify_32.h>
#include <sodium/crypto_aead_chacha20poly1305.h>

#define GET_FIELD_STRING(name, type, jtype) field_##name = std::string(json[#name].GetString(), json[#name].GetStringLength())
#define GET_FIELD_OTHER(name, type, jtype) field_##name = static_cast<type>(json[#name].Get##jtype())

#define GET_STRING_FROM_JSON(json, name, type, mandatory, def) \
  GET_FIELD_FROM_JSON_EX(json, name, type, String, mandatory, def, GET_FIELD_STRING)

#define GET_FIELD_FROM_JSON(json, name, type, jtype, mandatory, def) \
  GET_FIELD_FROM_JSON_EX(json, name, type, jtype, mandatory, def, GET_FIELD_OTHER)

#define GET_FIELD_FROM_JSON_EX(json, name, type, jtype, mandatory, def, VAL) \
  type field_##name = static_cast<type>(def);                        \
  bool field_##name##_found = false;                                 \
  (void)field_##name##_found;                                        \
  do if (json.HasMember(#name))                                      \
  {                                                                  \
    if (json[#name].Is##jtype())                                     \
    {                                                                \
      VAL(name, type, jtype);                                        \
      field_##name##_found = true;                                   \
    }                                                                \
    else                                                             \
    {                                                                \
      throw std::invalid_argument("Field " #name " found in JSON, but not " #jtype); \
    }                                                                \
  }                                                                  \
  else if (mandatory)                                                \
  {                                                                  \
    throw std::invalid_argument("Field " #name " not found in JSON");\
  } while(0)


namespace hw{
namespace trezor{
namespace protocol{

  std::string key_to_string(const ::crypto::ec_point & key){
    return std::string(key.data, sizeof(key.data));
  }

  std::string key_to_string(const ::crypto::ec_scalar & key){
    return std::string(key.data, sizeof(key.data));
  }

  std::string key_to_string(const ::crypto::hash & key){
    return std::string(key.data, sizeof(key.data));
  }

  std::string key_to_string(const ::rct::key & key){
    return std::string(reinterpret_cast<const char*>(key.bytes), sizeof(key.bytes));
  }

  void string_to_key(::crypto::ec_scalar & key, const std::string & str){
    if (str.size() != sizeof(key.data)){
      throw std::invalid_argument(std::string("Key has to have ") + std::to_string(sizeof(key.data)) + " B");
    }
    memcpy(key.data, str.data(), sizeof(key.data));
  }

  void string_to_key(::crypto::ec_point & key, const std::string & str){
    if (str.size() != sizeof(key.data)){
      throw std::invalid_argument(std::string("Key has to have ") + std::to_string(sizeof(key.data)) + " B");
    }
    memcpy(key.data, str.data(), sizeof(key.data));
  }

  void string_to_key(::rct::key & key, const std::string & str){
    if (str.size() != sizeof(key.bytes)){
      throw std::invalid_argument(std::string("Key has to have ") + std::to_string(sizeof(key.bytes)) + " B");
    }
    memcpy(key.bytes, str.data(), sizeof(key.bytes));
  }

namespace crypto {
namespace chacha {

  void decrypt(const void* ciphertext, size_t length, const uint8_t* key, const uint8_t* iv, char* plaintext, size_t *plaintext_len){
    CHECK_AND_ASSERT_THROW_MES(length >= TAG_SIZE, "Ciphertext length too small");
    CHECK_AND_ASSERT_THROW_MES(!plaintext_len || *plaintext_len >= (length - TAG_SIZE), "Plaintext length too small");

    unsigned long long int res_len = plaintext_len ? *plaintext_len : length;
    auto r = crypto_aead_chacha20poly1305_ietf_decrypt(
        reinterpret_cast<unsigned char *>(plaintext), &res_len, nullptr,
        static_cast<const unsigned char *>(ciphertext), length, nullptr, 0, iv, key);

    if (r != 0){
      throw exc::Poly1305TagInvalid();
    }

    if (plaintext_len){
      *plaintext_len = (size_t) res_len;
    }
  }

}
}


// Cold Key image sync
namespace ki {

  bool key_image_data(wallet_shim * wallet,
                      const std::vector<tools::wallet2::transfer_details> & transfers,
                      std::vector<MoneroTransferDetails> & res)
  {
    for(auto & td : transfers){
      ::crypto::public_key tx_pub_key = wallet->get_tx_pub_key_from_received_outs(td);
      const std::vector<::crypto::public_key> additional_tx_pub_keys = cryptonote::get_additional_tx_pub_keys_from_extra(td.m_tx);

      res.emplace_back();
      auto & cres = res.back();
      cres.set_out_key(key_to_string(td.get_public_key()));
      cres.set_tx_pub_key(key_to_string(tx_pub_key));
      cres.set_internal_output_index(td.m_internal_output_index);
      cres.set_sub_addr_major(td.m_subaddr_index.major);
      cres.set_sub_addr_minor(td.m_subaddr_index.minor);
      if (!additional_tx_pub_keys.empty() && additional_tx_pub_keys.size() > td.m_internal_output_index) {
        cres.add_additional_tx_pub_keys(key_to_string(additional_tx_pub_keys[td.m_internal_output_index]));
      }
    }

    return true;
  }

  std::string compute_hash(const MoneroTransferDetails & rr){
    KECCAK_CTX kck;
    uint8_t md[32];

    CHECK_AND_ASSERT_THROW_MES(rr.out_key().size() == 32, "Invalid out_key size");
    CHECK_AND_ASSERT_THROW_MES(rr.tx_pub_key().size() == 32, "Invalid tx_pub_key size");

    keccak_init(&kck);
    keccak_update(&kck, reinterpret_cast<const uint8_t *>(rr.out_key().data()), 32);
    keccak_update(&kck, reinterpret_cast<const uint8_t *>(rr.tx_pub_key().data()), 32);
    for (const auto &aux : rr.additional_tx_pub_keys()){
      CHECK_AND_ASSERT_THROW_MES(aux.size() == 32, "Invalid aux size");
      keccak_update(&kck, reinterpret_cast<const uint8_t *>(aux.data()), 32);
    }

    auto index_serialized = tools::get_varint_data(rr.internal_output_index());
    keccak_update(&kck, reinterpret_cast<const uint8_t *>(index_serialized.data()), index_serialized.size());
    keccak_finish(&kck, md);
    return std::string(reinterpret_cast<const char*>(md), sizeof(md));
  }

  void generate_commitment(std::vector<MoneroTransferDetails> & mtds,
                           const std::vector<tools::wallet2::transfer_details> & transfers,
                           std::shared_ptr<messages::monero::MoneroKeyImageExportInitRequest> & req)
  {
    req = std::make_shared<messages::monero::MoneroKeyImageExportInitRequest>();

    KECCAK_CTX kck;
    uint8_t final_hash[32];
    keccak_init(&kck);

    for(auto &cur : mtds){
      auto hash = compute_hash(cur);
      keccak_update(&kck, reinterpret_cast<const uint8_t *>(hash.data()), hash.size());
    }
    keccak_finish(&kck, final_hash);

    req = std::make_shared<messages::monero::MoneroKeyImageExportInitRequest>();
    req->set_hash(std::string(reinterpret_cast<const char*>(final_hash), 32));
    req->set_num(transfers.size());

    std::unordered_map<uint32_t, std::set<uint32_t>> sub_indices;
    for (auto &cur : transfers){
      auto search = sub_indices.emplace(cur.m_subaddr_index.major, std::set<uint32_t>());
      auto & st = search.first->second;
      st.insert(cur.m_subaddr_index.minor);
    }
  }

  void live_refresh_ack(const ::crypto::secret_key & view_key_priv,
                        const ::crypto::public_key& out_key,
                        const std::shared_ptr<messages::monero::MoneroLiveRefreshStepAck> & ack,
                        ::cryptonote::keypair& in_ephemeral,
                        ::crypto::key_image& ki)
  {
    std::string str_out_key(out_key.data, sizeof(out_key.data));
    auto enc_key = protocol::tx::compute_enc_key(view_key_priv, str_out_key, ack->salt());

    const size_t len_ciphertext = ack->key_image().size();  // IV || keys
    CHECK_AND_ASSERT_THROW_MES(len_ciphertext > crypto::chacha::IV_SIZE + crypto::chacha::TAG_SIZE, "Invalid size");

    size_t ki_len = len_ciphertext - crypto::chacha::IV_SIZE - crypto::chacha::TAG_SIZE;
    std::unique_ptr<uint8_t[]> plaintext(new uint8_t[ki_len]);
    uint8_t * buff = plaintext.get();

    protocol::crypto::chacha::decrypt(
        ack->key_image().data() + crypto::chacha::IV_SIZE,
        len_ciphertext - crypto::chacha::IV_SIZE,
        reinterpret_cast<const uint8_t *>(enc_key.data),
        reinterpret_cast<const uint8_t *>(ack->key_image().data()),
        reinterpret_cast<char *>(buff), &ki_len);

    CHECK_AND_ASSERT_THROW_MES(ki_len == 3*32, "Invalid size");
    ::crypto::signature sig{};
    memcpy(ki.data, buff, 32);
    memcpy(sig.c.data, buff + 32, 32);
    memcpy(sig.r.data, buff + 64, 32);
    in_ephemeral.pub = out_key;
    in_ephemeral.sec = ::crypto::null_skey;

    // Verification
    std::vector<const ::crypto::public_key*> pkeys;
    pkeys.push_back(&out_key);

    CHECK_AND_ASSERT_THROW_MES(rct::scalarmultKey(rct::ki2rct(ki), rct::curveOrder()) == rct::identity(),
                               "Key image out of validity domain: key image " << epee::string_tools::pod_to_hex(ki));

    CHECK_AND_ASSERT_THROW_MES(::crypto::check_ring_signature((const ::crypto::hash&)ki, ki, pkeys, &sig),
                               "Signature failed for key image " << epee::string_tools::pod_to_hex(ki)
                                                                 << ", signature " + epee::string_tools::pod_to_hex(sig)
                                                                 << ", pubkey " + epee::string_tools::pod_to_hex(*pkeys[0]));
  }
}

// Cold transaction signing
namespace tx {

  void translate_address(MoneroAccountPublicAddress * dst, const cryptonote::account_public_address * src){
    dst->set_view_public_key(key_to_string(src->m_view_public_key));
    dst->set_spend_public_key(key_to_string(src->m_spend_public_key));
  }

  void translate_dst_entry(MoneroTransactionDestinationEntry * dst, const cryptonote::tx_destination_entry * src){
    dst->set_amount(src->amount);
    dst->set_is_subaddress(src->is_subaddress);
    dst->set_is_integrated(src->is_integrated);
    dst->set_original(src->original);
    translate_address(dst->mutable_addr(), &(src->addr));
  }

  void translate_klrki(MoneroMultisigKLRki * dst, const rct::multisig_kLRki * src){
    dst->set_k(key_to_string(src->k));
    dst->set_l(key_to_string(src->L));
    dst->set_r(key_to_string(src->R));
    dst->set_ki(key_to_string(src->ki));
  }

  void translate_rct_key(MoneroRctKey * dst, const rct::ctkey * src){
    dst->set_dest(key_to_string(src->dest));
    dst->set_commitment(key_to_string(src->mask));
  }

  std::string hash_addr(const MoneroAccountPublicAddress * addr, boost::optional<uint64_t> amount, boost::optional<bool> is_subaddr){
    return hash_addr(addr->spend_public_key(), addr->view_public_key(), amount, is_subaddr);
  }

  std::string hash_addr(const std::string & spend_key, const std::string & view_key, boost::optional<uint64_t> amount, boost::optional<bool> is_subaddr){
    ::crypto::public_key spend{}, view{};
    if (spend_key.size() != 32 || view_key.size() != 32){
      throw std::invalid_argument("Public keys have invalid sizes");
    }

    memcpy(spend.data, spend_key.data(), 32);
    memcpy(view.data, view_key.data(), 32);
    return hash_addr(&spend, &view, amount, is_subaddr);
  }

  std::string hash_addr(const ::crypto::public_key * spend_key, const ::crypto::public_key * view_key, boost::optional<uint64_t> amount, boost::optional<bool> is_subaddr){
    char buff[64+8+1];
    size_t offset = 0;

    memcpy(buff + offset, spend_key->data, 32); offset += 32;
    memcpy(buff + offset, view_key->data, 32); offset += 32;

    if (amount){
      memcpy(buff + offset, (uint8_t*) &(amount.get()), sizeof(amount.get())); offset += sizeof(amount.get());
    }

    if (is_subaddr){
      buff[offset] = is_subaddr.get();
      offset += 1;
    }

    return std::string(buff, offset);
  }

  ::crypto::secret_key compute_enc_key(const ::crypto::secret_key & private_view_key, const std::string & aux, const std::string & salt)
  {
    uint8_t hash[32];
    KECCAK_CTX ctx;
    ::crypto::secret_key res;

    keccak_init(&ctx);
    keccak_update(&ctx, (const uint8_t *) private_view_key.data, sizeof(private_view_key.data));
    if (!aux.empty()){
      keccak_update(&ctx, (const uint8_t *) aux.data(), aux.size());
    }
    keccak_finish(&ctx, hash);
    keccak(hash, sizeof(hash), hash, sizeof(hash));

    hmac_keccak_hash(hash, (const uint8_t *) salt.data(), salt.size(), hash, sizeof(hash));
    memcpy(res.data, hash, sizeof(hash));
    memwipe(hash, sizeof(hash));
    return res;
  }

  std::string compute_sealing_key(const std::string & master_key, size_t idx, bool is_iv)
  {
    // master-key-32B || domain-sep-12B || index-4B
    uint8_t hash[32] = {0};
    KECCAK_CTX ctx;
    std::string sep = is_iv ? "sig-iv" : "sig-key";
    std::string idx_data = tools::get_varint_data(idx);
    if (idx_data.size() > 4){
      throw std::invalid_argument("index is too big");
    }

    keccak_init(&ctx);
    keccak_update(&ctx, (const uint8_t *) master_key.data(), master_key.size());
    keccak_update(&ctx, (const uint8_t *) sep.data(), sep.size());
    keccak_update(&ctx, hash, 12 - sep.size());
    keccak_update(&ctx, (const uint8_t *) idx_data.data(), idx_data.size());
    if (idx_data.size() < 4) {
      keccak_update(&ctx, hash, 4 - idx_data.size());
    }

    keccak_finish(&ctx, hash);
    keccak(hash, sizeof(hash), hash, sizeof(hash));
    return std::string((const char*) hash, 32);
  }

  TData::TData() {
    rsig_type = 0;
    bp_version = 0;
    cur_input_idx = 0;
    cur_output_idx = 0;
    cur_batch_idx = 0;
    cur_output_in_batch_idx = 0;
  }

  Signer::Signer(wallet_shim *wallet2, const unsigned_tx_set * unsigned_tx, size_t tx_idx, hw::tx_aux_data * aux_data) {
    m_wallet2 = wallet2;
    m_unsigned_tx = unsigned_tx;
    m_aux_data = aux_data;
    m_tx_idx = tx_idx;
    m_ct.tx_data = cur_src_tx();
    m_multisig = false;
    m_client_version = 3;
  }

  void Signer::extract_payment_id(){
    const std::vector<uint8_t>& tx_extra = cur_tx().extra;
    m_ct.tsx_data.set_payment_id("");

    std::vector<cryptonote::tx_extra_field> tx_extra_fields;
    cryptonote::parse_tx_extra(tx_extra, tx_extra_fields); // ok if partially parsed
    cryptonote::tx_extra_nonce extra_nonce;

    ::crypto::hash payment_id{};
    if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
    {
      ::crypto::hash8 payment_id8{};
      if(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
      {
        m_ct.tsx_data.set_payment_id(std::string(payment_id8.data, 8));
      }
      else if (cryptonote::get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
      {
        m_ct.tsx_data.set_payment_id(std::string(payment_id.data, 32));
      }
    }
  }

  static unsigned get_rsig_type(const rct::RCTConfig &rct_config, size_t num_outputs){
    if (rct_config.range_proof_type == rct::RangeProofBorromean){
      return rct::RangeProofBorromean;
    } else if (num_outputs > BULLETPROOF_MAX_OUTPUTS){
      return rct::RangeProofMultiOutputBulletproof;
    } else {
      return rct::RangeProofPaddedBulletproof;
    }
  }

  static void generate_rsig_batch_sizes(std::vector<uint64_t> &batches, unsigned rsig_type, size_t num_outputs){
    size_t amount_batched = 0;

    while(amount_batched < num_outputs){
      if (rsig_type == rct::RangeProofBorromean || rsig_type == rct::RangeProofBulletproof) {
        batches.push_back(1);
        amount_batched += 1;

      } else if (rsig_type == rct::RangeProofPaddedBulletproof){
        if (num_outputs > BULLETPROOF_MAX_OUTPUTS){
          throw std::invalid_argument("BP padded can support only BULLETPROOF_MAX_OUTPUTS statements");
        }
        batches.push_back(num_outputs);
        amount_batched += num_outputs;

      } else if (rsig_type == rct::RangeProofMultiOutputBulletproof){
        size_t batch_size = 1;
        while (batch_size * 2 + amount_batched <= num_outputs && batch_size * 2 <= BULLETPROOF_MAX_OUTPUTS){
          batch_size *= 2;
        }
        batch_size = std::min(batch_size, num_outputs - amount_batched);
        batches.push_back(batch_size);
        amount_batched += batch_size;

      } else {
        throw std::invalid_argument("Unknown rsig type");
      }
    }
  }

  void Signer::set_tx_input(MoneroTransactionSourceEntry * dst, size_t idx, bool need_ring_keys, bool need_ring_indices){
    const cryptonote::tx_source_entry & src = cur_tx().sources[idx];
    const tools::wallet2::transfer_details & transfer = get_source_transfer(idx);

    dst->set_real_output(src.real_output);
    for(size_t i = 0; i < src.outputs.size(); ++i){
      auto & cur = src.outputs[i];
      auto out = dst->add_outputs();

      if (i == src.real_output || need_ring_indices) {
        out->set_idx(cur.first);
      }
      if (i == src.real_output || need_ring_keys) {
        translate_rct_key(out->mutable_key(), &(cur.second));
      }
    }

    dst->set_real_out_tx_key(key_to_string(src.real_out_tx_key));
    dst->set_real_output_in_tx_index(src.real_output_in_tx_index);
    if (!src.real_out_additional_tx_keys.empty()) {
      dst->add_real_out_additional_tx_keys(key_to_string(src.real_out_additional_tx_keys.at(src.real_output_in_tx_index)));
    }
    dst->set_amount(src.amount);
    dst->set_rct(src.rct);
    dst->set_mask(key_to_string(src.mask));
    translate_klrki(dst->mutable_multisig_klrki(), &(src.multisig_kLRki));
    dst->set_subaddr_minor(transfer.m_subaddr_index.minor);
  }

  void Signer::compute_integrated_indices(TsxData * tsx_data){
    auto & chg = tsx_data->change_dts();
    std::string change_hash = hash_addr(&chg.addr(), chg.amount(), chg.is_subaddress());
    std::vector<uint32_t> integrated_indices;

    ssize_t idx = -1;
    for (auto & cur : tsx_data->outputs()){
      idx += 1;

      std::string c_hash = hash_addr(&cur.addr(), cur.amount(), cur.is_subaddress());
      if (c_hash == change_hash || cur.is_subaddress()){
        continue;
      }

      if (cur.is_integrated()){
        integrated_indices.push_back((uint32_t)idx);
      }
    }

    if (!integrated_indices.empty()){
      assign_to_repeatable(tsx_data->mutable_integrated_indices(), integrated_indices.begin(), integrated_indices.end());
    }
  }

  std::shared_ptr<messages::monero::MoneroTransactionInitRequest> Signer::step_init(){
    // extract payment ID from construction data
    auto & tsx_data = m_ct.tsx_data;
    auto & tx = cur_tx();
    const size_t input_size = tx.sources.size();

    m_ct.tx.version = 2;
    m_ct.tx.unlock_time = tx.unlock_time;
    m_client_version = (m_aux_data->client_version ? m_aux_data->client_version.get() : 3);

    tsx_data.set_version(1);
    tsx_data.set_client_version(client_version());
    tsx_data.set_unlock_time(tx.unlock_time);
    tsx_data.set_num_inputs(static_cast<google::protobuf::uint32>(input_size));
    tsx_data.set_mixin(static_cast<google::protobuf::uint32>(tx.sources[0].outputs.size() - 1));
    tsx_data.set_account(tx.subaddr_account);
    tsx_data.set_monero_version(std::string(MONERO_VERSION) + "|" + MONERO_VERSION_TAG);
    tsx_data.set_hard_fork(m_aux_data->hard_fork ? m_aux_data->hard_fork.get() : 0);

    // Rsig decision
    auto rsig_data = tsx_data.mutable_rsig_data();
    m_ct.rsig_type = get_rsig_type(tx.rct_config, tx.splitted_dsts.size());
    rsig_data->set_rsig_type(m_ct.rsig_type);
    CHECK_AND_ASSERT_THROW_MES(tx.rct_config.range_proof_type != rct::RangeProofBorromean, "Borromean rsig not supported");
    m_ct.bp_version = (m_aux_data->bp_version ? m_aux_data->bp_version.get() : 1);
    rsig_data->set_bp_version((uint32_t) m_ct.bp_version);

    generate_rsig_batch_sizes(m_ct.grouping_vct, m_ct.rsig_type, tx.splitted_dsts.size());
    assign_to_repeatable(rsig_data->mutable_grouping(), m_ct.grouping_vct.begin(), m_ct.grouping_vct.end());

    translate_dst_entry(tsx_data.mutable_change_dts(), &(tx.change_dts));
    for(auto & cur : tx.splitted_dsts){
      auto dst = tsx_data.mutable_outputs()->Add();
      translate_dst_entry(dst, &cur);
    }

    m_ct.source_permutation.clear();
    for (size_t n = 0; n < input_size; ++n){
      m_ct.source_permutation.push_back(n);
    }

    compute_integrated_indices(&tsx_data);

    int64_t fee = 0;
    for(auto & cur_in : tx.sources){
      fee += cur_in.amount;
    }
    for(auto & cur_out : tx.splitted_dsts){
      fee -= cur_out.amount;
    }
    if (fee < 0){
      throw std::invalid_argument("Fee cannot be negative");
    }

    tsx_data.set_fee(static_cast<google::protobuf::uint64>(fee));
    this->extract_payment_id();

    auto init_req = std::make_shared<messages::monero::MoneroTransactionInitRequest>();
    init_req->set_version(0);
    init_req->mutable_tsx_data()->CopyFrom(tsx_data);
    return init_req;
  }

  void Signer::step_init_ack(std::shared_ptr<const messages::monero::MoneroTransactionInitAck> ack){
    if (ack->has_rsig_data()){
      m_ct.rsig_param = std::make_shared<MoneroRsigData>(ack->rsig_data());
    }

    assign_from_repeatable(&(m_ct.tx_out_entr_hmacs), ack->hmacs().begin(), ack->hmacs().end());
  }

  std::shared_ptr<messages::monero::MoneroTransactionSetInputRequest> Signer::step_set_input(size_t idx){
    CHECK_AND_ASSERT_THROW_MES(idx < cur_tx().sources.size(), "Invalid source index");
    m_ct.cur_input_idx = idx;
    auto res = std::make_shared<messages::monero::MoneroTransactionSetInputRequest>();
    set_tx_input(res->mutable_src_entr(), idx, false, true);
    return res;
  }

  void Signer::step_set_input_ack(std::shared_ptr<const messages::monero::MoneroTransactionSetInputAck> ack){
    auto & vini_str = ack->vini();

    cryptonote::txin_v vini;
    if (!cn_deserialize(vini_str.data(), vini_str.size(), vini)){
      throw exc::ProtocolException("Cannot deserialize vin[i]");
    }

    m_ct.tx.vin.emplace_back(vini);
    m_ct.tx_in_hmacs.push_back(ack->vini_hmac());
    m_ct.pseudo_outs.push_back(ack->pseudo_out());
    m_ct.pseudo_outs_hmac.push_back(ack->pseudo_out_hmac());
    m_ct.alphas.push_back(ack->pseudo_out_alpha());
    m_ct.spend_encs.push_back(ack->spend_key());
  }

  void Signer::sort_ki(){
    const size_t input_size = cur_tx().sources.size();

    CHECK_AND_ASSERT_THROW_MES(m_ct.tx.vin.size() == input_size, "Invalid vector size");
    std::sort(m_ct.source_permutation.begin(), m_ct.source_permutation.end(), [&](const size_t i0, const size_t i1) {
      const cryptonote::txin_to_key &tk0 = boost::get<cryptonote::txin_to_key>(m_ct.tx.vin[i0]);
      const cryptonote::txin_to_key &tk1 = boost::get<cryptonote::txin_to_key>(m_ct.tx.vin[i1]);
      return memcmp(&tk0.k_image, &tk1.k_image, sizeof(tk0.k_image)) > 0;
    });

    CHECK_AND_ASSERT_THROW_MES(m_ct.tx_in_hmacs.size() == input_size, "Invalid vector size");
    CHECK_AND_ASSERT_THROW_MES(m_ct.pseudo_outs.size() == input_size, "Invalid vector size");
    CHECK_AND_ASSERT_THROW_MES(m_ct.pseudo_outs_hmac.size() == input_size, "Invalid vector size");
    CHECK_AND_ASSERT_THROW_MES(m_ct.alphas.size() == input_size, "Invalid vector size");
    CHECK_AND_ASSERT_THROW_MES(m_ct.spend_encs.size() == input_size, "Invalid vector size");
    CHECK_AND_ASSERT_THROW_MES(m_ct.tx_data.sources.size() == input_size, "Invalid vector size");

    tools::apply_permutation(m_ct.source_permutation, [&](size_t i0, size_t i1){
      std::swap(m_ct.tx.vin[i0], m_ct.tx.vin[i1]);
      std::swap(m_ct.tx_in_hmacs[i0], m_ct.tx_in_hmacs[i1]);
      std::swap(m_ct.pseudo_outs[i0], m_ct.pseudo_outs[i1]);
      std::swap(m_ct.pseudo_outs_hmac[i0], m_ct.pseudo_outs_hmac[i1]);
      std::swap(m_ct.alphas[i0], m_ct.alphas[i1]);
      std::swap(m_ct.spend_encs[i0], m_ct.spend_encs[i1]);
      std::swap(m_ct.tx_data.sources[i0], m_ct.tx_data.sources[i1]);
    });
  }

  std::shared_ptr<messages::monero::MoneroTransactionInputViniRequest> Signer::step_set_vini_input(size_t idx){
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.tx_data.sources.size(), "Invalid transaction index");
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.tx.vin.size(), "Invalid transaction index");
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.tx_in_hmacs.size(), "Invalid transaction index");

    m_ct.cur_input_idx = idx;
    auto tx = m_ct.tx_data;
    auto res = std::make_shared<messages::monero::MoneroTransactionInputViniRequest>();
    auto & vini = m_ct.tx.vin[idx];
    set_tx_input(res->mutable_src_entr(), idx, false, false);
    res->set_vini(cryptonote::t_serializable_object_to_blob(vini));
    res->set_vini_hmac(m_ct.tx_in_hmacs[idx]);
    res->set_orig_idx(m_ct.source_permutation[idx]);
    return res;
  }

  void Signer::step_set_vini_input_ack(std::shared_ptr<const messages::monero::MoneroTransactionInputViniAck> ack){

  }

  std::shared_ptr<messages::monero::MoneroTransactionAllInputsSetRequest> Signer::step_all_inputs_set(){
    return std::make_shared<messages::monero::MoneroTransactionAllInputsSetRequest>();
  }

  void Signer::step_all_inputs_set_ack(std::shared_ptr<const messages::monero::MoneroTransactionAllInputsSetAck> ack){
  }

  std::shared_ptr<messages::monero::MoneroTransactionSetOutputRequest> Signer::step_set_output(size_t idx){
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.tx_data.splitted_dsts.size(), "Invalid transaction index");
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.tx_out_entr_hmacs.size(), "Invalid transaction index");
    CHECK_AND_ASSERT_THROW_MES(is_req_bulletproof(), "Borromean rsig not supported");

    m_ct.cur_output_idx = idx;
    m_ct.cur_output_in_batch_idx += 1;   // assumes sequential call to step_set_output()

    auto res = std::make_shared<messages::monero::MoneroTransactionSetOutputRequest>();
    auto & cur_dst = m_ct.tx_data.splitted_dsts[idx];
    translate_dst_entry(res->mutable_dst_entr(), &cur_dst);
    res->set_dst_entr_hmac(m_ct.tx_out_entr_hmacs[idx]);
    return res;
  }

  void Signer::step_set_output_ack(std::shared_ptr<const messages::monero::MoneroTransactionSetOutputAck> ack){
    CHECK_AND_ASSERT_THROW_MES(is_req_bulletproof(), "Borromean rsig not supported");
    cryptonote::tx_out tx_out;
    rct::Bulletproof bproof{};
    rct::BulletproofPlus bproof_plus{};
    rct::ctkey out_pk{};
    rct::ecdhTuple ecdh{};

    bool has_rsig = false;
    std::string rsig_buff;

    if (ack->has_rsig_data()){
      auto & rsig_data = ack->rsig_data();

      if (rsig_data.has_rsig() && !rsig_data.rsig().empty()){
        has_rsig = true;
        rsig_buff = rsig_data.rsig();
      }

      if (rsig_data.has_mask()){
        rct::key cmask{};
        string_to_key(cmask, rsig_data.mask());
        m_ct.rsig_gamma.emplace_back(cmask);
      }
    }

    if (!cn_deserialize(ack->tx_out(), tx_out)){
      throw exc::ProtocolException("Cannot deserialize vout[i]");
    }

    if (!cn_deserialize(ack->out_pk(), out_pk)){
      throw exc::ProtocolException("Cannot deserialize out_pk");
    }

    if (m_ct.bp_version <= 1) {
      if (!cn_deserialize(ack->ecdh_info(), ecdh)){
        throw exc::ProtocolException("Cannot deserialize ecdhtuple");
      }
    } else {
      CHECK_AND_ASSERT_THROW_MES(8 == ack->ecdh_info().size(), "Invalid ECDH.amount size");
      memcpy(ecdh.amount.bytes, ack->ecdh_info().data(), 8);
    }

    m_ct.tx.vout.emplace_back(tx_out);
    m_ct.tx_out_hmacs.push_back(ack->vouti_hmac());
    m_ct.tx_out_pk.emplace_back(out_pk);
    m_ct.tx_out_ecdh.emplace_back(ecdh);

    rsig_v bp_obj{};
    if (has_rsig) {
      bool deserialize_success;
      if (is_req_bulletproof_plus()) {
        deserialize_success = cn_deserialize(rsig_buff, bproof_plus);
        bp_obj = bproof_plus;
      } else {
        deserialize_success = cn_deserialize(rsig_buff, bproof);
        bp_obj = bproof;
      }
      if (!deserialize_success) {
        throw exc::ProtocolException("Cannot deserialize bulletproof rangesig");
      }
    }

    // Generates BP after all masks in the current batch are generated
    if (!has_rsig || is_offloading()){
      return;
    }

    process_bproof(bp_obj);
    m_ct.cur_batch_idx += 1;
    m_ct.cur_output_in_batch_idx = 0;
  }

  bool Signer::should_compute_bp_now() const {
    CHECK_AND_ASSERT_THROW_MES(m_ct.cur_batch_idx < m_ct.grouping_vct.size(), "Invalid batch index");
    return m_ct.grouping_vct[m_ct.cur_batch_idx] <= m_ct.cur_output_in_batch_idx;
  }

  void Signer::compute_bproof(messages::monero::MoneroTransactionRsigData & rsig_data){
    auto batch_size = m_ct.grouping_vct[m_ct.cur_batch_idx];
    std::vector<uint64_t> amounts;
    rct::keyV masks;
    CHECK_AND_ASSERT_THROW_MES(m_ct.cur_output_idx + 1 >= batch_size, "Invalid index for batching");

    for(size_t i = 0; i < batch_size; ++i){
      const size_t bidx = 1 + m_ct.cur_output_idx - batch_size + i;
      CHECK_AND_ASSERT_THROW_MES(bidx < m_ct.tx_data.splitted_dsts.size(), "Invalid gamma index");
      CHECK_AND_ASSERT_THROW_MES(bidx < m_ct.rsig_gamma.size(), "Invalid gamma index");

      amounts.push_back(m_ct.tx_data.splitted_dsts[bidx].amount);
      masks.push_back(m_ct.rsig_gamma[bidx]);
    }

    std::string serRsig;
    if (is_req_bulletproof_plus()) {
      auto bp = bulletproof_plus_PROVE(amounts, masks);
      serRsig = cn_serialize(bp);
      m_ct.tx_out_rsigs.emplace_back(bp);
    } else {
      auto bp = bulletproof_PROVE(amounts, masks);
      serRsig = cn_serialize(bp);
      m_ct.tx_out_rsigs.emplace_back(bp);
    }

    rsig_data.set_rsig(serRsig);
  }

  void Signer::process_bproof(rsig_v & bproof){
    CHECK_AND_ASSERT_THROW_MES(m_ct.cur_batch_idx < m_ct.grouping_vct.size(), "Invalid batch index");
    auto batch_size = m_ct.grouping_vct[m_ct.cur_batch_idx];
    for (size_t i = 0; i < batch_size; ++i){
      const size_t bidx = 1 + m_ct.cur_output_idx - batch_size + i;
      CHECK_AND_ASSERT_THROW_MES(bidx < m_ct.tx_out_pk.size(), "Invalid out index");

      rct::key commitment = m_ct.tx_out_pk[bidx].mask;
      commitment = rct::scalarmultKey(commitment, rct::INV_EIGHT);
      if (is_req_bulletproof_plus()) {
        boost::get<rct::BulletproofPlus>(bproof).V.push_back(commitment);
      } else {
        boost::get<rct::Bulletproof>(bproof).V.push_back(commitment);
      }
    }

    m_ct.tx_out_rsigs.emplace_back(bproof);
    if (is_req_bulletproof_plus()) {
      if (!rct::bulletproof_plus_VERIFY(boost::get<rct::BulletproofPlus>(m_ct.tx_out_rsigs.back()))) {
        throw exc::ProtocolException("Returned range signature is invalid");
      }
    } else {
      if (!rct::bulletproof_VERIFY(boost::get<rct::Bulletproof>(m_ct.tx_out_rsigs.back()))) {
        throw exc::ProtocolException("Returned range signature is invalid");
      }
    }
  }

  std::shared_ptr<messages::monero::MoneroTransactionSetOutputRequest> Signer::step_rsig(size_t idx){
    if (!is_offloading() || !should_compute_bp_now()){
      return nullptr;
    }

    auto res = std::make_shared<messages::monero::MoneroTransactionSetOutputRequest>();
    auto & cur_dst = m_ct.tx_data.splitted_dsts[idx];
    translate_dst_entry(res->mutable_dst_entr(), &cur_dst);
    res->set_dst_entr_hmac(m_ct.tx_out_entr_hmacs[idx]);

    compute_bproof(*(res->mutable_rsig_data()));
    res->set_is_offloaded_bp(true);
    return res;
  }

  void Signer::step_set_rsig_ack(std::shared_ptr<const messages::monero::MoneroTransactionSetOutputAck> ack){
    m_ct.cur_batch_idx += 1;
    m_ct.cur_output_in_batch_idx = 0;
  }

  std::shared_ptr<messages::monero::MoneroTransactionAllOutSetRequest> Signer::step_all_outs_set(){
    return std::make_shared<messages::monero::MoneroTransactionAllOutSetRequest>();
  }

  void Signer::step_all_outs_set_ack(std::shared_ptr<const messages::monero::MoneroTransactionAllOutSetAck> ack, hw::device &hwdev){
    CHECK_AND_ASSERT_THROW_MES(is_req_bulletproof(), "Borromean rsig not supported");
    m_ct.rv = std::make_shared<rct::rctSig>();
    m_ct.rv->txnFee = ack->rv().txn_fee();
    m_ct.rv->type = static_cast<uint8_t>(ack->rv().rv_type());
    string_to_key(m_ct.rv->message, ack->rv().message());

    // Extra copy
    m_ct.tx.extra.clear();
    auto extra = ack->extra();
    auto extra_data = extra.data();
    m_ct.tx.extra.reserve(extra.size());
    for(size_t i = 0; i < extra.size(); ++i){
      m_ct.tx.extra.push_back(static_cast<uint8_t>(extra_data[i]));
    }

    ::crypto::hash tx_prefix_hash{};
    cryptonote::get_transaction_prefix_hash(m_ct.tx, tx_prefix_hash);
    m_ct.tx_prefix_hash = key_to_string(tx_prefix_hash);
    if (crypto_verify_32(reinterpret_cast<const unsigned char *>(tx_prefix_hash.data),
        reinterpret_cast<const unsigned char *>(ack->tx_prefix_hash().data()))){
      throw exc::proto::SecurityException("Transaction prefix has does not match to the computed value");
    }

    // RctSig
    auto num_sources = m_ct.tx_data.sources.size();
    auto dst = &m_ct.rv->p.pseudoOuts;
    dst->clear();
    for (const auto &pseudo_out : m_ct.pseudo_outs) {
      dst->emplace_back();
      string_to_key(dst->back(), pseudo_out);
    }

    m_ct.rv->mixRing.resize(num_sources);

    CHECK_AND_ASSERT_THROW_MES(m_ct.tx_out_pk.size() == m_ct.tx_out_ecdh.size(), "Invalid vector sizes");
    for(size_t i = 0; i < m_ct.tx_out_ecdh.size(); ++i){
      m_ct.rv->outPk.push_back(m_ct.tx_out_pk[i]);
      m_ct.rv->ecdhInfo.push_back(m_ct.tx_out_ecdh[i]);
    }

    for(size_t i = 0; i < m_ct.tx_out_rsigs.size(); ++i){
      if (is_req_bulletproof_plus()) {
        m_ct.rv->p.bulletproofs_plus.push_back(boost::get<rct::BulletproofPlus>(m_ct.tx_out_rsigs[i]));
      } else {
        m_ct.rv->p.bulletproofs.push_back(boost::get<rct::Bulletproof>(m_ct.tx_out_rsigs[i]));
      }
    }

    rct::key hash_computed = rct::get_pre_mlsag_hash(*(m_ct.rv), hwdev);
    auto & hash = ack->full_message_hash();

    if (hash.size() != 32){
      throw exc::ProtocolException("Returned mlsag hash has invalid size");
    }

    if (crypto_verify_32(reinterpret_cast<const unsigned char *>(hash_computed.bytes),
                         reinterpret_cast<const unsigned char *>(hash.data()))){
      throw exc::proto::SecurityException("Computed MLSAG does not match");
    }
  }

  std::shared_ptr<messages::monero::MoneroTransactionSignInputRequest> Signer::step_sign_input(size_t idx){
    m_ct.cur_input_idx = idx;

    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.tx_data.sources.size(), "Invalid transaction index");
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.tx.vin.size(), "Invalid transaction index");
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.tx_in_hmacs.size(), "Invalid transaction index");
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.alphas.size(), "Invalid transaction index");
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.spend_encs.size(), "Invalid transaction index");

    auto res = std::make_shared<messages::monero::MoneroTransactionSignInputRequest>();
    set_tx_input(res->mutable_src_entr(), idx, true, true);
    res->set_vini(cryptonote::t_serializable_object_to_blob(m_ct.tx.vin[idx]));
    res->set_vini_hmac(m_ct.tx_in_hmacs[idx]);
    res->set_pseudo_out_alpha(m_ct.alphas[idx]);
    res->set_spend_key(m_ct.spend_encs[idx]);
    res->set_orig_idx(m_ct.source_permutation[idx]);

    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.pseudo_outs.size(), "Invalid transaction index");
    CHECK_AND_ASSERT_THROW_MES(idx < m_ct.pseudo_outs_hmac.size(), "Invalid transaction index");
    res->set_pseudo_out(m_ct.pseudo_outs[idx]);
    res->set_pseudo_out_hmac(m_ct.pseudo_outs_hmac[idx]);
    return res;
  }

  void Signer::step_sign_input_ack(std::shared_ptr<const messages::monero::MoneroTransactionSignInputAck> ack){
    m_ct.signatures.push_back(ack->signature());

    // Sync updated pseudo_outputs
    if (ack->has_pseudo_out()){
      CHECK_AND_ASSERT_THROW_MES(m_ct.cur_input_idx < m_ct.pseudo_outs.size(), "Invalid pseudo-out index");
      m_ct.pseudo_outs[m_ct.cur_input_idx] = ack->pseudo_out();
      if (is_bulletproof()){
        CHECK_AND_ASSERT_THROW_MES(m_ct.cur_input_idx < m_ct.rv->p.pseudoOuts.size(), "Invalid pseudo-out index");
        string_to_key(m_ct.rv->p.pseudoOuts[m_ct.cur_input_idx], ack->pseudo_out());
      } else {
        CHECK_AND_ASSERT_THROW_MES(m_ct.cur_input_idx < m_ct.rv->pseudoOuts.size(), "Invalid pseudo-out index");
        string_to_key(m_ct.rv->pseudoOuts[m_ct.cur_input_idx], ack->pseudo_out());
      }
    }
  }

  std::shared_ptr<messages::monero::MoneroTransactionFinalRequest> Signer::step_final(){
    return std::make_shared<messages::monero::MoneroTransactionFinalRequest>();
  }

  void Signer::step_final_ack(std::shared_ptr<const messages::monero::MoneroTransactionFinalAck> ack){
    CHECK_AND_ASSERT_THROW_MES(is_clsag(), "Only CLSAGs signatures are supported");

    if (m_multisig){
      auto & cout_key = ack->cout_key();
      for(auto & cur : m_ct.couts){
        if (cur.size() != crypto::chacha::IV_SIZE + 32){
          throw std::invalid_argument("Encrypted cout has invalid length");
        }

        char buff[32];
        auto data = cur.data();

        crypto::chacha::decrypt(data + crypto::chacha::IV_SIZE, 32, reinterpret_cast<const uint8_t *>(cout_key.data()), reinterpret_cast<const uint8_t *>(data), buff);
        m_ct.couts_dec.emplace_back(buff, 32);
      }
    }

    m_ct.enc_salt1 = ack->salt();
    m_ct.enc_salt2 = ack->rand_mult();
    m_ct.enc_keys = ack->tx_enc_keys();

    // Opening the sealed signatures
    if(!ack->has_opening_key()){
      throw exc::ProtocolException("Client version 3+ requires sealed signatures");
    }

    for(size_t i = 0; i < m_ct.signatures.size(); ++i){
      CHECK_AND_ASSERT_THROW_MES(m_ct.signatures[i].size() > crypto::chacha::TAG_SIZE, "Invalid signature size");
      std::string nonce = compute_sealing_key(ack->opening_key(), i, true);
      std::string key = compute_sealing_key(ack->opening_key(), i, false);
      size_t plen = m_ct.signatures[i].size() - crypto::chacha::TAG_SIZE;
      std::unique_ptr<uint8_t[]> plaintext(new uint8_t[plen]);
      uint8_t * buff = plaintext.get();

      protocol::crypto::chacha::decrypt(
          m_ct.signatures[i].data(),
          m_ct.signatures[i].size(),
          reinterpret_cast<const uint8_t *>(key.data()),
          reinterpret_cast<const uint8_t *>(nonce.data()),
          reinterpret_cast<char *>(buff), &plen);
      m_ct.signatures[i].assign(reinterpret_cast<const char *>(buff), plen);
    }

    m_ct.rv->p.CLSAGs.reserve(m_ct.signatures.size());
    for (size_t i = 0; i < m_ct.signatures.size(); ++i) {
      rct::clsag clsag;
      if (!cn_deserialize(m_ct.signatures[i], clsag)) {
        throw exc::ProtocolException("Cannot deserialize clsag[i]");
      }
      m_ct.rv->p.CLSAGs.push_back(clsag);
    }

    m_ct.tx.rct_signatures = *(m_ct.rv);
  }

  std::string Signer::store_tx_aux_info(){
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    rapidjson::Document json;
    json.SetObject();

    rapidjson::Value valueS(rapidjson::kStringType);
    rapidjson::Value valueI(rapidjson::kNumberType);

    valueI.SetInt(1);
    json.AddMember("version", valueI, json.GetAllocator());

    valueS.SetString(m_ct.enc_salt1.c_str(), m_ct.enc_salt1.size());
    json.AddMember("salt1", valueS, json.GetAllocator());

    valueS.SetString(m_ct.enc_salt2.c_str(), m_ct.enc_salt2.size());
    json.AddMember("salt2", valueS, json.GetAllocator());

    valueS.SetString(m_ct.tx_prefix_hash.c_str(), m_ct.tx_prefix_hash.size());
    json.AddMember("tx_prefix_hash", valueS, json.GetAllocator());

    valueS.SetString(m_ct.enc_keys.c_str(), m_ct.enc_keys.size());
    json.AddMember("enc_keys", valueS, json.GetAllocator());

    json.Accept(writer);
    return sb.GetString();
  }

  void load_tx_key_data(hw::device_cold::tx_key_data_t & res, const std::string & data)
  {
    rapidjson::Document json;

    // The contents should be JSON if the wallet follows the new format.
    if (json.Parse(data.c_str()).HasParseError())
    {
      throw std::invalid_argument("Data parsing error");
    }
    else if(!json.IsObject())
    {
      throw std::invalid_argument("Data parsing error - not an object");
    }

    GET_FIELD_FROM_JSON(json, version, int, Int, true, -1);
    GET_STRING_FROM_JSON(json, salt1, std::string, true, std::string());
    GET_STRING_FROM_JSON(json, salt2, std::string, true, std::string());
    GET_STRING_FROM_JSON(json, enc_keys, std::string, true, std::string());
    GET_STRING_FROM_JSON(json, tx_prefix_hash, std::string, false, std::string());

    if (field_version != 1)
    {
      throw std::invalid_argument("Unknown version");
    }

    res.salt1 = field_salt1;
    res.salt2 = field_salt2;
    res.tx_enc_keys = field_enc_keys;
    res.tx_prefix_hash = field_tx_prefix_hash;
  }

  std::shared_ptr<messages::monero::MoneroGetTxKeyRequest> get_tx_key(
      const hw::device_cold::tx_key_data_t & tx_data)
  {
    auto req = std::make_shared<messages::monero::MoneroGetTxKeyRequest>();
    req->set_salt1(tx_data.salt1);
    req->set_salt2(tx_data.salt2);
    req->set_tx_enc_keys(tx_data.tx_enc_keys);
    req->set_tx_prefix_hash(tx_data.tx_prefix_hash);
    req->set_reason(0);

    return req;
  }

  void get_tx_key_ack(
      std::vector<::crypto::secret_key> & tx_keys,
      const std::string & tx_prefix_hash,
      const ::crypto::secret_key & view_key_priv,
      std::shared_ptr<const messages::monero::MoneroGetTxKeyAck> ack
  )
  {
    auto enc_key = protocol::tx::compute_enc_key(view_key_priv, tx_prefix_hash, ack->salt());
    auto & encrypted_keys = ack->has_tx_derivations() ? ack->tx_derivations() : ack->tx_keys();

    const size_t len_ciphertext = encrypted_keys.size();  // IV || keys || TAG
    CHECK_AND_ASSERT_THROW_MES(len_ciphertext > crypto::chacha::IV_SIZE + crypto::chacha::TAG_SIZE, "Invalid size");

    size_t keys_len = len_ciphertext - crypto::chacha::IV_SIZE - crypto::chacha::TAG_SIZE;
    std::unique_ptr<uint8_t[]> plaintext(new uint8_t[keys_len]);

    protocol::crypto::chacha::decrypt(
        encrypted_keys.data() + crypto::chacha::IV_SIZE,
        len_ciphertext - crypto::chacha::IV_SIZE,
        reinterpret_cast<const uint8_t *>(enc_key.data),
        reinterpret_cast<const uint8_t *>(encrypted_keys.data()),
        reinterpret_cast<char *>(plaintext.get()), &keys_len);

    CHECK_AND_ASSERT_THROW_MES(keys_len % 32 == 0, "Invalid size");
    tx_keys.resize(keys_len / 32);

    for(unsigned i = 0; i < keys_len / 32; ++i)
    {
      memcpy(tx_keys[i].data, plaintext.get() + 32 * i, 32);
    }
    memwipe(plaintext.get(), keys_len);
  }

}
}
}
}
