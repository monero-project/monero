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

// (may contain code and/or modifications by other developers)
// developer rfree: this code is caller of our new network code, and is modded; e.g. for rate limiting

#include <boost/interprocess/detail/atomic.hpp>
#include <list>
#include <unordered_map>

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "profile_tools.h"
#include "p2p/network_throttle-detail.hpp"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.cn"

#define MLOG_P2P_MESSAGE(x) MCINFO("net.p2p.msg", context << x)

#define BLOCK_QUEUE_NBLOCKS_THRESHOLD 10 // chunks of N blocks
#define BLOCK_QUEUE_SIZE_THRESHOLD (100*1024*1024) // MB
#define REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD (5 * 1000000) // microseconds
#define IDLE_PEER_KICK_TIME (45 * 1000000) // microseconds

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
    else if(context.m_state == cryptonote_connection_context::state_standby)
    {
      context.m_state = cryptonote_connection_context::state_synchronizing;
      try_add_next_blocks(context);
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

    m_p2p->for_each_connection([&](const connection_context& cntxt, nodetool::peerid_type peer_id, uint32_t support_flags)
    {
      bool local_ip = cntxt.m_remote_address.is_local();
      auto connection_time = time(NULL) - cntxt.m_started;
      ss << std::setw(30) << std::left << std::string(cntxt.m_is_income ? " [INC]":"[OUT]") +
        cntxt.m_remote_address.str()
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
        << std::left << (cntxt.m_remote_address.is_loopback() ? "[LOCALHOST]" : "") // 127.0.0.1
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

      cnx.address = cntxt.m_remote_address.str();
      cnx.host = cntxt.m_remote_address.host_str();
      cnx.ip = "";
      cnx.port = "";
      if (cntxt.m_remote_address.get_type_id() == epee::net_utils::ipv4_network_address::ID)
      {
        cnx.ip = cnx.host;
        cnx.port = std::to_string(cntxt.m_remote_address.as<epee::net_utils::ipv4_network_address>().port());
      }

      std::stringstream peer_id_str;
      peer_id_str << std::hex << std::setw(16) << peer_id;
      peer_id_str >> cnx.peer_id;
      
      cnx.support_flags = support_flags;

      cnx.recv_count = cntxt.m_recv_cnt;
      cnx.recv_idle_time = timestamp - std::max(cntxt.m_started, cntxt.m_last_recv);

      cnx.send_count = cntxt.m_send_cnt;
      cnx.send_idle_time = timestamp - std::max(cntxt.m_started, cntxt.m_last_send);

      cnx.state = get_protocol_state_string(cntxt.m_state);

      cnx.live_time = timestamp - cntxt.m_started;

      cnx.localhost = cntxt.m_remote_address.is_loopback();
      cnx.local_ip = cntxt.m_remote_address.is_local();

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

      cnx.connection_id = cntxt.m_connection_id;

      cnx.height = cntxt.m_remote_blockchain_height;

      connections.push_back(cnx);

      return true;
    });

    return connections;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::process_payload_sync_data(const CORE_SYNC_DATA& hshd, cryptonote_connection_context& context, bool is_inital)
  {
    if(context.m_state == cryptonote_connection_context::state_before_handshake && !is_inital)
      return true;

    if(context.m_state == cryptonote_connection_context::state_synchronizing)
      return true;

    // from v6, if the peer advertises a top block version, reject if it's not what it should be (will only work if no voting)
    const uint8_t version = m_core.get_ideal_hard_fork_version(hshd.current_height - 1);
    if (version >= 6 && version != hshd.top_version)
    {
      if (version < hshd.top_version)
        MCLOG_RED(el::Level::Warning, "global", context << " peer claims higher version that we think - we may be forked from the network and a software upgrade may be needed");
      LOG_DEBUG_CC(context, "Ignoring due to wrong top version for block " << (hshd.current_height - 1) << ": " << (unsigned)hshd.top_version << ", expected " << (unsigned)version);
      return false;
    }

    context.m_remote_blockchain_height = hshd.current_height;

    uint64_t target = m_core.get_target_blockchain_height();
    if (target == 0)
      target = m_core.get_current_blockchain_height();

    if(m_core.have_block(hshd.top_id))
    {
      context.m_state = cryptonote_connection_context::state_normal;
      if(is_inital && target == m_core.get_current_blockchain_height())
        on_connection_synchronized();
      return true;
    }

    if (hshd.current_height > target)
    {
    /* As I don't know if accessing hshd from core could be a good practice,
    I prefer pushing target height to the core at the same time it is pushed to the user.
    Nz. */
    m_core.set_target_blockchain_height((hshd.current_height));
    int64_t diff = static_cast<int64_t>(hshd.current_height) - static_cast<int64_t>(m_core.get_current_blockchain_height());
    uint64_t abs_diff = std::abs(diff);
    uint64_t max_block_height = max(hshd.current_height,m_core.get_current_blockchain_height());
    uint64_t last_block_v1 = m_core.get_testnet() ? 624633 : 1009826;
    uint64_t diff_v2 = max_block_height > last_block_v1 ? min(abs_diff, max_block_height - last_block_v1) : 0;
    MCLOG(is_inital ? el::Level::Info : el::Level::Debug, "global", context <<  "Sync data returned a new top block candidate: " << m_core.get_current_blockchain_height() << " -> " << hshd.current_height
      << " [Your node is " << abs_diff << " blocks (" << ((abs_diff - diff_v2) / (24 * 60 * 60 / DIFFICULTY_TARGET_V1)) + (diff_v2 / (24 * 60 * 60 / DIFFICULTY_TARGET_V2)) << " days) "
      << (0 <= diff ? std::string("behind") : std::string("ahead"))
      << "] " << ENDL << "SYNCHRONIZATION started");
      m_core.safesyncmode(false);
    }
    LOG_PRINT_L1("Remote blockchain height: " << hshd.current_height << ", id: " << hshd.top_id);
    context.m_state = cryptonote_connection_context::state_synchronizing;
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
    hshd.top_version = m_core.get_ideal_hard_fork_version(hshd.current_height);
    hshd.cumulative_difficulty = m_core.get_block_cumulative_difficulty(hshd.current_height);
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
    if(!is_synchronized()) // can happen if a peer connection goes to normal but another thread still hasn't finished adding queued blocks
    {
      LOG_DEBUG_CC(context, "Received new block while syncing, ignored");
      return 1;
    }
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
        drop_connection(context, false, false);
        m_core.cleanup_handle_incoming_blocks();
        m_core.resume_mine();
        return 1;
      }
    }

    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    m_core.handle_incoming_block(arg.b.block, bvc); // got block from handle_notify_new_block
    if (!m_core.cleanup_handle_incoming_blocks(true))
    {
      LOG_PRINT_CCONTEXT_L0("Failure in cleanup_handle_incoming_blocks");
      m_core.resume_mine();
      return 1;
    }
    m_core.resume_mine();
    if(bvc.m_verifivation_failed)
    {
      LOG_PRINT_CCONTEXT_L0("Block verification failed, dropping connection");
      drop_connection(context, true, false);
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
    if(!is_synchronized()) // can happen if a peer connection goes to normal but another thread still hasn't finished adding queued blocks
    {
      LOG_DEBUG_CC(context, "Received new block while syncing, ignored");
      return 1;
    }
    
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
          
          drop_connection(context, false, false);
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
              
              drop_connection(context, false, false);
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
                        
            drop_connection(context, false, false);
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
              
              drop_connection(context, false, false);
              m_core.resume_mine();
              return 1;
            }
            
            context.m_requested_objects.erase(req_tx_it);
          }          
          
          // we might already have the tx that the peer
          // sent in our pool, so don't verify again..
          if(!m_core.pool_has_tx(tx_hash))
          {
            MDEBUG("Incoming tx " << tx_hash << " not in pool, adding");
            cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);                        
            if(!m_core.handle_incoming_tx(tx_blob, tvc, true, true, false) || tvc.m_verifivation_failed)
            {
              LOG_PRINT_CCONTEXT_L1("Block verification failed: transaction verification failed, dropping connection");
              drop_connection(context, false, false);
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
            
          drop_connection(context, false, false);
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
        
        drop_connection(context, false, false);
        m_core.resume_mine();
        return 1;
      }      
      
      size_t tx_idx = 0;
      for(auto& tx_hash: new_block.tx_hashes)
      {
        cryptonote::blobdata txblob;
        if(m_core.get_pool_transaction(tx_hash, txblob))
        {
          have_tx.push_back(txblob);
        }
        else
        {
          std::vector<crypto::hash> tx_ids;
          std::list<transaction> txes;
          std::list<crypto::hash> missing;
          tx_ids.push_back(tx_hash);
          if (m_core.get_transactions(tx_ids, txes, missing) && missing.empty())
          {
            if (txes.size() == 1)
            {
              have_tx.push_back(tx_to_blob(txes.front()));
            }
            else
            {
              MERROR("1 tx requested, none not found, but " << txes.size() << " returned");
              m_core.resume_mine();
              return 1;
            }
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
        if (!m_core.cleanup_handle_incoming_blocks(true))
        {
          LOG_PRINT_CCONTEXT_L0("Failure in cleanup_handle_incoming_blocks");
          m_core.resume_mine();
          return 1;
        }
        m_core.resume_mine();
        
        if( bvc.m_verifivation_failed )
        {
          LOG_PRINT_CCONTEXT_L0("Block verification failed, dropping connection");
          drop_connection(context, true, false);
          return 1;
        }
        if( bvc.m_added_to_main_chain )
        {
          ++arg.hop;
          //TODO: Add here announce protocol usage
          NOTIFY_NEW_BLOCK::request reg_arg = AUTO_VAL_INIT(reg_arg);
          reg_arg.hop = arg.hop;
          reg_arg.current_blockchain_height = arg.current_blockchain_height;
          reg_arg.b = b;
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
      drop_connection(context, false, false);
        
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
      drop_connection(context, false, false);
      return 1;
    }

    std::vector<crypto::hash> txids;
    NOTIFY_NEW_FLUFFY_BLOCK::request fluffy_response;
    fluffy_response.b.block = t_serializable_object_to_blob(b);
    fluffy_response.current_blockchain_height = arg.current_blockchain_height;
    fluffy_response.hop = arg.hop;    
    for(auto& tx_idx: arg.missing_tx_indices)
    {
      if(tx_idx < b.tx_hashes.size())
      {
        MDEBUG("  tx " << b.tx_hashes[tx_idx]);
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
        
        drop_connection(context, false, false);
        return 1;
      }
    }    

    std::list<cryptonote::transaction> txs;
    std::list<crypto::hash> missed;
    if (!m_core.get_transactions(txids, txs, missed))
    {
      LOG_ERROR_CCONTEXT("Failed to handle request NOTIFY_REQUEST_FLUFFY_MISSING_TX, "
        << "failed to get requested transactions");
      drop_connection(context, false, false);
      return 1;
    }
    if (!missed.empty() || txs.size() != txids.size())
    {
      LOG_ERROR_CCONTEXT("Failed to handle request NOTIFY_REQUEST_FLUFFY_MISSING_TX, "
        << missed.size() << " requested transactions not found" << ", dropping connection");
      drop_connection(context, false, false);
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

    // while syncing, core will lock for a long time, so we ignore
    // those txes as they aren't really needed anyway, and avoid a
    // long block before replying
    if(!is_synchronized())
    {
      LOG_DEBUG_CC(context, "Received new tx while syncing, ignored");
      return 1;
    }

    for(auto tx_blob_it = arg.txs.begin(); tx_blob_it!=arg.txs.end();)
    {
      cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      m_core.handle_incoming_tx(*tx_blob_it, tvc, false, true, false);
      if(tvc.m_verifivation_failed)
      {
        LOG_PRINT_CCONTEXT_L1("Tx verification failed, dropping connection");
        drop_connection(context, false, false);
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
      drop_connection(context, false, false);
      return 1;
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
  double t_cryptonote_protocol_handler<t_core>::get_avg_block_size()
  {
    CRITICAL_REGION_LOCAL(m_buffer_mutex);
    if (m_avg_buffer.empty()) {
      MWARNING("m_avg_buffer.size() == 0");
      return 500;
    }
    double avg = 0;
    for (const auto &element : m_avg_buffer) avg += element;
    return avg / m_avg_buffer.size();
  }


  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_response_get_objects(int command, NOTIFY_RESPONSE_GET_OBJECTS::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_RESPONSE_GET_OBJECTS (" << arg.blocks.size() << " blocks, " << arg.txs.size() << " txes)");

    // calculate size of request
    size_t size = 0;
    for (const auto &element : arg.txs) size += element.size();

    size_t blocks_size = 0;
    for (const auto &element : arg.blocks) {
      blocks_size += element.block.size();
      for (const auto &tx : element.txs)
        blocks_size += tx.size();
    }
    size += blocks_size;

    for (const auto &element : arg.missed_ids)
      size += sizeof(element.data);

    size += sizeof(arg.current_blockchain_height);
    {
      CRITICAL_REGION_LOCAL(m_buffer_mutex);
      m_avg_buffer.push_back(size);
    }
    MDEBUG(context << " downloaded " << size << " bytes worth of blocks");

    /*using namespace boost::chrono;
      auto point = steady_clock::now();
      auto time_from_epoh = point.time_since_epoch();
      auto sec = duration_cast< seconds >( time_from_epoh ).count();*/

    //epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());

    if(context.m_last_response_height > arg.current_blockchain_height)
    {
      LOG_ERROR_CCONTEXT("sent wrong NOTIFY_HAVE_OBJECTS: arg.m_current_blockchain_height=" << arg.current_blockchain_height
        << " < m_last_response_height=" << context.m_last_response_height << ", dropping connection");
      drop_connection(context, false, false);
      return 1;
    }

    context.m_remote_blockchain_height = arg.current_blockchain_height;

    std::vector<crypto::hash> block_hashes;
    block_hashes.reserve(arg.blocks.size());
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    uint64_t start_height = std::numeric_limits<uint64_t>::max();
    cryptonote::block b;
    for(const block_complete_entry& block_entry: arg.blocks)
    {
      if (m_stopping)
      {
        return 1;
      }

      if(!parse_and_validate_block_from_blob(block_entry.block, b))
      {
        LOG_ERROR_CCONTEXT("sent wrong block: failed to parse and validate block: "
          << epee::string_tools::buff_to_hex_nodelimer(block_entry.block) << ", dropping connection");
        drop_connection(context, false, false);
        return 1;
      }
      if (b.miner_tx.vin.size() != 1 || b.miner_tx.vin.front().type() != typeid(txin_gen))
      {
        LOG_ERROR_CCONTEXT("sent wrong block: block: miner tx does not have exactly one txin_gen input"
          << epee::string_tools::buff_to_hex_nodelimer(block_entry.block) << ", dropping connection");
        drop_connection(context, false, false);
        return 1;
      }
      if (start_height == std::numeric_limits<uint64_t>::max())
        start_height = boost::get<txin_gen>(b.miner_tx.vin[0]).height;

      const crypto::hash block_hash = get_block_hash(b);
      auto req_it = context.m_requested_objects.find(block_hash);
      if(req_it == context.m_requested_objects.end())
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << epee::string_tools::pod_to_hex(get_blob_hash(block_entry.block))
          << " wasn't requested, dropping connection");
        drop_connection(context, false, false);
        return 1;
      }
      if(b.tx_hashes.size() != block_entry.txs.size())
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << epee::string_tools::pod_to_hex(get_blob_hash(block_entry.block))
          << ", tx_hashes.size()=" << b.tx_hashes.size() << " mismatch with block_complete_entry.m_txs.size()=" << block_entry.txs.size() << ", dropping connection");
        drop_connection(context, false, false);
        return 1;
      }

      context.m_requested_objects.erase(req_it);
      block_hashes.push_back(block_hash);
    }

    if(context.m_requested_objects.size())
    {
      MERROR("returned not all requested objects (context.m_requested_objects.size()="
        << context.m_requested_objects.size() << "), dropping connection");
      drop_connection(context, false, false);
      return 1;
    }

    // get the last parsed block, which should be the highest one
    if(m_core.have_block(cryptonote::get_block_hash(b)))
    {
      const uint64_t subchain_height = start_height + arg.blocks.size();
      LOG_DEBUG_CC(context, "These are old blocks, ignoring: blocks " << start_height << " - " << (subchain_height-1) << ", blockchain height " << m_core.get_current_blockchain_height());
      m_block_queue.remove_spans(context.m_connection_id, start_height);
      goto skip;
    }

    {
      MLOG_YELLOW(el::Level::Debug, context << " Got NEW BLOCKS inside of " << __FUNCTION__ << ": size: " << arg.blocks.size()
          << ", blocks: " << start_height << " - " << (start_height + arg.blocks.size() - 1));

      // add that new span to the block queue
      const boost::posix_time::time_duration dt = now - context.m_last_request_time;
      const float rate = size * 1e6 / (dt.total_microseconds() + 1);
      MDEBUG(context << " adding span: " << arg.blocks.size() << " at height " << start_height << ", " << dt.total_microseconds()/1e6 << " seconds, " << (rate/1e3) << " kB/s, size now " << (m_block_queue.get_data_size() + blocks_size) / 1048576.f << " MB");
      m_block_queue.add_blocks(start_height, arg.blocks, context.m_connection_id, rate, blocks_size);

      context.m_last_known_hash = cryptonote::get_blob_hash(arg.blocks.back().block);

      if (!m_core.get_test_drop_download() || !m_core.get_test_drop_download_height()) { // DISCARD BLOCKS for testing
        return 1;
      }
    }

skip:
    try_add_next_blocks(context);
    return 1;
  }

  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::try_add_next_blocks(cryptonote_connection_context& context)
  {
    bool force_next_span = false;

    {
      // We try to lock the sync lock. If we can, it means no other thread is
      // currently adding blocks, so we do that for as long as we can from the
      // block queue. Then, we go back to download.
      const boost::unique_lock<boost::mutex> sync{m_sync_lock, boost::try_to_lock};
      if (!sync.owns_lock())
      {
        MINFO("Failed to lock m_sync_lock, going back to download");
        goto skip;
      }
      MDEBUG(context << " lock m_sync_lock, adding blocks to chain...");

      {
        m_core.pause_mine();
        epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler(
          boost::bind(&t_core::resume_mine, &m_core));

        while (1)
        {
          const uint64_t previous_height = m_core.get_current_blockchain_height();
          uint64_t start_height;
          std::list<cryptonote::block_complete_entry> blocks;
          boost::uuids::uuid span_connection_id;
          if (!m_block_queue.get_next_span(start_height, blocks, span_connection_id))
          {
            MDEBUG(context << " no next span found, going back to download");
            break;
          }
          MDEBUG(context << " next span in the queue has blocks " << start_height << "-" << (start_height + blocks.size() - 1)
              << ", we need " << previous_height);


          block new_block;
          if (!parse_and_validate_block_from_blob(blocks.front().block, new_block))
          {
            MERROR("Failed to parse block, but it should already have been parsed");
            break;
          }
          bool parent_known = m_core.have_block(new_block.prev_id);
          if (!parent_known)
          {
            // it could be:
            //  - later in the current chain
            //  - later in an alt chain
            //  - orphan
            // if it was requested, then it'll be resolved later, otherwise it's an orphan
            bool parent_requested = m_block_queue.requested(new_block.prev_id);
            if (!parent_requested)
            {
              // this can happen if a connection was sicced onto a late span, if it did not have those blocks,
              // since we don't know that at the sic time
              LOG_ERROR_CCONTEXT("Got block with unknown parent which was not requested - querying block hashes");
              context.m_needed_objects.clear();
              context.m_last_response_height = 0;
              goto skip;
            }

            // parent was requested, so we wait for it to be retrieved
            MINFO(context << " parent was requested, we'll get back to it");
            break;
          }

          const boost::posix_time::ptime start = boost::posix_time::microsec_clock::universal_time();
          context.m_last_request_time = start;

          m_core.prepare_handle_incoming_blocks(blocks);

          uint64_t block_process_time_full = 0, transactions_process_time_full = 0;
          size_t num_txs = 0;
          for(const block_complete_entry& block_entry: blocks)
          {
            if (m_stopping)
            {
                m_core.cleanup_handle_incoming_blocks();
                return 1;
            }

            // process transactions
            TIME_MEASURE_START(transactions_process_time);
            num_txs += block_entry.txs.size();
            std::vector<tx_verification_context> tvc;
            m_core.handle_incoming_txs(block_entry.txs, tvc, true, true, false);
            std::list<blobdata>::const_iterator it = block_entry.txs.begin();
            for (size_t i = 0; i < tvc.size(); ++i, ++it)
            {
              if(tvc[i].m_verifivation_failed)
              {
                if (!m_p2p->for_connection(span_connection_id, [&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t f)->bool{
                  LOG_ERROR_CCONTEXT("transaction verification failed on NOTIFY_RESPONSE_GET_OBJECTS, tx_id = "
                      << epee::string_tools::pod_to_hex(get_blob_hash(*it)) << ", dropping connection");
                  drop_connection(context, false, true);
                  return true;
                }))
                  LOG_ERROR_CCONTEXT("span connection id not found");

                if (!m_core.cleanup_handle_incoming_blocks())
                {
                  LOG_PRINT_CCONTEXT_L0("Failure in cleanup_handle_incoming_blocks");
                  return 1;
                }
                // in case the peer had dropped beforehand, remove the span anyway so other threads can wake up and get it
                m_block_queue.remove_spans(span_connection_id, start_height);
                return 1;
              }
            }
            TIME_MEASURE_FINISH(transactions_process_time);
            transactions_process_time_full += transactions_process_time;

            // process block

            TIME_MEASURE_START(block_process_time);
            block_verification_context bvc = boost::value_initialized<block_verification_context>();

            m_core.handle_incoming_block(block_entry.block, bvc, false); // <--- process block

            if(bvc.m_verifivation_failed)
            {
              if (!m_p2p->for_connection(span_connection_id, [&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t f)->bool{
                LOG_PRINT_CCONTEXT_L1("Block verification failed, dropping connection");
                drop_connection(context, true, true);
                return true;
              }))
                LOG_ERROR_CCONTEXT("span connection id not found");

              if (!m_core.cleanup_handle_incoming_blocks())
              {
                LOG_PRINT_CCONTEXT_L0("Failure in cleanup_handle_incoming_blocks");
                return 1;
              }

              // in case the peer had dropped beforehand, remove the span anyway so other threads can wake up and get it
              m_block_queue.remove_spans(span_connection_id, start_height);
              return 1;
            }
            if(bvc.m_marked_as_orphaned)
            {
              if (!m_p2p->for_connection(span_connection_id, [&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t f)->bool{
                LOG_PRINT_CCONTEXT_L1("Block received at sync phase was marked as orphaned, dropping connection");
                drop_connection(context, true, true);
                return true;
              }))
                LOG_ERROR_CCONTEXT("span connection id not found");

              if (!m_core.cleanup_handle_incoming_blocks())
              {
                LOG_PRINT_CCONTEXT_L0("Failure in cleanup_handle_incoming_blocks");
                return 1;
              }

              // in case the peer had dropped beforehand, remove the span anyway so other threads can wake up and get it
              m_block_queue.remove_spans(span_connection_id, start_height);
              return 1;
            }

            TIME_MEASURE_FINISH(block_process_time);
            block_process_time_full += block_process_time;

          } // each download block

          MCINFO("sync-info", "Block process time (" << blocks.size() << " blocks, " << num_txs << " txs): " << block_process_time_full + transactions_process_time_full << " (" << transactions_process_time_full << "/" << block_process_time_full << ") ms");

          if (!m_core.cleanup_handle_incoming_blocks())
          {
            LOG_PRINT_CCONTEXT_L0("Failure in cleanup_handle_incoming_blocks");
            return 1;
          }

          m_block_queue.remove_spans(span_connection_id, start_height);

          if (m_core.get_current_blockchain_height() > previous_height)
          {
            const boost::posix_time::time_duration dt = boost::posix_time::microsec_clock::universal_time() - start;
            std::string timing_message = "";
            if (ELPP->vRegistry()->allowed(el::Level::Info, "sync-info"))
              timing_message = std::string(" (") + std::to_string(dt.total_microseconds()/1e6) + " sec, "
                + std::to_string((m_core.get_current_blockchain_height() - previous_height) * 1e6 / dt.total_microseconds())
                + " blocks/sec), " + std::to_string(m_block_queue.get_data_size() / 1048576.f) + " MB queued";
            if (ELPP->vRegistry()->allowed(el::Level::Debug, "sync-info"))
              timing_message += std::string(": ") + m_block_queue.get_overview();
            MGINFO_YELLOW(context << " Synced " << m_core.get_current_blockchain_height() << "/" << m_core.get_target_blockchain_height()
                << timing_message);
          }
        }
      }

      if (should_download_next_span(context))
      {
        context.m_needed_objects.clear();
        context.m_last_response_height = 0;
        force_next_span = true;
      }
    }

skip:
    if (!request_missing_objects(context, true, force_next_span))
    {
      LOG_ERROR_CCONTEXT("Failed to request missing objects, dropping connection");
      drop_connection(context, false, false);
      return 1;
    }
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::on_idle()
  {
    m_idle_peer_kicker.do_call(boost::bind(&t_cryptonote_protocol_handler<t_core>::kick_idle_peers, this));
    return m_core.on_idle();
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::kick_idle_peers()
  {
    MTRACE("Checking for idle peers...");
    m_p2p->for_each_connection([&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)->bool
    {
      if (context.m_state == cryptonote_connection_context::state_synchronizing)
      {
        const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
        const boost::posix_time::time_duration dt = now - context.m_last_request_time;
        if (dt.total_microseconds() > IDLE_PEER_KICK_TIME)
        {
          MINFO(context << " kicking idle peer");
          ++context.m_callback_request_count;
          m_p2p->request_callback(context);
        }
      }
      return true;
    });
    return true;
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
      drop_connection(context, false, false);
      return 1;
    }
    LOG_PRINT_CCONTEXT_L2("-->>NOTIFY_RESPONSE_CHAIN_ENTRY: m_start_height=" << r.start_height << ", m_total_height=" << r.total_height << ", m_block_ids.size()=" << r.m_block_ids.size());
    post_notify<NOTIFY_RESPONSE_CHAIN_ENTRY>(r, context);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::should_download_next_span(cryptonote_connection_context& context) const
  {
    std::list<crypto::hash> hashes;
    boost::uuids::uuid span_connection_id;
    boost::posix_time::ptime request_time;
    std::pair<uint64_t, uint64_t> span;

    span = m_block_queue.get_start_gap_span();
    if (span.second > 0)
    {
      MDEBUG(context << " we should download it as there is a gap");
      return true;
    }

    // if the next span is not scheduled (or there is none)
    span = m_block_queue.get_next_span_if_scheduled(hashes, span_connection_id, request_time);
    if (span.second == 0)
    {
      // we might be in a weird case where there is a filled next span,
      // but it starts higher than the current height
      uint64_t height;
      std::list<cryptonote::block_complete_entry> bcel;
      if (!m_block_queue.get_next_span(height, bcel, span_connection_id, true))
        return false;
      if (height > m_core.get_current_blockchain_height())
      {
        MDEBUG(context << " we should download it as the next block isn't scheduled");
        return true;
      }
      return false;
    }
    // if it was scheduled by this particular peer
    if (span_connection_id == context.m_connection_id)
      return false;

    float span_speed = m_block_queue.get_speed(span_connection_id);
    float speed = m_block_queue.get_speed(context.m_connection_id);
    MDEBUG(context << " next span is scheduled for " << span_connection_id << ", speed " << span_speed << ", ours " << speed);

    // we try for that span too if:
    //  - we're substantially faster, or:
    //  - we're the fastest and the other one isn't (avoids a peer being waaaay slow but yet unmeasured)
    //  - the other one asked at least 5 seconds ago
    if (span_speed < .25 && speed > .75f)
    {
      MDEBUG(context << " we should download it as we're substantially faster");
      return true;
    }
    if (speed == 1.0f && span_speed != 1.0f)
    {
      MDEBUG(context << " we should download it as we're the fastest peer");
      return true;
    }
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    if ((now - request_time).total_microseconds() > REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD)
    {
      MDEBUG(context << " we should download it as this span was requested long ago");
      return true;
    }
    return false;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::request_missing_objects(cryptonote_connection_context& context, bool check_having_blocks, bool force_next_span)
  {
    // flush stale spans
    std::set<boost::uuids::uuid> live_connections;
    m_p2p->for_each_connection([&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)->bool{
      live_connections.insert(context.m_connection_id);
      return true;
    });
    m_block_queue.flush_stale_spans(live_connections);

    // if we don't need to get next span, and the block queue is full enough, wait a bit
    bool start_from_current_chain = false;
    if (!force_next_span)
    {
      bool first = true;
      while (1)
      {
        size_t nblocks = m_block_queue.get_num_filled_spans();
        size_t size = m_block_queue.get_data_size();
        if (nblocks < BLOCK_QUEUE_NBLOCKS_THRESHOLD || size < BLOCK_QUEUE_SIZE_THRESHOLD)
        {
          if (!first)
          {
            LOG_DEBUG_CC(context, "Block queue is " << nblocks << " and " << size << ", resuming");
          }
          break;
        }

        if (should_download_next_span(context))
        {
          MDEBUG(context << " we should try for that next span too, we think we could get it faster, resuming");
          force_next_span = true;
          break;
        }

        if (first)
        {
          LOG_DEBUG_CC(context, "Block queue is " << nblocks << " and " << size << ", pausing");
          first = false;
          context.m_state = cryptonote_connection_context::state_standby;
        }

        // this needs doing after we went to standby, so the callback knows what to do
        bool filled;
        if (m_block_queue.has_next_span(context.m_connection_id, filled) && !filled)
        {
          MDEBUG(context << " we have the next span, and it is scheduled, resuming");
          ++context.m_callback_request_count;
          m_p2p->request_callback(context);
          return 1;
        }

        for (size_t n = 0; n < 50; ++n)
        {
          if (m_stopping)
            return 1;
          boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }
      }
      context.m_state = cryptonote_connection_context::state_synchronizing;
    }

    MDEBUG(context << " request_missing_objects: check " << check_having_blocks << ", force_next_span " << force_next_span << ", m_needed_objects " << context.m_needed_objects.size() << " lrh " << context.m_last_response_height << ", chain " << m_core.get_current_blockchain_height());
    if(context.m_needed_objects.size() || force_next_span)
    {
      //we know objects that we need, request this objects
      NOTIFY_REQUEST_GET_OBJECTS::request req;
      bool is_next = false;
      size_t count = 0;
      const size_t count_limit = m_core.get_block_sync_size(m_core.get_current_blockchain_height());
      std::pair<uint64_t, uint64_t> span = std::make_pair(0, 0);
      {
        MDEBUG(context << " checking for gap");
        span = m_block_queue.get_start_gap_span();
        if (span.second > 0)
        {
          const uint64_t first_block_height_known = context.m_last_response_height - context.m_needed_objects.size() + 1;
          const uint64_t last_block_height_known = context.m_last_response_height;
          const uint64_t first_block_height_needed = span.first;
          const uint64_t last_block_height_needed = span.first + std::min(span.second, (uint64_t)count_limit) - 1;
          MDEBUG(context << " gap found, span: " << span.first << " - " << span.first + span.second - 1 << " (" << last_block_height_needed << ")");
          MDEBUG(context << " current known hashes from from " << first_block_height_known << " to " << last_block_height_known);
          if (first_block_height_needed < first_block_height_known || last_block_height_needed > last_block_height_known)
          {
            MDEBUG(context << " we are missing some of the necessary hashes for this gap, requesting chain again");
            context.m_needed_objects.clear();
            context.m_last_response_height = 0;
            start_from_current_chain = true;
            goto skip;
          }
          MDEBUG(context << " we have the hashes for this gap");
        }
      }
      if (force_next_span)
      {
        if (span.second == 0)
        {
          std::list<crypto::hash> hashes;
          boost::uuids::uuid span_connection_id;
          boost::posix_time::ptime time;
          span = m_block_queue.get_next_span_if_scheduled(hashes, span_connection_id, time);
          if (span.second > 0)
          {
            is_next = true;
            for (const auto &hash: hashes)
            {
              req.blocks.push_back(hash);
              context.m_requested_objects.insert(hash);
            }
          }
        }
      }
      if (span.second == 0)
      {
        MDEBUG(context << " span size is 0");
        if (context.m_last_response_height + 1 < context.m_needed_objects.size())
        {
          MERROR(context << " ERROR: inconsistent context: lrh " << context.m_last_response_height << ", nos " << context.m_needed_objects.size());
          context.m_needed_objects.clear();
          context.m_last_response_height = 0;
          goto skip;
        }
        // take out blocks we already have
        while (!context.m_needed_objects.empty() && m_core.have_block(context.m_needed_objects.front()))
        {
          context.m_needed_objects.pop_front();
        }
        const uint64_t first_block_height = context.m_last_response_height - context.m_needed_objects.size() + 1;
        span = m_block_queue.reserve_span(first_block_height, context.m_last_response_height, count_limit, context.m_connection_id, context.m_needed_objects);
        MDEBUG(context << " span from " << first_block_height << ": " << span.first << "/" << span.second);
      }
      if (span.second == 0 && !force_next_span)
      {
        MDEBUG(context << " still no span reserved, we may be in the corner case of next span scheduled and everything else scheduled/filled");
        std::list<crypto::hash> hashes;
        boost::uuids::uuid span_connection_id;
        boost::posix_time::ptime time;
        span = m_block_queue.get_next_span_if_scheduled(hashes, span_connection_id, time);
        if (span.second > 0)
        {
          is_next = true;
          for (const auto &hash: hashes)
          {
            req.blocks.push_back(hash);
            ++count;
            context.m_requested_objects.insert(hash);
            // that's atrocious O(n) wise, but this is rare
            auto i = std::find(context.m_needed_objects.begin(), context.m_needed_objects.end(), hash);
            if (i != context.m_needed_objects.end())
              context.m_needed_objects.erase(i);
          }
        }
      }
      MDEBUG(context << " span: " << span.first << "/" << span.second << " (" << span.first << " - " << (span.first + span.second - 1) << ")");
      if (span.second > 0)
      {
        if (!is_next)
        {
          const uint64_t first_context_block_height = context.m_last_response_height - context.m_needed_objects.size() + 1;
          uint64_t skip = span.first - first_context_block_height;
          if (skip > context.m_needed_objects.size())
          {
            MERROR("ERROR: skip " << skip << ", m_needed_objects " << context.m_needed_objects.size() << ", first_context_block_height" << first_context_block_height);
            return false;
          }
          while (skip--)
            context.m_needed_objects.pop_front();
          if (context.m_needed_objects.size() < span.second)
          {
            MERROR("ERROR: span " << span.first << "/" << span.second << ", m_needed_objects " << context.m_needed_objects.size());
            return false;
          }

          auto it = context.m_needed_objects.begin();
          for (size_t n = 0; n < span.second; ++n)
          {
            req.blocks.push_back(*it);
            ++count;
            context.m_requested_objects.insert(*it);
            auto j = it++;
            context.m_needed_objects.erase(j);
          }
        }

        context.m_last_request_time = boost::posix_time::microsec_clock::universal_time();
        LOG_PRINT_CCONTEXT_L1("-->>NOTIFY_REQUEST_GET_OBJECTS: blocks.size()=" << req.blocks.size() << ", txs.size()=" << req.txs.size()
            << "requested blocks count=" << count << " / " << count_limit << " from " << span.first << ", first hash " << req.blocks.front());
        //epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());

        post_notify<NOTIFY_REQUEST_GET_OBJECTS>(req, context);
        return true;
      }
    }

skip:
    context.m_needed_objects.clear();
    if(context.m_last_response_height < context.m_remote_blockchain_height-1)
    {//we have to fetch more objects ids, request blockchain entry

      NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
      m_core.get_short_chain_history(r.block_ids);

      if (!start_from_current_chain)
      {
        // we'll want to start off from where we are on that peer, which may not be added yet
        if (context.m_last_known_hash != cryptonote::null_hash && r.block_ids.front() != context.m_last_known_hash)
          r.block_ids.push_front(context.m_last_known_hash);
      }

      handler_request_blocks_history( r.block_ids ); // change the limit(?), sleep(?)

      //std::string blob; // for calculate size of request
      //epee::serialization::store_t_to_binary(r, blob);
      //epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());
      //LOG_PRINT_CCONTEXT_L1("r = " << 200);

      context.m_last_request_time = boost::posix_time::microsec_clock::universal_time();
      LOG_PRINT_CCONTEXT_L1("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() << ", start_from_current_chain " << start_from_current_chain);
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
      if (context.m_remote_blockchain_height >= m_core.get_target_blockchain_height())
      {
        if (m_core.get_current_blockchain_height() >= m_core.get_target_blockchain_height())
        {
          MGINFO_GREEN("SYNCHRONIZED OK");
          on_connection_synchronized();
        }
      }
      else
      {
        MINFO(context << " we've reached this peer's blockchain height");
      }
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
    m_core.safesyncmode(true);
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
      drop_connection(context, true, false);
      return 1;
    }

    context.m_remote_blockchain_height = arg.total_height;
    context.m_last_response_height = arg.start_height + arg.m_block_ids.size()-1;
    if(context.m_last_response_height > context.m_remote_blockchain_height)
    {
      LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_CHAIN_ENTRY, with m_total_height=" << arg.total_height
                                                                         << ", m_start_height=" << arg.start_height
                                                                         << ", m_block_ids.size()=" << arg.m_block_ids.size());
      drop_connection(context, false, false);
      return 1;
    }

    for(auto& bl_id: arg.m_block_ids)
    {
      context.m_needed_objects.push_back(bl_id);
    }

    if (!request_missing_objects(context, false))
    {
      LOG_ERROR_CCONTEXT("Failed to request missing objects, dropping connection");
      drop_connection(context, false, false);
      return 1;
    }

    if (arg.total_height > m_core.get_target_blockchain_height())
      m_core.set_target_blockchain_height(arg.total_height);

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
        if(m_core.fluffy_blocks_enabled() && (support_flags & P2P_SUPPORT_FLAG_FLUFFY_BLOCKS))
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
  void t_cryptonote_protocol_handler<t_core>::drop_connection(cryptonote_connection_context &context, bool add_fail, bool flush_all_spans)
  {
    if (add_fail)
      m_p2p->add_host_fail(context.m_remote_address);

    m_p2p->drop_connection(context);

    m_block_queue.flush_spans(context.m_connection_id, flush_all_spans);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_cryptonote_protocol_handler<t_core>::on_connection_close(cryptonote_connection_context &context)
  {
    uint64_t target = 0;
    m_p2p->for_each_connection([&](const connection_context& cntxt, nodetool::peerid_type peer_id, uint32_t support_flags) {
      if (cntxt.m_state >= cryptonote_connection_context::state_synchronizing && cntxt.m_connection_id != context.m_connection_id)
        target = std::max(target, cntxt.m_remote_blockchain_height);
      return true;
    });
    const uint64_t previous_target = m_core.get_target_blockchain_height();
    if (target < previous_target)
    {
      MINFO("Target height decreasing from " << previous_target << " to " << target);
      m_core.set_target_blockchain_height(target);
    }

    m_block_queue.flush_spans(context.m_connection_id, false);
  }

  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_cryptonote_protocol_handler<t_core>::stop()
  {
    m_stopping = true;
    m_core.stop();
  }
} // namespace
