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

#if defined(HAVE_HIDAPI) 

#include <hidapi/hidapi.h>
#include "device_io.hpp"

#pragma once

namespace hw {
  namespace io {



    /** HID class base. Commands are formated as follow:
     *
     * |----------------------------------------------------------|
     * |  2 bytes  |  1 byte  |  2 bytes  | 2 bytes  |  len bytes |
     * |-----------|----------|-----------|----------|------------|
     * |  channel  |    tag   |  sequence |   len    |  payload   |
     * |----------------------------------------------------------|
     */


    struct hid_conn_params {
      unsigned int vid; 
      unsigned int pid; 
      int interface_number;
      unsigned short usage_page;
      bool interface_OR_page ;
    };
    

    class device_io_hid: device_io {
      

    private:
     

      unsigned short channel;
      unsigned char  tag;
      unsigned int   packet_size; 
      unsigned int   timeout;

      unsigned int   usb_vid;
      unsigned int   usb_pid;
      hid_device     *usb_device;

      void io_hid_log(int read, unsigned char* buf, int buf_len);
      void io_hid_init();
      void io_hid_exit() ;
      void io_hid_open(int vid, int pid,  int mode);
      void io_hid_close (void);

      unsigned int wrapCommand(const unsigned char *command, size_t command_len, unsigned char *out, size_t out_len);
      unsigned int unwrapReponse(const unsigned char *data, size_t data_len, unsigned char *out, size_t out_len);
 
 
    public:
      bool hid_verbose = false;

      static const unsigned int  OR_SELECT = 1;
      static const unsigned int  AND_SELECT = 2;

      static const unsigned short DEFAULT_CHANNEL     = 0x0001;
      static const unsigned char  DEFAULT_TAG         = 0x01;
      static const unsigned int   DEFAULT_PACKET_SIZE = 64;
      static const unsigned int   DEFAULT_TIMEOUT     = 120000;

      device_io_hid(unsigned short channel, unsigned char tag, unsigned int packet_zize, unsigned int timeout);
      device_io_hid();
      ~device_io_hid() {};

      void init();  
      void connect(void *params);
      void connect(unsigned int vid, unsigned  int pid, int interface_number, unsigned short usage_page, bool interface_OR_page );
      bool connected() const;
      int  exchange(unsigned char *command, unsigned int cmd_len, unsigned char *response, unsigned int max_resp_len);
      void disconnect();
      void release();
    };
  };
};

#endif //#if defined(HAVE_HIDAPI) 
