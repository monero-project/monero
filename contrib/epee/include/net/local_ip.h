// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 


#pragma once

#include "int-util.h"

// IP addresses are kept in network byte order
// Masks below are little endian
// -> convert from network byte order to host byte order before comparing

namespace epee
{
  namespace net_utils
  {
    inline
    bool is_ip_local(uint32_t ip)
    {
      ip = SWAP32LE(ip);
      /*
      local ip area
      10.0.0.0 ... 10.255.255.255
      172.16.0.0 ... 172.31.255.255
      192.168.0.0 ... 192.168.255.255
      */
      if( (ip | 0xffffff00) == 0xffffff0a)
        return true;

      if( (ip | 0xffff0000) == 0xffffa8c0)
        return true;

      if( (ip | 0xffffff00) == 0xffffffac)
      {
        uint32_t second_num = (ip >> 8) & 0xff;
        if(second_num >= 16 && second_num <= 31 )
          return true;
      }
      return false;
    }
    inline
    bool is_ip_loopback(uint32_t ip)
    {
      ip = SWAP32LE(ip);
      if( (ip | 0xffffff00) == 0xffffff7f)
        return true;
      //MAKE_IP
      /*
      loopback ip
      127.0.0.0 ... 127.255.255.255
      */
      return false;
    }
   
  }
}

