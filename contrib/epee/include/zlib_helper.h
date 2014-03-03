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
extern "C" { 
#include "zlib/zlib.h"
}
#pragma comment(lib, "zlibstat.lib")

namespace epee 
{
namespace zlib_helper
{
	inline 
	bool pack(std::string& target){
		std::string result_packed_buff;

		z_stream    zstream = {0};
		int ret = deflateInit(&zstream, Z_DEFAULT_COMPRESSION);
		if(target.size())
		{

			
			result_packed_buff.resize(target.size()*2, 'X');

			zstream.next_in = (Bytef*)target.data();
			zstream.avail_in = (uInt)target.size();
			zstream.next_out = (Bytef*)result_packed_buff.data();
			zstream.avail_out = (uInt)result_packed_buff.size();

			ret = deflate(&zstream, Z_FINISH);
			CHECK_AND_ASSERT_MES(ret>=0, false, "Failed to deflate. err = " << ret);

			if(result_packed_buff.size() != zstream.avail_out)
				result_packed_buff.resize(result_packed_buff.size()-zstream.avail_out);

			
			result_packed_buff.erase(0, 2);
			target.swap(result_packed_buff);
		}

		deflateEnd(& zstream );
		return true;
	}

	inline bool unpack(std::string& target)
	{
		z_stream    zstream = {0};
		int ret = inflateInit(&zstream);//

		std::string decode_summary_buff;
		size_t	ungzip_buff_size = target.size() * 0x30;
		std::string current_decode_buff(ungzip_buff_size, 'X');

		while(target.size())
		{

			
			zstream.next_out = (Bytef*)current_decode_buff.data();
			zstream.avail_out = (uInt)ungzip_buff_size;

			int flag = Z_SYNC_FLUSH;
			
			static char dummy_head[2] =
			{
				0x8 + 0x7 * 0x10,
				(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
			};
			zstream.next_in = (Bytef*) dummy_head;
			zstream.avail_in = sizeof(dummy_head);
			ret = inflate(&zstream, Z_NO_FLUSH);
			if (ret != Z_OK)
			{
				LOCAL_ASSERT(0);
				return false;
			}
			
			zstream.next_in = (Bytef*)target.data();
			zstream.avail_in = (uInt)target.size();

			ret = inflate(&zstream, Z_SYNC_FLUSH);
			if (ret != Z_OK && ret != Z_STREAM_END)
			{
				LOCAL_ASSERT(0);
				return false;
			}

			
			target.erase(0, target.size()-zstream.avail_in);

			
			if(ungzip_buff_size == zstream.avail_out)
			{
				LOG_ERROR("Can't unpack buffer");
				return false;
			}

			
			current_decode_buff.resize(ungzip_buff_size - zstream.avail_out);
			if(decode_summary_buff.size())
				decode_summary_buff += current_decode_buff;
			else
				current_decode_buff.swap(decode_summary_buff);

			current_decode_buff.resize(ungzip_buff_size);
		}

		inflateEnd(&zstream );

		decode_summary_buff.swap(target);
		return 1;
	}

};
}//namespace epee
