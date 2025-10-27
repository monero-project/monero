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

#include "portable_storage_template_helper.h"
#include <boost/utility/string_ref.hpp>
#include <boost/utility/value_init.hpp>
#include <functional>
#include "byte_slice.h"
#include "span.h"
#include "net/levin_base.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net"

template<typename context_t>
void on_levin_traffic(const context_t &context, bool initiator, bool sent, bool error, size_t bytes, const char *category);

template<typename context_t>
void on_levin_traffic(const context_t &context, bool initiator, bool sent, bool error, size_t bytes, int command);

namespace
{
  static const constexpr epee::serialization::portable_storage::limits_t default_levin_limits = {
    8192, // objects
    16384, // fields
    16384, // strings
  };
}

namespace epee
{
  namespace net_utils
  {
    template<class t_result, class t_arg, class callback_t, class t_transport>
    bool async_invoke_remote_command2(const epee::net_utils::connection_context_base &context, int command, const t_arg& out_struct, t_transport& transport, const callback_t &cb, size_t inv_timeout = LEVIN_DEFAULT_TIMEOUT_PRECONFIGURED)
    {
      const boost::uuids::uuid &conn_id = context.m_connection_id;
      typename serialization::portable_storage stg;
      const_cast<t_arg&>(out_struct).store(stg);//TODO: add true const support to searilzation
      levin::message_writer to_send{16 * 1024};
      stg.store_to_binary(to_send.buffer);
      int res = transport.invoke_async(command, std::move(to_send), conn_id, [cb, command](int code, const epee::span<const uint8_t> buff, typename t_transport::connection_context& context)->bool
      {
        t_result result_struct = AUTO_VAL_INIT(result_struct);
        if( code <=0 )
        {
          if (!buff.empty())
            on_levin_traffic(context, true, false, true, buff.size(), command);
          LOG_PRINT_L1("Failed to invoke command " << command << " return code " << code);
          cb(code, result_struct, context);
          return false;
        }
        serialization::portable_storage stg_ret;
        if(!stg_ret.load_from_binary(buff, &default_levin_limits))
        {
          on_levin_traffic(context, true, false, true, buff.size(), command);
          LOG_ERROR("Failed to load_from_binary on command " << command);
          cb(LEVIN_ERROR_FORMAT, result_struct, context);
          return false;
        }
        if (!result_struct.load(stg_ret))
        {
          on_levin_traffic(context, true, false, true, buff.size(), command);
          LOG_ERROR("Failed to load result struct on command " << command);
          cb(LEVIN_ERROR_FORMAT, result_struct, context);
          return false;
        }
        on_levin_traffic(context, true, false, false, buff.size(), command);
        cb(code, result_struct, context);
        return true;
      }, inv_timeout);
      if( res <=0 )
      {
        LOG_PRINT_L1("Failed to invoke command " << command << " return code " << res);
        return false;
      }
      return true;
    }

    template<class t_arg, class t_transport>
    bool notify_remote_command2(const typename t_transport::connection_context &context, int command, const t_arg& out_struct, t_transport& transport)
    {
      const boost::uuids::uuid &conn_id = context.m_connection_id;
      serialization::portable_storage stg;
      out_struct.store(stg);
      levin::message_writer to_send;
      stg.store_to_binary(to_send.buffer);

      const bool ssl = (context.m_ssl != ssl_support_t::e_ssl_support_disabled);
      int res = transport.send(to_send.finalize_notify(command, ssl), conn_id);
      if(res <=0 )
      {
        MERROR("Failed to notify command " << command << " return code " << res);
        return false;
      }
      return true;
    }
    //----------------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------------
    template<class t_owner, class t_in_type, class t_out_type, class t_context, class callback_t>
    int buff_to_t_adapter(int command, const epee::span<const uint8_t> in_buff, byte_stream& buff_out, callback_t cb, t_context& context )
    {
      serialization::portable_storage strg;
      if(!strg.load_from_binary(in_buff, &default_levin_limits))
      {
        on_levin_traffic(context, false, false, true, in_buff.size(), command);
        LOG_ERROR("Failed to load_from_binary in command " << command);
        return -1;
      }
      boost::value_initialized<t_in_type> in_struct;
      boost::value_initialized<t_out_type> out_struct;

      if (!static_cast<t_in_type&>(in_struct).load(strg))
      {
        on_levin_traffic(context, false, false, true, in_buff.size(), command);
        LOG_ERROR("Failed to load in_struct in command " << command);
        return -1;
      }
      on_levin_traffic(context, false, false, false, in_buff.size(), command);
      int res = cb(command, static_cast<t_in_type&>(in_struct), static_cast<t_out_type&>(out_struct), context);
      serialization::portable_storage strg_out;
      static_cast<t_out_type&>(out_struct).store(strg_out);

      if(!strg_out.store_to_binary(buff_out))
      {
        LOG_ERROR("Failed to store_to_binary in command" << command);
        return -1;
      }

      return res;
    }

