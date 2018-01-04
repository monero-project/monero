#include <cstddef>
#include <string.h>
#include "misc_log_ex.h"
#include "device.hpp"
#include "log.hpp"

//#define IODUMMYCRYPT 1

namespace ledger { 



  /* ===================================================================== */
  /* ===                           Misc                               ==== */
  /* ===================================================================== */


#define ASSERT_RV(rv)   CHECK_AND_ASSERT_THROW_MES((rv)==SCARD_S_SUCCESS, "Fail SCard API : (" << (rv) << ") "<< pcsc_stringify_error(rv)<<" Device="<<this->id<<", hCard="<<hCard<<", hContext="<<hContext);
  
  static void buffer_to_str(char *to,  const char *buff, int len) {
    for (int i=0; i<len; i++) {
      sprintf(to+2*i, "%.02x", (unsigned char)buff[i]);
    }
  }
  static void bufferLE_to_str(char *to,  const char *buff, int len) {
    for (int i=0; i<len; i++) {
      sprintf(to+2*i, "%.02x", (unsigned char)buff[31-i]);
    }
  }


  void log_hexbuffer(std::string msg,  const char* buff, int len) {
    char logstr[1024];
    buffer_to_str(logstr,  buff, len);    
    MCDEBUG("ledger", msg<< ": " << logstr);
  }

  void log_hexbufferLE(std::string msg,  const char* buff, int len) {
    char logstr[1024];
    bufferLE_to_str(logstr,  buff, len);    
    MCDEBUG("ledger", msg<< ": " << logstr);
  }

  void log_message(std::string msg,  std::string info ) {
       
    MCDEBUG("ledger", msg << ": " << info);
  }


  void decrypt(char* buf, int len) {
#ifdef IODUMMYCRYPT    
   for (int i = 0; i<len;i++) {
        buf[i] ^= 0x55;
      }
#endif
  }

  static void check(std::string msg, std::string info, const char *h, const char *d, int len, bool crypted) {
    char dd[32];
    char logstr[128];

    memmove(dd,d,len);
    if (crypted) {
      decrypt(dd,len);
    }

    if (memcmp(h,dd,len)) {
        log_message("ASSERT EQ FAIL",  msg + ": "+ info ); 
        log_hexbuffer("    host  ", h, len);
        log_hexbuffer("    device", dd, len);

    } else {
      buffer_to_str(logstr,  dd, len);    
      log_message("ASSERT EQ OK",  msg + ": "+ info + ": "+ std::string(logstr) ); 
    }
  }

  void check32(std::string msg, std::string info, const char *h, const char *d, bool crypted) {
    check(msg, info, h, d, 32, crypted);
  }

  void check8(std::string msg, std::string info, const char *h, const char *d, bool crypted) {
    check(msg, info, h, d, 8, crypted);
  }


  /* ===================================================================== */
  /* ===                        Keymap                                ==== */
  /* ===================================================================== */
 
  ABPkeys::ABPkeys(const rct::key& A, const rct::key& B, size_t real_output_index, const rct::key& P, const rct::key& AK) {
    memcpy(&Aout,  &A, sizeof(rct::key));
    memcpy(&Bout,  &B, sizeof(rct::key));
    index = real_output_index;
    memcpy(&Pout,  &P, sizeof(rct::key));
    memcpy(&AKout, &AK, sizeof(rct::key));
  }

  ABPkeys::ABPkeys(const ABPkeys& keys) {
    memcpy(&Aout,  &keys.Aout, sizeof(rct::key));
    memcpy(&Bout,  &keys.Bout, sizeof(rct::key));
    index = keys.index;
    memcpy(&Pout,  &keys.Pout, sizeof(rct::key));
    memcpy(&AKout, &keys.AKout, sizeof(rct::key));
  }

  bool Keymap::find(const rct::key& P, ABPkeys& keys) const {
    int sz = ABP.size();
    for (int i=0; i<sz; i++) {
      if (memcmp(ABP[i].Pout.bytes, P.bytes,32) == 0) {
        keys = ABP[i];
        return true;
      }
    }
    return false;
  }

  void Keymap::add(const ABPkeys& keys) {
    ABP.push_back(keys);
  } 

  void Keymap::clear() {
    ABP.clear();
  }

  void Keymap::log() {
    log_message("keymap", "content");
    int sz = ABP.size();
    for (int i=0; i<sz; i++) {
      log_message("  keymap", std::to_string(i));
      log_hexbuffer("    Aout", (char*)ABP[i].Aout.bytes, 32);
      log_hexbuffer("    Bout", (char*)ABP[i].Bout.bytes, 32);
      log_message  ("   index", std::to_string(ABP[i].index));
      log_hexbuffer("    Pout", (char*)ABP[i].Pout.bytes, 32);
    }
  }

