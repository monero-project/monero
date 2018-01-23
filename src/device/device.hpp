
#pragma once

#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/subaddress_index.h"
#include "cryptonote_basic/blobdatatype.h"


namespace hw {
    namespace {
        //device funcion not supported
        inline bool dfns()  {throw std::runtime_error("device function not supported");return false;}
    }

    class Device {
    public:

        Device()  {}
        Device(const Device &device) {}
        virtual ~Device()   {}

        Device& operator=(const Device &device);
        explicit virtual operator bool() const = 0;

        static const int SIGNATURE_REAL = 0;
        static const int SIGNATURE_FAKE = 1;


        std::string  name;

        /* ======================================================================= */
        /*                              SETUP/TEARDOWN                             */
        /* ======================================================================= */
        virtual bool set_name(const std::string &name) {
            this->name = name;
            return true;
        }
        virtual std::string get_name() {return this->name;}

        virtual  bool init(void) {return dfns();}
        virtual bool release() {return dfns();}

        virtual bool connect(void) {return dfns();}
        virtual bool disconnect() {return dfns();}

        /* ======================================================================= */
        /*                             WALLET & ADDRESS                            */
        /* ======================================================================= */
        virtual bool  get_public_address(cryptonote::account_public_address &pubkey) {return dfns();}
        #ifdef DEBUGLEDGER
        virtual bool  get_secret_keys(crypto::secret_key &viewkey , crypto::secret_key &spendkey) {return dfns();}
        #endif
        virtual bool  generate_chacha_key(const cryptonote::account_keys &keys, crypto::chacha_key &key) {return dfns();}


        /* ======================================================================= */
        /*                               SUB ADDRESS                               */
        /* ======================================================================= */
        virtual bool  derive_subaddress_public_key(const crypto::public_key &pub, const crypto::key_derivation &derivation, const std::size_t output_index,  crypto::public_key &derived_pub) {return dfns();}
        virtual bool  get_subaddress_spend_public_key(const cryptonote::account_keys& keys, const cryptonote::subaddress_index& index, crypto::public_key &D) {return dfns();}
        virtual bool  get_subaddress(const cryptonote::account_keys& keys, const cryptonote::subaddress_index &index, cryptonote::account_public_address &address) {return dfns();}
        virtual bool  get_subaddress_secret_key(const crypto::secret_key &sec, const cryptonote::subaddress_index &index, crypto::secret_key &sub_sec) {return dfns();}

        /* ======================================================================= */
        /*                            DERIVATION & KEY                             */
        /* ======================================================================= */
        virtual bool  verify_keys(const crypto::secret_key &secret_key, const crypto::public_key &public_key) {return dfns();}
        virtual bool  scalarmultKey(const rct::key &pub, const rct::key &sec, rct::key &mulkey) {return dfns();}
        virtual bool  scalarmultBase(const rct::key &sec, rct::key &mulkey) {return dfns();}
        virtual bool  sc_secret_add(const crypto::secret_key &a, const crypto::secret_key &b, crypto::secret_key &r) {return dfns();}
        virtual bool  generate_keys(bool recover, const crypto::secret_key& recovery_key, crypto::public_key &pub, crypto::secret_key &sec, crypto::secret_key &rng) {return dfns();}
        virtual bool  generate_key_derivation(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_derivation &derivation) {return dfns();}
        virtual bool  derivation_to_scalar(const crypto::key_derivation &derivation, const size_t output_index, crypto::ec_scalar &res) {return dfns();}
        virtual bool  derive_secret_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::secret_key &sec,  crypto::secret_key &derived_sec) {return dfns();}
        virtual bool  derive_public_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::public_key &pub,  crypto::public_key &derived_pub) {return dfns();}
        virtual bool  secret_key_to_public_key(const crypto::secret_key &sec, crypto::public_key &pub) {return dfns();}
        virtual bool  generate_key_image(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_image &image) {return dfns();}

        /* ======================================================================= */
        /*                               TRANSACTION                               */
        /* ======================================================================= */

        virtual bool  open_tx(void) {return dfns();}

        virtual bool  get_additional_key(const bool subaddr, cryptonote::keypair &additional_txkey) {return dfns();}
        virtual bool  get_sub_tx_public_key(const crypto::public_key &dest_key, crypto::public_key &pub) {return dfns();}
        virtual bool  set_signature_mode(unsigned int sig_mode) {return dfns();}

        virtual bool  stealth(const crypto::public_key &public_key, const crypto::secret_key &secret_key, crypto::hash8 &payment_id ) {return dfns();}

        virtual bool  ecdhEncode(const rct::key &AKout, rct::ecdhTuple &unmasked) {return dfns();}
        virtual bool  ecdhDecode(const rct::key &AKout, rct::ecdhTuple &masked) {return dfns();}

        virtual bool  add_output_key_mapping(const crypto::public_key &Aout, const crypto::public_key &Bout, size_t real_output_index,
                                            const rct::key &amount_key,  const crypto::public_key &out_eph_public_key) {return dfns();}


        virtual bool  mlsag_prehash(const std::string &blob, size_t inputs_size, size_t outputs_size, const rct::keyV &hashes, const rct::ctkeyV &outPk, rct::key &prehash) {return dfns();}
        virtual bool  mlsag_prepare(const rct::key &H, const rct::key &xx, rct::key &a, rct::key &aG, rct::key &aHP, rct::key &rvII) {return dfns();}
        virtual bool  mlsag_prepare(rct::key &a, rct::key &aG) {return dfns();}
        virtual bool  mlsag_hash(const rct::keyV &long_message, rct::key &c) {return dfns();}
        virtual bool  mlsag_sign(const rct::key &c, const rct::keyV &xx, const rct::keyV &alpha, const int rows, const int dsRows, rct::keyV &ss) {return dfns();}

        virtual bool  close_tx(void) {return dfns();}
    };

    Device& get_default_device();
}