    template<class t_owner, class t_in_type, class t_context, class callback_t>
    int buff_to_t_adapter(t_owner* powner, int command, const epee::span<const uint8_t> in_buff, callback_t cb, t_context& context)
    {
      serialization::portable_storage strg;
      if(!strg.load_from_binary(in_buff, &default_levin_limits))
      {
        on_levin_traffic(context, false, false, true, in_buff.size(), command);
        LOG_ERROR("Failed to load_from_binary in notify " << command);
        return -1;
      }
      boost::value_initialized<t_in_type> in_struct;
      if (!static_cast<t_in_type&>(in_struct).load(strg))
      {
        on_levin_traffic(context, false, false, true, in_buff.size(), command);
        LOG_ERROR("Failed to load in_struct in notify " << command);
        return -1;
      }
      on_levin_traffic(context, false, false, false, in_buff.size(), command);
      return cb(command, in_struct, context);
    }

#define CHAIN_LEVIN_INVOKE_MAP2(context_type) \
  int invoke(int command, const epee::span<const uint8_t> in_buff, epee::byte_stream& buff_out, context_type& context) \
  { \
  bool handled = false; \
  return handle_invoke_map(false, command, in_buff, buff_out, context, handled); \
  } 

#define CHAIN_LEVIN_NOTIFY_MAP2(context_type) \
  int notify(int command, const epee::span<const uint8_t> in_buff, context_type& context) \
  { \
    bool handled = false; epee::byte_stream fake_str; \
    return handle_invoke_map(true, command, in_buff, fake_str, context, handled); \
  } 

#define BEGIN_INVOKE_MAP2(owner_type) \
  template <class t_context> int handle_invoke_map(bool is_notify, int command, const epee::span<const uint8_t> in_buff, epee::byte_stream& buff_out, t_context& context, bool& handled) \
  { \
  try { \
  typedef owner_type internal_owner_type_name;

#define HANDLE_INVOKE_T2(COMMAND, func) \
  if(!is_notify && COMMAND::ID == command) \
  {handled=true;return epee::net_utils::buff_to_t_adapter<internal_owner_type_name, typename COMMAND::request, typename COMMAND::response>(command, in_buff, buff_out, std::bind(func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), context);}

#define HANDLE_NOTIFY_T2(NOTIFY, func) \
  if(is_notify && NOTIFY::ID == command) \
  {handled=true;return epee::net_utils::buff_to_t_adapter<internal_owner_type_name, typename NOTIFY::request>(this, command, in_buff, std::bind(func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), context);}

#define CHAIN_INVOKE_MAP_TO_OBJ_FORCE_CONTEXT(obj, context_type) \
  { \
  int res = obj.handle_invoke_map(is_notify, command, in_buff, buff_out, static_cast<context_type>(context), handled); \
  if(handled) return res; \
  }


#define END_INVOKE_MAP2() \
  LOG_ERROR("Unknown command:" << command); \
  on_levin_traffic(context, false, false, true, in_buff.size(), "invalid-command"); \
  return LEVIN_ERROR_CONNECTION_HANDLER_NOT_DEFINED; \
  } \
  catch (const std::exception &e) { \
    MERROR("Error in handle_invoke_map: " << e.what()); \
    return LEVIN_ERROR_CONNECTION_TIMEDOUT; /* seems kinda appropriate */ \
  } \
  }

  }
}

