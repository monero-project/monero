/// @file
/// @author rfree (current maintainer/user in monero.cc project - most of code is from CryptoNote)
/// @brief This is the orginal cryptonote protocol network-events handler, modified by us

// Copyright (c) 2014-2016, The Monero Project
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

// (may contain code and/or modifications by other developers)
// developer rfree: this code is caller of our new network code, and is modded; e.g. for rate limiting

#include <boost/interprocess/detail/atomic.hpp>
#include <list>
#include <unordered_map>

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "profile_tools.h"
#include "../../src/p2p/network_throttle-detail.hpp"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.cn"

#define MLOG_P2P_MESSAGE(x) MCINFO("net.p2p.msg", context << x)

namespace cryptonote
{



  //-----------------------------------------------------------------------------------------------------------------------
  template<class t_core>
    t_cryptonote_protocol_handler<t_core>::t_cryptonote_protocol_handler(t_core& rcore, nodetool::i_p2p_endpoint<connection_context>* p_net_layout):m_core(rcore),
                                                                                                              m_p2p(p_net_layout),
                                                                                                              m_syncronized_connections_count(0),
                                                                                                              m_synchronized(false),
                                                                                                              m_stopping(false)

  {
    if(!m_p2p)
      m_p2p = &m_p2p_stub;
  }
  //-----------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::init(const boost::program_options::variables_map& vm)
  {
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::deinit()
  {
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_cryptonote_protocol_handler<t_core>::set_p2p_endpoint(nodetool::i_p2p_endpoint<connection_context>* p2p)
  {
    if(p2p)
      m_p2p = p2p;
    else
      m_p2p = &m_p2p_stub;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::on_callback(cryptonote_connection_context& context)
  {
    LOG_PRINT_CCONTEXT_L2("callback fired");
    CHECK_AND_ASSERT_MES_CC( context.m_callback_request_count > 0, false, "false callback fired, but context.m_callback_request_count=" << context.m_callback_request_count);
    --context.m_callback_request_count;

    if(context.m_state == cryptonote_connection_context::state_synchronizing)
    {
      NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
      m_core.get_short_chain_history(r.block_ids);
      LOG_PRINT_CCONTEXT_L2("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() );
      post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::get_stat_info(core_stat_info& stat_inf)
  {
    return m_core.get_stat_info(stat_inf);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_cryptonote_protocol_handler<t_core>::log_connections()
  {
    std::stringstream ss;
    ss.precision(1);

    double down_sum = 0.0;
    double down_curr_sum = 0.0;
    double up_sum = 0.0;
    double up_curr_sum = 0.0;

    ss << std::setw(30) << std::left << "Remote Host"
      << std::setw(20) << "Peer id"
      << std::setw(20) << "Support Flags"      
      << std::setw(30) << "Recv/Sent (inactive,sec)"
      << std::setw(25) << "State"
      << std::setw(20) << "Livetime(sec)"
      << std::setw(12) << "Down (kB/s)"
      << std::setw(14) << "Down(now)"
      << std::setw(10) << "Up (kB/s)"
      << std::setw(13) << "Up(now)"
      << ENDL;

    uint32_t ip;
    m_p2p->for_each_connection([&](const connection_context& cntxt, nodetool::peerid_type peer_id, uint32_t support_flags)
    {
      bool local_ip = false;
      ip = ntohl(cntxt.m_remote_ip);
      // TODO: local ip in calss A, B
      if (ip > 3232235520 && ip < 3232301055) // 192.168.x.x
      local_ip = true;
      auto connection_time = time(NULL) - cntxt.m_started;
      ss << std::setw(30) << std::left << std::string(cntxt.m_is_income ? " [INC]":"[OUT]") +
        epee::string_tools::get_ip_string_from_int32(cntxt.m_remote_ip) + ":" + std::to_string(cntxt.m_remote_port)
        << std::setw(20) << std::hex << peer_id
        << std::setw(20) << std::hex << support_flags
        << std::setw(30) << std::to_string(cntxt.m_recv_cnt)+ "(" + std::to_string(time(NULL) - cntxt.m_last_recv) + ")" + "/" + std::to_string(cntxt.m_send_cnt) + "(" + std::to_string(time(NULL) - cntxt.m_last_send) + ")"
        << std::setw(25) << get_protocol_state_string(cntxt.m_state)
        << std::setw(20) << std::to_string(time(NULL) - cntxt.m_started)
        << std::setw(12) << std::fixed << (connection_time == 0 ? 0.0 : cntxt.m_recv_cnt / connection_time / 1024)
        << std::setw(14) << std::fixed << cntxt.m_current_speed_down / 1024
        << std::setw(10) << std::fixed << (connection_time == 0 ? 0.0 : cntxt.m_send_cnt / connection_time / 1024)
        << std::setw(13) << std::fixed << cntxt.m_current_speed_up / 1024
        << (local_ip ? "[LAN]" : "")
        << std::left << (ip == LOCALHOST_INT ? "[LOCALHOST]" : "") // 127.0.0.1
        << ENDL;

      if (connection_time > 1)
      {
        down_sum += (cntxt.m_recv_cnt / connection_time / 1024);
        up_sum += (cntxt.m_send_cnt / connection_time / 1024);
      }

      down_curr_sum += (cntxt.m_current_speed_down / 1024);
      up_curr_sum += (cntxt.m_current_speed_up / 1024);

      return true;
    });
    ss << ENDL
      << std::setw(125) << " "
      << std::setw(12) << down_sum
      << std::setw(14) << down_curr_sum
      << std::setw(10) << up_sum
      << std::setw(13) << up_curr_sum
      << ENDL;
    LOG_PRINT_L0("Connections: " << ENDL << ss.str());
  }
  //------------------------------------------------------------------------------------------------------------------------
  // Returns a list of connection_info objects describing each open p2p connection
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  std::list<connection_info> t_cryptonote_protocol_handler<t_core>::get_connections()
  {
    std::list<connection_info> connections;

    m_p2p->for_each_connection([&](const connection_context& cntxt, nodetool::peerid_type peer_id, uint32_t support_flags)
    {
      connection_info cnx;
      auto timestamp = time(NULL);

      cnx.incoming = cntxt.m_is_income ? true : false;

      cnx.ip = epee::string_tools::get_ip_string_from_int32(cntxt.m_remote_ip);
      cnx.port = std::to_string(cntxt.m_remote_port);

      std::stringstream peer_id_str;
      peer_id_str << std::hex << peer_id;
      peer_id_str >> cnx.peer_id;
      
      cnx.support_flags = support_flags;

      cnx.recv_count = cntxt.m_recv_cnt;
      cnx.recv_idle_time = timestamp - cntxt.m_last_recv;

      cnx.send_count = cntxt.m_send_cnt;
      cnx.send_idle_time = timestamp;

      cnx.state = get_protocol_state_string(cntxt.m_state);

      cnx.live_time = timestamp - cntxt.m_started;

      uint32_t ip;
      ip = ntohl(cntxt.m_remote_ip);
      if (ip == LOCALHOST_INT)
      {
        cnx.localhost = true;
      }
      else
      {
        cnx.localhost = false;
      }

      if (ip > 3232235520 && ip < 3232301055) // 192.168.x.x
      {
        cnx.local_ip = true;
      }
      else
      {
        cnx.local_ip = false;
      }

      auto connection_time = time(NULL) - cntxt.m_started;
      if (connection_time == 0)
      {
        cnx.avg_download = 0;
        cnx.avg_upload = 0;
      }

      else
      {
        cnx.avg_download = cntxt.m_recv_cnt / connection_time / 1024;
        cnx.avg_upload = cntxt.m_send_cnt / connection_time / 1024;
      }

      cnx.current_download = cntxt.m_current_speed_down / 1024;
      cnx.current_upload = cntxt.m_current_speed_up / 1024;

      connections.push_back(cnx);

      return true;
    });

    return connections;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::process_payload_sync_data(const CORE_SYNC_DATA& hshd, cryptonote_connection_context& context, bool is_inital)
  {
    if(context.m_state == cryptonote_connection_context::state_befor_handshake && !is_inital)
      return true;

    if(context.m_state == cryptonote_connection_context::state_synchronizing)
      return true;

    if(m_core.have_block(hshd.top_id))
    {
      context.m_state = cryptonote_connection_context::state_normal;
      if(is_inital)
        on_connection_synchronized();
      return true;
    }

    /* As I don't know if accessing hshd from core could be a good practice,
    I prefer pushing target height to the core at the same time it is pushed to the user.
    Nz. */
    m_core.set_target_blockchain_height(static_cast<int64_t>(hshd.current_height));

    int64_t diff = static_cast<int64_t>(hshd.current_height) - static_cast<int64_t>(m_core.get_current_blockchain_height());
	int64_t max_block_height = max(static_cast<int64_t>(hshd.current_height),static_cast<int64_t>(m_core.get_current_blockchain_height()));
	int64_t last_block_v1 = 1009826;
	int64_t diff_v2 = max_block_height > last_block_v1 ? min(abs(diff), max_block_height - last_block_v1) : 0;
    MCLOG(is_inital ? el::Level::Info : el::Level::Debug, "global", context <<  "Sync data returned a new top block candidate: " << m_core.get_current_blockchain_height() << " -> " << hshd.current_height
      << " [Your node is " << std::abs(diff) << " blocks (" << ((abs(diff) - diff_v2) / (24 * 60 * 60 / DIFFICULTY_TARGET_V1)) + (diff_v2 / (24 * 60 * 60 / DIFFICULTY_TARGET_V2)) << " days) "
      << (0 <= diff ? std::string("behind") : std::string("ahead"))
      << "] " << ENDL << "SYNCHRONIZATION started");
    LOG_PRINT_L1("Remote blockchain height: " << hshd.current_height << ", id: " << hshd.top_id);
    context.m_state = cryptonote_connection_context::state_synchronizing;
    context.m_remote_blockchain_height = hshd.current_height;
    //let the socket to send response to handshake, but request callback, to let send request data after response
    LOG_PRINT_CCONTEXT_L2("requesting callback");
    ++context.m_callback_request_count;
    m_p2p->request_callback(context);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::get_payload_sync_data(CORE_SYNC_DATA& hshd)
  {
    m_core.get_blockchain_top(hshd.current_height, hshd.top_id);
    hshd.current_height +=1;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
    template<class t_core>
    bool t_cryptonote_protocol_handler<t_core>::get_payload_sync_data(blobdata& data)
  {
    CORE_SYNC_DATA hsd = boost::value_initialized<CORE_SYNC_DATA>();
    get_payload_sync_data(hsd);
    epee::serialization::store_t_to_binary(hsd, data);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
    template<class t_core>
    int t_cryptonote_protocol_handler<t_core>::handle_notify_new_block(int command, NOTIFY_NEW_BLOCK::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_NEW_BLOCK (hop " << arg.hop << ", " << arg.b.txs.size() << " txes)");
    if(context.m_state != cryptonote_connection_context::state_normal)
      return 1;
    m_core.pause_mine();
    std::list<block_complete_entry> blocks;
    blocks.push_back(arg.b);
    m_core.prepare_handle_incoming_blocks(blocks);
    for(auto tx_blob_it = arg.b.txs.begin(); tx_blob_it!=arg.b.txs.end();tx_blob_it++)
    {
      cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      m_core.handle_incoming_tx(*tx_blob_it, tvc, true, true, false);
      if(tvc.m_verifivation_failed)
      {
        LOG_PRINT_CCONTEXT_L1("Block verification failed: transaction verification failed, dropping connection");
        m_p2p->drop_connection(context);
        m_core.cleanup_handle_incoming_blocks();
        m_core.resume_mine();
        return 1;
      }
    }

    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    m_core.handle_incoming_block(arg.b.block, bvc); // got block from handle_notify_new_block
    m_core.cleanup_handle_incoming_blocks(true);
    m_core.resume_mine();
    if(bvc.m_verifivation_failed)
    {
      LOG_PRINT_CCONTEXT_L0("Block verification failed, dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }
    if(bvc.m_added_to_main_chain)
    {
      ++arg.hop;
      //TODO: Add here announce protocol usage
      relay_block(arg, context);
    }else if(bvc.m_marked_as_orphaned)
    {
      context.m_state = cryptonote_connection_context::state_synchronizing;
      NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
      m_core.get_short_chain_history(r.block_ids);
      LOG_PRINT_CCONTEXT_L2("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() );
      post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
    }

    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_notify_new_fluffy_block(int command, NOTIFY_NEW_FLUFFY_BLOCK::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_NEW_FLUFFY_BLOCK (height " << arg.current_blockchain_height << ", hop " << arg.hop << ", " << arg.b.txs.size() << " txes)");
    if(context.m_state != cryptonote_connection_context::state_normal)
      return 1;
    
    m_core.pause_mine();
      
    block new_block;
    transaction miner_tx;
    if(parse_and_validate_block_from_blob(arg.b.block, new_block))
    {
      // This is a second notification, we must have asked for some missing tx
      if(!context.m_requested_objects.empty())
      {
        // What we asked for != to what we received ..
        if(context.m_requested_objects.size() != arg.b.txs.size())
        {
          LOG_ERROR_CCONTEXT
          (
            "NOTIFY_NEW_FLUFFY_BLOCK -> request/response mismatch, " 
            << "block = " << epee::string_tools::pod_to_hex(get_blob_hash(arg.b.block))
            << ", requested = " << context.m_requested_objects.size() 
            << ", received = " << new_block.tx_hashes.size()
            << ", dropping connection"
          );
          
          m_p2p->drop_connection(context);
          m_core.resume_mine();
          return 1;
        }
      }      
      
      std::list<blobdata> have_tx;
      
      // Instead of requesting missing transactions by hash like BTC, 
      // we do it by index (thanks to a suggestion from moneromooo) because
      // we're way cooler .. and also because they're smaller than hashes.
      // 
      // Also, remember to pepper some whitespace changes around to bother
      // moneromooo ... only because I <3 him. 
      std::vector<size_t> need_tx_indices;
        
      transaction tx;
      crypto::hash tx_hash;

      for(auto& tx_blob: arg.b.txs)
      {
        if(parse_and_validate_tx_from_blob(tx_blob, tx))
        {
          try
          {
            if(!get_transaction_hash(tx, tx_hash))
            {
              LOG_PRINT_CCONTEXT_L1
              (
                  "NOTIFY_NEW_FLUFFY_BLOCK: get_transaction_hash failed"
                  << ", dropping connection"
              );
              
              m_p2p->drop_connection(context);
              m_core.resume_mine();
              return 1;
            }
          }
          catch(...)
          {
            LOG_PRINT_CCONTEXT_L1
            (
                "NOTIFY_NEW_FLUFFY_BLOCK: get_transaction_hash failed"
                << ", exception thrown"
                << ", dropping connection"
            );
                        
            m_p2p->drop_connection(context);
            m_core.resume_mine();
            return 1;
          }
          
          // hijacking m_requested objects in connection context to patch up
          // a possible DOS vector pointed out by @monero-moo where peers keep
          // sending (0...n-1) transactions.
          // If requested objects is not empty, then we must have asked for 
          // some missing transacionts, make sure that they're all there.
          //
          // Can I safely re-use this field? I think so, but someone check me!
          if(!context.m_requested_objects.empty()) 
          {
            auto req_tx_it = context.m_requested_objects.find(tx_hash);
            if(req_tx_it == context.m_requested_objects.end())
            {
              LOG_ERROR_CCONTEXT
              (
                "Peer sent wrong transaction (NOTIFY_NEW_FLUFFY_BLOCK): "
                << "transaction with id = " << tx_hash << " wasn't requested, "
                << "dropping connection"
              );
              
              m_p2p->drop_connection(context);
              m_core.resume_mine();
              return 1;
            }
            
            context.m_requested_objects.erase(req_tx_it);
          }          
          
          // we might already have the tx that the peer
          // sent in our pool, so don't verify again..
          if(!m_core.get_pool_transaction(tx_hash, tx))
          {
            MDEBUG("Incoming tx " << tx_hash << " not in pool, adding");
            cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);                        
            if(!m_core.handle_incoming_tx(tx_blob, tvc, true, true, false) || tvc.m_verifivation_failed)
            {
              LOG_PRINT_CCONTEXT_L1("Block verification failed: transaction verification failed, dropping connection");
              m_p2p->drop_connection(context);
              m_core.resume_mine();
              return 1;
            }
            
            //
            // future todo: 
            // tx should only not be added to pool if verification failed, but
            // maybe in the future could not be added for other reasons 
            // according to monero-moo so keep track of these separately ..
            //
          }
        }
        else
        {
          LOG_ERROR_CCONTEXT
          (
            "sent wrong tx: failed to parse and validate transaction: "
            << epee::string_tools::buff_to_hex_nodelimer(tx_blob) 
            << ", dropping connection"
          );
            
          m_p2p->drop_connection(context);
          m_core.resume_mine();
          return 1;
        }
      }
      
      // The initial size equality check could have been fooled if the sender
      // gave us the number of transactions we asked for, but not the right 
      // ones. This check make sure the transactions we asked for were the
      // ones we received.
      if(context.m_requested_objects.size())
      {
        MERROR
        (
          "NOTIFY_NEW_FLUFFY_BLOCK: peer sent the number of transaction requested"
          << ", but not the actual transactions requested"
          << ", context.m_requested_objects.size() = " << context.m_requested_objects.size() 
          << ", dropping connection"
        );
        
        m_p2p->drop_connection(context);
        m_core.resume_mine();
        return 1;
      }      
      
      size_t tx_idx = 0;
      for(auto& tx_hash: new_block.tx_hashes)
      {
        if(m_core.get_pool_transaction(tx_hash, tx))
        {
          have_tx.push_back(tx_to_blob(tx));
        }
        else
        {
          std::vector<crypto::hash> tx_ids;
          std::list<transaction> txes;
          std::list<crypto::hash> missing;
          tx_ids.push_back(tx_hash);
          if (m_core.get_transactions(tx_ids, txes, missing) && missing.empty())
          {
            have_tx.push_back(tx_to_blob(tx));
          }
          else
          {
            MDEBUG("Tx " << tx_hash << " not found in pool");
            need_tx_indices.push_back(tx_idx);
          }
        }
        
        ++tx_idx;
      }
        
      if(!need_tx_indices.empty()) // drats, we don't have everything..
      {
        // request non-mempool txs
        MDEBUG("We are missing " << need_tx_indices.size() << " txes for this fluffy block");
        for (auto txidx: need_tx_indices)
          MDEBUG("  tx " << new_block.tx_hashes[txidx]);
        NOTIFY_REQUEST_FLUFFY_MISSING_TX::request missing_tx_req;
        missing_tx_req.block_hash = get_block_hash(new_block);
        missing_tx_req.hop = arg.hop;
        missing_tx_req.current_blockchain_height = arg.current_blockchain_height;
        missing_tx_req.missing_tx_indices = std::move(need_tx_indices);
        
        m_core.resume_mine();
        post_notify<NOTIFY_REQUEST_FLUFFY_MISSING_TX>(missing_tx_req, context);
      }
      else // whoo-hoo we've got em all ..
      {
        MDEBUG("We have all needed txes for this fluffy block");

        block_complete_entry b;
        b.block = arg.b.block;
        b.txs = have_tx;

        std::list<block_complete_entry> blocks;
        blocks.push_back(b);
        m_core.prepare_handle_incoming_blocks(blocks);
          
        block_verification_context bvc = boost::value_initialized<block_verification_context>();
        m_core.handle_incoming_block(arg.b.block, bvc); // got block from handle_notify_new_block
        m_core.cleanup_handle_incoming_blocks(true);
        m_core.resume_mine();
        
        if( bvc.m_verifivation_failed )
        {
          LOG_PRINT_CCONTEXT_L0("Block verification failed, dropping connection");
          m_p2p->drop_connection(context);
          return 1;
        }
        if( bvc.m_added_to_main_chain )
        {
          ++arg.hop;
          //TODO: Add here announce protocol usage
          NOTIFY_NEW_BLOCK::request reg_arg = AUTO_VAL_INIT(reg_arg);
          reg_arg.hop = arg.hop;
          reg_arg.current_blockchain_height = arg.current_blockchain_height;
          reg_arg.b.block = b.block;
          relay_block(reg_arg, context);
        }
        else if( bvc.m_marked_as_orphaned )
        {
          context.m_state = cryptonote_connection_context::state_synchronizing;
          NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
          m_core.get_short_chain_history(r.block_ids);
          LOG_PRINT_CCONTEXT_L2("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() );
          post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
        }            
      }
    } 
    else
    {
      LOG_ERROR_CCONTEXT
      (
        "sent wrong block: failed to parse and validate block: "
        << epee::string_tools::buff_to_hex_nodelimer(arg.b.block) 
        << ", dropping connection"
      );
        
      m_core.resume_mine();
      m_p2p->drop_connection(context);
        
      return 1;     
    }
        
    return 1;
  }  
  //------------------------------------------------------------------------------------------------------------------------  
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_request_fluffy_missing_tx(int command, NOTIFY_REQUEST_FLUFFY_MISSING_TX::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_REQUEST_FLUFFY_MISSING_TX (" << arg.missing_tx_indices.size() << " txes), block hash " << arg.block_hash);
    
    std::list<std::pair<cryptonote::blobdata, block>> local_blocks;
    std::list<cryptonote::blobdata> local_txs;

    block b;
    if (!m_core.get_block_by_hash(arg.block_hash, b))
    {
      LOG_ERROR_CCONTEXT("failed to find block: " << arg.block_hash << ", dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }

    for (auto txidx: arg.missing_tx_indices)
      MDEBUG("  tx " << b.tx_hashes[txidx]);

    std::vector<crypto::hash> txids;
    NOTIFY_NEW_FLUFFY_BLOCK::request fluffy_response;
    fluffy_response.b.block = t_serializable_object_to_blob(b);
    fluffy_response.current_blockchain_height = arg.current_blockchain_height;
    fluffy_response.hop = arg.hop;    
    for(auto& tx_idx: arg.missing_tx_indices)
    {
      if(tx_idx < b.tx_hashes.size())
      {
        txids.push_back(b.tx_hashes[tx_idx]);
      }
      else
      {
        LOG_ERROR_CCONTEXT
        (
          "Failed to handle request NOTIFY_REQUEST_FLUFFY_MISSING_TX"
          << ", request is asking for a tx whose index is out of bounds "
          << ", tx index = " << tx_idx << ", block tx count " << b.tx_hashes.size()
          << ", block_height = " << arg.current_blockchain_height
          << ", dropping connection"
        );
        
        m_p2p->drop_connection(context);
        return 1;
      }
    }    

    std::list<cryptonote::transaction> txs;
    std::list<crypto::hash> missed;
    if (!m_core.get_transactions(txids, txs, missed))
    {
      LOG_ERROR_CCONTEXT("Failed to handle request NOTIFY_REQUEST_FLUFFY_MISSING_TX, "
        << "failed to get requested transactions");
      m_p2p->drop_connection(context);
      return 1;
    }
    if (!missed.empty() || txs.size() != txids.size())
    {
      LOG_ERROR_CCONTEXT("Failed to handle request NOTIFY_REQUEST_FLUFFY_MISSING_TX, "
        << missed.size() << " requested transactions not found" << ", dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }

    for(auto& tx: txs)
    {
      fluffy_response.b.txs.push_back(t_serializable_object_to_blob(tx));
    }

    LOG_PRINT_CCONTEXT_L2
    (
        "-->>NOTIFY_RESPONSE_FLUFFY_MISSING_TX: " 
        << ", txs.size()=" << fluffy_response.b.txs.size()
        << ", rsp.current_blockchain_height=" << fluffy_response.current_blockchain_height
    );
           
    post_notify<NOTIFY_NEW_FLUFFY_BLOCK>(fluffy_response, context);    
    return 1;        
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_notify_new_transactions(int command, NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_NEW_TRANSACTIONS (" << arg.txs.size() << " txes)");
    if(context.m_state != cryptonote_connection_context::state_normal)
      return 1;

    for(auto tx_blob_it = arg.txs.begin(); tx_blob_it!=arg.txs.end();)
    {
      cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      m_core.handle_incoming_tx(*tx_blob_it, tvc, false, true, false);
      if(tvc.m_verifivation_failed)
      {
        LOG_PRINT_CCONTEXT_L1("Tx verification failed, dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }
      if(tvc.m_should_be_relayed)
        ++tx_blob_it;
      else
        arg.txs.erase(tx_blob_it++);
    }

    if(arg.txs.size())
    {
      //TODO: add announce usage here
      relay_transactions(arg, context);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_request_get_objects(int command, NOTIFY_REQUEST_GET_OBJECTS::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_REQUEST_GET_OBJECTS (" << arg.blocks.size() << " blocks, " << arg.txs.size() << " txes)");
    NOTIFY_RESPONSE_GET_OBJECTS::request rsp;
    if(!m_core.handle_get_objects(arg, rsp, context))
    {
      LOG_ERROR_CCONTEXT("failed to handle request NOTIFY_REQUEST_GET_OBJECTS, dropping connection");
      m_p2p->drop_connection(context);
    }
    LOG_PRINT_CCONTEXT_L2("-->>NOTIFY_RESPONSE_GET_OBJECTS: blocks.size()=" << rsp.blocks.size() << ", txs.size()=" << rsp.txs.size()
                            << ", rsp.m_current_blockchain_height=" << rsp.current_blockchain_height << ", missed_ids.size()=" << rsp.missed_ids.size());
    post_notify<NOTIFY_RESPONSE_GET_OBJECTS>(rsp, context);
    //handler_response_blocks_now(sizeof(rsp)); // XXX
    //handler_response_blocks_now(200);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------


  template<class t_core>
  double t_cryptonote_protocol_handler<t_core>::get_avg_block_size() {
    // return m_core.get_blockchain_storage().get_avg_block_size(count); // this does not count too well the actuall network-size of data we need to download

    CRITICAL_REGION_LOCAL(m_buffer_mutex);
    double avg = 0;
    if (m_avg_buffer.size() == 0) {
      _warn("m_avg_buffer.size() == 0");
      return 500;
    }

    const bool dbg_poke_lock = 0; // debug: try to trigger an error by poking around with locks. TODO: configure option
    long int dbg_repeat=0;
    do {
      for (auto element : m_avg_buffer) avg += element;
    } while(dbg_poke_lock && (dbg_repeat++)<100000); // in debug/poke mode, repeat this calculation to trigger hidden locking error if there is one
    return avg / m_avg_buffer.size();
  }


  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_response_get_objects(int command, NOTIFY_RESPONSE_GET_OBJECTS::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_RESPONSE_GET_OBJECTS (" << arg.blocks.size() << " blocks, " << arg.txs.size() << " txes)");

    // calculate size of request - mainly for logging/debug
    size_t size = 0;
    for (auto element : arg.txs) size += element.size();

    for (auto element : arg.blocks) {
      size += element.block.size();
      for (auto tx : element.txs)
        size += tx.size();
    }

    for (auto element : arg.missed_ids)
      size += sizeof(element.data);

    size += sizeof(arg.current_blockchain_height);
    {
      CRITICAL_REGION_LOCAL(m_buffer_mutex);
      m_avg_buffer.push_back(size);

      const bool dbg_poke_lock = 0; // debug: try to trigger an error by poking around with locks. TODO: configure option
      long int dbg_repeat=0;
      do {
        m_avg_buffer.push_back(666); // a test value
        m_avg_buffer.erase_end(1);
      } while(dbg_poke_lock && (dbg_repeat++)<100000); // in debug/poke mode, repeat this calculation to trigger hidden locking error if there is one
    }
    /*using namespace boost::chrono;
      auto point = steady_clock::now();
      auto time_from_epoh = point.time_since_epoch();
      auto sec = duration_cast< seconds >( time_from_epoh ).count();*/

    //epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());

    if(context.m_last_response_height > arg.current_blockchain_height)
    {
      LOG_ERROR_CCONTEXT("sent wrong NOTIFY_HAVE_OBJECTS: arg.m_current_blockchain_height=" << arg.current_blockchain_height
        << " < m_last_response_height=" << context.m_last_response_height << ", dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }

    context.m_remote_blockchain_height = arg.current_blockchain_height;

    size_t count = 0;
    std::vector<crypto::hash> block_hashes;
    block_hashes.reserve(arg.blocks.size());
    for(const block_complete_entry& block_entry: arg.blocks)
    {
      if (m_stopping)
      {
        return 1;
      }

      ++count;
      block b;
      if(!parse_and_validate_block_from_blob(block_entry.block, b))
      {
        LOG_ERROR_CCONTEXT("sent wrong block: failed to parse and validate block: "
          << epee::string_tools::buff_to_hex_nodelimer(block_entry.block) << ", dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }
      //to avoid concurrency in core between connections, suspend connections which delivered block later then first one
      const crypto::hash block_hash = get_block_hash(b);
      if(count == 2)
      {
        if(m_core.have_block(block_hash))
        {
          context.m_state = cryptonote_connection_context::state_idle;
          context.m_needed_objects.clear();
          context.m_requested_objects.clear();
          LOG_PRINT_CCONTEXT_L1("Connection set to idle state.");
          return 1;
        }
      }

      auto req_it = context.m_requested_objects.find(block_hash);
      if(req_it == context.m_requested_objects.end())
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << epee::string_tools::pod_to_hex(get_blob_hash(block_entry.block))
          << " wasn't requested, dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }
      if(b.tx_hashes.size() != block_entry.txs.size())
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << epee::string_tools::pod_to_hex(get_blob_hash(block_entry.block))
          << ", tx_hashes.size()=" << b.tx_hashes.size() << " mismatch with block_complete_entry.m_txs.size()=" << block_entry.txs.size() << ", dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }

      context.m_requested_objects.erase(req_it);
      block_hashes.push_back(block_hash);
    }

    if(context.m_requested_objects.size())
    {
      MERROR("returned not all requested objects (context.m_requested_objects.size()="
        << context.m_requested_objects.size() << "), dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }


    {
      m_core.pause_mine();
      epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler(
        boost::bind(&t_core::resume_mine, &m_core));

      MLOG_YELLOW(el::Level::Debug, "Got NEW BLOCKS inside of " << __FUNCTION__ << ": size: " << arg.blocks.size());

      if (m_core.get_test_drop_download() && m_core.get_test_drop_download_height()) { // DISCARD BLOCKS for testing

        uint64_t previous_height = m_core.get_current_blockchain_height();

        // we lock all the rest to avoid having multiple connections redo a lot
        // of the same work, and one of them doing it for nothing: subsequent
        // connections will wait until the current one's added its blocks, then
        // will add any extra it has, if any
        CRITICAL_REGION_LOCAL(m_sync_lock);

        // dismiss what another connection might already have done (likely everything)
        uint64_t top_height;
        crypto::hash top_hash;
        if (m_core.get_blockchain_top(top_height, top_hash)) {
          uint64_t dismiss = 1;
          for (const auto &h: block_hashes) {
            if (top_hash == h) {
              LOG_DEBUG_CC(context, "Found current top block in synced blocks, dismissing "
                  << dismiss << "/" << arg.blocks.size() << " blocks");
              while (dismiss--)
                arg.blocks.pop_front();
              break;
            }
            ++dismiss;
          }
        }

        m_core.prepare_handle_incoming_blocks(arg.blocks);

        for(const block_complete_entry& block_entry: arg.blocks)
        {
          if (m_stopping)
          {
              m_core.cleanup_handle_incoming_blocks();
              return 1;
          }

          // process transactions
          TIME_MEASURE_START(transactions_process_time);
          for(auto& tx_blob: block_entry.txs)
          {
            tx_verification_context tvc = AUTO_VAL_INIT(tvc);
            m_core.handle_incoming_tx(tx_blob, tvc, true, true, false);
            if(tvc.m_verifivation_failed)
            {
              LOG_ERROR_CCONTEXT("transaction verification failed on NOTIFY_RESPONSE_GET_OBJECTS, tx_id = "
                  << epee::string_tools::pod_to_hex(get_blob_hash(tx_blob)) << ", dropping connection");
              m_p2p->drop_connection(context);
              m_core.cleanup_handle_incoming_blocks();
              return 1;
            }
          }
          TIME_MEASURE_FINISH(transactions_process_time);

          // process block

          TIME_MEASURE_START(block_process_time);
          block_verification_context bvc = boost::value_initialized<block_verification_context>();

          m_core.handle_incoming_block(block_entry.block, bvc, false); // <--- process block

          if(bvc.m_verifivation_failed)
          {
            LOG_PRINT_CCONTEXT_L1("Block verification failed, dropping connection");
            m_p2p->drop_connection(context);
            m_p2p->add_ip_fail(context.m_remote_ip);
            m_core.cleanup_handle_incoming_blocks();
            return 1;
          }
          if(bvc.m_marked_as_orphaned)
          {
            LOG_PRINT_CCONTEXT_L1("Block received at sync phase was marked as orphaned, dropping connection");
            m_p2p->drop_connection(context);
            m_p2p->add_ip_fail(context.m_remote_ip);
            m_core.cleanup_handle_incoming_blocks();
            return 1;
          }

          TIME_MEASURE_FINISH(block_process_time);
          LOG_PRINT_CCONTEXT_L2("Block process time: " << block_process_time + transactions_process_time << "(" << transactions_process_time << "/" << block_process_time << ")ms");

        } // each download block
        m_core.cleanup_handle_incoming_blocks();

        if (m_core.get_current_blockchain_height() > previous_height)
        {
          MGINFO_YELLOW(context << " Synced " << m_core.get_current_blockchain_height() << "/" << m_core.get_target_blockchain_height());
        }
      } // if not DISCARD BLOCK


    }
    request_missing_objects(context, true);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::on_idle()
  {
    return m_core.on_idle();
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_request_chain(int command, NOTIFY_REQUEST_CHAIN::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_REQUEST_CHAIN (" << arg.block_ids.size() << " blocks");
    NOTIFY_RESPONSE_CHAIN_ENTRY::request r;
    if(!m_core.find_blockchain_supplement(arg.block_ids, r))
    {
      LOG_ERROR_CCONTEXT("Failed to handle NOTIFY_REQUEST_CHAIN.");
      m_p2p->drop_connection(context);
      return 1;
    }
    LOG_PRINT_CCONTEXT_L2("-->>NOTIFY_RESPONSE_CHAIN_ENTRY: m_start_height=" << r.start_height << ", m_total_height=" << r.total_height << ", m_block_ids.size()=" << r.m_block_ids.size());
    post_notify<NOTIFY_RESPONSE_CHAIN_ENTRY>(r, context);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::request_missing_objects(cryptonote_connection_context& context, bool check_having_blocks)
  {
    //if (!m_one_request == false)
      //return true;
    m_one_request = false;
    // save request size to log (dr monero)
    /*using namespace boost::chrono;
      auto point = steady_clock::now();
      auto time_from_epoh = point.time_since_epoch();
      auto sec = duration_cast< seconds >( time_from_epoh ).count();*/

    if(context.m_needed_objects.size())
    {
      //we know objects that we need, request this objects
      NOTIFY_REQUEST_GET_OBJECTS::request req;
      size_t count = 0;
      auto it = context.m_needed_objects.begin();

      const size_t count_limit = m_core.get_block_sync_size();
      MDEBUG("Setting count_limit: " << count_limit);
      while(it != context.m_needed_objects.end() && count < count_limit)
      {
        if( !(check_having_blocks && m_core.have_block(*it)))
        {
          req.blocks.push_back(*it);
          ++count;
          context.m_requested_objects.insert(*it);
        }
        context.m_needed_objects.erase(it++);
      }
      LOG_PRINT_CCONTEXT_L1("-->>NOTIFY_REQUEST_GET_OBJECTS: blocks.size()=" << req.blocks.size() << ", txs.size()=" << req.txs.size()
          << "requested blocks count=" << count << " / " << count_limit);
      //epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());

      post_notify<NOTIFY_REQUEST_GET_OBJECTS>(req, context);
    }else if(context.m_last_response_height < context.m_remote_blockchain_height-1)
    {//we have to fetch more objects ids, request blockchain entry

      NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
      m_core.get_short_chain_history(r.block_ids);
      handler_request_blocks_history( r.block_ids ); // change the limit(?), sleep(?)

      //std::string blob; // for calculate size of request
      //epee::serialization::store_t_to_binary(r, blob);
      //epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());
      LOG_PRINT_CCONTEXT_L1("r = " << 200);

      LOG_PRINT_CCONTEXT_L1("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() );
      post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
    }else
    {
      CHECK_AND_ASSERT_MES(context.m_last_response_height == context.m_remote_blockchain_height-1
                           && !context.m_needed_objects.size()
                           && !context.m_requested_objects.size(), false, "request_missing_blocks final condition failed!"
                           << "\r\nm_last_response_height=" << context.m_last_response_height
                           << "\r\nm_remote_blockchain_height=" << context.m_remote_blockchain_height
                           << "\r\nm_needed_objects.size()=" << context.m_needed_objects.size()
                           << "\r\nm_requested_objects.size()=" << context.m_requested_objects.size()
                           << "\r\non connection [" << epee::net_utils::print_connection_context_short(context)<< "]");

      context.m_state = cryptonote_connection_context::state_normal;
      MGINFO_GREEN("SYNCHRONIZED OK");
      on_connection_synchronized();
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::on_connection_synchronized()
  {
    bool val_expected = false;
    if(m_synchronized.compare_exchange_strong(val_expected, true))
    {
      MGINFO_YELLOW(ENDL << "**********************************************************************" << ENDL
        << "You are now synchronized with the network. You may now start monero-wallet-cli." << ENDL
        << ENDL
        << "Use the \"help\" command to see the list of available commands." << ENDL
        << "**********************************************************************");
      m_core.on_synchronized();
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  size_t t_cryptonote_protocol_handler<t_core>::get_synchronizing_connections_count()
  {
    size_t count = 0;
    m_p2p->for_each_connection([&](cryptonote_connection_context& context, nodetool::peerid_type peer_id)->bool{
      if(context.m_state == cryptonote_connection_context::state_synchronizing)
        ++count;
      return true;
    });
    return count;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_response_chain_entry(int command, NOTIFY_RESPONSE_CHAIN_ENTRY::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_RESPONSE_CHAIN_ENTRY: m_block_ids.size()=" << arg.m_block_ids.size()
      << ", m_start_height=" << arg.start_height << ", m_total_height=" << arg.total_height);

    if(!arg.m_block_ids.size())
    {
      LOG_ERROR_CCONTEXT("sent empty m_block_ids, dropping connection");
      m_p2p->drop_connection(context);
      m_p2p->add_ip_fail(context.m_remote_ip);
      return 1;
    }

    if(!m_core.have_block(arg.m_block_ids.front()))
    {
      LOG_ERROR_CCONTEXT("sent m_block_ids starting from unknown id: "
                                              << epee::string_tools::pod_to_hex(arg.m_block_ids.front()) << " , dropping connection");
      m_p2p->drop_connection(context);
      m_p2p->add_ip_fail(context.m_remote_ip);
      return 1;
    }

    context.m_remote_blockchain_height = arg.total_height;
    context.m_last_response_height = arg.start_height + arg.m_block_ids.size()-1;
    if(context.m_last_response_height > context.m_remote_blockchain_height)
    {
      LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_CHAIN_ENTRY, with m_total_height=" << arg.total_height
                                                                         << ", m_start_height=" << arg.start_height
                                                                         << ", m_block_ids.size()=" << arg.m_block_ids.size());
      m_p2p->drop_connection(context);
    }

    for(auto& bl_id: arg.m_block_ids)
    {
      if(!m_core.have_block(bl_id))
        context.m_needed_objects.push_back(bl_id);
    }

    request_missing_objects(context, false);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::relay_block(NOTIFY_NEW_BLOCK::request& arg, cryptonote_connection_context& exclude_context)
  {
    NOTIFY_NEW_FLUFFY_BLOCK::request fluffy_arg = AUTO_VAL_INIT(fluffy_arg);
    fluffy_arg.hop = arg.hop;
    fluffy_arg.current_blockchain_height = arg.current_blockchain_height;    
    std::list<blobdata> fluffy_txs;
    fluffy_arg.b = arg.b;
    fluffy_arg.b.txs = fluffy_txs;

    // pre-serialize them
    std::string fullBlob, fluffyBlob;
    epee::serialization::store_t_to_binary(arg, fullBlob);
    epee::serialization::store_t_to_binary(fluffy_arg, fluffyBlob);

    // sort peers between fluffy ones and others
    std::list<boost::uuids::uuid> fullConnections, fluffyConnections;
    m_p2p->for_each_connection([this, &arg, &fluffy_arg, &exclude_context, &fullConnections, &fluffyConnections](connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)
    {
      if (peer_id && exclude_context.m_connection_id != context.m_connection_id)
      {
        if(m_core.get_testnet() && (support_flags & P2P_SUPPORT_FLAG_FLUFFY_BLOCKS))
        {
          LOG_DEBUG_CC(context, "PEER SUPPORTS FLUFFY BLOCKS - RELAYING THIN/COMPACT WHATEVER BLOCK");
          fluffyConnections.push_back(context.m_connection_id);
        }
        else
        {
          LOG_DEBUG_CC(context, "PEER DOESN'T SUPPORT FLUFFY BLOCKS - RELAYING FULL BLOCK");
          fullConnections.push_back(context.m_connection_id);
        }
      }
      return true;
    });

    // send fluffy ones first, we want to encourage people to run that
    m_p2p->relay_notify_to_list(NOTIFY_NEW_FLUFFY_BLOCK::ID, fluffyBlob, fluffyConnections);
    m_p2p->relay_notify_to_list(NOTIFY_NEW_BLOCK::ID, fullBlob, fullConnections);

    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::relay_transactions(NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& exclude_context)
  {
    // no check for success, so tell core they're relayed unconditionally
    for(auto tx_blob_it = arg.txs.begin(); tx_blob_it!=arg.txs.end(); ++tx_blob_it)
      m_core.on_transaction_relayed(*tx_blob_it);
    return relay_post_notify<NOTIFY_NEW_TRANSACTIONS>(arg, exclude_context);
  }

  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_cryptonote_protocol_handler<t_core>::stop()
  {
    m_stopping = true;
    m_core.stop();
  }
} // namespace
