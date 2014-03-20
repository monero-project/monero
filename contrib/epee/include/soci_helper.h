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
#include "soci.h"
#include "soci-postgresql.h"

using namespace epee;
namespace soci
{

		template <>
		struct type_conversion<uint64_t>
		{
			typedef long long base_type;

			static void from_base(base_type a_, indicator ind, uint64_t & mi)
			{
				if (ind == i_null)
				{
					mi = 0;
					//throw soci_error("Null value not allowed for this type");
				}
				mi = (uint64_t)a_;
				//mi.set(i);
			}

			static void to_base(const uint64_t & mi, base_type & i, indicator & ind)
			{
				i = (base_type)mi;
				ind = i_ok;
			}
		};



		template <>
		struct type_conversion<bool>
		{
			typedef int base_type;

			static void from_base(base_type a_, indicator ind, bool& mi)
			{
				if (ind == i_null)
				{
					mi = false;
					//throw soci_error("Null value not allowed for this type");
				}
				mi = a_? true:false;
				//mi.set(i);
			}

			static void to_base(const bool & mi, base_type & i, indicator & ind)
			{
				i = mi? 1:0;
				ind = i_ok;
			}
		};



	class per_thread_session
	{
	public:
		bool init(const std::string& connection_string)
		{
			m_connection_string = connection_string;

			return true;
		}

		soci::session& get()
		{

			//soci::session 

			m_db_connections_lock.lock();
      boost::shared_ptr<soci::session>& conn_ptr = m_db_connections[epee::misc_utils::get_thread_string_id()];
			m_db_connections_lock.unlock();
			if(!conn_ptr.get())
			{
				conn_ptr.reset(new soci::session(soci::postgresql, m_connection_string));
			}
			//init new connection
			return *conn_ptr.get();
		}

    bool reopen()
    {
      //soci::session 

      m_db_connections_lock.lock();
      boost::shared_ptr<soci::session>& conn_ptr = m_db_connections[misc_utils::get_thread_string_id()];
      m_db_connections_lock.unlock();
      if(conn_ptr.get())
      {
        conn_ptr->close();
        conn_ptr.reset(new soci::session(soci::postgresql, m_connection_string));
      }

      //init new connection
      return true;
    }

		//----------------------------------------------------------------------------------------------
		bool check_status()
		{
			return true;
		}

	protected:
	private:
		std::map<std::string, boost::shared_ptr<soci::session> > m_db_connections;
    epee::critical_section m_db_connections_lock;
		std::string m_connection_string;
	};
}
/*}*/