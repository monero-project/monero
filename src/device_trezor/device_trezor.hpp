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

#ifndef MONERO_DEVICE_TREZOR_H
#define MONERO_DEVICE_TREZOR_H


#include <cstddef>
#include <string>
#include "device/device.hpp"
#include "device/device_default.hpp"
#include "device/device_cold.hpp"
#include <boost/scope_exit.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "cryptonote_config.h"
#include "trezor.hpp"
#include "device_trezor_base.hpp"

namespace hw {
namespace trezor {

  void register_all();
  void register_all(std::map<std::string, std::unique_ptr<device>> &registry);

#ifdef WITH_DEVICE_TREZOR
  class device_trezor;

  /**
   * Main device
   */
  class device_trezor : public hw::trezor::device_trezor_base, public hw::device_cold {
    protected:
      // To speed up blockchain parsing the view key maybe handle here.
      crypto::secret_key viewkey;
      bool has_view_key;

    public:
      device_trezor();
      virtual ~device_trezor() override;

      device_trezor(const device_trezor &device) = delete ;
      device_trezor& operator=(const device_trezor &device) = delete;

      explicit operator bool() const override {return true;}

      device_protocol_t device_protocol() const override { return PROTOCOL_COLD; };

      bool  has_ki_cold_sync() const override { return true; }
      bool  has_tx_cold_sign() const override { return true; }
      void  set_network_type(cryptonote::network_type network_type) override { this->network_type = network_type; }

      /* ======================================================================= */
      /*                             WALLET & ADDRESS                            */
      /* ======================================================================= */
      bool  get_public_address(cryptonote::account_public_address &pubkey) override;
      bool  get_secret_keys(crypto::secret_key &viewkey , crypto::secret_key &spendkey) override;

      /* ======================================================================= */
      /*                              TREZOR PROTOCOL                            */
      /* ======================================================================= */

      /**
       * Get address. Throws.
       */
      std::shared_ptr<messages::monero::MoneroAddress> get_address(
          const boost::optional<std::vector<uint32_t>> & path = boost::none,
          const boost::optional<cryptonote::network_type> & network_type = boost::none);

      /**
       * Get watch key from device. Throws.
       */
      std::shared_ptr<messages::monero::MoneroWatchKey> get_view_key(
          const boost::optional<std::vector<uint32_t>> & path = boost::none,
          const boost::optional<cryptonote::network_type> & network_type = boost::none);

      /**
       * Key image sync with the Trezor.
       */
      void ki_sync(wallet_shim * wallet,
                   const std::vector<::tools::wallet2::transfer_details> & transfers,
                   hw::device_cold::exported_key_image & ski) override;

      /**
       * Signs particular transaction idx in the unsigned set, keeps state in the signer
       */
      void tx_sign(wallet_shim * wallet,
                   const ::tools::wallet2::unsigned_tx_set & unsigned_tx,
                   size_t idx,
                   hw::tx_aux_data & aux_data,
                   std::shared_ptr<protocol::tx::Signer> & signer);

      /**
       * Signs unsigned transaction with the Trezor.
       */
      void tx_sign(wallet_shim * wallet,
                   const ::tools::wallet2::unsigned_tx_set & unsigned_tx,
                   ::tools::wallet2::signed_tx_set & signed_tx,
                   hw::tx_aux_data & aux_data) override;
    };

#endif

}
}
#endif //MONERO_DEVICE_TREZOR_H
