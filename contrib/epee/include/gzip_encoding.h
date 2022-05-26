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




#ifndef _GZIP_ENCODING_H_
#define _GZIP_ENCODING_H_
#include "net/http_client_base.h"
#include "zlib/zlib.h"
//#include "http.h"


namespace epee
{
namespace net_utils
{



	class content_encoding_gzip: public i_sub_handler
	{
	public:
		/*! \brief
		*  Function content_encoding_gzip : Constructor
		*
		*/
		inline 
		content_encoding_gzip(i_target_handler* powner_filter, bool is_deflate_mode = false):m_powner_filter(powner_filter), 
			m_is_stream_ended(false), 
			m_is_deflate_mode(is_deflate_mode),
			m_is_first_update_in(true)
		{
			memset(&m_zstream_in, 0, sizeof(m_zstream_in));
			memset(&m_zstream_out, 0, sizeof(m_zstream_out));
			int ret = 0;
			if(is_deflate_mode)
			{
				ret = inflateInit(&m_zstream_in);	
				ret = deflateInit(&m_zstream_out, Z_DEFAULT_COMPRESSION);
			}else
			{
				ret = inflateInit2(&m_zstream_in, 0x1F);
				ret = deflateInit2(&m_zstream_out, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 0x1F, 8, Z_DEFAULT_STRATEGY);
			}	
		}
		/*! \brief
		*  Function content_encoding_gzip : Destructor
		*
		*/
		inline 
		~content_encoding_gzip()
		{
			inflateEnd(& m_zstream_in );
			deflateEnd(& m_zstream_out );
		}
		/*! \brief
		*  Function update_in : Entry point for income data
		*
		*/
		inline 
		virtual bool update_in( std::string& piece_of_transfer)
		{	

			bool is_first_time_here = m_is_first_update_in;
			m_is_first_update_in = false;

			if(m_pre_decode.size())
				m_pre_decode += piece_of_transfer;
			else
				m_pre_decode.swap(piece_of_transfer);
			piece_of_transfer.clear();

			std::string decode_summary_buff;

			size_t	ungzip_size = m_pre_decode.size() * 0x30;
			std::string current_decode_buff(ungzip_size, 'X');

			//Here the cycle is introduced where we unpack the buffer, the cycle is required
			//because of the case where if after unpacking the data will exceed the awaited size, we will not halt with error
			bool continue_unpacking = true;
			bool first_step = true;
			while(m_pre_decode.size() && continue_unpacking)
			{

				//fill buffers
				m_zstream_in.next_in = (Bytef*)m_pre_decode.data();
				m_zstream_in.avail_in = (uInt)m_pre_decode.size();
				m_zstream_in.next_out = (Bytef*)current_decode_buff.data();
				m_zstream_in.avail_out = (uInt)ungzip_size;

				int flag = Z_SYNC_FLUSH;
				int ret = inflate(&m_zstream_in, flag);
				CHECK_AND_ASSERT_MES(ret>=0 || m_zstream_in.avail_out ||m_is_deflate_mode, false, "content_encoding_gzip::update_in() Failed to inflate. err = " << ret);

				if(Z_STREAM_END == ret)
					m_is_stream_ended = true;
				else if(Z_DATA_ERROR == ret && is_first_time_here && m_is_deflate_mode&& first_step)
				{
					// some servers (notably Apache with mod_deflate) don't generate zlib headers
					// insert a dummy header and try again
					static char dummy_head[2] =
					{
						0x8 + 0x7 * 0x10,
						(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
					};
					inflateReset(&m_zstream_in);
					m_zstream_in.next_in = (Bytef*) dummy_head;
					m_zstream_in.avail_in = sizeof(dummy_head);

					ret = inflate(&m_zstream_in, Z_NO_FLUSH);
					if (ret != Z_OK)
					{
						LOCAL_ASSERT(0);
						m_pre_decode.swap(piece_of_transfer);
						return false;
					}
					m_zstream_in.next_in = (Bytef*)m_pre_decode.data();
					m_zstream_in.avail_in = (uInt)m_pre_decode.size();

					ret = inflate(&m_zstream_in, Z_NO_FLUSH);
					if (ret != Z_OK)
					{
						LOCAL_ASSERT(0);
						m_pre_decode.swap(piece_of_transfer);
						return false;
					}
				}


				//leave only unpacked part in the output buffer to start with it the next time
				m_pre_decode.erase(0, m_pre_decode.size()-m_zstream_in.avail_in);
				//if decoder gave nothing to return, then everything is ahead, now simply break
				if(ungzip_size == m_zstream_in.avail_out)
					break;

				//decode_buff currently stores data parts that were unpacked, fix this size
				current_decode_buff.resize(ungzip_size - m_zstream_in.avail_out);
				if(decode_summary_buff.size())
					decode_summary_buff += current_decode_buff;
				else
					current_decode_buff.swap(decode_summary_buff);

				current_decode_buff.resize(ungzip_size);
				first_step = false;
			}

			//Process these data if required
			bool res = true;

			res = m_powner_filter->handle_target_data(decode_summary_buff);

			return true;

		}
		/*! \brief
		*  Function stop : Entry point for stop signal and flushing cached data buffer.
		*
		*/
		inline 
		virtual void stop(std::string& OUT collect_remains)
		{
		}
	protected:
	private:
		/*! \brief
		*	Pointer to parent HTTP-parser
		*/
		i_target_handler* m_powner_filter;
		/*! \brief
		*	ZLIB object for income stream
		*/
		z_stream    m_zstream_in;
		/*! \brief
		*	ZLIB object for outcome stream
		*/
		z_stream    m_zstream_out;
		/*! \brief
		*	Data that could not be unpacked immediately, left to wait for the next packet of data
		*/
		std::string m_pre_decode;
		/*! \brief
		*	The data are accumulated for a package in the buffer to send the web client
		*/
		std::string m_pre_encode;
		/*! \brief
		*	Signals that stream looks like ended
		*/
		bool		m_is_stream_ended;
		/*! \brief
		*	If this flag is set, income data is in HTTP-deflate mode
		*/
		bool		m_is_deflate_mode;
		/*! \brief
		*	Marks that it is a first data packet 
		*/
		bool		m_is_first_update_in;
	};
}
}



#endif //_GZIP_ENCODING_H_
