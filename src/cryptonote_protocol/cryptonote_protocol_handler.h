/// @file
/// @author rfree (current maintainer/user in monero.cc project - most of code is from CryptoNote)
/// @brief This is the orginal cryptonote protocol network-events handler, modified by us

// Copyright (c) 2014-2017, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <boost/program_options/variables_map.hpp>
#include <string>
#include <ctime>

#include "math_helper.h"
#include "storages/levin_abstract_invoke2.h"
#include "warnings.h"
#include "cryptonote_protocol_defs.h"
#include "cryptonote_protocol_handler_common.h"
#include "block_queue.h"
#include "cryptonote_basic/connection_context.h"
#include "cryptonote_basic/cryptonote_stat_info.h"
#include "cryptonote_basic/verification_context.h"
// #include <netinet/in.h>
#include <boost/circular_buffer.hpp>

PUSH_WARNINGS
DISABLE_VS_WARNINGS(4355)

#define LOCALHOST_INT 2130706433

namespace cryptonote
{

	class cryptonote_protocol_handler_base_pimpl;
	class cryptonote_protocol_handler_base {
		private:
			std::unique_ptr<cryptonote_protocol_handler_base_pimpl> mI;

		public:
			cryptonote_protocol_handler_base();
			virtual ~cryptonote_protocol_handler_base();
			void handler_request_blocks_history(std::list<crypto::hash>& ids); // before asking for list of objects, we can change the list still
			void handler_response_blocks_now(size_t packet_size);
			
			virtual double get_avg_block_size() = 0;
			virtual double estimate_one_block_size() noexcept; // for estimating size of blocks to download
	};

  template<class t_core>
  class t_cryptonote_protocol_handler:  public i_cryptonote_protocol, cryptonote_protocol_handler_base
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
      HANDLE_NOTIFY_T2(NOTIFY_NEW_FLUFFY_BLOCK, &cryptonote_protocol_handler::handle_notify_new_fluffy_block)			
      HANDLE_NOTIFY_T2(NOTIFY_REQUEST_FLUFFY_MISSING_TX, &cryptonote_protocol_handler::handle_request_fluffy_missing_tx)						
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
    std::list<connection_info> get_connections();
    const block_queue &get_block_queue() const { return m_block_queue; }
    void stop();
    void on_connection_close(cryptonote_connection_context &context);
  private:
    //----------------- commands handlers ----------------------------------------------
    int handle_notify_new_block(int command, NOTIFY_NEW_BLOCK::request& arg, cryptonote_connection_context& context);
    int handle_notify_new_transactions(int command, NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& context);
    int handle_request_get_objects(int command, NOTIFY_REQUEST_GET_OBJECTS::request& arg, cryptonote_connection_context& context);
    int handle_response_get_objects(int command, NOTIFY_RESPONSE_GET_OBJECTS::request& arg, cryptonote_connection_context& context);
    int handle_request_chain(int command, NOTIFY_REQUEST_CHAIN::request& arg, cryptonote_connection_context& context);
    int handle_response_chain_entry(int command, NOTIFY_RESPONSE_CHAIN_ENTRY::request& arg, cryptonote_connection_context& context);
    int handle_notify_new_fluffy_block(int command, NOTIFY_NEW_FLUFFY_BLOCK::request& arg, cryptonote_connection_context& context);
    int handle_request_fluffy_missing_tx(int command, NOTIFY_REQUEST_FLUFFY_MISSING_TX::request& arg, cryptonote_connection_context& context);
		
    //----------------- i_bc_protocol_layout ---------------------------------------
    virtual bool relay_block(NOTIFY_NEW_BLOCK::request& arg, cryptonote_connection_context& exclude_context);
    virtual bool relay_transactions(NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& exclude_context);
    //----------------------------------------------------------------------------------
    //bool get_payload_sync_data(HANDSHAKE_DATA::request& hshd, cryptonote_connection_context& context);
    bool request_missing_objects(cryptonote_connection_context& context, bool check_having_blocks, bool force_next_span = false);
    size_t get_synchronizing_connections_count();
    bool on_connection_synchronized();
    bool should_download_next_span(cryptonote_connection_context& context) const;
    void drop_connection(cryptonote_connection_context &context, bool add_fail, bool flush_all_spans);
    bool kick_idle_peers();
    int try_add_next_blocks(cryptonote_connection_context &context);

    t_core& m_core;

    nodetool::p2p_endpoint_stub<connection_context> m_p2p_stub;
    nodetool::i_p2p_endpoint<connection_context>* m_p2p;
    std::atomic<uint32_t> m_syncronized_connections_count;
    std::atomic<bool> m_synchronized;
    std::atomic<bool> m_stopping;
    boost::mutex m_sync_lock;
    block_queue m_block_queue;
    epee::math_helper::once_a_time_seconds<30> m_idle_peer_kicker;

    boost::mutex m_buffer_mutex;
    double get_avg_block_size();
    boost::circular_buffer<size_t> m_avg_buffer = boost::circular_buffer<size_t>(10);

    template<class t_parameter>
      bool post_notify(typename t_parameter::request& arg, cryptonote_connection_context& context)
      {
        LOG_PRINT_L2("[" << epee::net_utils::print_connection_context_short(context) << "] post " << typeid(t_parameter).name() << " -->");
        std::string blob;
        epee::serialization::store_t_to_binary(arg, blob);
        //handler_response_blocks_now(blob.size()); // XXX
        return m_p2p->invoke_notify_to_peer(t_parameter::ID, blob, context);
      }

      template<class t_parameter>
      bool relay_post_notify(typename t_parameter::request& arg, cryptonote_connection_context& exclude_context)
      {
        LOG_PRINT_L2("[" << epee::net_utils::print_connection_context_short(exclude_context) << "] post relay " << typeid(t_parameter).name() << " -->");
        std::string arg_buff;
        epee::serialization::store_t_to_binary(arg, arg_buff);
        return m_p2p->relay_notify_to_all(t_parameter::ID, arg_buff, exclude_context);
      }
  };

} // namespace


#include "cryptonote_protocol_handler.inl"

POP_WARNINGS
