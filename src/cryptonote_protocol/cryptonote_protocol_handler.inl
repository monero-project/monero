/// @file
/// @author rfree (current maintainer/user in monero.cc project - most of code is from CryptoNote)
/// @brief This is the original cryptonote protocol network-events handler, modified by us

// Copyright (c) 2014-2019, The Monero Project
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
#include <ctime>

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "profile_tools.h"
#include "net/network_throttle-detail.hpp"
#include "common/pruning.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.cn"

#define MLOG_P2P_MESSAGE(x) MCINFO("net.p2p.msg", context << x)
#define MLOGIF_P2P_MESSAGE(init, test, x) \
  do { \
    const auto level = el::Level::Info; \
    const char *cat = "net.p2p.msg"; \
    if (ELPP->vRegistry()->allowed(level, cat)) { \
      init; \
      if (test) \
        el::base::Writer(level, el::Color::Default, __FILE__, __LINE__, ELPP_FUNC, el::base::DispatchAction::NormalLog).construct(cat) << x; \
    } \
  } while(0)

#define MLOG_PEER_STATE(x) \
  MCINFO(MONERO_DEFAULT_LOG_CATEGORY, context << "[" << epee::string_tools::to_string_hex(context.m_pruning_seed) << "] state: " << x << " in state " << cryptonote::get_protocol_state_string(context.m_state))

#define BLOCK_QUEUE_NSPANS_THRESHOLD 10 // chunks of N blocks
#define BLOCK_QUEUE_SIZE_THRESHOLD (100*1024*1024) // MB
#define BLOCK_QUEUE_FORCE_DOWNLOAD_NEAR_BLOCKS 1000
#define REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD_STANDBY (5 * 1000000) // microseconds
#define REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD (30 * 1000000) // microseconds
#define IDLE_PEER_KICK_TIME (600 * 1000000) // microseconds
#define PASSIVE_PEER_KICK_TIME (60 * 1000000) // microseconds
#define DROP_ON_SYNC_WEDGE_THRESHOLD (30 * 1000000000ull) // nanoseconds
#define LAST_ACTIVITY_STALL_THRESHOLD (2.0f) // seconds

namespace cryptonote
{



  //-----------------------------------------------------------------------------------------------------------------------
  template<class t_core>
    t_cryptonote_protocol_handler<t_core>::t_cryptonote_protocol_handler(t_core& rcore, nodetool::i_p2p_endpoint<connection_context>* p_net_layout, bool offline):m_core(rcore),
                                                                                                              m_p2p(p_net_layout),
                                                                                                              m_syncronized_connections_count(0),
                                                                                                              m_synchronized(offline),
                                                                                                              m_ask_for_txpool_complement(true),
                                                                                                              m_stopping(false),
                                                                                                              m_no_sync(false)

