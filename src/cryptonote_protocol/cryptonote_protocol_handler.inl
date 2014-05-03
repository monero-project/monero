// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/interprocess/detail/atomic.hpp>
#include "cryptonote_core/cryptonote_format_utils.h"
#include "profile_tools.h"
namespace cryptonote
{

  //-----------------------------------------------------------------------------------------------------------------------  
  template<class t_core>
    t_cryptonote_protocol_handler<t_core>::t_cryptonote_protocol_handler(t_core& rcore, nodetool::i_p2p_endpoint<connection_context>* p_net_layout):m_core(rcore), 
                                                                                                              m_p2p(p_net_layout),
                                                                                                              m_syncronized_connections_count(0),
                                                                                                              m_synchronized(false)

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

    ss << std::setw(25) << std::left << "Remote Host" 
      << std::setw(20) << "Peer id"
      << std::setw(25) << "Recv/Sent (inactive,sec)"
      << std::setw(25) << "State"
      << std::setw(20) << "Livetime(seconds)" << ENDL;

    m_p2p->for_each_connection([&](const connection_context& cntxt, nodetool::peerid_type peer_id)
    {
      ss << std::setw(25) << std::left << std::string(cntxt.m_is_income ? " [INC]":"[OUT]") + 
        string_tools::get_ip_string_from_int32(cntxt.m_remote_ip) + ":" + std::to_string(cntxt.m_remote_port) 
        << std::setw(20) << std::hex << peer_id
        << std::setw(25) << std::to_string(cntxt.m_recv_cnt)+ "(" + std::to_string(time(NULL) - cntxt.m_last_recv) + ")" + "/" + std::to_string(cntxt.m_send_cnt) + "(" + std::to_string(time(NULL) - cntxt.m_last_send) + ")"
        << std::setw(25) << get_protocol_state_string(cntxt.m_state)
        << std::setw(20) << std::to_string(time(NULL) - cntxt.m_started) << ENDL;
      return true;
    });
    LOG_PRINT_L0("Connections: " << ENDL << ss.str());
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

    LOG_PRINT_CCONTEXT_YELLOW("Sync data returned unknown top block: " << m_core.get_current_blockchain_height() << "->" << hshd.current_height 
      << "[" << static_cast<int64_t>(hshd.current_height - m_core.get_current_blockchain_height()) << " blocks(" << (hshd.current_height - m_core.get_current_blockchain_height()) /1440 << " days) behind] " << ENDL 
      << "remote top: "  << hshd.top_id << "[" << hshd.current_height << "]" << ", set SYNCHRONIZATION mode", (is_inital ? LOG_LEVEL_0:LOG_LEVEL_1));
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
    LOG_PRINT_CCONTEXT_L2("NOTIFY_NEW_BLOCK (hop " << arg.hop << ")");
    if(context.m_state != cryptonote_connection_context::state_normal)
      return 1;

