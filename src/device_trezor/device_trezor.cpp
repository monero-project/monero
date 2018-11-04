// Copyright (c) 2017-2018, The Monero Project
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

#include "device_trezor.hpp"

namespace hw {
namespace trezor {

#ifdef WITH_DEVICE_TREZOR

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "device.trezor"

#define HW_TREZOR_NAME "Trezor"

    static device_trezor *trezor_device = nullptr;
    static device_trezor *ensure_trezor_device(){
      if (!trezor_device) {
        trezor_device = new device_trezor();
        trezor_device->set_name(HW_TREZOR_NAME);
      }
      return trezor_device;
    }

    void register_all(std::map<std::string, std::unique_ptr<device>> &registry) {
      registry.insert(std::make_pair(HW_TREZOR_NAME, std::unique_ptr<device>(ensure_trezor_device())));
    }

    void register_all() {
      hw::register_device(HW_TREZOR_NAME, ensure_trezor_device());
    }

    device_trezor::device_trezor() {

    }

    device_trezor::~device_trezor() {
      try {
        disconnect();
        release();
      } catch(std::exception const& e){
        MWARNING("Could not disconnect and release: " << e.what());
      }
    }

    /* ======================================================================= */
    /*                             WALLET & ADDRESS                            */
    /* ======================================================================= */

    bool device_trezor::get_public_address(cryptonote::account_public_address &pubkey) {
      try {
        auto res = get_address();

        cryptonote::address_parse_info info{};
        bool r = cryptonote::get_account_address_from_str(info, this->network_type, res->address());
        CHECK_AND_ASSERT_MES(r, false, "Could not parse returned address. Address parse failed: " + res->address());
        CHECK_AND_ASSERT_MES(!info.is_subaddress, false, "Trezor returned a sub address");

        pubkey = info.address;
        return true;

      } catch(std::exception const& e){
        MERROR("Get public address exception: " << e.what());
        return false;
      }
    }

    bool device_trezor::get_secret_keys(crypto::secret_key &viewkey , crypto::secret_key &spendkey) {
      try {
        MDEBUG("Loading view-only key from the Trezor. Please check the Trezor for a confirmation.");
        auto res = get_view_key();
        CHECK_AND_ASSERT_MES(res->watch_key().size() == 32, false, "Trezor returned invalid view key");

        spendkey = crypto::null_skey; // not given
        memcpy(viewkey.data, res->watch_key().data(), 32);

        return true;

      } catch(std::exception const& e){
        MERROR("Get secret keys exception: " << e.what());
        return false;
      }
    }

    /* ======================================================================= */
    /*  Helpers                                                                */
    /* ======================================================================= */

    /* ======================================================================= */
    /*                              TREZOR PROTOCOL                            */
    /* ======================================================================= */

    std::shared_ptr<messages::monero::MoneroAddress> device_trezor::get_address(
        const boost::optional<std::vector<uint32_t>> & path,
        const boost::optional<cryptonote::network_type> & network_type){
      AUTO_LOCK_CMD();
      require_connected();
      test_ping();

      auto req = std::make_shared<messages::monero::MoneroGetAddress>();
      this->set_msg_addr<messages::monero::MoneroGetAddress>(req.get(), path, network_type);

      auto response = this->client_exchange<messages::monero::MoneroAddress>(req);
      MTRACE("Get address response received");
      return response;
    }

    std::shared_ptr<messages::monero::MoneroWatchKey> device_trezor::get_view_key(
        const boost::optional<std::vector<uint32_t>> & path,
        const boost::optional<cryptonote::network_type> & network_type){
      AUTO_LOCK_CMD();
      require_connected();
      test_ping();

      auto req = std::make_shared<messages::monero::MoneroGetWatchKey>();
      this->set_msg_addr<messages::monero::MoneroGetWatchKey>(req.get(), path, network_type);

      auto response = this->client_exchange<messages::monero::MoneroWatchKey>(req);
      MTRACE("Get watch key response received");
      return response;
    }