  {
    if(!m_p2p)
      m_p2p = &m_p2p_stub;
  }
  //-----------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::init(const boost::program_options::variables_map& vm)
  {
    m_sync_timer.pause();
    m_sync_timer.reset();
    m_add_timer.pause();
    m_add_timer.reset();
    m_last_add_end_time = 0;
    m_sync_spans_downloaded = 0;
    m_sync_old_spans_downloaded = 0;
    m_sync_bad_spans_downloaded = 0;
    m_sync_download_chain_size = 0;
    m_sync_download_objects_size = 0;

    m_block_download_max_size = command_line::get_arg(vm, cryptonote::arg_block_download_max_size);
    m_sync_pruned_blocks = command_line::get_arg(vm, cryptonote::arg_sync_pruned_blocks);

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
      NOTIFY_REQUEST_CHAIN::request r = {};
      context.m_needed_objects.clear();
      m_core.get_short_chain_history(r.block_ids);
      handler_request_blocks_history( r.block_ids ); // change the limit(?), sleep(?)
      r.prune = m_sync_pruned_blocks;
      MLOG_P2P_MESSAGE("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() );
      post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
      MLOG_PEER_STATE("requesting chain");
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
        << std::setw(20) << nodetool::peerid_to_string(peer_id)
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
      if (cntxt.m_remote_address.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
      {
        cnx.ip = cnx.host;
        cnx.port = std::to_string(cntxt.m_remote_address.as<epee::net_utils::ipv4_network_address>().port());
      }
      cnx.rpc_port = cntxt.m_rpc_port;
      cnx.rpc_credits_per_hash = cntxt.m_rpc_credits_per_hash;

      cnx.peer_id = nodetool::peerid_to_string(peer_id);
      
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

      cnx.connection_id = epee::string_tools::pod_to_hex(cntxt.m_connection_id);
      cnx.ssl = cntxt.m_ssl;

      cnx.height = cntxt.m_remote_blockchain_height;
      cnx.pruning_seed = cntxt.m_pruning_seed;
      cnx.address_type = (uint8_t)cntxt.m_remote_address.get_type_id();

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
    if (hshd.current_height > 0)
    {
      const uint8_t version = m_core.get_ideal_hard_fork_version(hshd.current_height - 1);
      if (version >= 6 && version != hshd.top_version)
      {
        if (version < hshd.top_version && version == m_core.get_ideal_hard_fork_version())
          MCLOG_RED(el::Level::Warning, "global", context << " peer claims higher version than we think (" <<
              (unsigned)hshd.top_version << " for " << (hshd.current_height - 1) << " instead of " << (unsigned)version <<
              ") - we may be forked from the network and a software upgrade may be needed");
        return false;
      }
    }

    // reject weird pruning schemes
    if (hshd.pruning_seed)
    {
      const uint32_t log_stripes = tools::get_pruning_log_stripes(hshd.pruning_seed);
      if (log_stripes != CRYPTONOTE_PRUNING_LOG_STRIPES || tools::get_pruning_stripe(hshd.pruning_seed) > (1u << log_stripes))
      {
        MWARNING(context << " peer claim unexpected pruning seed " << epee::string_tools::to_string_hex(hshd.pruning_seed) << ", disconnecting");
        return false;
      }
    }

    context.m_remote_blockchain_height = hshd.current_height;
    context.m_pruning_seed = hshd.pruning_seed;
#ifdef CRYPTONOTE_PRUNING_DEBUG_SPOOF_SEED
    context.m_pruning_seed = tools::make_pruning_seed(1 + (context.m_remote_address.as<epee::net_utils::ipv4_network_address>().ip()) % (1 << CRYPTONOTE_PRUNING_LOG_STRIPES), CRYPTONOTE_PRUNING_LOG_STRIPES);
    LOG_INFO_CC(context, "New connection posing as pruning seed " << epee::string_tools::to_string_hex(context.m_pruning_seed) << ", seed address " << &context.m_pruning_seed);
#endif

    uint64_t target = m_core.get_target_blockchain_height();
    if (target == 0)
      target = m_core.get_current_blockchain_height();

    if(m_core.have_block(hshd.top_id))
    {
      context.m_state = cryptonote_connection_context::state_normal;
      if(is_inital  && hshd.current_height >= target && target == m_core.get_current_blockchain_height())
        on_connection_synchronized();
      return true;
    }

    // No chain synchronization over hidden networks (tor, i2p, etc.)
    if(context.m_remote_address.get_zone() != epee::net_utils::zone::public_)
    {
      context.m_state = cryptonote_connection_context::state_normal;
      return true;
    }

    if (hshd.current_height > target)
    {
    /* As I don't know if accessing hshd from core could be a good practice,
    I prefer pushing target height to the core at the same time it is pushed to the user.
    Nz. */
    int64_t diff = static_cast<int64_t>(hshd.current_height) - static_cast<int64_t>(m_core.get_current_blockchain_height());
    uint64_t abs_diff = std::abs(diff);
    uint64_t max_block_height = std::max(hshd.current_height,m_core.get_current_blockchain_height());
    uint64_t last_block_v1 = m_core.get_nettype() == TESTNET ? 624633 : m_core.get_nettype() == MAINNET ? 1009826 : (uint64_t)-1;
    uint64_t diff_v2 = max_block_height > last_block_v1 ? std::min(abs_diff, max_block_height - last_block_v1) : 0;
    MCLOG(is_inital ? el::Level::Info : el::Level::Debug, "global", el::Color::Yellow, context <<  "Sync data returned a new top block candidate: " << m_core.get_current_blockchain_height() << " -> " << hshd.current_height
      << " [Your node is " << abs_diff << " blocks (" << ((abs_diff - diff_v2) / (24 * 60 * 60 / DIFFICULTY_TARGET_V1)) + (diff_v2 / (24 * 60 * 60 / DIFFICULTY_TARGET_V2)) << " days) "
      << (0 <= diff ? std::string("behind") : std::string("ahead"))
      << "] " << ENDL << "SYNCHRONIZATION started");
      if (hshd.current_height >= m_core.get_current_blockchain_height() + 5) // don't switch to unsafe mode just for a few blocks
      {
        m_core.safesyncmode(false);
      }
      if (m_core.get_target_blockchain_height() == 0) // only when sync starts
      {
        m_sync_timer.resume();
        m_sync_timer.reset();
        m_add_timer.pause();
        m_add_timer.reset();
        m_last_add_end_time = 0;
        m_sync_spans_downloaded = 0;
        m_sync_old_spans_downloaded = 0;
        m_sync_bad_spans_downloaded = 0;
        m_sync_download_chain_size = 0;
        m_sync_download_objects_size = 0;
      }
    m_core.set_target_blockchain_height((hshd.current_height));
    }
    MINFO(context << "Remote blockchain height: " << hshd.current_height << ", id: " << hshd.top_id);

    if (m_no_sync)
    {
      context.m_state = cryptonote_connection_context::state_normal;
      return true;
    }

    context.m_state = cryptonote_connection_context::state_synchronizing;
    //let the socket to send response to handshake, but request callback, to let send request data after response
    LOG_PRINT_CCONTEXT_L2("requesting callback");
    ++context.m_callback_request_count;
    m_p2p->request_callback(context);
    MLOG_PEER_STATE("requesting callback");
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::get_payload_sync_data(CORE_SYNC_DATA& hshd)
  {
    m_core.get_blockchain_top(hshd.current_height, hshd.top_id);
    hshd.top_version = m_core.get_ideal_hard_fork_version(hshd.current_height);
    difficulty_type wide_cumulative_difficulty = m_core.get_block_cumulative_difficulty(hshd.current_height);
    hshd.cumulative_difficulty = (wide_cumulative_difficulty & 0xffffffffffffffff).convert_to<uint64_t>();
    hshd.cumulative_difficulty_top64 = ((wide_cumulative_difficulty >> 64) & 0xffffffffffffffff).convert_to<uint64_t>();
    hshd.current_height +=1;
    hshd.pruning_seed = m_core.get_blockchain_pruning_seed();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
    template<class t_core>
    bool t_cryptonote_protocol_handler<t_core>::get_payload_sync_data(blobdata& data)
  {
    CORE_SYNC_DATA hsd = {};
    get_payload_sync_data(hsd);
    epee::serialization::store_t_to_binary(hsd, data);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
    template<class t_core>
    int t_cryptonote_protocol_handler<t_core>::handle_notify_new_block(int command, NOTIFY_NEW_BLOCK::request& arg, cryptonote_connection_context& context)
  {
    MLOGIF_P2P_MESSAGE(crypto::hash hash; cryptonote::block b; bool ret = cryptonote::parse_and_validate_block_from_blob(arg.b.block, b, &hash);, ret, "Received NOTIFY_NEW_BLOCK " << hash << " (height " << arg.current_blockchain_height << ", " << arg.b.txs.size() << " txes)");
    if(context.m_state != cryptonote_connection_context::state_normal)
      return 1;
    if(!is_synchronized() || m_no_sync) // can happen if a peer connection goes to normal but another thread still hasn't finished adding queued blocks
    {
      LOG_DEBUG_CC(context, "Received new block while syncing, ignored");
      return 1;
    }
    m_core.pause_mine();
    std::vector<block_complete_entry> blocks;
    blocks.push_back(arg.b);
    std::vector<block> pblocks;
    if (!m_core.prepare_handle_incoming_blocks(blocks, pblocks))
    {
      LOG_PRINT_CCONTEXT_L1("Block verification failed: prepare_handle_incoming_blocks failed, dropping connection");
      drop_connection(context, false, false);
      m_core.resume_mine();
      return 1;
    }
    for(auto tx_blob_it = arg.b.txs.begin(); tx_blob_it!=arg.b.txs.end();tx_blob_it++)
    {
      cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      m_core.handle_incoming_tx(*tx_blob_it, tvc, relay_method::block, true);
      if(tvc.m_verifivation_failed)
      {
        LOG_PRINT_CCONTEXT_L1("Block verification failed: transaction verification failed, dropping connection");
        drop_connection(context, false, false);
        m_core.cleanup_handle_incoming_blocks();
        m_core.resume_mine();
        return 1;
      }
    }

    block_verification_context bvc = {};
    m_core.handle_incoming_block(arg.b.block, pblocks.empty() ? NULL : &pblocks[0], bvc); // got block from handle_notify_new_block
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
      drop_connection_with_score(context, bvc.m_bad_pow ? P2P_IP_FAILS_BEFORE_BLOCK : 1, false);
      return 1;
    }
    if(bvc.m_added_to_main_chain)
    {
      //TODO: Add here announce protocol usage
      relay_block(arg, context);
    }else if(bvc.m_marked_as_orphaned)
    {
      context.m_needed_objects.clear();
      context.m_state = cryptonote_connection_context::state_synchronizing;
      NOTIFY_REQUEST_CHAIN::request r = {};
      m_core.get_short_chain_history(r.block_ids);
      r.prune = m_sync_pruned_blocks;
      handler_request_blocks_history( r.block_ids ); // change the limit(?), sleep(?)
      MLOG_P2P_MESSAGE("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() );
      post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
      MLOG_PEER_STATE("requesting chain");
    }

    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_notify_new_fluffy_block(int command, NOTIFY_NEW_FLUFFY_BLOCK::request& arg, cryptonote_connection_context& context)
  {
    MLOGIF_P2P_MESSAGE(crypto::hash hash; cryptonote::block b; bool ret = cryptonote::parse_and_validate_block_from_blob(arg.b.block, b, &hash);, ret, "Received NOTIFY_NEW_FLUFFY_BLOCK " << hash << " (height " << arg.current_blockchain_height << ", " << arg.b.txs.size() << " txes)");
    if(context.m_state != cryptonote_connection_context::state_normal)
      return 1;
    if(!is_synchronized() || m_no_sync) // can happen if a peer connection goes to normal but another thread still hasn't finished adding queued blocks
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

      std::vector<tx_blob_entry> have_tx;

      // Instead of requesting missing transactions by hash like BTC, 
      // we do it by index (thanks to a suggestion from moneromooo) because
      // we're way cooler .. and also because they're smaller than hashes.
      // 
      // Also, remember to pepper some whitespace changes around to bother
      // moneromooo ... only because I <3 him. 
      std::vector<uint64_t> need_tx_indices;
        
      transaction tx;
      crypto::hash tx_hash;

      for(auto& tx_blob: arg.b.txs)
      {
        if(parse_and_validate_tx_from_blob(tx_blob.blob, tx))
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
            if(!m_core.handle_incoming_tx(tx_blob, tvc, relay_method::block, true) || tvc.m_verifivation_failed)
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
            << epee::string_tools::buff_to_hex_nodelimer(tx_blob.blob)
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
        if(m_core.get_pool_transaction(tx_hash, txblob, relay_category::broadcasted))
        {
          have_tx.push_back({txblob, crypto::null_hash});
        }
        else
        {
          std::vector<crypto::hash> tx_ids;
          std::vector<transaction> txes;
          std::vector<crypto::hash> missing;
          tx_ids.push_back(tx_hash);
          if (m_core.get_transactions(tx_ids, txes, missing) && missing.empty())
          {
            if (txes.size() == 1)
            {
              have_tx.push_back({tx_to_blob(txes.front()), crypto::null_hash});
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
        missing_tx_req.current_blockchain_height = arg.current_blockchain_height;
        missing_tx_req.missing_tx_indices = std::move(need_tx_indices);
        
        m_core.resume_mine();
        MLOG_P2P_MESSAGE("-->>NOTIFY_REQUEST_FLUFFY_MISSING_TX: missing_tx_indices.size()=" << missing_tx_req.missing_tx_indices.size() );
        post_notify<NOTIFY_REQUEST_FLUFFY_MISSING_TX>(missing_tx_req, context);
      }
      else // whoo-hoo we've got em all ..
      {
        MDEBUG("We have all needed txes for this fluffy block");

        block_complete_entry b;
        b.block = arg.b.block;
        b.txs = have_tx;

        std::vector<block_complete_entry> blocks;
        blocks.push_back(b);
        std::vector<block> pblocks;
        if (!m_core.prepare_handle_incoming_blocks(blocks, pblocks))
        {
          LOG_PRINT_CCONTEXT_L0("Failure in prepare_handle_incoming_blocks");
          m_core.resume_mine();
          return 1;
        }
          
        block_verification_context bvc = {};
        m_core.handle_incoming_block(arg.b.block, pblocks.empty() ? NULL : &pblocks[0], bvc); // got block from handle_notify_new_block
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
          drop_connection_with_score(context, bvc.m_bad_pow ? P2P_IP_FAILS_BEFORE_BLOCK : 1, false);
          return 1;
        }
        if( bvc.m_added_to_main_chain )
        {
          //TODO: Add here announce protocol usage
          NOTIFY_NEW_BLOCK::request reg_arg = AUTO_VAL_INIT(reg_arg);
          reg_arg.current_blockchain_height = arg.current_blockchain_height;
          reg_arg.b = b;
          relay_block(reg_arg, context);
        }
        else if( bvc.m_marked_as_orphaned )
        {
          context.m_needed_objects.clear();
          context.m_state = cryptonote_connection_context::state_synchronizing;
          NOTIFY_REQUEST_CHAIN::request r = {};
          m_core.get_short_chain_history(r.block_ids);
          handler_request_blocks_history( r.block_ids ); // change the limit(?), sleep(?)
          r.prune = m_sync_pruned_blocks;
          MLOG_P2P_MESSAGE("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() );
          post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
          MLOG_PEER_STATE("requesting chain");
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
    
    std::vector<std::pair<cryptonote::blobdata, block>> local_blocks;
    std::vector<cryptonote::blobdata> local_txs;

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
    std::vector<bool> seen(b.tx_hashes.size(), false);
    for(auto& tx_idx: arg.missing_tx_indices)
    {
      if(tx_idx < b.tx_hashes.size())
      {
        MDEBUG("  tx " << b.tx_hashes[tx_idx]);
        if (seen[tx_idx])
        {
          LOG_ERROR_CCONTEXT
          (
            "Failed to handle request NOTIFY_REQUEST_FLUFFY_MISSING_TX"
            << ", request is asking for duplicate tx "
            << ", tx index = " << tx_idx << ", block tx count " << b.tx_hashes.size()
            << ", block_height = " << arg.current_blockchain_height
            << ", dropping connection"
          );
          drop_connection(context, true, false);
          return 1;
        }
        txids.push_back(b.tx_hashes[tx_idx]);
        seen[tx_idx] = true;
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

    std::vector<cryptonote::transaction> txs;
    std::vector<crypto::hash> missed;
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
      fluffy_response.b.txs.push_back({t_serializable_object_to_blob(tx), crypto::null_hash});
    }

    MLOG_P2P_MESSAGE
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
  int t_cryptonote_protocol_handler<t_core>::handle_notify_get_txpool_complement(int command, NOTIFY_GET_TXPOOL_COMPLEMENT::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_GET_TXPOOL_COMPLEMENT (" << arg.hashes.size() << " txes)");

    std::vector<std::pair<cryptonote::blobdata, block>> local_blocks;
    std::vector<cryptonote::blobdata> local_txs;

    std::vector<cryptonote::blobdata> txes;
    if (!m_core.get_txpool_complement(arg.hashes, txes))
    {
      LOG_ERROR_CCONTEXT("failed to get txpool complement");
      return 1;
    }

    NOTIFY_NEW_TRANSACTIONS::request new_txes;
    new_txes.txs = std::move(txes);

    MLOG_P2P_MESSAGE
    (
        "-->>NOTIFY_NEW_TRANSACTIONS: "
        << ", txs.size()=" << new_txes.txs.size()
    );

    post_notify<NOTIFY_NEW_TRANSACTIONS>(new_txes, context);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_notify_new_transactions(int command, NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_NEW_TRANSACTIONS (" << arg.txs.size() << " txes)");
    for (const auto &blob: arg.txs)
      MLOGIF_P2P_MESSAGE(cryptonote::transaction tx; crypto::hash hash; bool ret = cryptonote::parse_and_validate_tx_from_blob(blob, tx, hash);, ret, "Including transaction " << hash);

    if(context.m_state != cryptonote_connection_context::state_normal)
      return 1;

    // while syncing, core will lock for a long time, so we ignore
    // those txes as they aren't really needed anyway, and avoid a
    // long block before replying
    if(!is_synchronized() || m_no_sync)
    {
      LOG_DEBUG_CC(context, "Received new tx while syncing, ignored");
      return 1;
    }

    std::vector<cryptonote::blobdata> newtxs;
    newtxs.reserve(arg.txs.size());
    for (size_t i = 0; i < arg.txs.size(); ++i)
    {
      cryptonote::tx_verification_context tvc{};
      m_core.handle_incoming_tx({arg.txs[i], crypto::null_hash}, tvc, relay_method::fluff, true);
      if(tvc.m_verifivation_failed)
      {
        LOG_PRINT_CCONTEXT_L1("Tx verification failed, dropping connection");
        drop_connection(context, false, false);
        return 1;
      }
      if(tvc.m_should_be_relayed)
        newtxs.push_back(std::move(arg.txs[i]));
    }
    arg.txs = std::move(newtxs);

    if(arg.txs.size())
    {
      //TODO: add announce usage here
      relay_transactions(arg, context.m_connection_id, context.m_remote_address.get_zone());
    }

    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_request_get_objects(int command, NOTIFY_REQUEST_GET_OBJECTS::request& arg, cryptonote_connection_context& context)
  {
    MLOG_P2P_MESSAGE("Received NOTIFY_REQUEST_GET_OBJECTS (" << arg.blocks.size() << " blocks)");
    if (arg.blocks.size() > CURRENCY_PROTOCOL_MAX_OBJECT_REQUEST_COUNT)
      {
        LOG_ERROR_CCONTEXT(
            "Requested objects count is too big ("
            << arg.blocks.size() << ") expected not more then "
            << CURRENCY_PROTOCOL_MAX_OBJECT_REQUEST_COUNT);
        drop_connection(context, false, false);
        return 1;
      }

    NOTIFY_RESPONSE_GET_OBJECTS::request rsp;
    if(!m_core.handle_get_objects(arg, rsp, context))
    {
      LOG_ERROR_CCONTEXT("failed to handle request NOTIFY_REQUEST_GET_OBJECTS, dropping connection");
      drop_connection(context, false, false);
      return 1;
    }
    MLOG_P2P_MESSAGE("-->>NOTIFY_RESPONSE_GET_OBJECTS: blocks.size()="
                     << rsp.blocks.size() << ", rsp.m_current_blockchain_height=" << rsp.current_blockchain_height
                     << ", missed_ids.size()=" << rsp.missed_ids.size());
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
    MLOG_P2P_MESSAGE("Received NOTIFY_RESPONSE_GET_OBJECTS (" << arg.blocks.size() << " blocks)");
    MLOG_PEER_STATE("received objects");

    boost::posix_time::ptime request_time = context.m_last_request_time;
    context.m_last_request_time = boost::date_time::not_a_date_time;

    // calculate size of request
    size_t size = 0;
    size_t blocks_size = 0;
    for (const auto &element : arg.blocks) {
      blocks_size += element.block.size();
      for (const auto &tx : element.txs)
        blocks_size += tx.blob.size();
    }
    size += blocks_size;

    for (const auto &element : arg.missed_ids)
      size += sizeof(element.data);

    size += sizeof(arg.current_blockchain_height);
    {
      CRITICAL_REGION_LOCAL(m_buffer_mutex);
      m_avg_buffer.push_back(size);
    }
    ++m_sync_spans_downloaded;
    m_sync_download_objects_size += size;
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
      ++m_sync_bad_spans_downloaded;
      return 1;
    }

    context.m_remote_blockchain_height = arg.current_blockchain_height;
    if (context.m_remote_blockchain_height > m_core.get_target_blockchain_height())
      m_core.set_target_blockchain_height(context.m_remote_blockchain_height);

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

      crypto::hash block_hash;
      if(!parse_and_validate_block_from_blob(block_entry.block, b, block_hash))
      {
        LOG_ERROR_CCONTEXT("sent wrong block: failed to parse and validate block: "
          << epee::string_tools::buff_to_hex_nodelimer(block_entry.block) << ", dropping connection");
        drop_connection(context, false, false);
        ++m_sync_bad_spans_downloaded;
        return 1;
      }
      if (b.miner_tx.vin.size() != 1 || b.miner_tx.vin.front().type() != typeid(txin_gen))
      {
        LOG_ERROR_CCONTEXT("sent wrong block: block: miner tx does not have exactly one txin_gen input"
          << epee::string_tools::buff_to_hex_nodelimer(block_entry.block) << ", dropping connection");
        drop_connection(context, false, false);
        ++m_sync_bad_spans_downloaded;
        return 1;
      }
      if (start_height == std::numeric_limits<uint64_t>::max())
        start_height = boost::get<txin_gen>(b.miner_tx.vin[0]).height;

      auto req_it = context.m_requested_objects.find(block_hash);
      if(req_it == context.m_requested_objects.end())
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << epee::string_tools::pod_to_hex(get_blob_hash(block_entry.block))
          << " wasn't requested, dropping connection");
        drop_connection(context, false, false);
        ++m_sync_bad_spans_downloaded;
        return 1;
      }
      if(b.tx_hashes.size() != block_entry.txs.size())
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << epee::string_tools::pod_to_hex(get_blob_hash(block_entry.block))
          << ", tx_hashes.size()=" << b.tx_hashes.size() << " mismatch with block_complete_entry.m_txs.size()=" << block_entry.txs.size() << ", dropping connection");
        drop_connection(context, false, false);
        ++m_sync_bad_spans_downloaded;
        return 1;
      }

      context.m_requested_objects.erase(req_it);
      block_hashes.push_back(block_hash);
    }

    if(!context.m_requested_objects.empty())
    {
      MERROR(context << "returned not all requested objects (context.m_requested_objects.size()="
        << context.m_requested_objects.size() << "), dropping connection");
      drop_connection(context, false, false);
      ++m_sync_bad_spans_downloaded;
      return 1;
    }

    const bool pruned_ok = should_ask_for_pruned_data(context, start_height, arg.blocks.size(), true);
    if (!pruned_ok)
    {
      // if we don't want pruned data, check we did not get any
      for (block_complete_entry& block_entry: arg.blocks)
      {
        if (block_entry.pruned)
        {
          MERROR(context << "returned a pruned block, dropping connection");
          drop_connection(context, false, false);
          ++m_sync_bad_spans_downloaded;
          return 1;
        }
        if (block_entry.block_weight)
        {
          MERROR(context << "returned a block weight for a non pruned block, dropping connection");
          drop_connection(context, false, false);
          ++m_sync_bad_spans_downloaded;
          return 1;
        }
        for (const tx_blob_entry &tx_entry: block_entry.txs)
        {
          if (tx_entry.prunable_hash != crypto::null_hash)
          {
            MERROR(context << "returned at least one pruned object which we did not expect, dropping connection");
            drop_connection(context, false, false);
            ++m_sync_bad_spans_downloaded;
            return 1;
          }
        }
      }
    }
    else
    {
      // we accept pruned data, check that if we got some, then no weights are zero
      for (block_complete_entry& block_entry: arg.blocks)
      {
        if (block_entry.block_weight == 0 && block_entry.pruned)
        {
          MERROR(context << "returned at least one pruned block with 0 weight, dropping connection");
          drop_connection(context, false, false);
          ++m_sync_bad_spans_downloaded;
          return 1;
        }
      }
    }

    {
      MLOG_YELLOW(el::Level::Debug, context << " Got NEW BLOCKS inside of " << __FUNCTION__ << ": size: " << arg.blocks.size()
          << ", blocks: " << start_height << " - " << (start_height + arg.blocks.size() - 1) <<
          " (pruning seed " << epee::string_tools::to_string_hex(context.m_pruning_seed) << ")");

      // add that new span to the block queue
      const boost::posix_time::time_duration dt = now - request_time;
      const float rate = size * 1e6 / (dt.total_microseconds() + 1);
      MDEBUG(context << " adding span: " << arg.blocks.size() << " at height " << start_height << ", " << dt.total_microseconds()/1e6 << " seconds, " << (rate/1024) << " kB/s, size now " << (m_block_queue.get_data_size() + blocks_size) / 1048576.f << " MB");
      m_block_queue.add_blocks(start_height, arg.blocks, context.m_connection_id, rate, blocks_size);

      const crypto::hash last_block_hash = cryptonote::get_block_hash(b);
      context.m_last_known_hash = last_block_hash;

      if (!m_core.get_test_drop_download() || !m_core.get_test_drop_download_height()) { // DISCARD BLOCKS for testing
        return 1;
      }
    }

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
        MINFO(context << "Failed to lock m_sync_lock, going back to download");
        goto skip;
      }
      MDEBUG(context << " lock m_sync_lock, adding blocks to chain...");
      MLOG_PEER_STATE("adding blocks");

      {
        m_core.pause_mine();
        m_add_timer.resume();
        bool starting = true;
        epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([this, &starting]() {
          m_add_timer.pause();
          m_core.resume_mine();
          if (!starting)
            m_last_add_end_time = tools::get_tick_count();
        });

        while (1)
        {
          const uint64_t previous_height = m_core.get_current_blockchain_height();
          uint64_t start_height;
          std::vector<cryptonote::block_complete_entry> blocks;
          boost::uuids::uuid span_connection_id;
          if (!m_block_queue.get_next_span(start_height, blocks, span_connection_id))
          {
            MDEBUG(context << " no next span found, going back to download");
            break;
          }

          if (blocks.empty())
          {
            MERROR(context << "Next span has no blocks");
            m_block_queue.remove_spans(span_connection_id, start_height);
            continue;
          }

          MDEBUG(context << " next span in the queue has blocks " << start_height << "-" << (start_height + blocks.size() - 1)
              << ", we need " << previous_height);

          block new_block;
          crypto::hash last_block_hash;
          if (!parse_and_validate_block_from_blob(blocks.back().block, new_block, last_block_hash))
          {
            MERROR(context << "Failed to parse block, but it should already have been parsed");
            m_block_queue.remove_spans(span_connection_id, start_height);
            continue;
          }
          if (m_core.have_block(last_block_hash))
          {
            const uint64_t subchain_height = start_height + blocks.size();
            LOG_DEBUG_CC(context, "These are old blocks, ignoring: blocks " << start_height << " - " << (subchain_height-1) << ", blockchain height " << m_core.get_current_blockchain_height());
            m_block_queue.remove_spans(span_connection_id, start_height);
            ++m_sync_old_spans_downloaded;
            continue;
          }
          if (!parse_and_validate_block_from_blob(blocks.front().block, new_block))
          {
            MERROR(context << "Failed to parse block, but it should already have been parsed");
            m_block_queue.remove_spans(span_connection_id, start_height);
            continue;
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
              // we might be able to ask for that block directly, as we now can request out of order,
              // otherwise we continue out of order, unless this block is the one we need, in which
              // case we request block hashes, though it might be safer to disconnect ?
              if (start_height > previous_height)
              {
                if (should_drop_connection(context, get_next_needed_pruning_stripe().first))
                {
                  MDEBUG(context << "Got block with unknown parent which was not requested, but peer does not have that block - dropping connection");
                  if (!context.m_is_income)
                    m_p2p->add_used_stripe_peer(context);
                  drop_connection(context, false, true);
                  return 1;
                }
                MDEBUG(context << "Got block with unknown parent which was not requested, but peer does not have that block - back to download");

                goto skip;
              }

              // this can happen if a connection was sicced onto a late span, if it did not have those blocks,
              // since we don't know that at the sic time
              LOG_ERROR_CCONTEXT("Got block with unknown parent which was not requested - querying block hashes");
              m_block_queue.remove_spans(span_connection_id, start_height);
              context.m_needed_objects.clear();
              context.m_last_response_height = 0;
              goto skip;
            }

            // parent was requested, so we wait for it to be retrieved
            MINFO(context << " parent was requested, we'll get back to it");
            break;
          }

          const boost::posix_time::ptime start = boost::posix_time::microsec_clock::universal_time();

          if (starting)
          {
            starting = false;
            if (m_last_add_end_time)
            {
              const uint64_t tnow = tools::get_tick_count();
              const uint64_t ns = tools::ticks_to_ns(tnow - m_last_add_end_time);
              MINFO("Restarting adding block after idle for " << ns/1e9 << " seconds");
            }
          }

          std::vector<block> pblocks;
          if (!m_core.prepare_handle_incoming_blocks(blocks, pblocks))
          {
            LOG_ERROR_CCONTEXT("Failure in prepare_handle_incoming_blocks");
            return 1;
          }
          if (!pblocks.empty() && pblocks.size() != blocks.size())
          {
            m_core.cleanup_handle_incoming_blocks();
            LOG_ERROR_CCONTEXT("Internal error: blocks.size() != block_entry.txs.size()");
            return 1;
          }

          uint64_t block_process_time_full = 0, transactions_process_time_full = 0;
          size_t num_txs = 0, blockidx = 0;
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
            m_core.handle_incoming_txs(block_entry.txs, tvc, relay_method::block, true);
            if (tvc.size() != block_entry.txs.size())
            {
              LOG_ERROR_CCONTEXT("Internal error: tvc.size() != block_entry.txs.size()");
              if (!m_core.cleanup_handle_incoming_blocks())
              {
                LOG_PRINT_CCONTEXT_L0("Failure in cleanup_handle_incoming_blocks");
                return 1;
              }
              return 1;
            }
            std::vector<tx_blob_entry>::const_iterator it = block_entry.txs.begin();
            for (size_t i = 0; i < tvc.size(); ++i, ++it)
            {
              if(tvc[i].m_verifivation_failed)
              {
                if (!m_p2p->for_connection(span_connection_id, [&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t f)->bool{
                  cryptonote::transaction tx;
                  crypto::hash txid;
                  if (it->prunable_hash == crypto::null_hash)
                  {
                    parse_and_validate_tx_from_blob(it->blob, tx, txid); // must succeed if we got here
                  }
                  else
                  {
                    parse_and_validate_tx_base_from_blob(it->blob, tx); // must succeed if we got here
                    txid = get_pruned_transaction_hash(tx, it->prunable_hash);
                  }
                  LOG_ERROR_CCONTEXT("transaction verification failed on NOTIFY_RESPONSE_GET_OBJECTS, tx_id = "
                      << epee::string_tools::pod_to_hex(txid) << ", dropping connection");
                  drop_connection(context, false, true);
                  return 1;
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
            block_verification_context bvc = {};

            m_core.handle_incoming_block(block_entry.block, pblocks.empty() ? NULL : &pblocks[blockidx], bvc, false); // <--- process block

            if(bvc.m_verifivation_failed)
            {
              if (!m_p2p->for_connection(span_connection_id, [&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t f)->bool{
                LOG_PRINT_CCONTEXT_L1("Block verification failed, dropping connection");
                drop_connection_with_score(context, bvc.m_bad_pow ? P2P_IP_FAILS_BEFORE_BLOCK : 1, true);
                return 1;
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
                return 1;
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
            ++blockidx;

          } // each download block

          MDEBUG(context << "Block process time (" << blocks.size() << " blocks, " << num_txs << " txs): " << block_process_time_full + transactions_process_time_full << " (" << transactions_process_time_full << "/" << block_process_time_full << ") ms");

          if (!m_core.cleanup_handle_incoming_blocks())
          {
            LOG_PRINT_CCONTEXT_L0("Failure in cleanup_handle_incoming_blocks");
            return 1;
          }

          m_block_queue.remove_spans(span_connection_id, start_height);

          const uint64_t current_blockchain_height = m_core.get_current_blockchain_height();
          if (current_blockchain_height > previous_height)
          {
            const uint64_t target_blockchain_height = m_core.get_target_blockchain_height();
            const boost::posix_time::time_duration dt = boost::posix_time::microsec_clock::universal_time() - start;
            std::string progress_message = "";
            if (current_blockchain_height < target_blockchain_height)
            {
              uint64_t completion_percent = (current_blockchain_height * 100 / target_blockchain_height);
              if (completion_percent == 100) // never show 100% if not actually up to date
                completion_percent = 99;
              progress_message = " (" + std::to_string(completion_percent) + "%, "
                  + std::to_string(target_blockchain_height - current_blockchain_height) + " left)";
            }
            const uint32_t previous_stripe = tools::get_pruning_stripe(previous_height, target_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES);
            const uint32_t current_stripe = tools::get_pruning_stripe(current_blockchain_height, target_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES);
            std::string timing_message = "";
            if (ELPP->vRegistry()->allowed(el::Level::Info, "sync-info"))
              timing_message = std::string(" (") + std::to_string(dt.total_microseconds()/1e6) + " sec, "
                + std::to_string((current_blockchain_height - previous_height) * 1e6 / dt.total_microseconds())
                + " blocks/sec), " + std::to_string(m_block_queue.get_data_size() / 1048576.f) + " MB queued in "
                + std::to_string(m_block_queue.get_num_filled_spans()) + " spans, stripe "
                + std::to_string(previous_stripe) + " -> " + std::to_string(current_stripe);
            if (ELPP->vRegistry()->allowed(el::Level::Debug, "sync-info"))
              timing_message += std::string(": ") + m_block_queue.get_overview(current_blockchain_height);
            MGINFO_YELLOW("Synced " << current_blockchain_height << "/" << target_blockchain_height
                << progress_message << timing_message);
            if (previous_stripe != current_stripe)
              notify_new_stripe(context, current_stripe);
          }
        }
      }

      MLOG_PEER_STATE("stopping adding blocks");

      if (should_download_next_span(context, false))
      {
        force_next_span = true;
      }
      else if (should_drop_connection(context, get_next_needed_pruning_stripe().first))
      {
        if (!context.m_is_income)
        {
          m_p2p->add_used_stripe_peer(context);
          drop_connection(context, false, false);
        }
        return 1;
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
  void t_cryptonote_protocol_handler<t_core>::notify_new_stripe(cryptonote_connection_context& cntxt, uint32_t stripe)
  {
    m_p2p->for_each_connection([&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)->bool
    {
      if (cntxt.m_connection_id == context.m_connection_id)
        return true;
      if (context.m_state == cryptonote_connection_context::state_normal)
      {
        const uint32_t peer_stripe = tools::get_pruning_stripe(context.m_pruning_seed);
        if (stripe && peer_stripe && peer_stripe != stripe)
          return true;
        context.m_state = cryptonote_connection_context::state_synchronizing;
        LOG_PRINT_CCONTEXT_L2("requesting callback");
        ++context.m_callback_request_count;
        m_p2p->request_callback(context);
        MLOG_PEER_STATE("requesting callback");
      }
      return true;
    });
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::on_idle()
  {
    m_idle_peer_kicker.do_call(boost::bind(&t_cryptonote_protocol_handler<t_core>::kick_idle_peers, this));
    m_standby_checker.do_call(boost::bind(&t_cryptonote_protocol_handler<t_core>::check_standby_peers, this));
    m_sync_search_checker.do_call(boost::bind(&t_cryptonote_protocol_handler<t_core>::update_sync_search, this));
    return m_core.on_idle();
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::kick_idle_peers()
  {
    MTRACE("Checking for idle peers...");
    m_p2p->for_each_connection([&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)->bool
    {
      if (context.m_state == cryptonote_connection_context::state_synchronizing && context.m_last_request_time != boost::date_time::not_a_date_time)
      {
        const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
        const boost::posix_time::time_duration dt = now - context.m_last_request_time;
        if (dt.total_microseconds() > IDLE_PEER_KICK_TIME)
        {
          MINFO(context << " kicking idle peer, last update " << (dt.total_microseconds() / 1.e6) << " seconds ago");
          LOG_PRINT_CCONTEXT_L2("requesting callback");
          context.m_last_request_time = boost::date_time::not_a_date_time;
          context.m_state = cryptonote_connection_context::state_standby; // we'll go back to adding, then (if we can't), download
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
  bool t_cryptonote_protocol_handler<t_core>::update_sync_search()
  {
    const uint64_t target = m_core.get_target_blockchain_height();
    const uint64_t height = m_core.get_current_blockchain_height();
    if (target > height) // if we're not synced yet, don't do it
      return true;

    MTRACE("Checking for outgoing syncing peers...");
    unsigned n_syncing = 0, n_synced = 0;
    boost::uuids::uuid last_synced_peer_id(boost::uuids::nil_uuid());
    m_p2p->for_each_connection([&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)->bool
    {
      if (!peer_id || context.m_is_income) // only consider connected outgoing peers
        return true;
      if (context.m_state == cryptonote_connection_context::state_synchronizing)
        ++n_syncing;
      if (context.m_state == cryptonote_connection_context::state_normal)
      {
        ++n_synced;
        if (!context.m_anchor)
          last_synced_peer_id = context.m_connection_id;
      }
      return true;
    });
    MTRACE(n_syncing << " syncing, " << n_synced << " synced");

    // if we're at max out peers, and not enough are syncing
    if (n_synced + n_syncing >= m_max_out_peers && n_syncing < P2P_DEFAULT_SYNC_SEARCH_CONNECTIONS_COUNT && last_synced_peer_id != boost::uuids::nil_uuid())
    {
      if (!m_p2p->for_connection(last_synced_peer_id, [&](cryptonote_connection_context& ctx, nodetool::peerid_type peer_id, uint32_t f)->bool{
        MINFO(ctx << "dropping synced peer, " << n_syncing << " syncing, " << n_synced << " synced");
        drop_connection(ctx, false, false);
        return true;
      }))
        MDEBUG("Failed to find peer we wanted to drop");
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::check_standby_peers()
  {
    m_p2p->for_each_connection([&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)->bool
    {
      if (context.m_state == cryptonote_connection_context::state_standby)
      {
        LOG_PRINT_CCONTEXT_L2("requesting callback");
        ++context.m_callback_request_count;
        m_p2p->request_callback(context);
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
    if(!m_core.find_blockchain_supplement(arg.block_ids, !arg.prune, r))
    {
      LOG_ERROR_CCONTEXT("Failed to handle NOTIFY_REQUEST_CHAIN.");
      drop_connection(context, false, false);
      return 1;
    }
    MLOG_P2P_MESSAGE("-->>NOTIFY_RESPONSE_CHAIN_ENTRY: m_start_height=" << r.start_height << ", m_total_height=" << r.total_height << ", m_block_ids.size()=" << r.m_block_ids.size());
    post_notify<NOTIFY_RESPONSE_CHAIN_ENTRY>(r, context);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::should_download_next_span(cryptonote_connection_context& context, bool standby)
  {
    std::vector<crypto::hash> hashes;
    boost::uuids::uuid span_connection_id;
    boost::posix_time::ptime request_time;
    boost::uuids::uuid connection_id;
    std::pair<uint64_t, uint64_t> span;
    bool filled;

    const uint64_t blockchain_height = m_core.get_current_blockchain_height();
    if (context.m_remote_blockchain_height <= blockchain_height)
      return false;
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    const bool has_next_block = tools::has_unpruned_block(blockchain_height, context.m_remote_blockchain_height, context.m_pruning_seed);
    if (has_next_block)
    {
      if (!m_block_queue.has_next_span(blockchain_height, filled, request_time, connection_id))
      {
        MDEBUG(context << " we should download it as no peer reserved it");
        return true;
      }
      if (!filled)
      {
        const long dt = (now - request_time).total_microseconds();
        if (dt >= REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD)
        {
          MDEBUG(context << " we should download it as it's not been received yet after " << dt/1e6);
          return true;
        }

        // in standby, be ready to double download early since we're idling anyway
        // let the fastest peer trigger first
        long threshold;
        const double dl_speed = context.m_max_speed_down;
        if (standby && dt >= REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD_STANDBY && dl_speed > 0)
        {
          bool download = false;
          if (m_p2p->for_connection(connection_id, [&](cryptonote_connection_context& ctx, nodetool::peerid_type peer_id, uint32_t f)->bool{
            const time_t nowt = time(NULL);
            const time_t time_since_last_recv = nowt - ctx.m_last_recv;
            const float last_activity = std::min((float)time_since_last_recv, dt/1e6f);
            const bool stalled = last_activity > LAST_ACTIVITY_STALL_THRESHOLD;
            if (stalled)
            {
              MDEBUG(context << " we should download it as the downloading peer is stalling for " << nowt - ctx.m_last_recv << " seconds");
              download = true;
              return true;
            }

            // estimate the standby peer can give us 80% of its max speed
            // and let it download if that speed is > N times as fast as the current one
            // N starts at 10 after REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD_STANDBY,
            // decreases to 1.25 at REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD,
            // so that at times goes without the download being done, a retry becomes easier
            const float max_multiplier = 10.f;
            const float min_multiplier = 1.25f;
            float multiplier = max_multiplier;
            if (dt >= REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD_STANDBY)
            {
              multiplier = max_multiplier - (dt-REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD_STANDBY) * (max_multiplier - min_multiplier) / (REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD - REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD_STANDBY);
              multiplier = std::min(max_multiplier, std::max(min_multiplier, multiplier));
            }
            if (dl_speed * .8f > ctx.m_current_speed_down * multiplier)
            {
              MDEBUG(context << " we should download it as we are substantially faster (" << dl_speed << " vs "
                  << ctx.m_current_speed_down << ", multiplier " << multiplier << " after " << dt/1e6 << " seconds)");
              download = true;
              return true;
            }
            return true;
          }))
          {
            if (download)
              return true;
          }
          else
          {
            MWARNING(context << " we should download it as the downloading peer is unexpectedly not known to us");
            return true;
          }
        }
      }
    }

    return false;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::should_drop_connection(cryptonote_connection_context& context, uint32_t next_stripe)
  {
    if (context.m_anchor)
    {
      MDEBUG(context << "This is an anchor peer, not dropping");
      return false;
    }
    if (context.m_pruning_seed == 0)
    {
      MDEBUG(context << "This peer is not striped, not dropping");
      return false;
    }

    const uint32_t peer_stripe = tools::get_pruning_stripe(context.m_pruning_seed);
    if (next_stripe == peer_stripe)
    {
      MDEBUG(context << "This peer has needed stripe " << peer_stripe << ", not dropping");
      return false;
    }
    const uint32_t local_stripe = tools::get_pruning_stripe(m_core.get_blockchain_pruning_seed());
    if (m_sync_pruned_blocks && local_stripe && next_stripe != local_stripe)
    {
      MDEBUG(context << "We can sync pruned blocks off this peer, not dropping");
      return false;
    }

    if (!context.m_needed_objects.empty())
    {
      const uint64_t next_available_block_height = context.m_last_response_height - context.m_needed_objects.size() + 1;
      if (tools::has_unpruned_block(next_available_block_height, context.m_remote_blockchain_height, context.m_pruning_seed))
      {
        MDEBUG(context << "This peer has unpruned next block at height " << next_available_block_height << ", not dropping");
        return false;
      }
    }

    if (next_stripe > 0)
    {
      unsigned int n_out_peers = 0, n_peers_on_next_stripe = 0;
      m_p2p->for_each_connection([&](cryptonote_connection_context& ctx, nodetool::peerid_type peer_id, uint32_t support_flags)->bool{
        if (!ctx.m_is_income)
          ++n_out_peers;
        if (ctx.m_state >= cryptonote_connection_context::state_synchronizing && tools::get_pruning_stripe(ctx.m_pruning_seed) == next_stripe)
          ++n_peers_on_next_stripe;
        return true;
      });
      const uint32_t distance = (peer_stripe + (1<<CRYPTONOTE_PRUNING_LOG_STRIPES) - next_stripe) % (1<<CRYPTONOTE_PRUNING_LOG_STRIPES);
      if ((n_out_peers >= m_max_out_peers && n_peers_on_next_stripe == 0) || (distance > 1 && n_peers_on_next_stripe <= 2) || distance > 2)
      {
        MDEBUG(context << "we want seed " << next_stripe << ", and either " << n_out_peers << " is at max out peers ("
            << m_max_out_peers << ") or distance " << distance << " from " << next_stripe << " to " << peer_stripe <<
            " is too large and we have only " << n_peers_on_next_stripe << " peers on next seed, dropping connection to make space");
        return true;
      }
    }
    MDEBUG(context << "End of checks, not dropping");
    return false;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_cryptonote_protocol_handler<t_core>::skip_unneeded_hashes(cryptonote_connection_context& context, bool check_block_queue) const
  {
    // take out blocks we already have
    size_t skip = 0;
    while (skip < context.m_needed_objects.size() && (m_core.have_block(context.m_needed_objects[skip].first) || (check_block_queue && m_block_queue.have(context.m_needed_objects[skip].first))))
    {
      // if we're popping the last hash, record it so we can ask again from that hash,
      // this prevents never being able to progress on peers we get old hash lists from
      if (skip + 1 == context.m_needed_objects.size())
        context.m_last_known_hash = context.m_needed_objects[skip].first;
      ++skip;
    }
    if (skip > 0)
    {
      MDEBUG(context << "skipping " << skip << "/" << context.m_needed_objects.size() << " blocks");
      context.m_needed_objects = std::vector<std::pair<crypto::hash, uint64_t>>(context.m_needed_objects.begin() + skip, context.m_needed_objects.end());
    }
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::should_ask_for_pruned_data(cryptonote_connection_context& context, uint64_t first_block_height, uint64_t nblocks, bool check_block_weights) const
  {
    if (!m_sync_pruned_blocks)
      return false;
    if (!m_core.is_within_compiled_block_hash_area(first_block_height + nblocks - 1))
      return false;
    const uint32_t local_stripe = tools::get_pruning_stripe(m_core.get_blockchain_pruning_seed());
    if (local_stripe == 0)
      return false;
    // assumes the span size is less or equal to the stripe size
    bool full_data_needed = tools::get_pruning_stripe(first_block_height, context.m_remote_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES) == local_stripe
        || tools::get_pruning_stripe(first_block_height + nblocks - 1, context.m_remote_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES) == local_stripe;
    if (full_data_needed)
      return false;
    if (check_block_weights && !m_core.has_block_weights(first_block_height, nblocks))
      return false;
    return true;
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
      do
      {
        size_t nspans = m_block_queue.get_num_filled_spans();
        size_t size = m_block_queue.get_data_size();
        const uint64_t bc_height = m_core.get_current_blockchain_height();
        const auto next_needed_pruning_stripe = get_next_needed_pruning_stripe();
        const uint32_t add_stripe = tools::get_pruning_stripe(bc_height, context.m_remote_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES);
        const uint32_t peer_stripe = tools::get_pruning_stripe(context.m_pruning_seed);
        const uint32_t local_stripe = tools::get_pruning_stripe(m_core.get_blockchain_pruning_seed());
        const size_t block_queue_size_threshold = m_block_download_max_size ? m_block_download_max_size : BLOCK_QUEUE_SIZE_THRESHOLD;
        bool queue_proceed = nspans < BLOCK_QUEUE_NSPANS_THRESHOLD || size < block_queue_size_threshold;
        // get rid of blocks we already requested, or already have
        skip_unneeded_hashes(context, true);
        uint64_t next_needed_height = m_block_queue.get_next_needed_height(bc_height);
        uint64_t next_block_height;
        if (context.m_needed_objects.empty())
          next_block_height = next_needed_height;
        else
          next_block_height = context.m_last_response_height - context.m_needed_objects.size() + 1;
        bool stripe_proceed_main = ((m_sync_pruned_blocks && local_stripe && add_stripe != local_stripe) || add_stripe == 0 || peer_stripe == 0 || add_stripe == peer_stripe) && (next_block_height < bc_height + BLOCK_QUEUE_FORCE_DOWNLOAD_NEAR_BLOCKS || next_needed_height < bc_height + BLOCK_QUEUE_FORCE_DOWNLOAD_NEAR_BLOCKS);
        bool stripe_proceed_secondary = tools::has_unpruned_block(next_block_height, context.m_remote_blockchain_height, context.m_pruning_seed);
        bool proceed = stripe_proceed_main || (queue_proceed && stripe_proceed_secondary);
        if (!stripe_proceed_main && !stripe_proceed_secondary && should_drop_connection(context, tools::get_pruning_stripe(next_block_height, context.m_remote_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES)))
        {
          if (!context.m_is_income)
            m_p2p->add_used_stripe_peer(context);
          return false; // drop outgoing connections
        }

        MDEBUG(context << "proceed " << proceed << " (queue " << queue_proceed << ", stripe " << stripe_proceed_main << "/" <<
          stripe_proceed_secondary << "), " << next_needed_pruning_stripe.first << "-" << next_needed_pruning_stripe.second <<
          " needed, bc add stripe " << add_stripe << ", we have " << peer_stripe << "), bc_height " << bc_height);
        MDEBUG(context << "  - next_block_height " << next_block_height << ", seed " << epee::string_tools::to_string_hex(context.m_pruning_seed) <<
          ", next_needed_height "<< next_needed_height);
        MDEBUG(context << "  - last_response_height " << context.m_last_response_height << ", m_needed_objects size " << context.m_needed_objects.size());

        // if we're waiting for next span, try to get it before unblocking threads below,
        // or a runaway downloading of future spans might happen
        if (stripe_proceed_main && should_download_next_span(context, true))
        {
          MDEBUG(context << " we should try for that next span too, we think we could get it faster, resuming");
          force_next_span = true;
          MLOG_PEER_STATE("resuming");
          break;
        }

        if (proceed)
        {
          if (context.m_state != cryptonote_connection_context::state_standby)
          {
            LOG_DEBUG_CC(context, "Block queue is " << nspans << " and " << size << ", resuming");
            MLOG_PEER_STATE("resuming");
          }
          break;
        }

        // this one triggers if all threads are in standby, which should not happen,
        // but happened at least once, so we unblock at least one thread if so
        boost::unique_lock<boost::mutex> sync{m_sync_lock, boost::try_to_lock};
        if (sync.owns_lock())
        {
          bool filled = false;
          boost::posix_time::ptime time;
          boost::uuids::uuid connection_id;
          if (m_block_queue.has_next_span(m_core.get_current_blockchain_height(), filled, time, connection_id) && filled)
          {
            LOG_DEBUG_CC(context, "No other thread is adding blocks, and next span needed is ready, resuming");
            MLOG_PEER_STATE("resuming");
            context.m_state = cryptonote_connection_context::state_standby;
            ++context.m_callback_request_count;
            m_p2p->request_callback(context);
            return true;
          }
          else
          {
            sync.unlock();

            // if this has gone on for too long, drop incoming connection to guard against some wedge state
            if (!context.m_is_income)
            {
              const uint64_t now = tools::get_tick_count();
              const uint64_t dt = now - m_last_add_end_time;
              if (m_last_add_end_time && tools::ticks_to_ns(dt) >= DROP_ON_SYNC_WEDGE_THRESHOLD)
              {
                MDEBUG(context << "ns " << tools::ticks_to_ns(dt) << " from " << m_last_add_end_time << " and " << now);
                MDEBUG(context << "Block addition seems to have wedged, dropping connection");
                return false;
              }
            }
          }
        }

        if (context.m_state != cryptonote_connection_context::state_standby)
        {
          if (!queue_proceed)
            LOG_DEBUG_CC(context, "Block queue is " << nspans << " and " << size << ", pausing");
          else if (!stripe_proceed_main && !stripe_proceed_secondary)
            LOG_DEBUG_CC(context, "We do not have the stripe required to download another block, pausing");
          context.m_state = cryptonote_connection_context::state_standby;
          MLOG_PEER_STATE("pausing");
        }

        return true;
      } while(0);
      context.m_state = cryptonote_connection_context::state_synchronizing;
    }

    MDEBUG(context << " request_missing_objects: check " << check_having_blocks << ", force_next_span " << force_next_span
        << ", m_needed_objects " << context.m_needed_objects.size() << " lrh " << context.m_last_response_height << ", chain "
        << m_core.get_current_blockchain_height() << ", pruning seed " << epee::string_tools::to_string_hex(context.m_pruning_seed));
    if(context.m_needed_objects.size() || force_next_span)
    {
      //we know objects that we need, request this objects
      NOTIFY_REQUEST_GET_OBJECTS::request req;
      bool is_next = false;
      size_t count = 0;
      const size_t count_limit = m_core.get_block_sync_size(m_core.get_current_blockchain_height());
      std::pair<uint64_t, uint64_t> span = std::make_pair(0, 0);
      if (force_next_span)
      {
        if (span.second == 0)
        {
          std::vector<crypto::hash> hashes;
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
            m_block_queue.reset_next_span_time();
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
        skip_unneeded_hashes(context, false);

        const uint64_t first_block_height = context.m_last_response_height - context.m_needed_objects.size() + 1;
        bool sync_pruned_blocks = m_sync_pruned_blocks && m_core.get_blockchain_pruning_seed();
        span = m_block_queue.reserve_span(first_block_height, context.m_last_response_height, count_limit, context.m_connection_id, sync_pruned_blocks, m_core.get_blockchain_pruning_seed(), context.m_pruning_seed, context.m_remote_blockchain_height, context.m_needed_objects);
        MDEBUG(context << " span from " << first_block_height << ": " << span.first << "/" << span.second);
        if (span.second > 0)
        {
          const uint32_t stripe = tools::get_pruning_stripe(span.first, context.m_remote_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES);
          if (context.m_pruning_seed && stripe != tools::get_pruning_stripe(context.m_pruning_seed))
          {
            MDEBUG(context << " starting early on next seed (" << span.first << "  with stripe " << stripe <<
                ", context seed " << epee::string_tools::to_string_hex(context.m_pruning_seed) << ")");
          }
        }
      }
      if (span.second == 0 && !force_next_span)
      {
        MDEBUG(context << " still no span reserved, we may be in the corner case of next span scheduled and everything else scheduled/filled");
        std::vector<crypto::hash> hashes;
        boost::uuids::uuid span_connection_id;
        boost::posix_time::ptime time;
        span = m_block_queue.get_next_span_if_scheduled(hashes, span_connection_id, time);
        if (span.second > 0 && !tools::has_unpruned_block(span.first, context.m_remote_blockchain_height, context.m_pruning_seed))
          span = std::make_pair(0, 0);
        if (span.second > 0)
        {
          is_next = true;
          for (const auto &hash: hashes)
          {
            req.blocks.push_back(hash);
            ++count;
            context.m_requested_objects.insert(hash);
            // that's atrocious O(n) wise, but this is rare
            auto i = std::find_if(context.m_needed_objects.begin(), context.m_needed_objects.end(),
                [&hash](const std::pair<crypto::hash, uint64_t> &o) { return o.first == hash; });
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
          if (skip > 0)
            context.m_needed_objects = std::vector<std::pair<crypto::hash, uint64_t>>(context.m_needed_objects.begin() + skip, context.m_needed_objects.end());
          if (context.m_needed_objects.size() < span.second)
          {
            MERROR("ERROR: span " << span.first << "/" << span.second << ", m_needed_objects " << context.m_needed_objects.size());
            return false;
          }

          for (size_t n = 0; n < span.second; ++n)
          {
            req.blocks.push_back(context.m_needed_objects[n].first);
            ++count;
            context.m_requested_objects.insert(context.m_needed_objects[n].first);
          }
          context.m_needed_objects = std::vector<std::pair<crypto::hash, uint64_t>>(context.m_needed_objects.begin() + span.second, context.m_needed_objects.end());
        }

        req.prune = should_ask_for_pruned_data(context, span.first, span.second, true);

        // if we need to ask for full data and that peer does not have the right stripe, we can't ask it
        if (!req.prune && context.m_pruning_seed)
        {
          const uint32_t peer_stripe = tools::get_pruning_stripe(context.m_pruning_seed);
          const uint32_t first_stripe = tools::get_pruning_stripe(span.first, context.m_remote_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES);
          const uint32_t last_stripe = tools::get_pruning_stripe(span.first + span.second - 1, context.m_remote_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES);
          if ((((first_stripe && peer_stripe != first_stripe) || (last_stripe && peer_stripe != last_stripe)) && !m_sync_pruned_blocks) || (m_sync_pruned_blocks && req.prune))
          {
            MDEBUG(context << "We need full data, but the peer does not have it, dropping peer");
            return false;
          }
        }
        context.m_last_request_time = boost::posix_time::microsec_clock::universal_time();
        MLOG_P2P_MESSAGE("-->>NOTIFY_REQUEST_GET_OBJECTS: blocks.size()=" << req.blocks.size()
            << "requested blocks count=" << count << " / " << count_limit << " from " << span.first << ", first hash " << req.blocks.front());
        //epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());

        MDEBUG("Asking for " << (req.prune ? "pruned" : "full") << " data, start/end "
          << tools::get_pruning_stripe(span.first, context.m_remote_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES)
          << "/" << tools::get_pruning_stripe(span.first + span.second - 1, context.m_remote_blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES)
          << ", ours " << tools::get_pruning_stripe(m_core.get_blockchain_pruning_seed()) << ", peer stripe " << tools::get_pruning_stripe(context.m_pruning_seed));

        post_notify<NOTIFY_REQUEST_GET_OBJECTS>(req, context);
        MLOG_PEER_STATE("requesting objects");
        return true;
      }

      // if we're still around, we might be at a point where the peer is pruned, so we could either
      // drop it to make space for other peers, or ask for a span further down the line
      const uint32_t next_stripe = get_next_needed_pruning_stripe().first;
      const uint32_t peer_stripe = tools::get_pruning_stripe(context.m_pruning_seed);
      const uint32_t local_stripe = tools::get_pruning_stripe(m_core.get_blockchain_pruning_seed());
      if (!(m_sync_pruned_blocks && peer_stripe == local_stripe) && next_stripe && peer_stripe && next_stripe != peer_stripe)
      {
        // at this point, we have to either close the connection, or start getting blocks past the
        // current point, or become dormant
        MDEBUG(context << "this peer is pruned at seed " << epee::string_tools::to_string_hex(context.m_pruning_seed) <<
            ", next stripe needed is " << next_stripe);
        if (!context.m_is_income)
        {
          if (should_drop_connection(context, next_stripe))
          {
            m_p2p->add_used_stripe_peer(context);
            return false; // drop outgoing connections
          }
        }
        // we'll get back stuck waiting for the go ahead
        context.m_state = cryptonote_connection_context::state_normal;
        MLOG_PEER_STATE("Nothing to do for now, switching to normal state");
        return true;
      }
    }

skip:
    context.m_needed_objects.clear();

    // we might have been called from the "received chain entry" handler, and end up
    // here because we can't use any of those blocks (maybe because all of them are
    // actually already requested). In this case, if we can add blocks instead, do so
    if (m_core.get_current_blockchain_height() < m_core.get_target_blockchain_height())
    {
      const boost::unique_lock<boost::mutex> sync{m_sync_lock, boost::try_to_lock};
      if (sync.owns_lock())
      {
        uint64_t start_height;
        std::vector<cryptonote::block_complete_entry> blocks;
        boost::uuids::uuid span_connection_id;
        bool filled = false;
        if (m_block_queue.get_next_span(start_height, blocks, span_connection_id, filled) && filled)
        {
          LOG_DEBUG_CC(context, "No other thread is adding blocks, resuming");
          MLOG_PEER_STATE("will try to add blocks next");
          context.m_state = cryptonote_connection_context::state_standby;
          ++context.m_callback_request_count;
          m_p2p->request_callback(context);
          return true;
        }
      }
    }

    if(context.m_last_response_height < context.m_remote_blockchain_height-1)
    {//we have to fetch more objects ids, request blockchain entry

      NOTIFY_REQUEST_CHAIN::request r = {};
      m_core.get_short_chain_history(r.block_ids);
      CHECK_AND_ASSERT_MES(!r.block_ids.empty(), false, "Short chain history is empty");

      if (!start_from_current_chain)
      {
        // we'll want to start off from where we are on that peer, which may not be added yet
        if (context.m_last_known_hash != crypto::null_hash && r.block_ids.front() != context.m_last_known_hash)
          r.block_ids.push_front(context.m_last_known_hash);
      }

      handler_request_blocks_history( r.block_ids ); // change the limit(?), sleep(?)
      r.prune = m_sync_pruned_blocks;

      //std::string blob; // for calculate size of request
      //epee::serialization::store_t_to_binary(r, blob);
      //epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());
      //LOG_PRINT_CCONTEXT_L1("r = " << 200);

      context.m_last_request_time = boost::posix_time::microsec_clock::universal_time();
      MLOG_P2P_MESSAGE("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() << ", start_from_current_chain " << start_from_current_chain);
      post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
      MLOG_PEER_STATE("requesting chain");
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
    if(!m_core.is_within_compiled_block_hash_area(m_core.get_current_blockchain_height()) && m_synchronized.compare_exchange_strong(val_expected, true))
    {
      MGINFO_YELLOW(ENDL << "**********************************************************************" << ENDL
        << "You are now synchronized with the network. You may now start monero-wallet-cli." << ENDL
        << ENDL
        << "Use the \"help\" command to see a simplified list of available commands." << ENDL
        << "Use the \"help_advanced\" command to see an advanced list of available commands." << ENDL
        << "**********************************************************************");
      m_sync_timer.pause();
      if (ELPP->vRegistry()->allowed(el::Level::Info, "sync-info"))
      {
        const uint64_t sync_time = m_sync_timer.value();
        const uint64_t add_time = m_add_timer.value();
        if (sync_time && add_time)
        {
          MCLOG_YELLOW(el::Level::Info, "sync-info", "Sync time: " << sync_time/1e9/60 << " min, idle time " <<
              (100.f * (1.0f - add_time / (float)sync_time)) << "%" << ", " <<
              (10 * m_sync_download_objects_size / 1024 / 1024) / 10.f << " + " <<
              (10 * m_sync_download_chain_size / 1024 / 1024) / 10.f << " MB downloaded, " <<
              100.0f * m_sync_old_spans_downloaded / m_sync_spans_downloaded << "% old spans, " <<
              100.0f * m_sync_bad_spans_downloaded / m_sync_spans_downloaded << "% bad spans");
        }
      }
      m_core.on_synchronized();
    }
    m_core.safesyncmode(true);
    m_p2p->clear_used_stripe_peers();

    // ask for txpool complement from any suitable node if we did not yet
    val_expected = true;
    if (m_ask_for_txpool_complement.compare_exchange_strong(val_expected, false))
    {
      m_p2p->for_each_connection([&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)->bool
      {
        if(context.m_state < cryptonote_connection_context::state_synchronizing)
        {
          MDEBUG(context << "not ready, ignoring");
          return true;
        }
        if (!request_txpool_complement(context))
        {
          MERROR(context << "Failed to request txpool complement");
          return true;
        }
        return false;
      });
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  size_t t_cryptonote_protocol_handler<t_core>::get_synchronizing_connections_count()
  {
    size_t count = 0;
    m_p2p->for_each_connection([&](cryptonote_connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)->bool{
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
    MLOG_PEER_STATE("received chain");

    context.m_last_request_time = boost::date_time::not_a_date_time;

    m_sync_download_chain_size += arg.m_block_ids.size() * sizeof(crypto::hash);

    if(!arg.m_block_ids.size())
    {
      LOG_ERROR_CCONTEXT("sent empty m_block_ids, dropping connection");
      drop_connection(context, true, false);
      return 1;
    }
    if (arg.total_height < arg.m_block_ids.size() || arg.start_height > arg.total_height - arg.m_block_ids.size())
    {
      LOG_ERROR_CCONTEXT("sent invalid start/nblocks/height, dropping connection");
      drop_connection(context, true, false);
      return 1;
    }
    if (!arg.m_block_weights.empty() && arg.m_block_weights.size() != arg.m_block_ids.size())
    {
      LOG_ERROR_CCONTEXT("sent invalid block weight array, dropping connection");
      drop_connection(context, true, false);
      return 1;
    }
    MDEBUG(context << "first block hash " << arg.m_block_ids.front() << ", last " << arg.m_block_ids.back());

    if (arg.total_height >= CRYPTONOTE_MAX_BLOCK_NUMBER || arg.m_block_ids.size() >= CRYPTONOTE_MAX_BLOCK_NUMBER)
    {
      LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_CHAIN_ENTRY, with total_height=" << arg.total_height << " and block_ids=" << arg.m_block_ids.size());
      drop_connection(context, false, false);
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

    uint64_t n_use_blocks = m_core.prevalidate_block_hashes(arg.start_height, arg.m_block_ids, arg.m_block_weights);
    if (n_use_blocks + HASH_OF_HASHES_STEP <= arg.m_block_ids.size())
    {
      LOG_ERROR_CCONTEXT("Most blocks are invalid, dropping connection");
      drop_connection(context, true, false);
      return 1;
    }

    context.m_needed_objects.clear();
    uint64_t added = 0;
    for (size_t i = 0; i < arg.m_block_ids.size(); ++i)
    {
      const uint64_t block_weight = arg.m_block_weights.empty() ? 0 : arg.m_block_weights[i];
      context.m_needed_objects.push_back(std::make_pair(arg.m_block_ids[i], block_weight));
      if (++added == n_use_blocks)
        break;
    }
    context.m_last_response_height -= arg.m_block_ids.size() - n_use_blocks;

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
    fluffy_arg.current_blockchain_height = arg.current_blockchain_height;    
    std::vector<tx_blob_entry> fluffy_txs;
    fluffy_arg.b = arg.b;
    fluffy_arg.b.txs = fluffy_txs;

    // sort peers between fluffy ones and others
    std::vector<std::pair<epee::net_utils::zone, boost::uuids::uuid>> fullConnections, fluffyConnections;
    m_p2p->for_each_connection([this, &exclude_context, &fullConnections, &fluffyConnections](connection_context& context, nodetool::peerid_type peer_id, uint32_t support_flags)
    {
      if (peer_id && exclude_context.m_connection_id != context.m_connection_id && context.m_remote_address.get_zone() == epee::net_utils::zone::public_)
      {
        if(m_core.fluffy_blocks_enabled() && (support_flags & P2P_SUPPORT_FLAG_FLUFFY_BLOCKS))
        {
          LOG_DEBUG_CC(context, "PEER SUPPORTS FLUFFY BLOCKS - RELAYING THIN/COMPACT WHATEVER BLOCK");
          fluffyConnections.push_back({context.m_remote_address.get_zone(), context.m_connection_id});
        }
        else
        {
          LOG_DEBUG_CC(context, "PEER DOESN'T SUPPORT FLUFFY BLOCKS - RELAYING FULL BLOCK");
          fullConnections.push_back({context.m_remote_address.get_zone(), context.m_connection_id});
        }
      }
      return true;
    });

    // send fluffy ones first, we want to encourage people to run that
    if (!fluffyConnections.empty())
    {
      std::string fluffyBlob;
      epee::serialization::store_t_to_binary(fluffy_arg, fluffyBlob);
      m_p2p->relay_notify_to_list(NOTIFY_NEW_FLUFFY_BLOCK::ID, epee::strspan<uint8_t>(fluffyBlob), std::move(fluffyConnections));
    }
    if (!fullConnections.empty())
    {
      std::string fullBlob;
      epee::serialization::store_t_to_binary(arg, fullBlob);
      m_p2p->relay_notify_to_list(NOTIFY_NEW_BLOCK::ID, epee::strspan<uint8_t>(fullBlob), std::move(fullConnections));
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::relay_transactions(NOTIFY_NEW_TRANSACTIONS::request& arg, const boost::uuids::uuid& source, epee::net_utils::zone zone)
  {
    /* Push all outgoing transactions to this function. The behavior needs to
       identify how the transaction is going to be relayed, and then update the
       local mempool before doing the relay. The code was already updating the
       DB twice on received transactions - it is difficult to workaround this
       due to the internal design. */
    return m_p2p->send_txs(std::move(arg.txs), zone, source, m_core) != epee::net_utils::zone::invalid;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::request_txpool_complement(cryptonote_connection_context &context)
  {
    NOTIFY_GET_TXPOOL_COMPLEMENT::request r = {};
    if (!m_core.get_pool_transaction_hashes(r.hashes, false))
    {
      MERROR("Failed to get txpool hashes");
      return false;
    }
    MLOG_P2P_MESSAGE("-->>NOTIFY_GET_TXPOOL_COMPLEMENT: hashes.size()=" << r.hashes.size() );
    post_notify<NOTIFY_GET_TXPOOL_COMPLEMENT>(r, context);
    MLOG_PEER_STATE("requesting txpool complement");
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  std::string t_cryptonote_protocol_handler<t_core>::get_peers_overview() const
  {
    std::stringstream ss;
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    m_p2p->for_each_connection([&](const connection_context &ctx, nodetool::peerid_type peer_id, uint32_t support_flags) {
      const uint32_t stripe = tools::get_pruning_stripe(ctx.m_pruning_seed);
      char state_char = cryptonote::get_protocol_state_char(ctx.m_state);
      ss << stripe + state_char;
      if (ctx.m_last_request_time != boost::date_time::not_a_date_time)
        ss << (((now - ctx.m_last_request_time).total_microseconds() > IDLE_PEER_KICK_TIME) ? "!" : "?");
      ss <<  + " ";
      return true;
    });
    return ss.str();
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  std::pair<uint32_t, uint32_t> t_cryptonote_protocol_handler<t_core>::get_next_needed_pruning_stripe() const
  {
    const uint64_t want_height_from_blockchain = m_core.get_current_blockchain_height();
    const uint64_t want_height_from_block_queue = m_block_queue.get_next_needed_height(want_height_from_blockchain);
    const uint64_t want_height = std::max(want_height_from_blockchain, want_height_from_block_queue);
    uint64_t blockchain_height = m_core.get_target_blockchain_height();
    // if we don't know the remote chain size yet, assume infinitely large so we get the right stripe if we're not near the tip
    if (blockchain_height == 0)
      blockchain_height = CRYPTONOTE_MAX_BLOCK_NUMBER;
    const uint32_t next_pruning_stripe = tools::get_pruning_stripe(want_height, blockchain_height, CRYPTONOTE_PRUNING_LOG_STRIPES);
    if (next_pruning_stripe == 0)
      return std::make_pair(0, 0);
    // if we already have a few peers on this stripe, but none on next one, try next one
    unsigned int n_next = 0, n_subsequent = 0, n_others = 0;
    const uint32_t subsequent_pruning_stripe = 1 + next_pruning_stripe % (1<<CRYPTONOTE_PRUNING_LOG_STRIPES);
    m_p2p->for_each_connection([&](const connection_context &context, nodetool::peerid_type peer_id, uint32_t support_flags) {
      if (context.m_state >= cryptonote_connection_context::state_synchronizing)
      {
        if (context.m_pruning_seed == 0 || tools::get_pruning_stripe(context.m_pruning_seed) == next_pruning_stripe)
          ++n_next;
        else if (tools::get_pruning_stripe(context.m_pruning_seed) == subsequent_pruning_stripe)
          ++n_subsequent;
        else
          ++n_others;
      }
      return true;
    });
    const bool use_next = (n_next > m_max_out_peers / 2 && n_subsequent <= 1) || (n_next > 2 && n_subsequent == 0);
    const uint32_t ret_stripe = use_next ? subsequent_pruning_stripe: next_pruning_stripe;
    MIDEBUG(const std::string po = get_peers_overview(), "get_next_needed_pruning_stripe: want height " << want_height << " (" <<
        want_height_from_blockchain << " from blockchain, " << want_height_from_block_queue << " from block queue), stripe " <<
        next_pruning_stripe << " (" << n_next << "/" << m_max_out_peers << " on it and " << n_subsequent << " on " <<
        subsequent_pruning_stripe << ", " << n_others << " others) -> " << ret_stripe << " (+" <<
        (ret_stripe - next_pruning_stripe + (1 << CRYPTONOTE_PRUNING_LOG_STRIPES)) % (1 << CRYPTONOTE_PRUNING_LOG_STRIPES) <<
        "), current peers " << po);
    return std::make_pair(next_pruning_stripe, ret_stripe);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_cryptonote_protocol_handler<t_core>::needs_new_sync_connections() const
  {
    const uint64_t target = m_core.get_target_blockchain_height();
    const uint64_t height = m_core.get_current_blockchain_height();
    if (target && target <= height)
      return false;
    size_t n_out_peers = 0;
    m_p2p->for_each_connection([&](cryptonote_connection_context& ctx, nodetool::peerid_type peer_id, uint32_t support_flags)->bool{
      if (!ctx.m_is_income)
        ++n_out_peers;
      return true;
    });
    if (n_out_peers >= m_max_out_peers)
      return false;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_cryptonote_protocol_handler<t_core>::drop_connection_with_score(cryptonote_connection_context &context, unsigned score, bool flush_all_spans)
  {
    LOG_DEBUG_CC(context, "dropping connection id " << context.m_connection_id << " (pruning seed " <<
        epee::string_tools::to_string_hex(context.m_pruning_seed) <<
        "), score " << score << ", flush_all_spans " << flush_all_spans);

    if (score > 0)
      m_p2p->add_host_fail(context.m_remote_address, score);

    m_block_queue.flush_spans(context.m_connection_id, flush_all_spans);

    m_p2p->drop_connection(context);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_cryptonote_protocol_handler<t_core>::drop_connection(cryptonote_connection_context &context, bool add_fail, bool flush_all_spans)
  {
    return drop_connection_with_score(context, add_fail ? 1 : 0, flush_all_spans);
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
      if (target == 0 && context.m_state > cryptonote_connection_context::state_before_handshake && !m_stopping)
      {
        MCWARNING("global", "monerod is now disconnected from the network");
        m_ask_for_txpool_complement = true;
      }
    }

    m_block_queue.flush_spans(context.m_connection_id, false);
    MLOG_PEER_STATE("closed");
  }

  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_cryptonote_protocol_handler<t_core>::stop()
  {
    m_stopping = true;
    m_core.stop();
  }
} // namespace

