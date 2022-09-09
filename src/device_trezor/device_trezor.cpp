// Copyright (c) 2017-2022, The Monero Project
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
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

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
      m_live_refresh_in_progress = false;
      m_live_refresh_enabled = true;
      m_live_refresh_thread_running = false;
    }

    device_trezor::~device_trezor() {
      try {
        device_trezor::disconnect();
        device_trezor::release();
      } catch(std::exception const& e){
        MWARNING("Could not disconnect and release: " << e.what());
      }
    }

    bool device_trezor::init()
    {
      m_live_refresh_in_progress = false;
      bool r = device_trezor_base::init();
      if (r && !m_live_refresh_thread)
      {
        m_live_refresh_thread_running = true;
        m_live_refresh_thread.reset(new boost::thread(boost::bind(&device_trezor::live_refresh_thread_main, this)));
      }
      return r;
    }

    bool device_trezor::release()
    {
      m_live_refresh_in_progress = false;
      m_live_refresh_thread_running = false;
      if (m_live_refresh_thread)
      {
        m_live_refresh_thread->join();
        m_live_refresh_thread = nullptr;
      }
      return device_trezor_base::release();
    }

    bool device_trezor::disconnect()
    {
      m_live_refresh_in_progress = false;
      return device_trezor_base::disconnect();
    }

    void device_trezor::device_state_initialize_unsafe()
    {
      require_connected();
      if (m_live_refresh_in_progress)
      {
        try
        {
          live_refresh_finish_unsafe();
        }
        catch(const std::exception & e)
        {
          MERROR("Live refresh could not be terminated: " << e.what());
        }
      }

      m_live_refresh_in_progress = false;
      device_trezor_base::device_state_initialize_unsafe();
    }

    void device_trezor::live_refresh_thread_main()
    {
      while(m_live_refresh_thread_running)
      {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        if (!m_live_refresh_in_progress)
        {
          continue;
        }

        TREZOR_AUTO_LOCK_DEVICE();
        if (!m_transport || !m_live_refresh_in_progress)
        {
          continue;
        }

        auto current_time = std::chrono::steady_clock::now();
        if (current_time - m_last_live_refresh_time <= std::chrono::minutes(5))
        {
          continue;
        }

        MTRACE("Closing live refresh process due to inactivity");
        try
        {
          live_refresh_finish();
        }
        catch(const std::exception &e)
        {
          MWARNING("Live refresh auto-finish failed: " << e.what());
        }
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

    bool device_trezor::get_public_address_with_no_passphrase(cryptonote::account_public_address &pubkey) {
      m_reply_with_empty_passphrase = true;
      const auto empty_passphrase_reverter = epee::misc_utils::create_scope_leave_handler([&]() {
        m_reply_with_empty_passphrase = false;
      });

      return get_public_address(pubkey);
    }

    bool device_trezor::get_secret_keys(crypto::secret_key &viewkey , crypto::secret_key &spendkey) {
      try {
        MDEBUG("Loading view-only key from the Trezor. Please check the Trezor for a confirmation.");
        auto res = get_view_key();
        CHECK_AND_ASSERT_MES(res->watch_key().size() == 32, false, "Trezor returned invalid view key");

        // Trezor does not make use of spendkey of the device API.
        // Ledger loads encrypted spendkey, Trezor loads null key (never leaves device).
        // In the test (debugging mode) we need to leave this field intact as it is already set by
        // the debugging code and need to remain same for the testing purposes.
#ifndef WITH_TREZOR_DEBUGGING
        spendkey = crypto::null_skey; // not given
#endif

        memcpy(viewkey.data, res->watch_key().data(), 32);

        return true;

      } catch(std::exception const& e){
        MERROR("Get secret keys exception: " << e.what());
        return false;
      }
    }

    void device_trezor::display_address(const cryptonote::subaddress_index& index, const boost::optional<crypto::hash8> &payment_id) {
      get_address(index, payment_id, true);
    }

    void device_trezor::reset_session() {
      m_device_session_id.clear();
    }

    bool device_trezor::seen_passphrase_entry_prompt() {
      return m_seen_passphrase_entry_message;
    }

    void device_trezor::set_use_empty_passphrase(bool always_use_empty_passphrase) {
      m_always_use_empty_passphrase = always_use_empty_passphrase;
    }

    /* ======================================================================= */
    /*  Helpers                                                                */
    /* ======================================================================= */

    /* ======================================================================= */
    /*                              TREZOR PROTOCOL                            */
    /* ======================================================================= */

    std::shared_ptr<messages::monero::MoneroAddress> device_trezor::get_address(
        const boost::optional<cryptonote::subaddress_index> & subaddress,
        const boost::optional<crypto::hash8> & payment_id,
        bool show_address,
        const boost::optional<std::vector<uint32_t>> & path,
        const boost::optional<cryptonote::network_type> & network_type){
      CHECK_AND_ASSERT_THROW_MES(!payment_id || !subaddress || subaddress->is_zero(), "Subaddress cannot be integrated");
      TREZOR_AUTO_LOCK_CMD();
      require_connected();
      device_state_initialize_unsafe();
      require_initialized();

      auto req = std::make_shared<messages::monero::MoneroGetAddress>();
      this->set_msg_addr<messages::monero::MoneroGetAddress>(req.get(), path, network_type);
      req->set_show_display(show_address);
      if (subaddress){
        req->set_account(subaddress->major);
        req->set_minor(subaddress->minor);
      }
      if (payment_id){
        req->set_payment_id(std::string(payment_id->data, 8));
      }

      auto response = this->client_exchange<messages::monero::MoneroAddress>(req);
      MTRACE("Get address response received");
      return response;
    }

    std::shared_ptr<messages::monero::MoneroWatchKey> device_trezor::get_view_key(
        const boost::optional<std::vector<uint32_t>> & path,
        const boost::optional<cryptonote::network_type> & network_type){
      TREZOR_AUTO_LOCK_CMD();
      require_connected();
      device_state_initialize_unsafe();
      require_initialized();

      auto req = std::make_shared<messages::monero::MoneroGetWatchKey>();
      this->set_msg_addr<messages::monero::MoneroGetWatchKey>(req.get(), path, network_type);

      auto response = this->client_exchange<messages::monero::MoneroWatchKey>(req);
      MTRACE("Get watch key response received");
      return response;
    }

    bool device_trezor::is_get_tx_key_supported() const
    {
      require_initialized();
      return get_version() > pack_version(2, 0, 10);
    }

    void device_trezor::load_tx_key_data(::hw::device_cold::tx_key_data_t & res, const std::string & tx_aux_data)
    {
      protocol::tx::load_tx_key_data(res, tx_aux_data);
    }

    void device_trezor::get_tx_key(
        std::vector<::crypto::secret_key> & tx_keys,
        const ::hw::device_cold::tx_key_data_t & tx_aux_data,
        const ::crypto::secret_key & view_key_priv)
    {
      TREZOR_AUTO_LOCK_CMD();
      require_connected();
      device_state_initialize_unsafe();
      require_initialized();

      auto req = protocol::tx::get_tx_key(tx_aux_data);
      this->set_msg_addr<messages::monero::MoneroGetTxKeyRequest>(req.get());

      auto response = this->client_exchange<messages::monero::MoneroGetTxKeyAck>(req);
      MTRACE("Get TX key response received");

      protocol::tx::get_tx_key_ack(tx_keys, tx_aux_data.tx_prefix_hash, view_key_priv, response);
    }

    void device_trezor::ki_sync(wallet_shim * wallet,
                                const std::vector<tools::wallet2::transfer_details> & transfers,
                                hw::device_cold::exported_key_image & ski)
    {
#define EVENT_PROGRESS(P) do { if (m_callback) {(m_callback)->on_progress(device_cold::op_progress(P)); } }while(0)

      TREZOR_AUTO_LOCK_CMD();
      require_connected();
      device_state_initialize_unsafe();
      require_initialized();

      std::shared_ptr<messages::monero::MoneroKeyImageExportInitRequest> req;

      std::vector<protocol::ki::MoneroTransferDetails> mtds;
      std::vector<protocol::ki::MoneroExportedKeyImage> kis;
      protocol::ki::key_image_data(wallet, transfers, mtds);
      protocol::ki::generate_commitment(mtds, transfers, req);

      EVENT_PROGRESS(0.);
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
        EVENT_PROGRESS((double)cur * batch_size / mtds.size());
      }
      EVENT_PROGRESS(1.);

      auto final_req = std::make_shared<messages::monero::MoneroKeyImageSyncFinalRequest>();
      auto final_ack = this->client_exchange<messages::monero::MoneroKeyImageSyncFinalAck>(final_req);
      ski.reserve(kis.size());

      for(auto & sub : kis){
        ::crypto::signature sig{};
        ::crypto::key_image ki;
        char buff[sizeof(ki.data)*3];

        size_t buff_len = sizeof(buff);

        protocol::crypto::chacha::decrypt(sub.blob().data(), sub.blob().size(),
                                          reinterpret_cast<const uint8_t *>(final_ack->enc_key().data()),
                                          reinterpret_cast<const uint8_t *>(sub.iv().data()), buff, &buff_len);
        CHECK_AND_ASSERT_THROW_MES(buff_len == sizeof(buff), "Plaintext size invalid");

        memcpy(ki.data, buff, sizeof(ki.data));
        memcpy(sig.c.data, buff + sizeof(ki.data), sizeof(ki.data));
        memcpy(sig.r.data, buff + 2*sizeof(ki.data), sizeof(ki.data));
        ski.push_back(std::make_pair(ki, sig));
      }
#undef EVENT_PROGRESS
    }

    bool device_trezor::is_live_refresh_supported() const
    {
      require_initialized();
      return get_version() > pack_version(2, 0, 10);
    }

    bool device_trezor::is_live_refresh_enabled() const
    {
      return is_live_refresh_supported() && (mode == NONE || mode == TRANSACTION_PARSE) && m_live_refresh_enabled;
    }

    bool device_trezor::has_ki_live_refresh() const
    {
      try{
        return is_live_refresh_enabled();
      } catch(const std::exception & e){
        MERROR("Could not detect if live refresh is enabled: " << e.what());
      }
      return false;
    }

    void device_trezor::live_refresh_start()
    {
      TREZOR_AUTO_LOCK_CMD();
      require_connected();
      live_refresh_start_unsafe();
    }

    void device_trezor::live_refresh_start_unsafe()
    {
      device_state_initialize_unsafe();
      require_initialized();

      auto req = std::make_shared<messages::monero::MoneroLiveRefreshStartRequest>();
      this->set_msg_addr<messages::monero::MoneroLiveRefreshStartRequest>(req.get());
      this->client_exchange<messages::monero::MoneroLiveRefreshStartAck>(req);
      m_live_refresh_in_progress = true;
      m_last_live_refresh_time = std::chrono::steady_clock::now();
    }

    void device_trezor::live_refresh(
        const ::crypto::secret_key & view_key_priv,
        const crypto::public_key& out_key,
        const crypto::key_derivation& recv_derivation,
        size_t real_output_index,
        const cryptonote::subaddress_index& received_index,
        cryptonote::keypair& in_ephemeral,
        crypto::key_image& ki
    )
    {
      TREZOR_AUTO_LOCK_CMD();
      require_connected();

      if (!m_live_refresh_in_progress)
      {
        live_refresh_start_unsafe();
      }

      m_last_live_refresh_time = std::chrono::steady_clock::now();

      auto req = std::make_shared<messages::monero::MoneroLiveRefreshStepRequest>();
      req->set_out_key(out_key.data, 32);
      req->set_recv_deriv(recv_derivation.data, 32);
      req->set_real_out_idx(real_output_index);
      req->set_sub_addr_major(received_index.major);
      req->set_sub_addr_minor(received_index.minor);

      auto ack = this->client_exchange<messages::monero::MoneroLiveRefreshStepAck>(req);
      protocol::ki::live_refresh_ack(view_key_priv, out_key, ack, in_ephemeral, ki);
    }

    void device_trezor::live_refresh_finish_unsafe()
    {
      auto req = std::make_shared<messages::monero::MoneroLiveRefreshFinalRequest>();
      this->client_exchange<messages::monero::MoneroLiveRefreshFinalAck>(req);
      m_live_refresh_in_progress = false;
    }

    void device_trezor::live_refresh_finish()
    {
      TREZOR_AUTO_LOCK_CMD();
      require_connected();
      if (m_live_refresh_in_progress)
      {
        live_refresh_finish_unsafe();
      }
    }

    void device_trezor::computing_key_images(bool started)
    {
      try
      {
        if (!is_live_refresh_enabled())
        {
          return;
        }

        // React only on termination as the process can auto-start itself.
        if (!started && m_live_refresh_in_progress)
        {
          live_refresh_finish();
        }
      }
      catch(const std::exception & e)
      {
        MWARNING("KI computation state change failed, started: " << started << ", e: " << e.what());
      }
    }

    bool device_trezor::compute_key_image(
        const ::cryptonote::account_keys& ack,
        const ::crypto::public_key& out_key,
        const ::crypto::key_derivation& recv_derivation,
        size_t real_output_index,
        const ::cryptonote::subaddress_index& received_index,
        ::cryptonote::keypair& in_ephemeral,
        ::crypto::key_image& ki)
    {
      if (!is_live_refresh_enabled())
      {
        return false;
      }

      live_refresh(ack.m_view_secret_key, out_key, recv_derivation, real_output_index, received_index, in_ephemeral, ki);
      return true;
    }

    void device_trezor::tx_sign(wallet_shim * wallet,
                                const tools::wallet2::unsigned_tx_set & unsigned_tx,
                                tools::wallet2::signed_tx_set & signed_tx,
                                hw::tx_aux_data & aux_data)
    {
      CHECK_AND_ASSERT_THROW_MES(std::get<0>(unsigned_tx.transfers) == 0, "Unsuported non zero offset");

      TREZOR_AUTO_LOCK_CMD();
      require_connected();
      device_state_initialize_unsafe();
      require_initialized();
      transaction_versions_check(unsigned_tx, aux_data);

      const size_t num_tx = unsigned_tx.txes.size();
      m_num_transations_to_sign = num_tx;
      signed_tx.key_images.clear();
      signed_tx.key_images.resize(std::get<2>(unsigned_tx.transfers).size());

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
        cpend.fee = cpend.tx.rct_signatures.txnFee;
        cpend.dust_added_to_fee = false;
        cpend.change_dts = cdata.tx_data.change_dts;
        cpend.selected_transfers = cdata.tx_data.selected_transfers;
        cpend.key_images = "";
        cpend.dests = cdata.tx_data.dests;
        cpend.construction_data = cdata.tx_data;

        // Transaction check
        try {
          MDEBUG("signed transaction: " << cryptonote::get_transaction_hash(cpend.tx) << ENDL << cryptonote::obj_to_json_str(cpend.tx) << ENDL);
          transaction_check(cdata, aux_data);
        } catch(const std::exception &e){
          throw exc::ProtocolException(std::string("Transaction verification failed: ") + e.what());
        }

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
        for(size_t cidx=0, trans_max=std::get<2>(unsigned_tx.transfers).size(); cidx < trans_max; ++cidx){
          signed_tx.key_images[cidx] = std::get<2>(unsigned_tx.transfers)[cidx].m_key_image;
        }

        size_t num_sources = cdata.tx_data.sources.size();
        CHECK_AND_ASSERT_THROW_MES(num_sources == cdata.source_permutation.size(), "Invalid permutation size");
        CHECK_AND_ASSERT_THROW_MES(num_sources == cdata.tx.vin.size(), "Invalid tx.vin size");
        for(size_t src_idx = 0; src_idx < num_sources; ++src_idx){
          size_t idx_mapped = cdata.source_permutation[src_idx];
          CHECK_AND_ASSERT_THROW_MES(idx_mapped < cdata.tx_data.selected_transfers.size(), "Invalid idx_mapped");
          CHECK_AND_ASSERT_THROW_MES(src_idx < cdata.tx.vin.size(), "Invalid idx_mapped");

          size_t idx_map_src = cdata.tx_data.selected_transfers[idx_mapped];
          CHECK_AND_ASSERT_THROW_MES(idx_map_src >= std::get<0>(unsigned_tx.transfers), "Invalid offset");

          idx_map_src -= std::get<0>(unsigned_tx.transfers);
          CHECK_AND_ASSERT_THROW_MES(idx_map_src < signed_tx.key_images.size(), "Invalid key image index");

          const auto vini = boost::get<cryptonote::txin_to_key>(cdata.tx.vin[src_idx]);
          signed_tx.key_images[idx_map_src] = vini.k_image;
        }
      }

      if (m_callback){
        m_callback->on_progress(device_cold::tx_progress(m_num_transations_to_sign, m_num_transations_to_sign, 1, 1, 1, 1));
      }
    }

    void device_trezor::tx_sign(wallet_shim * wallet,
                   const tools::wallet2::unsigned_tx_set & unsigned_tx,
                   size_t idx,
                   hw::tx_aux_data & aux_data,
                   std::shared_ptr<protocol::tx::Signer> & signer)
    {
#define EVENT_PROGRESS(S, SUB, SUBMAX) do { if (m_callback) { \
      (m_callback)->on_progress(device_cold::tx_progress(idx, m_num_transations_to_sign, S, 10, SUB, SUBMAX)); \
} }while(0)

      require_connected();
      if (idx > 0)
        device_state_initialize_unsafe();

      require_initialized();
      EVENT_PROGRESS(0, 1, 1);

      CHECK_AND_ASSERT_THROW_MES(idx < unsigned_tx.txes.size(), "Invalid transaction index");
      signer = std::make_shared<protocol::tx::Signer>(wallet, &unsigned_tx, idx, &aux_data);
      const tools::wallet2::tx_construction_data & cur_tx = unsigned_tx.txes[idx];
      unsigned long num_sources = cur_tx.sources.size();
      unsigned long num_outputs = cur_tx.splitted_dsts.size();

      // Step: Init
      auto init_msg = signer->step_init();
      this->set_msg_addr(init_msg.get());
      transaction_pre_check(init_msg);
      EVENT_PROGRESS(1, 1, 1);

      auto response = this->client_exchange<messages::monero::MoneroTransactionInitAck>(init_msg);
      signer->step_init_ack(response);

      // Step: Set transaction inputs
      for(size_t cur_src = 0; cur_src < num_sources; ++cur_src){
        auto src = signer->step_set_input(cur_src);
        auto ack = this->client_exchange<messages::monero::MoneroTransactionSetInputAck>(src);
        signer->step_set_input_ack(ack);
        EVENT_PROGRESS(2, cur_src, num_sources);
      }

      // Step: sort
      signer->sort_ki();
      EVENT_PROGRESS(3, 1, 1);

      // Step: input_vini
      for(size_t cur_src = 0; cur_src < num_sources; ++cur_src){
        auto src = signer->step_set_vini_input(cur_src);
        auto ack = this->client_exchange<messages::monero::MoneroTransactionInputViniAck>(src);
        signer->step_set_vini_input_ack(ack);
        EVENT_PROGRESS(4, cur_src, num_sources);
      }

      // Step: all inputs set
      auto all_inputs_set = signer->step_all_inputs_set();
      auto ack_all_inputs = this->client_exchange<messages::monero::MoneroTransactionAllInputsSetAck>(all_inputs_set);
      signer->step_all_inputs_set_ack(ack_all_inputs);
      EVENT_PROGRESS(5, 1, 1);

      // Step: outputs
      for(size_t cur_dst = 0; cur_dst < num_outputs; ++cur_dst){
        auto src = signer->step_set_output(cur_dst);
        auto ack = this->client_exchange<messages::monero::MoneroTransactionSetOutputAck>(src);
        signer->step_set_output_ack(ack);

        // If BP is offloaded to host, another step with computed BP may be needed.
        auto offloaded_bp = signer->step_rsig(cur_dst);
        if (offloaded_bp){
          auto bp_ack = this->client_exchange<messages::monero::MoneroTransactionSetOutputAck>(offloaded_bp);
          signer->step_set_rsig_ack(ack);
        }

        EVENT_PROGRESS(6, cur_dst, num_outputs);
      }

      // Step: all outs set
      auto all_out_set = signer->step_all_outs_set();
      auto ack_all_out_set = this->client_exchange<messages::monero::MoneroTransactionAllOutSetAck>(all_out_set);
      signer->step_all_outs_set_ack(ack_all_out_set, *this);
      EVENT_PROGRESS(7, 1, 1);

      // Step: sign each input
      for(size_t cur_src = 0; cur_src < num_sources; ++cur_src){
        auto src = signer->step_sign_input(cur_src);
        auto ack_sign = this->client_exchange<messages::monero::MoneroTransactionSignInputAck>(src);
        signer->step_sign_input_ack(ack_sign);
        EVENT_PROGRESS(8, cur_src, num_sources);
      }

      // Step: final
      auto final_msg = signer->step_final();
      auto ack_final = this->client_exchange<messages::monero::MoneroTransactionFinalAck>(final_msg);
      signer->step_final_ack(ack_final);
      EVENT_PROGRESS(9, 1, 1);
