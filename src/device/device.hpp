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


#pragma once

#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/subaddress_index.h"

#ifndef USE_DEVICE_LEDGER
#define USE_DEVICE_LEDGER 1
#endif

#if !defined(HAVE_PCSC) 
#undef  USE_DEVICE_LEDGER
#define USE_DEVICE_LEDGER 0
#endif

#if USE_DEVICE_LEDGER
#define WITH_DEVICE_LEDGER
#endif

namespace hw {
    namespace {
        //device funcion not supported
        #define dfns()  \
           throw std::runtime_error(std::string("device function not supported: ")+ std::string(__FUNCTION__) + \
                                    std::string(" (device.hpp line ")+std::to_string(__LINE__)+std::string(").")); \
           return false;
    }


    class device {
    public:

        device()  {}
        device(const device &hwdev) {}
        virtual ~device()   {}

        explicit virtual operator bool() const = 0;

        static const int SIGNATURE_REAL = 0;
        static const int SIGNATURE_FAKE = 1;


        std::string  name;

        /* ======================================================================= */
        /*                              SETUP/TEARDOWN                             */
        /* ======================================================================= */
        virtual bool set_name(const std::string &name) = 0;
        virtual const std::string get_name() const = 0;

        virtual  bool init(void) = 0;
        virtual bool release() = 0;

        virtual bool connect(void) = 0;
        virtual bool disconnect() = 0;

        /* ======================================================================= */
        /*                             WALLET & ADDRESS                            */
        /* ======================================================================= */
        virtual bool  get_public_address(cryptonote::account_public_address &pubkey) = 0;
        virtual bool  get_secret_keys(crypto::secret_key &viewkey , crypto::secret_key &spendkey)  = 0;
        virtual bool  generate_chacha_key(const cryptonote::account_keys &keys, crypto::chacha_key &key) = 0;

        /* ======================================================================= */
        /*                               SUB ADDRESS                               */
        /* ======================================================================= */
        virtual bool  derive_subaddress_public_key(const crypto::public_key &pub, const crypto::key_derivation &derivation, const std::size_t output_index,  crypto::public_key &derived_pub) = 0;
        virtual bool  get_subaddress_spend_public_key(const cryptonote::account_keys& keys, const cryptonote::subaddress_index& index, crypto::public_key &D) = 0;
        virtual bool  get_subaddress_spend_public_keys(const cryptonote::account_keys &keys, uint32_t account, uint32_t begin, uint32_t end, std::vector<crypto::public_key> &pkeys) = 0;
        virtual bool  get_subaddress(const cryptonote::account_keys& keys, const cryptonote::subaddress_index &index, cryptonote::account_public_address &address) = 0;
        virtual bool  get_subaddress_secret_key(const crypto::secret_key &sec, const cryptonote::subaddress_index &index, crypto::secret_key &sub_sec) = 0;

        /* ======================================================================= */
        /*                            DERIVATION & KEY                             */
        /* ======================================================================= */
        virtual bool  verify_keys(const crypto::secret_key &secret_key, const crypto::public_key &public_key) = 0;
        virtual bool  scalarmultKey(rct::key & aP, const rct::key &P, const rct::key &a) = 0;
        virtual bool  scalarmultBase(rct::key &aG, const rct::key &a) = 0;
        virtual bool  sc_secret_add( crypto::secret_key &r, const crypto::secret_key &a, const crypto::secret_key &b) = 0;
        virtual bool  generate_keys(crypto::public_key &pub, crypto::secret_key &sec, const crypto::secret_key& recovery_key, bool recover, crypto::secret_key &rng) = 0;
        virtual bool  generate_key_derivation(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_derivation &derivation) = 0;
        virtual bool  derivation_to_scalar(const crypto::key_derivation &derivation, const size_t output_index, crypto::ec_scalar &res) = 0;
        virtual bool  derive_secret_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::secret_key &sec,  crypto::secret_key &derived_sec) = 0;
        virtual bool  derive_public_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::public_key &pub,  crypto::public_key &derived_pub) = 0;
        virtual bool  secret_key_to_public_key(const crypto::secret_key &sec, crypto::public_key &pub) = 0;
        virtual bool  generate_key_image(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_image &image) = 0;

        /* ======================================================================= */
        /*                               TRANSACTION                               */
        /* ======================================================================= */

        virtual bool  open_tx(crypto::secret_key &tx_key) = 0;

        virtual bool  set_signature_mode(unsigned int sig_mode) = 0;

        virtual bool  encrypt_payment_id(const crypto::public_key &public_key, const crypto::secret_key &secret_key, crypto::hash8 &payment_id ) = 0;

        virtual bool  ecdhEncode(rct::ecdhTuple & unmasked, const rct::key & sharedSec) = 0;
        virtual bool  ecdhDecode(rct::ecdhTuple & masked, const rct::key & sharedSec) = 0;

        virtual bool  add_output_key_mapping(const crypto::public_key &Aout, const crypto::public_key &Bout, size_t real_output_index,
                                            const rct::key &amount_key,  const crypto::public_key &out_eph_public_key) = 0;


        virtual bool  mlsag_prehash(const std::string &blob, size_t inputs_size, size_t outputs_size, const rct::keyV &hashes, const rct::ctkeyV &outPk, rct::key &prehash) = 0;
        virtual bool  mlsag_prepare(const rct::key &H, const rct::key &xx, rct::key &a, rct::key &aG, rct::key &aHP, rct::key &rvII) = 0;
        virtual bool  mlsag_prepare(rct::key &a, rct::key &aG) = 0;
        virtual bool  mlsag_hash(const rct::keyV &long_message, rct::key &c) = 0;
        virtual bool  mlsag_sign(const rct::key &c, const rct::keyV &xx, const rct::keyV &alpha, const size_t rows, const size_t dsRows, rct::keyV &ss) = 0;

        virtual bool  close_tx(void) = 0;
    } ;

    device& get_device(const std::string device_descriptor) ;
}

