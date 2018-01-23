#pragma once

#include <cstddef>
#include <string>
#include <mutex>
#include "device.hpp"
#include "PCSC/winscard.h"

namespace hw {

    namespace ledger {

    namespace {
        bool apdu_verbose =true;
    }
    void set_apdu_verbose(bool verbose) {apdu_verbose = verbose;}

    class ABPkeys {
    public:
        rct::key Aout;
        rct::key Bout;
        size_t   index;
        rct::key Pout;
        rct::key AKout;
        ABPkeys(const rct::key& A, const rct::key& B, size_t index, const rct::key& P,const rct::key& AK);
        ABPkeys(const ABPkeys& keys) ;
        ABPkeys() {index=0;}
    };

    class Keymap {
    public:
        std::vector<struct ABPkeys> ABP;

        bool find(const rct::key& P, ABPkeys& keys) const;
        void add(const ABPkeys& keys);
        void clear();
        void log();
    };

    #define BUFFER_SEND_SIZE 262

    class DeviceLedger : public hw::Device {
    private:
        mutable std::mutex   device_locker;
        mutable std::mutex   tx_locker;
        void lock_device() ;
        void unlock_device() ;
        void lock_tx() ;
        void unlock_tx() ;


        std::string  full_name;
        SCARDCONTEXT hContext;
        SCARDHANDLE  hCard;
        DWORD        length_send;
        BYTE         buffer_send[BUFFER_SEND_SIZE];
        DWORD        length_recv;
        BYTE         buffer_recv[BUFFER_SEND_SIZE];
        unsigned int id;

        Keymap key_map;


        void logCMD(void);
        void logRESP(void);
        unsigned int  exchange(unsigned int ok=0x9000, unsigned int mask=0xFFFF);
        void reset_buffer(void);




    public:
        DeviceLedger();
        DeviceLedger(const DeviceLedger &device);
        ~DeviceLedger();

        DeviceLedger& operator=(const DeviceLedger &device);
        explicit operator bool() const {return this->hContext != 0;}

        bool  reset(void);

        /* ======================================================================= */
        /*                              SETUP/TEARDOWN                             */
        /* ======================================================================= */
        bool set_name(const std::string &name);
        std::string get_name();
        bool init(void);
        bool release();
        bool connect(void);
        bool disconnect();

        /* ======================================================================= */
        /*                             WALLET & ADDRESS                            */
        /* ======================================================================= */
        bool  get_public_address(cryptonote::account_public_address &pubkey);
        #ifdef DEBUGLEDGER
        bool  get_secret_keys(crypto::secret_key &viewkey , crypto::secret_key &spendkey);
        #endif
        bool  generate_chacha_key(const cryptonote::account_keys &keys, crypto::chacha_key &key);


        /* ======================================================================= */
        /*                               SUB ADDRESS                               */
        /* ======================================================================= */
        bool  derive_subaddress_public_key(const crypto::public_key &pub, const crypto::key_derivation &derivation, const std::size_t output_index,  crypto::public_key &derived_pub);
        bool  get_subaddress_spend_public_key(const cryptonote::account_keys& keys, const cryptonote::subaddress_index& index, crypto::public_key &D);
        bool  get_subaddress(const cryptonote::account_keys& keys, const cryptonote::subaddress_index &index, cryptonote::account_public_address &address);
        bool  get_subaddress_secret_key(const crypto::secret_key &sec, const cryptonote::subaddress_index &index, crypto::secret_key &sub_sec);

        /* ======================================================================= */
        /*                            DERIVATION & KEY                             */
        /* ======================================================================= */
        bool  verify_keys(const crypto::secret_key &secret_key, const crypto::public_key &public_key);
        bool  scalarmultKey(const rct::key &pub, const rct::key &sec, rct::key &mulkey);
        bool  scalarmultBase(const rct::key &sec, rct::key &mulkey);
        bool  sc_secret_add(const crypto::secret_key &a, const crypto::secret_key &b, crypto::secret_key &r);
        bool  generate_keys(bool recover, const crypto::secret_key& recovery_key, crypto::public_key &pub, crypto::secret_key &sec, crypto::secret_key &rng);
        bool  generate_key_derivation(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_derivation &derivation);
        bool  derivation_to_scalar(const crypto::key_derivation &derivation, const size_t output_index, crypto::ec_scalar &res);
        bool  derive_secret_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::secret_key &sec,  crypto::secret_key &derived_sec);
        bool  derive_public_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::public_key &pub,  crypto::public_key &derived_pub);
        bool  secret_key_to_public_key(const crypto::secret_key &sec, crypto::public_key &pub);
        bool  generate_key_image(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_image &image);
        bool  verify_key(const crypto::secret_key &secret_key, const crypto::public_key &public_key);

        /* ======================================================================= */
        /*                               TRANSACTION                               */
        /* ======================================================================= */

        bool  open_tx(void);

        bool  get_additional_key(const bool subaddr, cryptonote::keypair &additional_txkey);
        bool  get_sub_tx_public_key(const crypto::public_key &dest_key, crypto::public_key &pub);
        bool  set_signature_mode(unsigned int sig_mode);

        bool  stealth(const crypto::public_key &public_key, const crypto::secret_key &secret_key, crypto::hash8 &payment_id );

        bool  ecdhEncode(const rct::key &AKout, rct::ecdhTuple &unmasked);
        bool  ecdhDecode(const rct::key &AKout, rct::ecdhTuple &masked);

        bool  add_output_key_mapping(const crypto::public_key &Aout, const crypto::public_key &Bout, size_t real_output_index,
                                            const rct::key &amount_key,  const crypto::public_key &out_eph_public_key);


        bool  mlsag_prehash(const std::string &blob, size_t inputs_size, size_t outputs_size, const rct::keyV &hashes, const rct::ctkeyV &outPk, rct::key &prehash);
        bool  mlsag_prepare(const rct::key &H, const rct::key &xx, rct::key &a, rct::key &aG, rct::key &aHP, rct::key &rvII);
        bool  mlsag_prepare(rct::key &a, rct::key &aG);
        bool  mlsag_hash(const rct::keyV &long_message, rct::key &c);
        bool  mlsag_sign( const rct::key &c, const rct::keyV &xx, const rct::keyV &alpha, const int rows, const int dsRows, rct::keyV &ss);

        bool  close_tx(void);

    };



    #ifdef DEBUGLEDGER
    extern crypto::secret_key viewkey;
    extern crypto::secret_key spendkey;
    #endif
}}