  /* ===================================================================== */
  /* ===                         Device                               ==== */
  /* ===================================================================== */

  static int device_id = 0;

  #define INS_NONE                            0x00
  #define INS_RESET                           0x02

  #define INS_GET_KEY                         0x20
  #define INS_PUT_KEY                         0x22
  #define INS_GET_CHACHA8_PREKEY              0x24
  #define INS_VERIFY_KEY                      0x26

  #define INS_SECRET_KEY_TO_PUBLIC_KEY        0x30
  #define INS_GEN_KEY_DERIVATION              0x32
  #define INS_DERIVATION_TO_SCALAR            0x34
  #define INS_DERIVE_PUBLIC_KEY               0x36
  #define INS_DERIVE_SECRET_KEY               0x38
  #define INS_GEN_KEY_IMAGE                   0x3A
  #define INS_SECRET_KEY_ADD                  0x3C
  #define INS_SECRET_KEY_SUB                  0x3E
  #define INS_GENERATE_KEYPAIR                0x40
  #define INS_SECRET_SCAL_MUL_KEY             0x42
  #define INS_SECRET_SCAL_MUL_BASE            0x44

  #define INS_DERIVE_SUBADDRESS_PUBLIC_KEY    0x46
  #define INS_GET_SUBADDRESS                  0x48
  #define INS_GET_SUBADDRESS_SPEND_PUBLIC_KEY 0x4A
  #define INS_GET_SUBADDRESS_SECRET_KEY       0x4C

  #define INS_OPEN_TX                         0x70
  #define INS_SET_SIGNATURE_MODE              0x72
  #define INS_GET_ADDITIONAL_KEY              0x74
  #define INS_STEALTH                         0x76
  #define INS_PROCESS_INPUT                   0x78 
  #define INS_PROCESS_OUTPUT                  0x7A
  #define INS_AMOUNT_KEY                      0x7C
  #define INS_BLIND                           0x7E
  #define INS_UNBLIND                         0x80
  #define INS_VALIDATE                        0x82
  #define INS_MLSAG                           0x84
  #define INS_CLOSE_TX                        0x86       

  
  #define INS_GET_RESPONSE                    0xc0

  Device no_device;


  void Device::logCMD() {
    if (apdu_verbose) {
      char  strbuffer[1024];
      sprintf(strbuffer, "%.02x %.02x %.02x %.02x %.02x ",
        this->buffer_send[0],
        this->buffer_send[1],
        this->buffer_send[2],
        this->buffer_send[3],
        this->buffer_send[4]
        );
      buffer_to_str(strbuffer+strlen(strbuffer), (char*)(this->buffer_send+5), this->length_send-5);
      MCDEBUG("ledger", "CMD  :" << std::string(strbuffer));
    }
  }

  void Device::logRESP() {
    if (apdu_verbose) {
      char  strbuffer[1024];
      sprintf(strbuffer, "%.02x%.02x ",
        this->buffer_recv[this->length_recv-2],
        this->buffer_recv[this->length_recv-1]
        );
      buffer_to_str(strbuffer+strlen(strbuffer), (char*)(this->buffer_recv), this->length_recv-2);
      MCDEBUG("ledger", "RESP :" << std::string(strbuffer));

    }
  }
  void set_apdu_verbose(bool verbose) {
    apdu_verbose = verbose;
  }

  /* -------------------------------------------------------------- */
  Device::Device() {
    this->id = device_id++;
    this->hCard   = 0;
    this->hContext = 0;
    this->reset_buffer();
    if (this->id) 
      MCDEBUG("ledger", "Device "<<this->id <<" Created");
  }

  Device::Device(const Device &device): Device() {
    this->set_name(device.name);
    MCDEBUG("ledger", "Device "<<device.id <<" Cloned. Device "<<this->id <<" Created");
  }

  Device::~Device() {
    this->release();
    if (this->id) {
      MCDEBUG("ledger", "Device "<<this->id <<" Destroyed");
    }
  }

  bool Device::set_name(const std::string & name) {
    this->name = name;
    return true;
  }

  std::string Device::get_name() {
    if (this->full_name.empty() || (this->hCard == 0)) {
      return std::string("<disconnected:").append(this->name).append(">");
    }
    return this->full_name;
  }

