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


#ifndef _DB_ADO_HELPER_H_
#define _DB_ADO_HELPER_H_

#include <vector>
#include <comutil.h>
#include "string_coding.h"
#include "math_helper.h"
#include "file_io_utils.h"
#include "global_stream_operators.h"



#define BEGIN_TRY_SECTION()  try {

#define CATCH_TRY_SECTION(ret_val)   CATCH_TRY_SECTION_MESS(ret_val, "")

#define CATCH_TRY_SECTION_MESS(ret_val, mess_where)  }\
	catch(const std::exception&ex)\
	{\
		LOG_PRINT_J("DB_ERROR: " << ex.what(), LOG_LEVEL_0);\
		return ret_val;\
	}\
	catch(const _com_error& comm_err)\
	{\
		const TCHAR* pstr = comm_err.Description();\
		std::string  descr = string_encoding::convert_to_ansii(pstr?pstr:TEXT(""));\
		const TCHAR* pmessage = comm_err.ErrorMessage();\
		pstr = comm_err.Source();\
		std::string source = string_encoding::convert_to_ansii(pstr?pstr:TEXT(""));\
		LOG_PRINT_J("COM_ERROR " << mess_where << ":\n\tDescriprion:" << descr << ", \n\t Message: " << string_encoding::convert_to_ansii(pmessage) << "\n\t Source: " << source, LOG_LEVEL_0);\
		return ret_val;\
	}\
	catch(...)\
	{\
		LOG_PRINT_J("..._ERROR: Unknown error.", LOG_LEVEL_0);\
		return ret_val;\
	}\

namespace epee
{
namespace ado_db_helper
{

	struct profile_entry
	{
		profile_entry():m_call_count(0), m_max_time(0), m_min_time(0)
		{}
		//std::string m_sql;
		math_helper::average<DWORD, 10> m_avrg;
		size_t m_call_count;
		DWORD m_max_time;
		DWORD m_min_time;
	};

	class profiler_manager
	{
	public: 
		typedef std::map<std::string, profile_entry> sqls_map;
		profiler_manager(){}

		static  bool sort_by_timing(const sqls_map::iterator& a, const sqls_map::iterator& b)
		{
			return a->second.m_avrg.get_avg() > b->second.m_avrg.get_avg();
		}

		bool flush_log(const std::string& path)
		{
			CRITICAL_REGION_BEGIN(m_sqls_lock);
			std::stringstream strm;
			strm << "SQL PROFILE:\r\nStatements: " << m_sqls.size() << "\r\n";
			std::list<sqls_map::iterator> m_sorted_by_time_sqls;
			for(std::map<std::string, profile_entry>::iterator it = m_sqls.begin();it!=m_sqls.end();it++)
				m_sorted_by_time_sqls.push_back(it);

			m_sorted_by_time_sqls.sort(sort_by_timing);

			for(std::list<sqls_map::iterator>::iterator it = m_sorted_by_time_sqls.begin();it!=m_sorted_by_time_sqls.end();it++)
			{
				strm << "---------------------------------------------------------------------------------------------------------\r\nSQL: " << (*it)->first << "\r\n";
				strm << "\tavrg: " << (*it)->second.m_avrg.get_avg() << "\r\n\tmax: " << (*it)->second.m_max_time << "\r\n\tmin: " << (*it)->second.m_min_time << "\r\n\tcount: " << (*it)->second.m_call_count << "\r\n"; 
			}

			return file_io_utils::save_string_to_file(path.c_str(), strm.str());
			CRITICAL_REGION_END();
		}

		bool push_entry(const std::string sql, DWORD time)
		{
			CRITICAL_REGION_BEGIN(m_sqls_lock);
			profile_entry& entry_ref = m_sqls[sql];
			entry_ref.m_avrg.push(time);
			entry_ref.m_call_count++;
			if(time > entry_ref.m_max_time) entry_ref.m_max_time = time;
			if(time < entry_ref.m_min_time || entry_ref.m_min_time == 0) entry_ref.m_min_time = time;
			CRITICAL_REGION_END();
			return true;
		}

		bool get_entry_avarege(const std::string sql, DWORD& time)
		{
			CRITICAL_REGION_BEGIN(m_sqls_lock);
			sqls_map::iterator it = m_sqls.find(sql);
			if(it==m_sqls.end())
				return false;

			time = static_cast<DWORD>(it->second.m_avrg.get_avg());
			CRITICAL_REGION_END();
			return true;
		}

	private:

