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
#include <wininet.h>
#include <atlutil.h>
#pragma comment(lib, "Wininet.lib")

namespace epee
{
namespace net_utils
{
	inline 
	bool http_ssl_invoke(const std::string& url, const std::string usr, const std::string psw, std::string& http_response_body, bool use_post = false)
	{
		bool final_res = false;

		ATL::CUrl url_obj;
		BOOL crack_rss = url_obj.CrackUrl(string_encoding::convert_to_t<std::basic_string<TCHAR> >(url).c_str());

		HINTERNET hinet = ::InternetOpenA(SHARED_JOBSCOMMON_HTTP_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if(!hinet)
		{
			int err = ::GetLastError();
			LOG_PRINT("Failed to call InternetOpenA, \nError: " << err << " " << log_space::get_win32_err_descr(err), LOG_LEVEL_0);
			return false;
		}

		DWORD dwFlags = 0;
		DWORD dwBuffLen = sizeof(dwFlags);

		if(usr.size())
		{
			dwFlags |=  INTERNET_FLAG_IGNORE_CERT_CN_INVALID|INTERNET_FLAG_IGNORE_CERT_DATE_INVALID|
				INTERNET_FLAG_PRAGMA_NOCACHE | SECURITY_FLAG_IGNORE_UNKNOWN_CA|INTERNET_FLAG_SECURE; 
		}else
		{
			dwFlags |=  INTERNET_FLAG_PRAGMA_NOCACHE; 
		}


		int port = url_obj.GetPortNumber();
		BOOL res = FALSE;

		HINTERNET hsession = ::InternetConnectA(hinet, string_encoding::convert_to_ansii(url_obj.GetHostName()).c_str(), port/*INTERNET_DEFAULT_HTTPS_PORT*/, usr.c_str(), psw.c_str(), INTERNET_SERVICE_HTTP, dwFlags, NULL);
		if(hsession)
		{
			const std::string uri = string_encoding::convert_to_ansii(url_obj.GetUrlPath()) + string_encoding::convert_to_ansii(url_obj.GetExtraInfo());

			HINTERNET hrequest = ::HttpOpenRequestA(hsession, use_post?"POST":NULL, uri.c_str(), NULL, NULL,NULL, dwFlags, NULL);
			if(hrequest)
			{
				while(true)
				{
					res = ::HttpSendRequestA(hrequest, NULL, 0, NULL, 0);
					if(!res)
					{
						//ERROR_INTERNET_INVALID_CA 45
						//ERROR_INTERNET_INVALID_URL              (INTERNET_ERROR_BASE + 5)
						int err = ::GetLastError();
						LOG_PRINT("Failed to call HttpSendRequestA, \nError: " << log_space::get_win32_err_descr(err), LOG_LEVEL_0);
						break;
					}

					DWORD code = 0;
					DWORD buf_len = sizeof(code);
					DWORD index = 0;
					res = ::HttpQueryInfo(hrequest,  HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_STATUS_CODE, &code, &buf_len, &index);
					if(!res)
					{
						//ERROR_INTERNET_INVALID_CA 45
						//ERROR_INTERNET_INVALID_URL              (INTERNET_ERROR_BASE + 5)
						int err = ::GetLastError();
						LOG_PRINT("Failed to call HttpQueryInfo, \nError: " << log_space::get_win32_err_descr(err), LOG_LEVEL_0);
						break;
					}
					if(code < 200 || code > 299)
					{
						LOG_PRINT("Wrong server response, HttpQueryInfo returned statuse code" << code , LOG_LEVEL_0);
						break;
					}


					char buff[100000] = {0};
					DWORD readed = 0;
					while(true)
					{
						res = ::InternetReadFile(hrequest, buff, sizeof(buff), &readed);
						if(!res)
						{
							int err = ::GetLastError();
							LOG_PRINT("Failed to call InternetReadFile, \nError: " << log_space::get_win32_err_descr(err), LOG_LEVEL_0);
							break;
						}
						if(readed)
						{
							http_response_body.append(buff, readed);
						}
						else
							break;
					}

					if(!res)
						break;


					//we success
					final_res = true;

					res = ::InternetCloseHandle(hrequest);
					if(!res)
					{
						int err = ::GetLastError();
						LOG_PRINT("Failed to call InternetCloseHandle, \nError: " << log_space::get_win32_err_descr(err), LOG_LEVEL_0);
					}

					break;
				}
			}
			else
			{
				//ERROR_INTERNET_INVALID_CA
				int err = ::GetLastError();
				LOG_PRINT("Failed to call InternetOpenUrlA, \nError: " << log_space::get_win32_err_descr(err), LOG_LEVEL_0);
				return false;
			}

			res = ::InternetCloseHandle(hsession);
			if(!res)
			{
				int err = ::GetLastError();
				LOG_PRINT("Failed to call InternetCloseHandle, \nError: " << log_space::get_win32_err_descr(err), LOG_LEVEL_0);
			}
		}else
		{
			int err = ::GetLastError();
			LOG_PRINT("Failed to call InternetConnectA(" << string_encoding::convert_to_ansii(url_obj.GetHostName()) << ", port " << port << " \nError: " << log_space::get_win32_err_descr(err), LOG_LEVEL_0);
		}



		res = ::InternetCloseHandle(hinet);
		if(!res)
		{
			int err = ::GetLastError();
			LOG_PRINT("Failed to call InternetCloseHandle, \nError: " << log_space::get_win32_err_descr(err), LOG_LEVEL_0);
		}
		return final_res;
	}
}
}