#undef EVENT_PROGRESS
    }

    unsigned device_trezor::client_version()
    {
      auto trezor_version = get_version();
      if (trezor_version < pack_version(2, 4, 3)){
        throw exc::TrezorException("Minimal Trezor firmware version is 2.4.3. Please update.");
      }

      unsigned client_version = 3;
      if (trezor_version >= pack_version(2, 5, 2)){
        client_version = 4;
      }

#ifdef WITH_TREZOR_DEBUGGING
      // Override client version for tests
      const char *env_trezor_client_version = nullptr;
      if ((env_trezor_client_version = getenv("TREZOR_CLIENT_VERSION")) != nullptr){
        auto succ = epee::string_tools::get_xtype_from_string(client_version, env_trezor_client_version);
        if (succ){
          MINFO("Trezor client version overriden by TREZOR_CLIENT_VERSION to: " << client_version);
        }
      }
#endif
      return client_version;
    }

    void device_trezor::transaction_versions_check(const ::tools::wallet2::unsigned_tx_set & unsigned_tx, hw::tx_aux_data & aux_data)
    {
      unsigned cversion = client_version();

      if (aux_data.client_version){
        auto wanted_client_version = aux_data.client_version.get();
        if (wanted_client_version > cversion){
          throw exc::TrezorException("Trezor has too old firmware version. Please update.");
        } else {
          cversion = wanted_client_version;
        }
      }
      aux_data.client_version = cversion;
    }

    void device_trezor::transaction_pre_check(std::shared_ptr<messages::monero::MoneroTransactionInitRequest> init_msg)
    {
      CHECK_AND_ASSERT_THROW_MES(init_msg, "TransactionInitRequest is empty");
      CHECK_AND_ASSERT_THROW_MES(init_msg->has_tsx_data(), "TransactionInitRequest has no transaction data");
      CHECK_AND_ASSERT_THROW_MES(m_features, "Device state not initialized");  // make sure the caller did not reset features
    }

    void device_trezor::transaction_check(const protocol::tx::TData & tdata, const hw::tx_aux_data & aux_data)
    {
      // Simple serialization check
      cryptonote::blobdata tx_blob;
      cryptonote::transaction tx_deserialized;
      bool r = cryptonote::t_serializable_object_to_blob(tdata.tx, tx_blob);
      CHECK_AND_ASSERT_THROW_MES(r, "Transaction serialization failed");
      r = cryptonote::parse_and_validate_tx_from_blob(tx_blob, tx_deserialized);
      CHECK_AND_ASSERT_THROW_MES(r, "Transaction deserialization failed");

      // Extras check
      std::vector<cryptonote::tx_extra_field> tx_extra_fields;
      cryptonote::tx_extra_nonce nonce;

      r = cryptonote::parse_tx_extra(tdata.tx.extra, tx_extra_fields);
      CHECK_AND_ASSERT_THROW_MES(r, "tx.extra parsing failed");

      const bool nonce_required = tdata.tsx_data.has_payment_id() && tdata.tsx_data.payment_id().size() > 0;
      const bool has_nonce = cryptonote::find_tx_extra_field_by_type(tx_extra_fields, nonce);
      CHECK_AND_ASSERT_THROW_MES(has_nonce || !nonce_required, "Transaction nonce not present");

      if (nonce_required){
        const std::string & payment_id = tdata.tsx_data.payment_id();
        if (payment_id.size() == 32){
          crypto::hash payment_id_long{};
          CHECK_AND_ASSERT_THROW_MES(cryptonote::get_payment_id_from_tx_extra_nonce(nonce.nonce, payment_id_long), "Long payment ID not present");

        } else if (payment_id.size() == 8){
          crypto::hash8 payment_id_short{};
          CHECK_AND_ASSERT_THROW_MES(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(nonce.nonce, payment_id_short), "Short payment ID not present");
        }
      }
    }

#else //WITH_DEVICE_TREZOR

    void register_all(std::map<std::string, std::unique_ptr<device>> &registry) {
    }

    void register_all() {
    }

#endif //WITH_DEVICE_TREZOR
}}