		sqls_map m_sqls;
		critical_section m_sqls_lock;
	};
inline 
	profiler_manager* get_set_profiler(bool need_to_set = false, profiler_manager** pprofiler = NULL)
	{
		static profiler_manager* pmanager = NULL;
		if(need_to_set)
			pmanager = *pprofiler;
		//else
		//	*pprofiler = pmanager;
		
		return pmanager;
	}
inline
	bool init() // INIT and DEINIT are NOT THREAD SAFE operations, CALL it BEFOR u start using this wrapper.
	{
		profiler_manager* pmanager = new profiler_manager();
		get_set_profiler(true, &pmanager);
		return true;
	}
inline		
	bool deinit()
	{
		profiler_manager* pmanager = get_set_profiler();
		//get_set_profiler(false, &pmanager);
		if(pmanager)
			delete pmanager;
		return true;
	}
	inline bool push_timing(const std::string sql, DWORD time)
	{
		profiler_manager* pmanager = get_set_profiler();
		//get_set_profiler(false, &pmanager);
		if(pmanager)
			return pmanager->push_entry(sql, time);
		return true;
	}

	inline bool flush_profiler(const std::string path)
	{
		profiler_manager* pmanager = get_set_profiler();
		//get_set_profiler(false, &pmanager);
		if(pmanager)
			return pmanager->flush_log(path);
		return true;
	}

	class timing_guard
	{
		DWORD m_start_time;
		std::string m_sql;
	
	public:
		timing_guard(const std::string& sql)
		{
			m_start_time = ::GetTickCount();
			m_sql = sql;
		}

		~timing_guard()
		{
			DWORD timing = ::GetTickCount() - m_start_time;
			push_timing(m_sql, timing);
		}
	};
#define PROFILE_SQL(sql) timing_guard local_timing(sql)


	typedef std::vector<std::vector<_variant_t> > table;