    for(auto tx_blob_it = arg.b.txs.begin(); tx_blob_it!=arg.b.txs.end();tx_blob_it++)
    {
      cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      m_core.handle_incoming_tx(*tx_blob_it, tvc, true);
      if(tvc.m_verifivation_failed)
      {
        LOG_PRINT_CCONTEXT_L0("Block verification failed: transaction verification failed, dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }
    }


    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    m_core.pause_mine();
    m_core.handle_incoming_block(arg.b.block, bvc);
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
  int t_cryptonote_protocol_handler<t_core>::handle_notify_new_transactions(int command, NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& context)
  {
    LOG_PRINT_CCONTEXT_L2("NOTIFY_NEW_TRANSACTIONS");
    if(context.m_state != cryptonote_connection_context::state_normal)
      return 1;

    for(auto tx_blob_it = arg.txs.begin(); tx_blob_it!=arg.txs.end();)
    {
      cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      m_core.handle_incoming_tx(*tx_blob_it, tvc, false);
      if(tvc.m_verifivation_failed)
      {
        LOG_PRINT_CCONTEXT_L0("Tx verification failed, dropping connection");
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
    LOG_PRINT_CCONTEXT_L2("NOTIFY_REQUEST_GET_OBJECTS");
    NOTIFY_RESPONSE_GET_OBJECTS::request rsp;
    if(!m_core.handle_get_objects(arg, rsp, context))
    {
      LOG_ERROR_CCONTEXT("failed to handle request NOTIFY_REQUEST_GET_OBJECTS, dropping connection");
      m_p2p->drop_connection(context);
    }
    LOG_PRINT_CCONTEXT_L2("-->>NOTIFY_RESPONSE_GET_OBJECTS: blocks.size()=" << rsp.blocks.size() << ", txs.size()=" << rsp.txs.size() 
                            << ", rsp.m_current_blockchain_height=" << rsp.current_blockchain_height << ", missed_ids.size()=" << rsp.missed_ids.size());
    post_notify<NOTIFY_RESPONSE_GET_OBJECTS>(rsp, context);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_cryptonote_protocol_handler<t_core>::handle_response_get_objects(int command, NOTIFY_RESPONSE_GET_OBJECTS::request& arg, cryptonote_connection_context& context)
  {
    LOG_PRINT_CCONTEXT_L2("NOTIFY_RESPONSE_GET_OBJECTS");
    if(context.m_last_response_height > arg.current_blockchain_height)
    {
      LOG_ERROR_CCONTEXT("sent wrong NOTIFY_HAVE_OBJECTS: arg.m_current_blockchain_height=" << arg.current_blockchain_height 
        << " < m_last_response_height=" << context.m_last_response_height << ", dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }

    context.m_remote_blockchain_height = arg.current_blockchain_height;

    size_t count = 0;
    BOOST_FOREACH(const block_complete_entry& block_entry, arg.blocks)
    {
      ++count;
      block b;
      if(!parse_and_validate_block_from_blob(block_entry.block, b))
      {
        LOG_ERROR_CCONTEXT("sent wrong block: failed to parse and validate block: \r\n" 
          << string_tools::buff_to_hex_nodelimer(block_entry.block) << "\r\n dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }      
      //to avoid concurrency in core between connections, suspend connections which delivered block later then first one
      if(count == 2)
      { 
        if(m_core.have_block(get_block_hash(b)))
        {
          context.m_state = cryptonote_connection_context::state_idle;
          context.m_needed_objects.clear();
          context.m_requested_objects.clear();
          LOG_PRINT_CCONTEXT_L1("Connection set to idle state.");
          return 1;
        }
      }
      
      auto req_it = context.m_requested_objects.find(get_block_hash(b));
      if(req_it == context.m_requested_objects.end())
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << string_tools::pod_to_hex(get_blob_hash(block_entry.block)) 
          << " wasn't requested, dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }
      if(b.tx_hashes.size() != block_entry.txs.size()) 
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << string_tools::pod_to_hex(get_blob_hash(block_entry.block)) 
          << ", tx_hashes.size()=" << b.tx_hashes.size() << " mismatch with block_complete_entry.m_txs.size()=" << block_entry.txs.size() << ", dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }

      context.m_requested_objects.erase(req_it);
    }

    if(context.m_requested_objects.size())
    {
      LOG_PRINT_CCONTEXT_RED("returned not all requested objects (context.m_requested_objects.size()=" 
        << context.m_requested_objects.size() << "), dropping connection", LOG_LEVEL_0);
      m_p2p->drop_connection(context);
      return 1;
    }

    {
      m_core.pause_mine();
      misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler(
        boost::bind(&t_core::resume_mine, &m_core));

      BOOST_FOREACH(const block_complete_entry& block_entry, arg.blocks)
      {
        //process transactions
        TIME_MEASURE_START(transactions_process_time);
        BOOST_FOREACH(auto& tx_blob, block_entry.txs)
        {
          tx_verification_context tvc = AUTO_VAL_INIT(tvc);
          m_core.handle_incoming_tx(tx_blob, tvc, true);
          if(tvc.m_verifivation_failed)
          {
            LOG_ERROR_CCONTEXT("transaction verification failed on NOTIFY_RESPONSE_GET_OBJECTS, \r\ntx_id = " 
              << string_tools::pod_to_hex(get_blob_hash(tx_blob)) << ", dropping connection");
            m_p2p->drop_connection(context);
            return 1;
          }
        }
        TIME_MEASURE_FINISH(transactions_process_time);

        //process block
        TIME_MEASURE_START(block_process_time);
        block_verification_context bvc = boost::value_initialized<block_verification_context>();

        m_core.handle_incoming_block(block_entry.block, bvc, false);

        if(bvc.m_verifivation_failed)
        {
          LOG_PRINT_CCONTEXT_L0("Block verification failed, dropping connection");
          m_p2p->drop_connection(context);
          return 1;
        }
        if(bvc.m_marked_as_orphaned)
        {
          LOG_PRINT_CCONTEXT_L0("Block received at sync phase was marked as orphaned, dropping connection");
          m_p2p->drop_connection(context);
          return 1;
        }

        TIME_MEASURE_FINISH(block_process_time);
        LOG_PRINT_CCONTEXT_L2("Block process time: " << block_process_time + transactions_process_time << "(" << transactions_process_time << "/" << block_process_time << ")ms");
      }
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
    LOG_PRINT_CCONTEXT_L2("NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << arg.block_ids.size());
    NOTIFY_RESPONSE_CHAIN_ENTRY::request r;
    if(!m_core.find_blockchain_supplement(arg.block_ids, r))
    {
      LOG_ERROR_CCONTEXT("Failed to handle NOTIFY_REQUEST_CHAIN.");
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
    if(context.m_needed_objects.size())
    {
      //we know objects that we need, request this objects
      NOTIFY_REQUEST_GET_OBJECTS::request req;
      size_t count = 0;
      auto it = context.m_needed_objects.begin();

      while(it != context.m_needed_objects.end() && count < BLOCKS_SYNCHRONIZING_DEFAULT_COUNT)
      {
        if( !(check_having_blocks && m_core.have_block(*it)))
        {
          req.blocks.push_back(*it);
          ++count;
          context.m_requested_objects.insert(*it);
        }
        context.m_needed_objects.erase(it++);
      }
      LOG_PRINT_CCONTEXT_L2("-->>NOTIFY_REQUEST_GET_OBJECTS: blocks.size()=" << req.blocks.size() << ", txs.size()=" << req.txs.size());
      post_notify<NOTIFY_REQUEST_GET_OBJECTS>(req, context);    
    }else if(context.m_last_response_height < context.m_remote_blockchain_height-1)
    {//we have to fetch more objects ids, request blockchain entry
     
      NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
      m_core.get_short_chain_history(r.block_ids);
      LOG_PRINT_CCONTEXT_L2("-->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() );
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
                           << "\r\non connection [" << net_utils::print_connection_context_short(context)<< "]");
      
      context.m_state = cryptonote_connection_context::state_normal;
      LOG_PRINT_CCONTEXT_GREEN(" SYNCHRONIZED OK", LOG_LEVEL_0);
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
      LOG_PRINT_L0(ENDL << "**********************************************************************" << ENDL 
        << "You are now synchronized with the network. You may now start simplewallet." << ENDL 
        << ENDL
        << "Please note, that the blockchain will be saved only after you quit the daemon with \"exit\" command or if you use \"save\" command." << ENDL 
        << "Otherwise, you will possibly need to synchronize the blockchain again." << ENDL
        << ENDL
        << "Use \"help\" command to see the list of available commands." << ENDL
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
    LOG_PRINT_CCONTEXT_L2("NOTIFY_RESPONSE_CHAIN_ENTRY: m_block_ids.size()=" << arg.m_block_ids.size() 
      << ", m_start_height=" << arg.start_height << ", m_total_height=" << arg.total_height);
    
    if(!arg.m_block_ids.size())
    {
      LOG_ERROR_CCONTEXT("sent empty m_block_ids, dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }

    if(!m_core.have_block(arg.m_block_ids.front()))
    {
      LOG_ERROR_CCONTEXT("sent m_block_ids starting from unknown id: "
                                              << string_tools::pod_to_hex(arg.m_block_ids.front()) << " , dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }
    
    context.m_remote_blockchain_height = arg.total_height;
    context.m_last_response_height = arg.start_height + arg.m_block_ids.size()-1;
    if(context.m_last_response_height > context.m_remote_blockchain_height)
    {
      LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_CHAIN_ENTRY, with \r\nm_total_height=" << arg.total_height
                                                                         << "\r\nm_start_height=" << arg.start_height
                                                                         << "\r\nm_block_ids.size()=" << arg.m_block_ids.size());
      m_p2p->drop_connection(context);
    }

    BOOST_FOREACH(auto& bl_id, arg.m_block_ids)
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
    return relay_post_notify<NOTIFY_NEW_BLOCK>(arg, exclude_context);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  bool t_cryptonote_protocol_handler<t_core>::relay_transactions(NOTIFY_NEW_TRANSACTIONS::request& arg, cryptonote_connection_context& exclude_context)
  {
    return relay_post_notify<NOTIFY_NEW_TRANSACTIONS>(arg, exclude_context);
  }
}
