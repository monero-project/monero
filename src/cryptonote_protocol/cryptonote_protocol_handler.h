// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/program_options/variables_map.hpp>

#include "storages/levin_abstract_invoke2.h"
#include "warnings.h"
#include "cryptonote_protocol_defs.h"
#include "cryptonote_protocol_handler_common.h"
#include "cryptonote_core/connection_context.h"
#include "cryptonote_core/cryptonote_stat_info.h"
#include "cryptonote_core/verification_context.h"

PUSH_WARNINGS
DISABLE_VS_WARNINGS(4355)

namespace cryptonote
{

  template<class t_core>
  class t_cryptonote_protocol_handler:  public i_cryptonote_protocol
  { 
  public:
    typedef cryptonote_connection_context connection_context;
    typedef core_stat_info stat_info;
    typedef t_cryptonote_protocol_handler<t_core> cryptonote_protocol_handler;
    typedef CORE_SYNC_DATA payload_type;

    t_cryptonote_protocol_handler(t_core& rcore, nodetool::i_p2p_endpoint<connection_context>* p_net_layout);

    BEGIN_INVOKE_MAP2(cryptonote_protocol_handler)
      HANDLE_NOTIFY_T2(NOTIFY_NEW_BLOCK, &cryptonote_protocol_handler::handle_notify_new_block)
      HANDLE_NOTIFY_T2(NOTIFY_NEW_TRANSACTIONS, &cryptonote_protocol_handler::handle_notify_new_transactions)
      HANDLE_NOTIFY_T2(NOTIFY_REQUEST_GET_OBJECTS, &cryptonote_protocol_handler::handle_request_get_objects)
      HANDLE_NOTIFY_T2(NOTIFY_RESPONSE_GET_OBJECTS, &cryptonote_protocol_handler::handle_response_get_objects)
      HANDLE_NOTIFY_T2(NOTIFY_REQUEST_CHAIN, &cryptonote_protocol_handler::handle_request_chain)
      HANDLE_NOTIFY_T2(NOTIFY_RESPONSE_CHAIN_ENTRY, &cryptonote_protocol_handler::handle_response_chain_entry)
    END_INVOKE_MAP2()

    bool on_idle();
    bool init(const boost::program_options::variables_map& vm);
    bool deinit();
    void set_p2p_endpoint(nodetool::i_p2p_endpoint<connection_context>* p2p);
    //bool process_handshake_data(const blobdata& data, cryptonote_connection_context& context);
    bool process_payload_sync_data(const CORE_SYNC_DATA& hshd, cryptonote_connection_context& context, bool is_inital);
    bool get_payload_sync_data(blobdata& data);
    bool get_payload_sync_data(CORE_SYNC_DATA& hshd);
    bool get_stat_info(core_stat_info& stat_inf);
    bool on_callback(cryptonote_connection_context& context);
    t_core& get_core(){return m_core;}
    bool is_synchronized(){return m_synchronized;}
    void log_connections();
  private:
    //----------------- commands handlers ----------------------------------------------
    int handle_notify_new_block(int command, NOTIFY_NEW_BLOCK::request& arg, cryptonote_connection_context& context);
    int handle_notify_new_transactions(int command, NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& context);
    int handle_request_get_objects(int command, NOTIFY_REQUEST_GET_OBJECTS::request& arg, cryptonote_connection_context& context);
    int handle_response_get_objects(int command, NOTIFY_RESPONSE_GET_OBJECTS::request& arg, cryptonote_connection_context& context);
    int handle_request_chain(int command, NOTIFY_REQUEST_CHAIN::request& arg, cryptonote_connection_context& context);
    int handle_response_chain_entry(int command, NOTIFY_RESPONSE_CHAIN_ENTRY::request& arg, cryptonote_connection_context& context);


    //----------------- i_bc_protocol_layout ---------------------------------------
    virtual bool relay_block(NOTIFY_NEW_BLOCK::request& arg, cryptonote_connection_context& exclude_context);
    virtual bool relay_transactions(NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& exclude_context);
    //----------------------------------------------------------------------------------
    //bool get_payload_sync_data(HANDSHAKE_DATA::request& hshd, cryptonote_connection_context& context);
    bool request_missing_objects(cryptonote_connection_context& context, bool check_having_blocks);
    size_t get_synchronizing_connections_count();
    bool on_connection_synchronized();
    t_core& m_core;

    nodetool::p2p_endpoint_stub<connection_context> m_p2p_stub;
    nodetool::i_p2p_endpoint<connection_context>* m_p2p;
    std::atomic<uint32_t> m_syncronized_connections_count;
    std::atomic<bool> m_synchronized;

    template<class t_parametr>
      bool post_notify(typename t_parametr::request& arg, cryptonote_connection_context& context)
      {
        LOG_PRINT_L2("[" << net_utils::print_connection_context_short(context) << "] post " << typeid(t_parametr).name() << " -->");
        std::string blob;
        epee::serialization::store_t_to_binary(arg, blob);
        return m_p2p->invoke_notify_to_peer(t_parametr::ID, blob, context);
      }

      template<class t_parametr>
      bool relay_post_notify(typename t_parametr::request& arg, cryptonote_connection_context& exlude_context)
      {
        LOG_PRINT_L2("[" << net_utils::print_connection_context_short(exlude_context) << "] post relay " << typeid(t_parametr).name() << " -->");
        std::string arg_buff;
        epee::serialization::store_t_to_binary(arg, arg_buff);
        return m_p2p->relay_notify_to_all(t_parametr::ID, arg_buff, exlude_context);
      }
  };
}


#include "cryptonote_protocol_handler.inl"

POP_WARNINGS