	inline bool add_parametr(ADODB::_CommandPtr cmd, const std::string& parametr)
	{
		_variant_t param(parametr.c_str());
		ADODB::ADO_LONGPTR size = sizeof(parametr);
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("", ADODB::adVarChar, ADODB::adParamInput, static_cast<long>(parametr.size()+1), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}

	inline bool add_parametr(ADODB::_CommandPtr cmd, const std::wstring& parametr)
	{
		_variant_t param(parametr.c_str());
		ADODB::ADO_LONGPTR size = sizeof(parametr);
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("", ADODB::adVarWChar, ADODB::adParamInput, static_cast<long>(parametr.size()+2), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}

	inline bool add_parametr(ADODB::_CommandPtr cmd, const __int64 parametr)
	{
		_variant_t param(parametr);
		ADODB::ADO_LONGPTR size = static_cast<long>(sizeof(parametr));
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adBigInt, ADODB::adParamInput,  static_cast<long>(size), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}

	inline bool add_parametr(ADODB::_CommandPtr cmd, const unsigned __int64 parametr)
	{
		_variant_t param(parametr);
		ADODB::ADO_LONGPTR size = static_cast<long>(sizeof(parametr));
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adUnsignedBigInt, ADODB::adParamInput,  static_cast<long>(size), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}


	inline bool add_parametr(ADODB::_CommandPtr cmd, const int parametr)
	{
		_variant_t param(parametr);
		ADODB::ADO_LONGPTR size = static_cast<long>(sizeof(parametr));
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adInteger, ADODB::adParamInput,  static_cast<long>(size), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}

	inline bool add_parametr(ADODB::_CommandPtr cmd, const unsigned int parametr)
	{
		_variant_t param(parametr);
		ADODB::ADO_LONGPTR size = static_cast<long>(sizeof(parametr));
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adUnsignedInt, ADODB::adParamInput,  static_cast<long>(size), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}

	inline bool add_parametr(ADODB::_CommandPtr cmd, float parametr)
	{
		_variant_t param;
		param.ChangeType(VT_R4);
		param.fltVal = parametr;
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adSingle, ADODB::adParamInput, static_cast<long>(sizeof(float)), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}

	inline bool add_parametr(ADODB::_CommandPtr cmd, bool parametr)
	{
		_variant_t param;
		param = parametr;
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adBoolean, ADODB::adParamInput, sizeof(parametr), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}


	inline bool add_parametr(ADODB::_CommandPtr cmd, _variant_t parametr)
	{
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adDBTimeStamp, ADODB::adParamInput, sizeof(parametr), parametr);
		cmd->Parameters->Append(param_obj);
		return true;
	}


	inline bool add_parametr_as_double(ADODB::_CommandPtr cmd, const DATE parametr)
	{
		_variant_t param;
		param.ChangeType(VT_R8);
		param.dblVal = parametr;
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adDouble, ADODB::adParamInput, sizeof(float), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}

	template<typename TParam>
	inline bool add_parametr(ADODB::_CommandPtr cmd, const std::list<TParam> params)
	{
		for(std::list<TParam>::const_iterator it = params.begin(); it!=params.end(); it++)
			if(!add_parametr(cmd, *it))
				return false;
		return true;
	}
	
	/*
	inline bool add_parametr(ADODB::_CommandPtr cmd, const size_t parametr)
	{
		_variant_t param;
		param.ChangeType(VT_I4);
		param.intVal = parametr;
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adInteger, ADODB::adParamInput, sizeof(parametr), param);
		cmd->Parameters->Append(param_obj);
		return true;
	}*/

	
	inline bool add_parametr(ADODB::_CommandPtr cmd, const DATE parametr)
	{
		/*_variant_t param;
		param.ChangeType(VT_R8);
		param.dblVal = parametr;
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adDouble, ADODB::adParamInput, sizeof(float), param);
		cmd->Parameters->Append(param_obj);*/
		
		_variant_t param;
		param.ChangeType(VT_DATE);
		param.date = parametr;
		ADODB::_ParameterPtr param_obj = cmd->CreateParameter("parametr", ADODB::adDBDate, ADODB::adParamInput, sizeof(parametr), param);
		cmd->Parameters->Append(param_obj);
		
		return true;
	}

	
	inline bool execute_helper(ADODB::_CommandPtr cmd, _variant_t* pcount_processed = NULL)
	{
		//BEGIN_TRY_SECTION();

		cmd->Execute(pcount_processed, NULL, ADODB::adExecuteNoRecords);


		//CATCH_TRY_SECTION(false);

		return true;
	}


	inline bool select_helper(ADODB::_CommandPtr cmd, table& result_vector)
	{
		result_vector.clear();
		//BEGIN_TRY_SECTION();

		ADODB::_RecordsetPtr precordset = cmd->Execute(NULL, NULL, NULL);
		if(!precordset)
		{
			LOG_ERROR("DB_ERROR: cmd->Execute returned NULL!!!");
			return false;
		}

		//if(precordset->EndOfFile == EOF)
		//{
		//	return true;
		//}
		/*try
		{
			if(precordset->MoveFirst()!= S_OK)
			{
				LOG_ERROR("DB_ERROR: Filed to move first!!!");
				return false;
			}
		}
		catch (...)
		{
			return true;
		}*/

		size_t current_record_index = 0;
		while(precordset->EndOfFile != EOF)
		{
			result_vector.push_back(table::value_type());
			size_t fields_count = precordset->Fields->Count;
			result_vector[current_record_index].resize(fields_count);
			for(size_t current_field_index = 0; current_field_index < fields_count; current_field_index++)
			{
				_variant_t var;
				var.ChangeType(VT_I2);
				var.intVal = static_cast<INT>(current_field_index);
				result_vector[current_record_index][current_field_index] =  precordset->Fields->GetItem(var)->Value;
			}
			precordset->MoveNext();
			current_record_index++;
		}
		//CATCH_TRY_SECTION(false);
		return true;
	}


	template<typename TParam1>
	struct adapter_zero
	{

	};

	template<typename TParam1>
	struct adapter_single
	{
		TParam1 tparam1;
	};
	template<typename TParam1, typename TParam2>
	struct adapter_double
	{
		TParam1 tparam1;
		TParam2 tparam2;
	};


	template<typename TParam1, typename TParam2, typename TParam3>
	struct adapter_triple
	{
		TParam1 tparam1;
		TParam2 tparam2;
		TParam3 tparam3;
	};

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4>
	struct adapter_quad
	{
		TParam1 tparam1;
		TParam2 tparam2;
		TParam3 tparam3;
		TParam4 tparam4;
	};
	
	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5>
	struct adapter_quanto
	{
		TParam1 tparam1;
		TParam2 tparam2;
		TParam3 tparam3;
		TParam4 tparam4;
		TParam5 tparam5;
	};

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6>
	struct adapter_sixto
	{
		TParam1 tparam1;
		TParam2 tparam2;
		TParam3 tparam3;
		TParam4 tparam4;
		TParam5 tparam5;
		TParam6 tparam6;
	};

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7>
	struct adapter_sevento
	{
		TParam1 tparam1;
		TParam2 tparam2;
		TParam3 tparam3;
		TParam4 tparam4;
		TParam5 tparam5;
		TParam6 tparam6;
		TParam7 tparam7;
	};

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, typename TParam8, typename TParam9>
	struct adapter_nine
	{
		TParam1 tparam1;
		TParam2 tparam2;
		TParam3 tparam3;
		TParam4 tparam4;
		TParam5 tparam5;
		TParam6 tparam6;
		TParam7 tparam7;
		TParam8 tparam8;
		TParam9 tparam9;
	};

	template<typename TParam1>
	bool add_parametrs_multi(ADODB::_CommandPtr cmd, const adapter_zero<TParam1>& params)
	{
		return true;
	}

	template<typename TParam1>
	bool add_parametrs_multi(ADODB::_CommandPtr cmd, const adapter_single<TParam1>& params)
	{
		return add_parametr(cmd, params.tparam1);
	}

	template<typename TParam1, typename TParam2>
	bool add_parametrs_multi(ADODB::_CommandPtr cmd, const adapter_double<TParam1, TParam2>& params)
	{
		if(!add_parametr(cmd, params.tparam1)) return false;
		return add_parametr(cmd, params.tparam2);
	}

	template<typename TParam1, typename TParam2, typename TParam3>
	bool add_parametrs_multi(ADODB::_CommandPtr cmd, const adapter_triple<TParam1, TParam2, TParam3>& params)
	{
		if(!add_parametr(cmd, params.tparam1)) return false;
		if(!add_parametr(cmd, params.tparam2)) return false;
		return add_parametr(cmd, params.tparam3);
	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4>
	bool add_parametrs_multi(ADODB::_CommandPtr cmd, const adapter_quad<TParam1, TParam2, TParam3, TParam4>& params)
	{
		if(!add_parametr(cmd, params.tparam1)) return false;
		if(!add_parametr(cmd, params.tparam2)) return false;
		if(!add_parametr(cmd, params.tparam3)) return false;
		return add_parametr(cmd, params.tparam4);
	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5>
	bool add_parametrs_multi(ADODB::_CommandPtr cmd, const adapter_quanto<TParam1, TParam2, TParam3, TParam4, TParam5>& params)
	{
		if(!add_parametr(cmd, params.tparam1)) return false;
		if(!add_parametr(cmd, params.tparam2)) return false;
		if(!add_parametr(cmd, params.tparam3)) return false;
		if(!add_parametr(cmd, params.tparam4)) return false;
		return add_parametr(cmd, params.tparam5);
	}
	
	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6>
	bool add_parametrs_multi(ADODB::_CommandPtr cmd, const adapter_sixto<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6>& params)
	{
		if(!add_parametr(cmd, params.tparam1)) return false;
		if(!add_parametr(cmd, params.tparam2)) return false;
		if(!add_parametr(cmd, params.tparam3)) return false;
		if(!add_parametr(cmd, params.tparam4)) return false;
		if(!add_parametr(cmd, params.tparam5)) return false;
		return add_parametr(cmd, params.tparam6);
	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7>
	bool add_parametrs_multi(ADODB::_CommandPtr cmd, const adapter_sevento<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7>& params)
	{
		if(!add_parametr(cmd, params.tparam1)) return false;
		if(!add_parametr(cmd, params.tparam2)) return false;
		if(!add_parametr(cmd, params.tparam3)) return false;
		if(!add_parametr(cmd, params.tparam4)) return false;
		if(!add_parametr(cmd, params.tparam5)) return false;
		if(!add_parametr(cmd, params.tparam6)) return false;
		return add_parametr(cmd, params.tparam7);
	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, typename TParam8, typename TParam9>
	bool add_parametrs_multi(ADODB::_CommandPtr cmd, const adapter_nine<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8, TParam9>& params)
	{
		if(!add_parametr(cmd, params.tparam1)) return false;
		if(!add_parametr(cmd, params.tparam2)) return false;
		if(!add_parametr(cmd, params.tparam3)) return false;
		if(!add_parametr(cmd, params.tparam4)) return false;
		if(!add_parametr(cmd, params.tparam5)) return false;
		if(!add_parametr(cmd, params.tparam6)) return false;
		if(!add_parametr(cmd, params.tparam7)) return false;
		if(!add_parametr(cmd, params.tparam8)) return false;
		return add_parametr(cmd, params.tparam9);
	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7>
	std::string print_parameters_multi(const adapter_sevento<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7>& params)
	{
		std::stringstream strm;
		strm << params.tparam1 << ", " << params.tparam2 << ", " << params.tparam3 << ", " << params.tparam4 << ", " << params.tparam5 << ", " << params.tparam6 << ", " << params.tparam7;
		return strm.str();
	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, typename TParam8, typename TParam9>
	std::string print_parameters_multi(const adapter_nine<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8, TParam9>& params)
	{
		std::stringstream strm;
		strm << params.tparam1 << ", " << params.tparam2 << ", " << params.tparam3 << ", " << params.tparam4 << ", " << params.tparam5 << ", " << params.tparam6 << ", " << params.tparam7 << ", " << params.tparam8 << ", " << params.tparam9;
		return strm.str();
	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6>
	std::string print_parameters_multi(const adapter_sixto<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6>& params)
	{
		std::stringstream strm;
		strm << params.tparam1 << ", " << params.tparam2 << ", " << params.tparam3 << ", " << params.tparam4 << ", " << params.tparam5 << ", " << params.tparam6;
		return strm.str();
	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5>
	std::string print_parameters_multi(const adapter_quanto<TParam1, TParam2, TParam3, TParam4, TParam5>& params)
	{
		std::stringstream strm;
		strm << params.tparam1 << ", " << params.tparam2 << ", " << params.tparam3 << ", " << params.tparam4 << ", " << params.tparam5;
		return strm.str();
	}


	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4>
	std::string print_parameters_multi(const adapter_quad<TParam1, TParam2, TParam3, TParam4>& params)
	{
		std::stringstream strm;
		strm << params.tparam1 << ", " << params.tparam2 << ", " << params.tparam3 << ", " << params.tparam4;
		return strm.str();
	}

	template<typename TParam1, typename TParam2, typename TParam3>
	std::string print_parameters_multi(const adapter_triple<TParam1, TParam2, TParam3>& params)
	{
		std::stringstream strm;
		strm << params.tparam1 << ", " << params.tparam2 << ", " << params.tparam3;
		return strm.str();
	}

	template<typename TParam>
	std::string get_str_param(const TParam& prm)
	{
		std::stringstream strm;
		strm << prm;
		return strm.str();
	}

	template<typename TParam>
	std::string get_str_param(const std::list<TParam>& prm_lst)
	{
		std::stringstream strm;
		for(std::list<TParam>::const_iterator it = prm_lst.begin();it!=prm_lst.end();it++)
			strm  << get_str_param(*it) << ", ";
		return strm.str();
	}


	template<typename TParam1, typename TParam2>
	std::string print_parameters_multi(const adapter_double<TParam1, TParam2>& params)
	{
		std::stringstream strm;
		strm << get_str_param(params.tparam1) << ", " << get_str_param(params.tparam2);
		return strm.str();
	}

	template<typename TParam1>
	std::string print_parameters_multi(const adapter_single<TParam1>& params)
	{
		std::stringstream strm;
		strm << get_str_param(params.tparam1);
		return strm.str();
	}

	template<typename TParam1>
	std::string print_parameters_multi(const adapter_zero<TParam1>& params)
	{
		std::stringstream strm;
		strm << "(no parametrs)";
		return strm.str();
	}


	template<typename TParams>
	bool execute_helper_multiparam(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParams& parametrs, _variant_t* pcount_processed = NULL)
	{
		PROFILE_SQL(sql_statment);
		bool res = false;
		BEGIN_TRY_SECTION();
		
			ADODB::_CommandPtr cmd;
			cmd.CreateInstance(__uuidof(ADODB::Command));
			cmd->CommandText = _bstr_t(sql_statment.c_str());	

			if(!add_parametrs_multi(cmd, parametrs))
				return false;

			cmd->ActiveConnection = pconnection;
			res = execute_helper(cmd, pcount_processed);	
		
		CATCH_TRY_SECTION_MESS(false, "while statment: " << sql_statment << " [params]: " << print_parameters_multi(parametrs));
		return res;
	}


	template<typename TParams>
	inline
		bool select_helper_multiparam(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParams& parametrs, table& result_vector)
	{
		PROFILE_SQL(sql_statment);
		bool res = false;
		BEGIN_TRY_SECTION();
			ADODB::_CommandPtr cmd;
			cmd.CreateInstance(__uuidof(ADODB::Command));
			cmd->CommandText = _bstr_t(sql_statment.c_str());	


			if(!add_parametrs_multi(cmd, parametrs))
				return false;

			cmd->ActiveConnection = pconnection;
			res = select_helper(cmd, result_vector);	
		CATCH_TRY_SECTION_MESS(false, "while statment: " << sql_statment << " [params]: " << print_parameters_multi(parametrs));
		return res;
	}


	template<typename TParams>
	inline
		bool select_helper_param_container(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParams& parametrs, table& result_vector)
	{
		PROFILE_SQL(sql_statment);
		bool res = false;
		BEGIN_TRY_SECTION();
		ADODB::_CommandPtr cmd;
		cmd.CreateInstance(__uuidof(ADODB::Command));
		cmd->CommandText = _bstr_t(sql_statment.c_str());	
		
		
		for(TParams::const_iterator it = parametrs.begin(); it!=parametrs.end(); it++)
		{
			add_parametr(cmd, *it);
		}

		cmd->ActiveConnection = pconnection;
		res = select_helper(cmd, result_vector);	

		CATCH_TRY_SECTION(false);
		return res;
	}


	inline
		bool execute_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, _variant_t* pvt = NULL)
	{
		adapter_zero<int> params;
		return execute_helper_multiparam(pconnection, sql_statment, params, pvt);
	}

	template<typename TParam>
		bool execute_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam& parametr)
	{
		adapter_single<TParam> params;
		params.tparam1 = parametr;
		return execute_helper_multiparam(pconnection, sql_statment, params);
	}


	template<typename TParam1, typename TParam2>
		bool execute_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1& parametr1, const TParam2& parametr2)
	{
		adapter_double<TParam1, TParam2> params;
		params.tparam1 = parametr1;
		params.tparam2 = parametr2;
		return execute_helper_multiparam(pconnection, sql_statment, params);

	}

	template<typename TParam1, typename TParam2, typename TParam3>
		bool execute_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1& parametr1, const TParam2& parametr2, const TParam3& parametr3)
	{
		adapter_triple<TParam1, TParam2, typename TParam3> params;
		params.tparam1 = parametr1;
		params.tparam2 = parametr2;
		params.tparam3 = parametr3;
		return execute_helper_multiparam(pconnection, sql_statment, params);
	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4>
		bool execute_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1& parametr1, const TParam2& parametr2, const TParam3& parametr3, const TParam4& parametr4)
	{
		adapter_quad<TParam1, TParam2, TParam3, TParam4> params;
		params.tparam1 = parametr1;
		params.tparam2 = parametr2;
		params.tparam3 = parametr3;
		params.tparam4 = parametr4;
		return execute_helper_multiparam(pconnection, sql_statment, params);
	}

		template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5>
		bool execute_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1& parametr1, const TParam2& parametr2, const TParam3& parametr3, const TParam4& parametr4, const TParam5& parametr5)
		{
			adapter_quanto<TParam1, TParam2, TParam3, TParam4, TParam5> params;
			params.tparam1 = parametr1;
			params.tparam2 = parametr2;
			params.tparam3 = parametr3;
			params.tparam4 = parametr4;
			params.tparam5 = parametr5;
			return execute_helper_multiparam(pconnection, sql_statment, params);
		}

		template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6>
		bool execute_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1& parametr1, const TParam2& parametr2, const TParam3& parametr3, const TParam4& parametr4, const TParam5& parametr5, const TParam6& parametr6)
		{
			adapter_sixto<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6> params;
			params.tparam1 = parametr1;
			params.tparam2 = parametr2;
			params.tparam3 = parametr3;
			params.tparam4 = parametr4;
			params.tparam5 = parametr5;
			params.tparam6 = parametr6;
			return execute_helper_multiparam(pconnection, sql_statment, params);
		}


		template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7>
		bool execute_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1& parametr1, const TParam2& parametr2, const TParam3& parametr3, const TParam4& parametr4, const TParam5& parametr5, const TParam6& parametr6, const TParam7& parametr7)
		{
			adapter_sevento<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7> params;
			params.tparam1 = parametr1;
			params.tparam2 = parametr2;
			params.tparam3 = parametr3;
			params.tparam4 = parametr4;
			params.tparam5 = parametr5;
			params.tparam6 = parametr6;
			params.tparam7 = parametr7;
			return execute_helper_multiparam(pconnection, sql_statment, params);
		}

	inline
		bool select_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, table& result_vector)
	{
		adapter_zero<int> params;
		return select_helper_multiparam(pconnection, sql_statment, params, result_vector);
	}


	template<typename TParam>
		bool select_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam& parametr, table& result_vector)
	{
		adapter_single<TParam> params;
		params.tparam1 = parametr;
		return select_helper_multiparam(pconnection, sql_statment, params, result_vector);
	}

