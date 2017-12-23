
#include <cstddef>
#include <string>

#pragma once
//#define DEBUGLEDGER

namespace ledger {
    namespace {
        bool apdu_verbose =true;
    }
    void set_apdu_verbose(bool verbose);
//#define TRACK printf("file %s:%d\n",__FILE__, __LINE__) 
#define TRACK MCDEBUG("ledger"," At file " << __FILE__ << ":" << __LINE__) 
//#define TRACK while(0);
#ifdef DEBUGLEDGER
    namespace {
        //bool check_verbose =false;
    }
    
    void log_hexbuffer(std::string msg,  const char* buff, int len);
    void log_hexbufferLE(std::string msg,  const char* buff, int len);
    void log_message(std::string msg,  std::string info );
    
    void check32(std::string msg, std::string info, const char *h, const char *d, bool crypted=false);
    void check8(std::string msg, std::string info, const char *h, const char *d,  bool crypted=false);

    void decrypt(char* buf, int len) ;
    void set_check_verbose(bool verbose);

#endif
}