  bool Device::init(void) {
   LONG  rv;
    this->release();
    rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM,0,0, &this->hContext);
    ASSERT_RV(rv);
    MCDEBUG("ledger", "Device "<<this->id <<" SCardContext created: hContext="<<this->hContext);
    this->hCard = 0;
    return true;
  }

  bool Device::release() {
    this->disconnect();
    if (this->hContext) {
      SCardReleaseContext(this->hContext);
      MCDEBUG("ledger", "Device "<<this->id <<" SCardContext released: hContext="<<this->hContext); 
      this->hContext = 0;
      this->full_name.clear();
    }
    return true;
  }

  bool Device::connect(void) {
    BYTE  pbAtr[MAX_ATR_SIZE];
    LPSTR mszReaders;
    DWORD dwReaders;
    LONG  rv;
    DWORD dwState, dwProtocol, dwAtrLen, dwReaderLen;

    this->disconnect();

    dwReaders = SCARD_AUTOALLOCATE;
    if ((rv = SCardListReaders(this->hContext, NULL, (LPSTR)&mszReaders, &dwReaders)) == SCARD_S_SUCCESS) {
      char* p;
      const char* prefix = this->name.c_str();
      
      p = mszReaders;
      MCDEBUG("ledger", "Looking for " << std::string(prefix));
      while (*p) {
        MCDEBUG("ledger", "Device Found: " <<  std::string(p));
        if ((memcmp(prefix, p, strlen(prefix))==0)) {
          MCDEBUG("ledger", "Device Match: " <<  std::string(p));
          if ((rv = SCardConnect(this->hContext, 
                                 p, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T0, 
                                 &this->hCard, &dwProtocol))!=SCARD_S_SUCCESS) {
            break;
          } 
          MCDEBUG("ledger", "Device "<<this->id <<" Connected: hCard="<<this->hCard);
          dwAtrLen = sizeof(pbAtr);
          if ((rv = SCardStatus(this->hCard, NULL, &dwReaderLen, &dwState, &dwProtocol, pbAtr, &dwAtrLen))!=SCARD_S_SUCCESS) {
            break;
          } 
          MCDEBUG("ledger", "Device "<<this->id <<" Status OK");
          rv = SCARD_S_SUCCESS ;
          this->full_name = std::string(p);
          break;
        }
        p += strlen(p) +1;
      }
    }

    if (mszReaders) {
      SCardFreeMemory(this->hContext, mszReaders);
      mszReaders = NULL;
    }
    if (rv != SCARD_S_SUCCESS) {
      if ( hCard) {
        SCardDisconnect(this->hCard, SCARD_UNPOWER_CARD);
        MCDEBUG("ledger", "Device "<<this->id <<" disconnected: hCard="<<this->hCard);
        this->hCard = 0;
      }
    } 
    ASSERT_RV(rv);

    this->reset();
    return rv==SCARD_S_SUCCESS;
  }

  bool Device::disconnect() {
    if (this->hCard) {
      SCardDisconnect(this->hCard, SCARD_UNPOWER_CARD);
      MCDEBUG("ledger", "Device "<<this->id <<" disconnected: hCard="<<this->hCard);
      this->hCard = 0;
    }
    
    return true;
  }

  int Device::exchange() {
    LONG rv;

    logCMD();

    this->length_recv = BUFFER_SEND_SIZE;
    rv = SCardTransmit(this->hCard, 
    SCARD_PCI_T0, this->buffer_send, this->length_send,
    NULL,         this->buffer_recv, &this->length_recv);
    ASSERT_RV(rv);

    logRESP();
    return  (this->buffer_recv[this->length_recv-2]<<8) | this->buffer_recv[this->length_recv-1];
  }

  void Device::reset_buffer() {
    this->length_send = 0;
    memset(this->buffer_send, 0, BUFFER_SEND_SIZE);
    this->length_recv = 0;
    memset(this->buffer_recv, 0, BUFFER_SEND_SIZE);    
  }

  bool Device::reset() {
    int offset;
  
    reset_buffer();
    
    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_RESET;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();
    return true;
  }

  /* ======================================================================= */
  /*                             WALLET & ADDRESS                            */
  /* ======================================================================= */

  /* Application API */ 
   bool Device::get_public_address(cryptonote::account_public_address &pubkey){
     int offset;

    reset_buffer();
    
    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_GET_KEY;
    this->buffer_send[2] = 0x01;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(pubkey.m_view_public_key.data, this->buffer_recv, 32); 
    memmove(pubkey.m_spend_public_key.data, this->buffer_recv+32, 32); 
    return true;
  }

  bool  Device::get_secret_keys(crypto::secret_key &viewkey , crypto::secret_key &spendkey) {
    int offset;

    reset_buffer();
    
    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_GET_KEY;
    this->buffer_send[2] = 0x02;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    
    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(viewkey.data,  this->buffer_recv,    32); 
    memmove(spendkey.data, this->buffer_recv+32, 32);
   
    return true;
  }

  bool  Device::get_chacha8_prekey(char  *prekey)  {
    int offset;

    reset_buffer();
    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_GET_CHACHA8_PREKEY;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    
    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(prekey, &this->buffer_recv[0], 200);
    return true;
  }

  /* ======================================================================= */
  /*                               SUB ADDRESS                               */
  /* ======================================================================= */

  static bool is_key_view_secret_key(const crypto::secret_key &sec) {
    for (int i =0; i <32; i++) {
      if (sec.data[i] != 0x00) return false;
    }
    return true;
  }

  static bool is_key_spend_secret_key(const crypto::secret_key &sec) {
    for (int i =0; i <32; i++) {
      if (sec.data[i] != (char)0xff) return false;
    }
    return true;
  }
  

  bool Device::derive_subaddress_public_key(const crypto::public_key &pub, const crypto::key_derivation &derivation, const std::size_t output_index, crypto::public_key &derived_pub){
    int offset;
    unsigned char options;
    
    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_DERIVE_SUBADDRESS_PUBLIC_KEY;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //pub 
    memmove(this->buffer_send+offset, pub.data, 32);
    offset += 32;
    //derivation
    memmove(this->buffer_send+offset, derivation.data, 32);
    offset += 32;
    //index
    this->buffer_send[offset+0] = output_index>>24;
    this->buffer_send[offset+1] = output_index>>16;
    this->buffer_send[offset+2] = output_index>>8;
    this->buffer_send[offset+3] = output_index>>0;
    offset += 4;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pub key
    memmove(derived_pub.data, &this->buffer_recv[0], 32);
    return true;
  }

  bool Device::get_subaddress_spend_public_key(const cryptonote::subaddress_index& index, crypto::public_key &D) {
    int offset;

    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_GET_SUBADDRESS_SPEND_PUBLIC_KEY;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //index
    assert(sizeof(cryptonote::subaddress_index) == 8);
    memmove(this->buffer_send+offset, &index, sizeof(cryptonote::subaddress_index));
    offset +=8 ;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(D.data, &this->buffer_recv[0], 32);
    return true;
  }

  bool Device::get_subaddress(const cryptonote::subaddress_index& index, cryptonote::account_public_address &address) {
    int offset;

    reset_buffer();
    
    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_GET_SUBADDRESS;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //index
    assert(sizeof(cryptonote::subaddress_index) == 8);
    memmove(this->buffer_send+offset, &index, sizeof(cryptonote::subaddress_index));
    offset +=8 ;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(address.m_view_public_key.data,  &this->buffer_recv[0],  32);
    memmove(address.m_spend_public_key.data, &this->buffer_recv[32], 32);
    return true;
  }

  bool  Device::get_subaddress_secret_key(const crypto::secret_key& sec, const cryptonote::subaddress_index& index, crypto::secret_key &sub_sec) {
    int offset,  options;

    reset_buffer();

    options = 0;
    if (is_key_view_secret_key(sec)) {
      options = 1;
    } 
    if (is_key_spend_secret_key(sec)) {
      options = 2;
    }

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_GET_SUBADDRESS_SECRET_KEY;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //sec 
    memmove(this->buffer_send+offset, sec.data, 32);
    offset += 32;
    //index
    assert(sizeof(cryptonote::subaddress_index) == 8);
    memmove(this->buffer_send+offset, &index, sizeof(cryptonote::subaddress_index));
    offset +=8 ;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(sub_sec.data,  &this->buffer_recv[0],  32);
    return true;
  }

  /* ======================================================================= */
  /*                            DERIVATION & KEY                             */
  /* ======================================================================= */
  bool  Device::verify_key(const crypto::public_key &view_public_key, const crypto::public_key & spend_public_key, bool with_spend_key)  {
    int offset,sw;
    reset_buffer();
    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_VERIFY_KEY;
    this->buffer_send[2] = with_spend_key?0x02:0x01;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //view
    memmove(this->buffer_send+offset, view_public_key.data, 32);
    offset += 32;
    //spend
    if (with_spend_key) {
      memmove(this->buffer_send+offset, spend_public_key.data, 32);
      offset += 32;
    }
    

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    sw = this->exchange();

    return sw == 0x9000;
  }

  bool Device::verify_key(const crypto::public_key &view_public_key) {
    crypto::public_key fake;
    return verify_key(view_public_key, fake, false );
  }

  bool Device::scalarmultKey(const rct::key &pub, const rct::key &sec, rct::key mulkey) {
    int offset, options;

    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_SECRET_SCAL_MUL_KEY;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //pub 
    memmove(this->buffer_send+offset, pub.bytes, 32);
    offset += 32;
    //sec
    memmove(this->buffer_send+offset, sec.bytes, 32);
    offset += 32;
    

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pub key
    memmove(mulkey.bytes, &this->buffer_recv[0], 32);
    return true;
  }
 
  bool Device::scalarmultBase(const rct::key &sec, rct::key mulkey) {
    int offset,options;

    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_SECRET_SCAL_MUL_BASE;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //sec
    memmove(this->buffer_send+offset, sec.bytes, 32);
    offset += 32;  

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pub key
    memmove(mulkey.bytes, &this->buffer_recv[0], 32);
    return true;
  }

  bool Device::sc_add(const crypto::secret_key &a, const crypto::secret_key &b, crypto::secret_key &r) {
    int offset;
    unsigned char options;
    
    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_SECRET_KEY_ADD;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //sec key
    memmove(this->buffer_send+offset, a.data, 32);
    offset += 32;
    //sec key
    memmove(this->buffer_send+offset, b.data, 32);
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pub key
    memmove(r.data, &this->buffer_recv[0], 32);
    return true;
  }

  bool  Device::generate_keypair(crypto::public_key &pub, crypto::secret_key &sec) {
    int offset;
    unsigned char options;
    
    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_GENERATE_KEYPAIR;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pub key
    memmove(pub.data, &this->buffer_recv[0], 32);
    memmove(sec.data, &this->buffer_recv[32], 32);
    return true;

  }

 bool Device::generate_key_derivation(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_derivation &derivation) {
    int offset;
    unsigned char options;
    
    reset_buffer();

    options = 0;
    if (is_key_view_secret_key(sec)) {
      options = 1;
    } 
    if (is_key_spend_secret_key(sec)) {
      options = 2;
    } 

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_GEN_KEY_DERIVATION;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //pub
    memmove(this->buffer_send+offset, pub.data, 32);
    offset += 32;
     //sec
    memmove(this->buffer_send+offset, sec.data, 32);
    offset += 32;
      
    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //derivattion data
    memmove(derivation.data, &this->buffer_recv[0], 32);
    return true;
  }

  
  bool Device::derivation_to_scalar(const crypto::key_derivation &derivation, const size_t output_index, crypto::ec_scalar &res){
    int offset;
    unsigned char options;
    
    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_DERIVATION_TO_SCALAR;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //derivattion
    memmove(this->buffer_send+offset, derivation.data, 32);
    offset += 32;
    //index
    this->buffer_send[offset+0] = output_index>>24;
    this->buffer_send[offset+1] = output_index>>16;
    this->buffer_send[offset+2] = output_index>>8;
    this->buffer_send[offset+3] = output_index>>0;
    offset += 4;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //derivattion data
    memmove(res.data, &this->buffer_recv[0], 32);
    return true;
  }

  bool Device::derive_secret_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::secret_key &sec, crypto::secret_key &derived_sec){
    int offset;
    unsigned char options;
    
    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_DERIVE_SECRET_KEY;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //derivation 
    memmove(this->buffer_send+offset, derivation.data, 32);
    offset += 32;
    //index
    this->buffer_send[offset+0] = output_index>>24;
    this->buffer_send[offset+1] = output_index>>16;
    this->buffer_send[offset+2] = output_index>>8;
    this->buffer_send[offset+3] = output_index>>0;
    offset += 4;
    //sec
    memmove(this->buffer_send+offset, sec.data, 32);
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pub key
    memmove(derived_sec.data, &this->buffer_recv[0], 32);
    return true;
  }

  bool Device::derive_public_key(const crypto::key_derivation &derivation, const std::size_t output_index, const crypto::public_key &pub, crypto::public_key &derived_pub){
    int offset;
    unsigned char options;
    
    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_DERIVE_PUBLIC_KEY;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //derivation 
    memmove(this->buffer_send+offset, derivation.data, 32);
    offset += 32;
    //index
    this->buffer_send[offset+0] = output_index>>24;
    this->buffer_send[offset+1] = output_index>>16;
    this->buffer_send[offset+2] = output_index>>8;
    this->buffer_send[offset+3] = output_index>>0;
    offset += 4;
    //pub
    memmove(this->buffer_send+offset, pub.data, 32);
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pub key
    memmove(derived_pub.data, &this->buffer_recv[0], 32);
    return true;
  }

  bool Device::secret_key_to_public_key(const crypto::secret_key &sec, crypto::public_key &pub) {
    int offset;
    unsigned char options;
    
    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_SECRET_KEY_TO_PUBLIC_KEY;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //sec key
    memmove(this->buffer_send+offset, sec.data, 32);
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pub key
    memmove(pub.data, &this->buffer_recv[0], 32);
    return true;
  }

  bool Device::generate_key_image(const crypto::public_key &pub, const crypto::secret_key &sec, crypto::key_image &image){
    int offset;
    unsigned char options;
    
    reset_buffer();

    options = 0;

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_DERIVE_SECRET_KEY;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = options;
    offset += 1;
    //pub 
    memmove(this->buffer_send+offset, pub.data, 32);
    offset += 32;
    //sec
    memmove(this->buffer_send+offset, sec.data, 32);
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pub key
    memmove(image.data, &this->buffer_recv[0], 32);
    return true;
  }


  /* ======================================================================= */
  /*                               TRANSACTION                               */
  /* ======================================================================= */

  /* --------------------------------------------------------------------- */
  /*                                                                       */
  /* --------------------------------------------------------------------- */

  bool Device::open_tx(cryptonote::keypair &txkey) {
    int offset;

    reset_buffer();
    key_map.clear();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_OPEN_TX;
    this->buffer_send[2] = 0x01;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //account
    this->buffer_send[offset+0] = 0x00;
    this->buffer_send[offset+1] = 0x00;
    this->buffer_send[offset+2] = 0x00;
    this->buffer_send[offset+3] = 0x00;
    offset += 4;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(txkey.pub.data, this->buffer_recv, 32); 
    memmove(txkey.sec.data, this->buffer_recv+32, 32);
    return true;
  }

  bool Device::get_sub_tx_public_key(const crypto::public_key &dest_key, crypto::public_key& pub) {
    int offset;
    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_OPEN_TX;
    this->buffer_send[2] = 0x02;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //Dout
    memmove(this->buffer_send+offset, dest_key.data, 32); 
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(pub.data, this->buffer_recv, 32); 
   
    return true;
  }

  bool  Device::get_additional_key(const bool subaddr, cryptonote::keypair &additional_txkey){
    int offset;
    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_GET_ADDITIONAL_KEY;
    this->buffer_send[2] = subaddr;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = subaddr?1:0;
    offset += 1;
    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(additional_txkey.pub.data, this->buffer_recv,    32); 
    memmove(additional_txkey.sec.data, this->buffer_recv+32, 32);
    return true;

  }

  bool  Device::set_signature_mode(unsigned int sig_mode) {
    int offset;
    reset_buffer();
    
    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_SET_SIGNATURE_MODE;
    this->buffer_send[2] = 0x01;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //account
    this->buffer_send[offset] = sig_mode;
    offset += 1;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();
    return true;


  }
  bool Device::stealth(crypto::hash8 &payment_id, 
                      const crypto::public_key &public_key) {
    int offset;

    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_STEALTH;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //dest view pub key
    memmove(&this->buffer_send[offset], public_key.data, 32);
    offset += 32;
      //id
    memmove(&this->buffer_send[offset], payment_id.data, 8);
    offset += 8;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();
    memmove(payment_id.data, &this->buffer_recv[0], 8);

    return true;
  }


  bool Device::add_output_key_mapping(const crypto::public_key &Aout, const crypto::public_key &Bout, size_t real_output_index,  
                               const crypto::secret_key &amount_key,  const crypto::public_key &out_eph_public_key)  {

    key_map.add(ABPkeys(rct::pk2rct(Aout),rct::pk2rct(Bout), real_output_index, rct::pk2rct(out_eph_public_key), rct::sk2rct(amount_key)));
    return true;
  }

 

  bool  Device::blind(rct::ecdhTuple & unmasked, const rct::key AKout){
    int offset;

    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_BLIND;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //value v
    memmove(this->buffer_send+offset, unmasked.amount.bytes, 32);
    offset += 32;
    //mask k
    memmove(this->buffer_send+offset, unmasked.mask.bytes, 32);
    offset += 32;
    // AKout
    memmove(this->buffer_send+offset, AKout.bytes, 32);
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(unmasked.amount.bytes, &this->buffer_recv[0],  32);
    memmove(unmasked.mask.bytes,  &this->buffer_recv[32], 32);
    return true;
  }

  bool  Device::unblind(rct::ecdhTuple & masked, const rct::key AKout){
    int offset;

    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_UNBLIND;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //value v
    memmove(this->buffer_send+offset, masked.amount.bytes, 32);
    offset += 32;
    //mask k
    memmove(this->buffer_send+offset, masked.mask.bytes, 32);
    offset += 32;
    // AKout
    memmove(this->buffer_send+offset, AKout.bytes, 32);
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(masked.amount.bytes, &this->buffer_recv[0],  32);
    memmove(masked.mask.bytes,  &this->buffer_recv[32], 32);
    return true;
  }

  bool Device::mlsag_prehash(const std::string &blob, size_t inputs_size, size_t outputs_size, 
                             const rct::keyV &hashes, const rct::ctkeyV &outPk,
                             rct::key &prehash) {
    unsigned int  data_offset, C_offset, kv_offset, i;
    const char *data;
    data = blob.data();
  
    if (inputs_size) {
      log_message("mlsag_prehash", (std::string("inputs_size not null: ") +  std::to_string(inputs_size)).c_str());
    }

    this->key_map.log();

    // ======  u8 type, varint txnfee ======
    int offset;
    
    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_VALIDATE;
    this->buffer_send[2] = 0x01;
    this->buffer_send[3] = 0x01;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = (inputs_size == 0)?0x00:0x80;
    offset += 1;
    
    //type
    this->buffer_send[offset] = data[0];
    offset += 1;
    
    //txnfee
    data_offset = 1;
    while (data[data_offset]&0x80) {
      this->buffer_send[offset] = data[data_offset];
      offset += 1;
      data_offset += 1;
    }
    this->buffer_send[offset] = data[data_offset];
    offset += 1;
    data_offset += 1;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    //pseudoOuts
    for ( i = 0; i < inputs_size; i++) {
      reset_buffer();
      this->buffer_send[0] = 0x00;
      this->buffer_send[1] = 0x5A;
      this->buffer_send[2] = 0x01;
      this->buffer_send[3] = i+2;
      this->buffer_send[4] = 0x00;
      offset = 5;
      //options
      this->buffer_send[offset] = (i==inputs_size-1)? 0x00:0x80;
      offset += 1;
      //pseudoOut
      memmove(this->buffer_send+offset, data+data_offset,32);
      offset += 32;
      data_offset += 32;

      this->buffer_send[4] = offset-5;
      this->length_send = offset;
      this->exchange();
    }

    // ======  Aout, Bout, AKout, C, v, k ======
    kv_offset = data_offset;
    C_offset = kv_offset+ (32*2)*outputs_size;
    for ( i = 0; i < outputs_size; i++) {
      ABPkeys outKeys;
      bool found;

      found = this->key_map.find(outPk[i].dest, outKeys);
      if (!found) {
        log_hexbuffer("Pout not found", (char*)outPk[i].dest.bytes, 32);
      }
      reset_buffer();
      
      this->buffer_send[0] = 0x00;
      this->buffer_send[1] = INS_VALIDATE;
      this->buffer_send[2] = 0x02;
      this->buffer_send[3] = i+1;
      this->buffer_send[4] = 0x00;
      offset = 5;
      //options
      this->buffer_send[offset] = (i==outputs_size-1)? 0x00:0x80 ;
      offset += 1;
      if (found) {
        //Aout
        memmove(this->buffer_send+offset, outKeys.Aout.bytes, 32);
        offset+=32;
        //Bout
        memmove(this->buffer_send+offset, outKeys.Bout.bytes, 32);
        offset+=32;
        //AKout
        memmove(this->buffer_send+offset, outKeys.AKout.bytes, 32);
        offset+=32;
      } else {
        // dummy: Aout Bout AKout
        offset += 32*3;
      }
      //C
      memmove(this->buffer_send+offset, data+C_offset,32);
      offset += 32;
      C_offset += 32;
      //k
      memmove(this->buffer_send+offset, data+kv_offset,32);
      offset += 32;
      //v
      kv_offset += 32;
      memmove(this->buffer_send+offset, data+kv_offset,32);
      offset += 32;
      kv_offset += 32;

      this->buffer_send[4] = offset-5;
      this->length_send = offset;
      this->exchange();
    }

    // ======   C[], message, proof======
    C_offset = kv_offset;
    for (i = 0; i < outputs_size; i++) {
      reset_buffer();

      this->buffer_send[0] = 0x00;
      this->buffer_send[1] = INS_VALIDATE;
      this->buffer_send[2] = 0x03;
      this->buffer_send[3] = i+1;
      this->buffer_send[4] = 0x00;
      offset = 5;
      //options
      this->buffer_send[offset] = 0x80 ;
      offset += 1;
      //C
      memmove(this->buffer_send+offset, data+C_offset,32);
      offset += 32;
      C_offset += 32;

      this->buffer_send[4] = offset-5;
      this->length_send = offset;
      this->exchange();

    }
    
    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_VALIDATE;
    this->buffer_send[2] = 0x03;
    this->buffer_send[3] = i+1;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] =  0x00;
    offset += 1;
    //message
    memmove(this->buffer_send+offset, hashes[0].bytes,32);
    offset += 32;
    //proof
    memmove(this->buffer_send+offset,  hashes[2].bytes,32);
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();
    