    void device_trezor::ki_sync(wallet_shim * wallet,
                                const std::vector<tools::wallet2::transfer_details> & transfers,
                                hw::device_cold::exported_key_image & ski)
    {
      AUTO_LOCK_CMD();
      require_connected();
      test_ping();

      std::shared_ptr<messages::monero::MoneroKeyImageExportInitRequest> req;

      std::vector<protocol::ki::MoneroTransferDetails> mtds;
      std::vector<protocol::ki::MoneroExportedKeyImage> kis;
      protocol::ki::key_image_data(wallet, transfers, mtds);
      protocol::ki::generate_commitment(mtds, transfers, req);

      this->set_msg_addr<messages::monero::MoneroKeyImageExportInitRequest>(req.get());
      auto ack1 = this->client_exchange<messages::monero::MoneroKeyImageExportInitAck>(req);

      const auto batch_size = 10;
      const auto num_batches = (mtds.size() + batch_size - 1) / batch_size;
      for(uint64_t cur = 0; cur < num_batches; ++cur){
        auto step_req = std::make_shared<messages::monero::MoneroKeyImageSyncStepRequest>();
        auto idx_finish = std::min(static_cast<uint64_t>((cur + 1) * batch_size), static_cast<uint64_t>(mtds.size()));
        for(uint64_t idx = cur * batch_size; idx < idx_finish; ++idx){
          auto added_tdis = step_req->add_tdis();
          CHECK_AND_ASSERT_THROW_MES(idx < mtds.size(), "Invalid transfer detail index");
          *added_tdis = mtds[idx];
        }

        auto step_ack = this->client_exchange<messages::monero::MoneroKeyImageSyncStepAck>(step_req);
        auto kis_size = step_ack->kis_size();
        kis.reserve(static_cast<size_t>(kis_size));
        for(int i = 0; i < kis_size; ++i){
          auto ckis = step_ack->kis(i);
          kis.push_back(ckis);
        }

        MTRACE("Batch " << cur << " / " << num_batches << " batches processed");
      }

      auto final_req = std::make_shared<messages::monero::MoneroKeyImageSyncFinalRequest>();
      auto final_ack = this->client_exchange<messages::monero::MoneroKeyImageSyncFinalAck>(final_req);
      ski.reserve(kis.size());

      for(auto & sub : kis){
        char buff[32*3];
        protocol::crypto::chacha::decrypt(sub.blob().data(), sub.blob().size(),
                                          reinterpret_cast<const uint8_t *>(final_ack->enc_key().data()),
                                          reinterpret_cast<const uint8_t *>(sub.iv().data()), buff);

        ::crypto::signature sig{};
        ::crypto::key_image ki;
        memcpy(ki.data, buff, 32);
        memcpy(sig.c.data, buff + 32, 32);
        memcpy(sig.r.data, buff + 64, 32);
        ski.push_back(std::make_pair(ki, sig));
      }
    }


    void device_trezor::tx_sign(wallet_shim * wallet,
                                const tools::wallet2::unsigned_tx_set & unsigned_tx,
                                tools::wallet2::signed_tx_set & signed_tx,
                                hw::tx_aux_data & aux_data)
    {
      CHECK_AND_ASSERT_THROW_MES(unsigned_tx.transfers.first == 0, "Unsuported non zero offset");
      size_t num_tx = unsigned_tx.txes.size();
      signed_tx.key_images.clear();
      signed_tx.key_images.resize(unsigned_tx.transfers.second.size());

      for(size_t tx_idx = 0; tx_idx < num_tx; ++tx_idx) {
        std::shared_ptr<protocol::tx::Signer> signer;
        tx_sign(wallet, unsigned_tx, tx_idx, aux_data, signer);

        auto & cdata = signer->tdata();
        auto aux_info_cur = signer->store_tx_aux_info();
        aux_data.tx_device_aux.emplace_back(aux_info_cur);

        // Pending tx reconstruction
        signed_tx.ptx.emplace_back();
        auto & cpend = signed_tx.ptx.back();
        cpend.tx = cdata.tx;
        cpend.dust = 0;
        cpend.fee = 0;
        cpend.dust_added_to_fee = false;
        cpend.change_dts = cdata.tx_data.change_dts;
        cpend.selected_transfers = cdata.tx_data.selected_transfers;
        cpend.key_images = "";
        cpend.dests = cdata.tx_data.dests;
        cpend.construction_data = cdata.tx_data;

        // Transaction check
        cryptonote::blobdata tx_blob;
        cryptonote::transaction tx_deserialized;
        bool r = cryptonote::t_serializable_object_to_blob(cpend.tx, tx_blob);
        CHECK_AND_ASSERT_THROW_MES(r, "Transaction serialization failed");
        r = cryptonote::parse_and_validate_tx_from_blob(tx_blob, tx_deserialized);
        CHECK_AND_ASSERT_THROW_MES(r, "Transaction deserialization failed");

        std::string key_images;
        bool all_are_txin_to_key = std::all_of(cdata.tx.vin.begin(), cdata.tx.vin.end(), [&](const cryptonote::txin_v& s_e) -> bool
        {
          CHECKED_GET_SPECIFIC_VARIANT(s_e, const cryptonote::txin_to_key, in, false);
          key_images += boost::to_string(in.k_image) + " ";
          return true;
        });
        if(!all_are_txin_to_key) {
          throw std::invalid_argument("Not all are txin_to_key");
        }
        cpend.key_images = key_images;

        // KI sync
        size_t num_sources = cdata.tx_data.sources.size();
        CHECK_AND_ASSERT_THROW_MES(num_sources == cdata.source_permutation.size(), "Invalid permutation size");
        CHECK_AND_ASSERT_THROW_MES(num_sources == cdata.tx.vin.size(), "Invalid tx.vin size");
        for(size_t src_idx = 0; src_idx < num_sources; ++src_idx){
          size_t idx_mapped = cdata.source_permutation[src_idx];
          CHECK_AND_ASSERT_THROW_MES(idx_mapped < cdata.tx_data.selected_transfers.size(), "Invalid idx_mapped");
          CHECK_AND_ASSERT_THROW_MES(src_idx < cdata.tx.vin.size(), "Invalid idx_mapped");

          size_t idx_map_src = cdata.tx_data.selected_transfers[idx_mapped];
          auto vini = boost::get<cryptonote::txin_to_key>(cdata.tx.vin[src_idx]);

          CHECK_AND_ASSERT_THROW_MES(idx_map_src < signed_tx.key_images.size(), "Invalid key image index");
          signed_tx.key_images[idx_map_src] = vini.k_image;
        }
      }
    }

