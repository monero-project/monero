#include <cstddef>
#include <string>
#include <mutex>

#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/subaddress_index.h"
#include "cryptonote_basic/blobdatatype.h"


#include "PCSC/winscard.h"

#pragma once

#include "log.hpp"


namespace ledger {
    

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

    class Device {
    private:
        mutable std::mutex   device_locker;
        mutable std::mutex   tx_locker;
        void lock_device() ;
        void unlock_device() ;
        void lock_tx() ;
        void unlock_tx() ;

        std::string  name;
        std::string  full_name;
        mutable SCARDCONTEXT hContext;
        mutable SCARDHANDLE  hCard;
        mutable DWORD        length_send;
        mutable BYTE         buffer_send[BUFFER_SEND_SIZE];
        mutable DWORD        length_recv;
        mutable BYTE         buffer_recv[BUFFER_SEND_SIZE];
        unsigned int id;

        Keymap key_map;
        

        void logCMD(void);
        void logRESP(void);
        int  exchange(void);
        void reset_buffer(void);

    public:
        Device();
        Device(const Device &device);
        ~Device();
        Device& operator=(const Device &device);
        explicit operator bool() const {return this->hContext != 0;}

        static const int SIGNATURE_REAL = 0;
        static const int SIGNATURE_FAKE = 1;

        bool  init(void);
        bool  release(void);
        bool  connect(void);
        bool  disconnect(void);
        bool  set_name(const std::string &name);
        std::string get_name(void);

        bool  reset(void);

        /* ======================================================================= */
        /*                             WALLET & ADDRESS                            */
        /* ======================================================================= */
        bool  get_public_address(cryptonote::account_public_address &pubkey);
        bool  get_secret_keys(crypto::secret_key &viewkey , crypto::secret_key &spendkey);
        bool  get_chacha8_prekey(char  *prekey);

        /* ======================================================================= */
        /*                               SUB ADDRESS                               */
        /* ======================================================================= */
        bool  derive_subaddress_public_key(const crypto::public_key &pub, const crypto::key_derivation &derivation, const std::size_t output_index,  crypto::public_key &derived_pub);
        bool  get_subaddress_spend_public_key(const cryptonote::subaddress_index& index, crypto::public_key &D);
        bool  get_subaddress(const cryptonote::subaddress_index &index, cryptonote::account_public_address &address);
        bool  get_subaddress_secret_key(const crypto::secret_key &sec, const cryptonote::subaddress_index &index, crypto::secret_key &sub_sec);
  
        /* ======================================================================= */
        /*                            DERIVATION & KEY                             */
        /* ======================================================================= */
        bool  scalarmultKey(const rct::key &pub, const rct::key &sec, rct::key &mulkey);
        bool  scalarmultBase(const rct::key &sec, rct::key &mulkey);
        bool  sc_add(const crypto::secret_key &a, const crypto::secret_key &b, crypto::secret_key &r);
        bool  generate_keypair(crypto::public_key &pub, crypto::secret_key &sec);
        bool  generate_key_derivation(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_derivation &derivation);
        bool  derivation_to_scalar(const crypto::key_derivation &derivation, const size_t output_index, crypto::ec_scalar &res);
        bool  derive_secret_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::secret_key &sec,  crypto::secret_key &derived_sec);
        bool  derive_public_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::public_key &pub,  crypto::public_key &derived_pub);
        bool  secret_key_to_public_key(const crypto::secret_key &sec, crypto::public_key &pub);
        bool  generate_key_image(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_image &image);      
        bool  verify_key(const crypto::public_key &view_public_key);
        bool  verify_key(const crypto::public_key &view_public_key, const crypto::public_key &spend_public_key, bool with_spend_key=true) ;

        /* ======================================================================= */
        /*                               TRANSACTION                               */
        /* ======================================================================= */

      bool add_output_key_mapping(const crypto::public_key &Aout, const crypto::public_key &Bout, size_t real_output_index,  
                               const rct::key &amount_key,  const crypto::public_key &out_eph_public_key) ;

        bool  open_tx(cryptonote::keypair &txkey);

        bool  get_additional_key(const bool subaddr, cryptonote::keypair &additional_txkey);
        bool  get_sub_tx_public_key(const crypto::public_key &dest_key, crypto::public_key &pub);
        bool  set_signature_mode(unsigned int sig_mode);
        
        bool  stealth(crypto::hash8 &payment_id, const crypto::public_key &public_key);
        
        bool  blind(rct::ecdhTuple &unmasked, const rct::key &AKout);
        bool  unblind(rct::ecdhTuple &masked, const rct::key &AKout);

        bool  mlsag_prehash(const std::string &blob, size_t inputs_size, size_t outputs_size, const rct::keyV &hashes, const rct::ctkeyV &outPk, rct::key &prehash);
        bool  mlsag_prepare(const rct::key &H, const rct::key &xx, rct::key &a, rct::key &aG, rct::key &aHP, rct::key &rvII);
        bool  mlsag_prepare(rct::key &a, rct::key &aG);
        bool  mlsag_hash(const rct::keyV &long_message, rct::key &c);
        bool  mlsag_sign(rct::keyV &ss, const rct::keyV &xx, const rct::keyV &alpha, int rows);

        bool  close_tx();        
    };

    extern Device no_device;
}
