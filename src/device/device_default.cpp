

#include "device_default.hpp"

#include "ringct/rctCryptoOps.h"
#include "ringct/rctOps.h"
#include "wallet/wallet2.h"

namespace hw {



    namespace core {

        DeviceDefault::DeviceDefault() {
        }

        DeviceDefault::DeviceDefault(const DeviceDefault &device): DeviceDefault() {
        }

        DeviceDefault::~DeviceDefault() {
        }

        DeviceDefault& DeviceDefault::operator=(const DeviceDefault &device) {
            return *this;
        }


        /* ======================================================================= */
        /*                             WALLET & ADDRESS                            */
        /* ======================================================================= */

        bool  DeviceDefault::generate_chacha_key(const cryptonote::account_keys &keys, crypto::chacha_key &key) {
            return cryptonote::generate_chacha_key_from_secret_keys(keys, key);
        }

        /* ======================================================================= */
        /*                               SUB ADDRESS                               */
        /* ======================================================================= */

        bool DeviceDefault::derive_subaddress_public_key(const crypto::public_key &out_key, const crypto::key_derivation &derivation, const std::size_t output_index, crypto::public_key &derived_key){
            return crypto::derive_subaddress_public_key(out_key, derivation, output_index,derived_key);
        }

        bool DeviceDefault::get_subaddress_spend_public_key(const cryptonote::account_keys& keys, const cryptonote::subaddress_index &index, crypto::public_key &D) {
            D = cryptonote::get_subaddress_spend_public_key(keys,index);
            return true;
        }

        bool DeviceDefault::get_subaddress(const cryptonote::account_keys& keys, const cryptonote::subaddress_index &index, cryptonote::account_public_address &address) {
            address = cryptonote::get_subaddress(keys,index);
            return true;
        }

        bool  DeviceDefault::get_subaddress_secret_key(const crypto::secret_key &a, const cryptonote::subaddress_index &index, crypto::secret_key &m) {
            m = cryptonote::get_subaddress_secret_key(a,index);
            return true;
        }

        /* ======================================================================= */
        /*                            DERIVATION & KEY                             */
        /* ======================================================================= */

        bool  DeviceDefault::verify_keys(const crypto::secret_key &secret_key, const crypto::public_key &public_key) {
            return cryptonote::verify_keys(secret_key, public_key);
        }

        bool DeviceDefault::scalarmultKey(const rct::key &P, const rct::key &a, rct::key &aP) {
            rct::scalarmultKey(aP, P,a);
            return true;
        }

        bool DeviceDefault::scalarmultBase(const rct::key &a, rct::key &aG) {
            rct::scalarmultBase(aG,a);
            return true;
        }

        bool DeviceDefault::sc_secret_add(const crypto::secret_key &a, const crypto::secret_key &b, crypto::secret_key &r) {
            sc_add((unsigned char*)&a, (unsigned char*)&b, (unsigned char*)&r);
            return true;
        }

        bool  DeviceDefault::generate_keys(bool recover, const crypto::secret_key& recovery_key, crypto::public_key &pub, crypto::secret_key &sec, crypto::secret_key &rng) {
            rng = crypto::generate_keys(pub, sec, recovery_key, recover);
            return true;
        }

        bool DeviceDefault::generate_key_derivation(const crypto::public_key &key1, const crypto::secret_key &key2, crypto::key_derivation &derivation) {
            return crypto::generate_key_derivation(key1, key2, derivation);
        }

        bool DeviceDefault::derivation_to_scalar(const crypto::key_derivation &derivation, const size_t output_index, crypto::ec_scalar &res){
            crypto::derivation_to_scalar(derivation,output_index, res);
            return true;
        }

        bool DeviceDefault::derive_secret_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::secret_key &base, crypto::secret_key &derived_key){
            crypto::derive_secret_key(derivation, output_index, base, derived_key);
            return true;
        }

        bool DeviceDefault::derive_public_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::public_key &base, crypto::public_key &derived_key){
            return crypto::derive_public_key(derivation, output_index, base, derived_key);
        }

        bool DeviceDefault::secret_key_to_public_key(const crypto::secret_key &sec, crypto::public_key &pub) {
            return crypto::secret_key_to_public_key(sec,pub);
        }

        bool DeviceDefault::generate_key_image(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_image &image){
            crypto::generate_key_image(pub, sec,image);
            return true;
        }

        /* ======================================================================= */
        /*                               TRANSACTION                               */
        /* ======================================================================= */

        bool DeviceDefault::open_tx() {
            return true;
        }


        bool DeviceDefault::add_output_key_mapping(const crypto::public_key &Aout, const crypto::public_key &Bout, size_t real_output_index,
                                                  const rct::key &amount_key,  const crypto::public_key &out_eph_public_key)  {
            return true;
        }

        bool  DeviceDefault::set_signature_mode(unsigned int sig_mode) {
            return true;
        }

        bool  DeviceDefault::stealth(const crypto::public_key &public_key, const crypto::secret_key &secret_key, crypto::hash8 &payment_id ) {
            return cryptonote::encrypt_payment_id(payment_id, public_key, secret_key);
        }

        bool  DeviceDefault::ecdhEncode(const rct::key &sharedSec, rct::ecdhTuple &unmasked) {
            rct::ecdhEncode(unmasked, sharedSec);
            return true;
        }

        bool  DeviceDefault::ecdhDecode(const rct::key &sharedSec, rct::ecdhTuple &masked) {
            rct::ecdhDecode(masked, sharedSec);
            return true;
        }

        bool DeviceDefault::mlsag_prepare(const rct::key &H, const rct::key &xx,
                                         rct::key &a, rct::key &aG, rct::key &aHP, rct::key &II) {
            rct::skpkGen(a, aG);
            rct::scalarmultKey(aHP, H, a);
            rct::scalarmultKey(II, H, xx);
            return true;
        }
        bool  DeviceDefault::mlsag_prepare(rct::key &a, rct::key &aG) {
            rct::skpkGen(a, aG);
            return true;
        }
        bool  DeviceDefault::mlsag_prehash(const std::string &blob, size_t inputs_size, size_t outputs_size, const rct::keyV &hashes, const rct::ctkeyV &outPk, rct::key &prehash) {
            prehash = rct::cn_fast_hash(hashes);
            return true;
        }


        bool DeviceDefault::mlsag_hash(const rct::keyV &toHash, rct::key &c_old) {
            c_old = rct::hash_to_scalar(toHash);
            return true;
        }

        bool DeviceDefault::mlsag_sign(const rct::key &c,  const rct::keyV &xx, const rct::keyV &alpha, const int rows, const int dsRows, rct::keyV &ss ) {
            for (int j = 0; j < rows; j++) {
                sc_mulsub(ss[j].bytes, c.bytes, xx[j].bytes, alpha[j].bytes);
            }
            return true;
        }

        bool DeviceDefault::close_tx() {
            return true;
        }

    }

    core::DeviceDefault default_core_device;


    Device& get_default_device() {
        return default_core_device;
    }
}