    void device_trezor::tx_sign(wallet_shim * wallet,
                   const tools::wallet2::unsigned_tx_set & unsigned_tx,
                   size_t idx,
                   hw::tx_aux_data & aux_data,
                   std::shared_ptr<protocol::tx::Signer> & signer)
    {
      AUTO_LOCK_CMD();
      require_connected();
      test_ping();

      CHECK_AND_ASSERT_THROW_MES(idx < unsigned_tx.txes.size(), "Invalid transaction index");
      signer = std::make_shared<protocol::tx::Signer>(wallet, &unsigned_tx, idx, &aux_data);
      const tools::wallet2::tx_construction_data & cur_tx = unsigned_tx.txes[idx];
      unsigned long num_sources = cur_tx.sources.size();
      unsigned long num_outputs = cur_tx.splitted_dsts.size();

      // Step: Init
      auto init_msg = signer->step_init();
      this->set_msg_addr(init_msg.get());

      auto response = this->client_exchange<messages::monero::MoneroTransactionInitAck>(init_msg);
      signer->step_init_ack(response);

      // Step: Set transaction inputs
      for(size_t cur_src = 0; cur_src < num_sources; ++cur_src){
        auto src = signer->step_set_input(cur_src);
        auto ack = this->client_exchange<messages::monero::MoneroTransactionSetInputAck>(src);
        signer->step_set_input_ack(ack);
      }

      // Step: sort
      auto perm_req = signer->step_permutation();
      if (perm_req){
        auto perm_ack = this->client_exchange<messages::monero::MoneroTransactionInputsPermutationAck>(perm_req);
        signer->step_permutation_ack(perm_ack);
      }

      // Step: input_vini
      if (!signer->in_memory()){
        for(size_t cur_src = 0; cur_src < num_sources; ++cur_src){
          auto src = signer->step_set_vini_input(cur_src);
          auto ack = this->client_exchange<messages::monero::MoneroTransactionInputViniAck>(src);
          signer->step_set_vini_input_ack(ack);
        }
      }

      // Step: all inputs set
      auto all_inputs_set = signer->step_all_inputs_set();
      auto ack_all_inputs = this->client_exchange<messages::monero::MoneroTransactionAllInputsSetAck>(all_inputs_set);
      signer->step_all_inputs_set_ack(ack_all_inputs);

      // Step: outputs
      for(size_t cur_dst = 0; cur_dst < num_outputs; ++cur_dst){
        auto src = signer->step_set_output(cur_dst);
        auto ack = this->client_exchange<messages::monero::MoneroTransactionSetOutputAck>(src);
        signer->step_set_output_ack(ack);
      }

      // Step: all outs set
      auto all_out_set = signer->step_all_outs_set();
      auto ack_all_out_set = this->client_exchange<messages::monero::MoneroTransactionAllOutSetAck>(all_out_set);
      signer->step_all_outs_set_ack(ack_all_out_set, *this);

      // Step: sign each input
      for(size_t cur_src = 0; cur_src < num_sources; ++cur_src){
        auto src = signer->step_sign_input(cur_src);
        auto ack_sign = this->client_exchange<messages::monero::MoneroTransactionSignInputAck>(src);
        signer->step_sign_input_ack(ack_sign);
      }

      // Step: final
      auto final_msg = signer->step_final();
      auto ack_final = this->client_exchange<messages::monero::MoneroTransactionFinalAck>(final_msg);
      signer->step_final_ack(ack_final);
    }

#else //WITH_DEVICE_TREZOR

    void register_all(std::map<std::string, std::unique_ptr<device>> &registry) {
    }

    void register_all() {
    }

#endif //WITH_DEVICE_TREZOR
}}
