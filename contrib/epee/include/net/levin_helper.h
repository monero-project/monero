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

#include "levin_base.h"
#include "serializeble_struct_helper.h"

namespace epee
{
namespace levin
{
	template<class t_struct>
	bool pack_struct_to_levin_message(const t_struct& t, std::string& buff, int command_id)
	{
		buff.resize(sizeof(levin::bucket_head));
		levin::bucket_head& head = *(levin::bucket_head*)(&buff[0]);
		head.m_signature = LEVIN_SIGNATURE;
		head.m_cb = 0;
		head.m_have_to_return_data = true;
		head.m_command = command_id;
		head.m_return_code = 1;
		head.m_reservedA = rand(); //probably some flags in future
		head.m_reservedB = rand(); //probably some check summ in future

		std::string buff_strg;
		if(!StorageNamed::save_struct_as_storage_to_buff_t<t_struct, StorageNamed::DefaultStorageType>(t, buff_strg))
			return false;
		
		head.m_cb = buff_strg.size();
		buff.append(buff_strg);
		return true;
	}

	
	bool pack_data_to_levin_message(const std::string& data, std::string& buff, int command_id)
	{
		buff.resize(sizeof(levin::bucket_head));
		levin::bucket_head& head = *(levin::bucket_head*)(&buff[0]);
		head.m_signature = LEVIN_SIGNATURE;
		head.m_cb = 0;
		head.m_have_to_return_data = true;
		head.m_command = command_id;
		head.m_return_code = 1;
		head.m_reservedA = rand(); //probably some flags in future
		head.m_reservedB = rand(); //probably some check summ in future

		head.m_cb = data.size();
		buff.append(data);
		return true;
	}

	bool load_levin_data_from_levin_message(std::string& levin_data, const std::string& buff, int& command)
	{
		if(buff.size() < sizeof(levin::bucket_head) )
		{
			LOG_PRINT_L3("size of buff(" << buff.size() << ") is too small, at load_struct_from_levin_message");
			return false;
		}

		levin::bucket_head& head = *(levin::bucket_head*)(&buff[0]);
		if(head.m_signature != LEVIN_SIGNATURE)
		{
			LOG_PRINT_L3("Failed to read signature in levin message, at load_struct_from_levin_message");
			return false;
		}
		if(head.m_cb != buff.size()-sizeof(levin::bucket_head))
		{
			LOG_PRINT_L3("sizes missmatch, at load_struct_from_levin_message");
			return false;
		}

		//std::string buff_strg;
		levin_data.assign(&buff[sizeof(levin::bucket_head)], buff.size()-sizeof(levin::bucket_head));
		command = head.m_command;
		return true;
	}

	template<class t_struct>
	bool load_struct_from_levin_message(t_struct& t, const std::string& buff, int& command)
	{
		if(buff.size() < sizeof(levin::bucket_head) )
		{
			LOG_ERROR("size of buff(" << buff.size() << ") is too small, at load_struct_from_levin_message");
			return false;
		}
		
		levin::bucket_head& head = *(levin::bucket_head*)(&buff[0]);
		if(head.m_signature != LEVIN_SIGNATURE)
		{
			LOG_ERROR("Failed to read signature in levin message, at load_struct_from_levin_message");
			return false;
		}
		if(head.m_cb != buff.size()-sizeof(levin::bucket_head))
		{
			LOG_ERROR("sizes missmatch, at load_struct_from_levin_message");
			return false;
		}

		std::string buff_strg;
		buff_strg.assign(&buff[sizeof(levin::bucket_head)], buff.size()-sizeof(levin::bucket_head));

		if(!StorageNamed::load_struct_from_storage_buff_t<t_struct, StorageNamed::DefaultStorageType>(t, buff_strg))
		{
			LOG_ERROR("Failed to read storage, at load_struct_from_levin_message");
			return false;
		}
		command = head.m_command;
		return true;
	}
}
}