#ifdef DEBUGLEDGER
    memmove(prehash.bytes, this->buffer_recv,  32);
#endif

    return true;
  }


  bool Device::mlsag_prepare(const rct::key &H, const rct::key &xx, 
                            rct::key &a, rct::key &aG, rct::key &aHP, rct::key &II) {


    int offset;

    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_MLSAG;
    this->buffer_send[2] = 0x01;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;
    //value H
    memmove(this->buffer_send+offset, H.bytes, 32);
    offset += 32;
    //mask xin
    memmove(this->buffer_send+offset, xx.bytes, 32);
    offset += 32;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(a.bytes,   &this->buffer_recv[32*0], 32);
    memmove(aG.bytes,  &this->buffer_recv[32*1], 32);
    memmove(aHP.bytes, &this->buffer_recv[32*2], 32);
    memmove(II.bytes,  &this->buffer_recv[32*3], 32);
    return true;
  }

  bool Device::mlsag_prepare(rct::key &a, rct::key &aG) {
    int offset;

    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_MLSAG;
    this->buffer_send[2] = 0x01;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    memmove(a.bytes,   &this->buffer_recv[32*0], 32);
    memmove(aG.bytes,  &this->buffer_recv[32*1], 32);
    return true;
  }

  bool Device::mlsag_hash(const rct::keyV &long_message, rct::key &c) {
    int offset;
    int cnt;
    cnt = long_message.size();
    for (int i = 0; i<cnt; i++) {
      reset_buffer();

      this->buffer_send[0] = 0x00;
      this->buffer_send[1] = INS_MLSAG;
      this->buffer_send[2] = 0x02;
      this->buffer_send[3] = i+1;
      this->buffer_send[4] = 0x00;
      offset = 5;
      //options
      this->buffer_send[offset] = 
          (i==(cnt-1))?0x00:0x80;  //last
      offset += 1;
      //msg part
      memmove(this->buffer_send+offset, long_message[i].bytes, 32);
      offset += 32;

      this->buffer_send[4] = offset-5;
      this->length_send = offset;
      this->exchange();
    }

    memmove(c.bytes, &this->buffer_recv[0], 32);
    return true;

  }

  bool Device::mlsag_sign(rct::keyV &ss, const rct::keyV &xx, const rct::keyV &alpha, int rows) {
    int offset;
    for (int j = 0; j < rows; j++) {
      reset_buffer();

      this->buffer_send[0] = 0x00;
      this->buffer_send[1] = INS_MLSAG;
      this->buffer_send[2] = 0x03;
      this->buffer_send[3] = j+1;
      this->buffer_send[4] = 0x00;
      offset = 5;
      //options
      this->buffer_send[offset] = 0x00;
      if (j==(rows-1)) {
        this->buffer_send[offset]  |= 0x80;  //last
      }
      offset += 1;
      //xx 
      memmove(this->buffer_send+offset, xx[j].bytes, 32);
      offset += 32;
      //alpa 
      memmove(this->buffer_send+offset, alpha[j].bytes, 32);
      offset += 32;

      this->buffer_send[4] = offset-5;
      this->length_send = offset;
      this->exchange();

      //ss
      memmove(ss[j].bytes, &this->buffer_recv[0], 32);
    }
    return true;
  }

  bool Device::close_tx() {
    int offset;

    reset_buffer();

    this->buffer_send[0] = 0x00;
    this->buffer_send[1] = INS_CLOSE_TX;
    this->buffer_send[2] = 0x00;
    this->buffer_send[3] = 0x00;
    this->buffer_send[4] = 0x00;
    offset = 5;
    //options
    this->buffer_send[offset] = 0x00;
    offset += 1;

    this->buffer_send[4] = offset-5;
    this->length_send = offset;
    this->exchange();

    return true;
  }

 
}
