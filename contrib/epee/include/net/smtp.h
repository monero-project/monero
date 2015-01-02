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
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>


namespace epee
{
namespace net_utils
{
	namespace smtp
	{

		using boost::asio::ip::tcp;
		using namespace boost::archive::iterators;
		typedef base64_from_binary<transform_width<const char *,6,8> > base64_text;
	
		/************************************************************************/
		/*                                                                      */
		/************************************************************************/
		class smtp_client
		{
		public:
			smtp_client(std::string pServer,unsigned int pPort,std::string pUser,std::string pPassword):
			  mServer(pServer),mPort(pPort),mUserName(pUser),mPassword(pPassword),mSocket(mIOService),mResolver(mIOService)
			  {
				  tcp::resolver::query qry(mServer,boost::lexical_cast<std::string>( mPort ));
				  mResolver.async_resolve(qry,boost::bind(&smtp_client::handleResolve,this,boost::asio::placeholders::error,
					  boost::asio::placeholders::iterator));
			  }
			  bool Send(std::string pFrom,std::string pTo,std::string pSubject,std::string pMessage)
			  {
				  mHasError = true;
				  mFrom=pFrom;
				  mTo=pTo;
				  mSubject=pSubject;
				  mMessage=pMessage;
				  mIOService.run();
				  return !mHasError;
			  }
		private:
			std::string encodeBase64(std::string pData)
			{
				std::stringstream os;
				size_t sz=pData.size();
				std::copy(base64_text(pData.c_str()),base64_text(pData.c_str()+sz),std::ostream_iterator<char>(os));
				return os.str();
			}
			void handleResolve(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator)
			{
				if(!err)
				{
					tcp::endpoint endpoint=*endpoint_iterator;
					mSocket.async_connect(endpoint,
						boost::bind(&smtp_client::handleConnect,this,boost::asio::placeholders::error,++endpoint_iterator));
				}
				else
				{
					mHasError=true;
					mErrorMsg= err.message();
				}
			}
			void writeLine(std::string pData)
			{
				std::ostream req_strm(&mRequest);
				req_strm << pData << "\r\n";
				boost::asio::write(mSocket,mRequest);
				req_strm.clear();
			}
			void readLine(std::string& pData)
			{
				boost::asio::streambuf response;
				boost::asio::read_until(mSocket, response, "\r\n");
				std::istream response_stream(&response);
				response_stream >> pData;
			}
			void handleConnect(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator)
			{
				if (!err)
				{
					std::string read_buff;
					// The connection was successful. Send the request.
					std::ostream req_strm(&mRequest);
					writeLine("EHLO "+mServer);
					readLine(read_buff);//220
					writeLine("AUTH LOGIN");
					readLine(read_buff);//
					writeLine(encodeBase64(mUserName));
					readLine(read_buff);
					writeLine(encodeBase64(mPassword));
					readLine(read_buff);
					writeLine( "MAIL FROM:<"+mFrom+">");
					writeLine( "RCPT TO:<"+mTo+">");
					writeLine( "DATA");
					writeLine( "SUBJECT:"+mSubject);
					writeLine( "From:"+mFrom);
					writeLine( "To:"+mTo);
					writeLine( "");
					writeLine( mMessage );
					writeLine( "\r\n.\r\n");
					readLine(read_buff);
					if(read_buff == "250")
						mHasError = false;
					writeLine( "QUIT");
				}
				else
				{
					mHasError=true;
					mErrorMsg= err.message();
				}
			}
			std::string mServer;
			std::string mUserName;
			std::string mPassword;
			std::string mFrom;
			std::string mTo;
			std::string mSubject;
			std::string mMessage;
			unsigned int mPort;
			boost::asio::io_service mIOService;
			tcp::resolver mResolver;
			tcp::socket mSocket;
			boost::asio::streambuf mRequest;
			boost::asio::streambuf mResponse;
			bool mHasError;
			std::string mErrorMsg;
		};


		bool send_mail(const std::string& server, int port, const std::string& login, const std::string& pass, const std::string& from_email, /*"STIL CRAWLER",*/
			const std::string& maillist, const std::string& subject, const std::string& body)
		{
			STD_TRY_BEGIN();
			//smtp_client mailc("yoursmtpserver.com",25,"user@yourdomain.com","password");
			//mailc.Send("from@yourdomain.com","to@somewhere.com","subject","Hello from C++ SMTP Client!");
			smtp_client mailc(server,port,login,pass);
			return mailc.Send(from_email,maillist,subject,body);
			STD_TRY_CATCH("at send_mail", false);
		}

	}
}
}

//#include "smtp.inl"