	template<typename TParam1, typename TParam2>
		bool select_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1 parametr1, const TParam2 parametr2, table& result_vector)
	{
		adapter_double<TParam1, TParam2> params;
		params.tparam1 = parametr1;
		params.tparam2 = parametr2;
		return select_helper_multiparam(pconnection, sql_statment, params, result_vector);

	}

	template<typename TParam1, typename TParam2, typename TParam3>
		bool select_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1 parametr1, const TParam2 parametr2, const TParam3 parametr3, table& result_vector)
	{
		adapter_triple<TParam1, TParam2, typename TParam3> params;
		params.tparam1 = parametr1;
		params.tparam2 = parametr2;
		params.tparam3 = parametr3;
		return select_helper_multiparam(pconnection, sql_statment, params, result_vector);

	}

	template<typename TParam1, typename TParam2, typename TParam3, typename TParam4>
		bool select_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1 parametr1, const TParam2 parametr2, const TParam3 parametr3, const TParam4 parametr4, table& result_vector)
	{
		adapter_quad<TParam1, TParam2, TParam3, TParam4> params;
		params.tparam1 = parametr1;
		params.tparam2 = parametr2;
		params.tparam3 = parametr3;
		params.tparam4 = parametr4;
		return select_helper_multiparam(pconnection, sql_statment, params, result_vector);
	}

		template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5>
		bool select_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1 parametr1, const TParam2 parametr2, const TParam3 parametr3, const TParam4 parametr4, const TParam5 parametr5, table& result_vector)
		{
			adapter_quanto<TParam1, TParam2, TParam3, TParam4, TParam5> params;
			params.tparam1 = parametr1;
			params.tparam2 = parametr2;
			params.tparam3 = parametr3;
			params.tparam4 = parametr4;
			params.tparam5 = parametr5;
			return select_helper_multiparam(pconnection, sql_statment, params, result_vector);
		}


		template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6>
		bool select_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1 parametr1, const TParam2 parametr2, const TParam3 parametr3, const TParam4 parametr4, const TParam5 parametr5, const TParam6 parametr6, table& result_vector)
		{
			adapter_sixto<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6> params;
			params.tparam1 = parametr1;
			params.tparam2 = parametr2;
			params.tparam3 = parametr3;
			params.tparam4 = parametr4;
			params.tparam5 = parametr5;
			params.tparam6 = parametr6;
			return select_helper_multiparam(pconnection, sql_statment, params, result_vector);
		}

		template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7>
		bool select_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1 parametr1, const TParam2 parametr2, const TParam3 parametr3, const TParam4 parametr4, const TParam5 parametr5, const TParam6 parametr6, const TParam7 parametr7, table& result_vector)
		{
			adapter_sevento<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7> params;
			params.tparam1 = parametr1;
			params.tparam2 = parametr2;
			params.tparam3 = parametr3;
			params.tparam4 = parametr4;
			params.tparam5 = parametr5;
			params.tparam6 = parametr6;
			params.tparam7 = parametr7;
			return select_helper_multiparam(pconnection, sql_statment, params, result_vector);
		}

		template<typename TParam1, typename TParam2, typename TParam3, typename TParam4, typename TParam5, typename TParam6, typename TParam7, typename TParam8, typename TParam9>
		bool select_helper(ADODB::_ConnectionPtr pconnection, const std::string& sql_statment, const TParam1 parametr1, const TParam2 parametr2, const TParam3 parametr3, const TParam4 parametr4, const TParam5 parametr5, const TParam6 parametr6, const TParam7 parametr7,const TParam8 parametr8,const TParam9 parametr9, table& result_vector)
		{
			adapter_nine<TParam1, TParam2, TParam3, TParam4, TParam5, TParam6, TParam7, TParam8, TParam9> params;
			params.tparam1 = parametr1;
			params.tparam2 = parametr2;
			params.tparam3 = parametr3;
			params.tparam4 = parametr4;
			params.tparam5 = parametr5;
			params.tparam6 = parametr6;
			params.tparam7 = parametr7;
			params.tparam8 = parametr8;
			params.tparam9 = parametr9;
			return select_helper_multiparam(pconnection, sql_statment, params, result_vector);
		}




		/************************************************************************/
		/*                                                                      */
		/************************************************************************/

		class per_thread_connection_pool
		{
		public:
			bool init(const std::string& connection_string, const std::string& login, const std::string&  pass)
			{
				m_connection_string = connection_string;
				m_login = login;
				m_password = pass;
				if(!get_db_connection().GetInterfacePtr())
					return false;

				return true;
			}

			ADODB::_ConnectionPtr& get_db_connection()
			{

				//soci::session 

				m_db_connections_lock.lock();
				boost::shared_ptr<ADODB::_ConnectionPtr>& conn_ptr = m_db_connections[::GetCurrentThreadId()];
				m_db_connections_lock.unlock();
				if(!conn_ptr.get())
				{
					conn_ptr.reset(new ADODB::_ConnectionPtr());
					ADODB::_ConnectionPtr& conn = *conn_ptr.get();
					//init new connection

					BEGIN_TRY_SECTION();
					//_bstr_t str = _bstr_t("Provider=SQLOLEDB;Data Source=SRV1;Integrated Security=SSPI;Initial Catalog=dispatcher;");

					if(S_OK != conn.CreateInstance(__uuidof(ADODB::Connection)))
					{
						LOG_ERROR("Failed to Create, instance, was CoInitialize called ???!");
						return conn;
					}

					HRESULT res = conn->Open(_bstr_t(m_connection_string.c_str()), _bstr_t(m_login.c_str()), _bstr_t(m_password.c_str()), NULL);
					if(res != S_OK) 
					{
						LOG_ERROR("Failed to connect do DB, connection str:" << m_connection_string);
						return conn;
					}
					CATCH_TRY_SECTION_MESS(conn, "while creating another connection");
					LOG_PRINT("New DB Connection added for threadid=" << ::GetCurrentThreadId(), LOG_LEVEL_0);
					ado_db_helper::execute_helper(conn, "set enable_seqscan=false;");
					return conn;
				}

				return *conn_ptr.get();
			}

			//----------------------------------------------------------------------------------------------
			bool check_status()
			{
				ADODB::_ConnectionPtr& rconn = get_db_connection();
				if(!ado_db_helper::execute_helper(rconn, "SET CLIENT_ENCODING TO 'SQL_ASCII'"))
				{

					try{
						HRESULT res = rconn->Close();
					}
					catch(...)
					{

					};
					BEGIN_TRY_SECTION();

					HRESULT res = rconn->Open(_bstr_t(m_connection_string.c_str()), _bstr_t(m_login.c_str()), _bstr_t(m_password.c_str()), NULL);
					if(res != S_OK) 
					{
						LOG_PRINT("Failed to restore connection to local AI DB", LOG_LEVEL_1);
						return false;
					}
					CATCH_TRY_SECTION(false);
				}

				return true;
			}
			
		protected:
		private:
			std::map<DWORD, boost::shared_ptr<ADODB::_ConnectionPtr> > m_db_connections;
			critical_section m_db_connections_lock;
			std::string m_connection_string;
			std::string m_login;
			std::string m_password;
		};


		template<typename TParam1, typename default_id_type, typename t_conn>
		bool find_or_add_t(const std::string& sql_select_statment, const std::string& sql_insert_statment, OUT default_id_type& id, OUT bool& new_object_added, TParam1 parametr_1, t_conn& c)
		{
			ado_db_helper::adapter_single<TParam1> params;
			params.tparam1 = parametr_1;
			return find_or_add_t_multiparametred(sql_select_statment, sql_insert_statment, id, new_object_added, params, c);
		}


		template<typename TParam1, typename TParam2, typename default_id_type, typename t_conn>
		bool find_or_add_t(const std::string& sql_select_statment, const std::string& sql_insert_statment, OUT default_id_type& id, OUT bool& new_object_added, TParam1 parametr_1, TParam2 parametr_2, t_conn& c)
		{
			ado_db_helper::adapter_double<TParam1, TParam2> params;
			params.tparam1 = parametr_1;
			params.tparam2 = parametr_2;
			return find_or_add_t_multiparametred(sql_select_statment, sql_insert_statment, id, new_object_added, params, c);
		}


		template<typename TParam1, typename TParam2, typename TParam3, typename default_id_type, typename t_conn>
		bool find_or_add_t(const std::string& sql_select_statment, const std::string& sql_insert_statment, OUT default_id_type& id, OUT bool& new_object_added, TParam1 parametr_1, TParam2 parametr_2, TParam3 parametr_3, t_conn& c)
		{
			ado_db_helper::adapter_triple<TParam1, TParam2, TParam3> params;
			params.tparam1 = parametr_1;
			params.tparam2 = parametr_2;
			params.tparam3 = parametr_3;
			return find_or_add_t_multiparametred(sql_select_statment, sql_insert_statment, id, new_object_added, params, c);
		}

		template<typename TParam1, typename TParam2,  typename TParam3,  typename TParam4, typename default_id_type, typename t_conn>
		bool find_or_add_t(const std::string& sql_select_statment, const std::string& sql_insert_statment, OUT default_id_type& id, OUT bool& new_object_added, TParam1 parametr_1, TParam2 parametr_2, TParam3 parametr_3, TParam4 parametr_4, t_conn& c)
		{
			ado_db_helper::adapter_quad<TParam1, TParam2, TParam3, TParam4> params;
			params.tparam1 = parametr_1;
			params.tparam2 = parametr_2;
			params.tparam3 = parametr_3;
			params.tparam4 = parametr_4;
			return find_or_add_t_multiparametred(sql_select_statment, sql_insert_statment, id, new_object_added, params, c);
		}

		template<typename TParams, typename default_id_type, typename t_conn>
		bool find_or_add_t_multiparametred(const std::string& sql_select_statment, const std::string& sql_insert_statment, OUT default_id_type& id, OUT bool& new_object_added, TParams params, t_conn& c)
		{

			//CHECK_CONNECTION(false);

			new_object_added = false;
			ado_db_helper::table result_table;	

			bool res = select_helper_multiparam(c.get_db_connection(), sql_select_statment, params, result_table);
			if(!result_table.size())
			{
				res = select_helper_multiparam(c.get_db_connection(), sql_insert_statment, params, result_table);
				if(!res || !result_table.size())
				{
					//last time try to select
					res = select_helper_multiparam(c.get_db_connection(), sql_select_statment, params, result_table);
					CHECK_AND_ASSERT_MES(res, false, "Failed to execute statment: " << sql_select_statment);
					CHECK_AND_ASSERT_MES(result_table.size(), false, "No records returned from statment: " << sql_select_statment);
				}else
				{
					new_object_added = true;
				}
			}

			BEGIN_TRY_SECTION()
			id = result_table[0][0];
			CATCH_TRY_SECTION_MESS(false, "while converting returned value [find_or_add_t_multiparametred()]");

			return true;
		}

}
}
#endif //!_DB_HELPER_H_
