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





#ifndef _LEVIN_CLIENT_H_
#define _LEVIN_CLIENT_H_

#include "net_helper.h"
#include "levin_base.h"


#ifndef MAKE_IP
#define MAKE_IP( a1, a2, a3, a4 )	(a1|(a2<<8)|(a3<<16)|(a4<<24))
#endif

namespace epee
{
namespace levin
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
	class levin_client_impl
	{
	public:
		levin_client_impl();
		virtual ~levin_client_impl();

		bool connect(u_long ip, int port, unsigned int timeout, const std::string& bind_ip = "0.0.0.0");
    bool connect(const std::string& addr, int port, unsigned int timeout, const std::string& bind_ip = "0.0.0.0");
		bool is_connected();
		bool disconnect();

		virtual int invoke(int command, const std::string& in_buff, std::string& buff_out);
		virtual int notify(int command, const std::string& in_buff);

	protected: 
		net_utils::blocked_mode_client m_transport;
	};


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class levin_client_impl2: public levin_client_impl
  {
  public:

    int invoke(int command, const std::string& in_buff, std::string& buff_out);
    int notify(int command, const std::string& in_buff);
  };

}
namespace net_utils
{
  typedef levin::levin_client_impl levin_client;
  typedef levin::levin_client_impl2 levin_client2;
}
}

#include "levin_client.inl"

#endif //_LEVIN_CLIENT_H_
