// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/lexical_cast.hpp>

#include "console_handler.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "common/util.h"
#include "crypto/hash.h"
#include "version.h"


class daemon_cmmands_handler
{
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >& m_srv;
public:
  daemon_cmmands_handler(nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >& srv):m_srv(srv)
  {
    m_cmd_binder.set_handler("help", boost::bind(&daemon_cmmands_handler::help, this, _1), "Show this help");
    m_cmd_binder.set_handler("print_pl", boost::bind(&daemon_cmmands_handler::print_pl, this, _1), "Print peer list");
    m_cmd_binder.set_handler("print_cn", boost::bind(&daemon_cmmands_handler::print_cn, this, _1), "Print connections");
    m_cmd_binder.set_handler("print_bc", boost::bind(&daemon_cmmands_handler::print_bc, this, _1), "Print blockchain info in a given blocks range, print_bc <begin_height> [<end_height>]");
    //m_cmd_binder.set_handler("print_bci", boost::bind(&daemon_cmmands_handler::print_bci, this, _1));
    //m_cmd_binder.set_handler("print_bc_outs", boost::bind(&daemon_cmmands_handler::print_bc_outs, this, _1));
    m_cmd_binder.set_handler("print_block", boost::bind(&daemon_cmmands_handler::print_block, this, _1), "Print block, print_block <block_hash> | <block_height>");
    m_cmd_binder.set_handler("print_tx", boost::bind(&daemon_cmmands_handler::print_tx, this, _1), "Print transaction, print_tx <transaction_hash>");
    m_cmd_binder.set_handler("start_mining", boost::bind(&daemon_cmmands_handler::start_mining, this, _1), "Start mining for specified address, start_mining <addr> [threads=1]");
    m_cmd_binder.set_handler("stop_mining", boost::bind(&daemon_cmmands_handler::stop_mining, this, _1), "Stop mining");
    m_cmd_binder.set_handler("print_pool", boost::bind(&daemon_cmmands_handler::print_pool, this, _1), "Print transaction pool (long format)");
    m_cmd_binder.set_handler("print_pool_sh", boost::bind(&daemon_cmmands_handler::print_pool_sh, this, _1), "Print transaction pool (short format)");
    m_cmd_binder.set_handler("show_hr", boost::bind(&daemon_cmmands_handler::show_hr, this, _1), "Start showing hash rate");
    m_cmd_binder.set_handler("hide_hr", boost::bind(&daemon_cmmands_handler::hide_hr, this, _1), "Stop showing hash rate");
    m_cmd_binder.set_handler("save", boost::bind(&daemon_cmmands_handler::save, this, _1), "Save blockchain");
    m_cmd_binder.set_handler("set_log", boost::bind(&daemon_cmmands_handler::set_log, this, _1), "set_log <level> - Change current log detalization level, <level> is a number 0-4");
    m_cmd_binder.set_handler("diff", boost::bind(&daemon_cmmands_handler::diff, this, _1), "Show difficulty");
  }

  bool start_handling()
  {
    m_cmd_binder.start_handling(&m_srv, "", "");
    return true;
  }

  void stop_handling()
  {
    m_cmd_binder.stop_handling();
  }

private:
  epee::srv_console_handlers_binder<nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > > m_cmd_binder;

  //--------------------------------------------------------------------------------
  std::string get_commands_str()
  {
    std::stringstream ss;
    ss << CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << ENDL;
    ss << "Commands: " << ENDL;
    std::string usage = m_cmd_binder.get_usage();
    boost::replace_all(usage, "\n", "\n  ");
    usage.insert(0, "  ");
    ss << usage << ENDL;
    return ss.str();
  }
  //--------------------------------------------------------------------------------
  bool help(const std::vector<std::string>& args)
  {
    std::cout << get_commands_str() << ENDL;
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_pl(const std::vector<std::string>& args)
  {
    m_srv.log_peerlist();
    return true;
  }
  //--------------------------------------------------------------------------------
  bool save(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_blockchain_storage().store_blockchain();
    return true;
  }
  //--------------------------------------------------------------------------------
  bool show_hr(const std::vector<std::string>& args)
  {
	if(!m_srv.get_payload_object().get_core().get_miner().is_mining()) 
	{
	  std::cout << "Mining is not started. You need start mining before you can see hash rate." << ENDL;
	} else 
	{
	  m_srv.get_payload_object().get_core().get_miner().do_print_hashrate(true);
	}
    return true;
  }
  //--------------------------------------------------------------------------------
  bool hide_hr(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_miner().do_print_hashrate(false);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool diff(const std::vector<std::string>& args)
  {
	  cryptonote::difficulty_type difficulty = m_srv.get_payload_object().get_core().get_blockchain_storage().get_difficulty_for_next_block();
	  uint64_t height = m_srv.get_payload_object().get_core().get_blockchain_storage().get_current_blockchain_height();

	  LOG_PRINT_GREEN("BH: " << height << ", DIFF: " << difficulty 
		  << ", HR: " << (int) difficulty / 60L << " H/s", LOG_LEVEL_0);

	  return true;
  } 
  //--------------------------------------------------------------------------------
  bool print_bc_outs(const std::vector<std::string>& args)
  {
    if(args.size() != 1)
    {
      std::cout << "need file path as parameter" << ENDL;
      return true;
    }
    m_srv.get_payload_object().get_core().print_blockchain_outs(args[0]);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_cn(const std::vector<std::string>& args)
  {
     m_srv.get_payload_object().log_connections();
     return true;
  }
  //--------------------------------------------------------------------------------
  bool print_bc(const std::vector<std::string>& args)
  {
    if(!args.size())
    {
      std::cout << "need block index parameter" << ENDL;
      return false;
    }
    uint64_t start_index = 0;
    uint64_t end_index = 0;
    uint64_t end_block_parametr = m_srv.get_payload_object().get_core().get_current_blockchain_height();
    if(!string_tools::get_xtype_from_string(start_index, args[0]))
    {
      std::cout << "wrong starter block index parameter" << ENDL;
      return false;
    }
    if(args.size() >1 && !string_tools::get_xtype_from_string(end_index, args[1]))
    {
      std::cout << "wrong end block index parameter" << ENDL;
      return false;
    }
    if (end_index == 0)
    {
      end_index = end_block_parametr;
    }
    if (end_index > end_block_parametr)
    {
      std::cout << "end block index parameter shouldn't be greater than " << end_block_parametr << ENDL;
      return false;
    }
    if (end_index <= start_index)
    {
      std::cout << "end block index should be greater than starter block index" << ENDL;
      return false;
    }

    m_srv.get_payload_object().get_core().print_blockchain(start_index, end_index);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_bci(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().print_blockchain_index();
    return true;
  }

  bool set_log(const std::vector<std::string>& args)
  {
    if(args.size() != 1)
    {
      std::cout << "use: set_log <log_level_number_0-4>" << ENDL;
      return true;
    }

    uint16_t l = 0;
    if(!string_tools::get_xtype_from_string(l, args[0]))
    {
      std::cout << "wrong number format, use: set_log <log_level_number_0-4>" << ENDL;
      return true;
    }

    if(LOG_LEVEL_4 < l)
    {
      std::cout << "wrong number range, use: set_log <log_level_number_0-4>" << ENDL;
      return true;
    }

    log_space::log_singletone::get_set_log_detalisation_level(true, l);

    return true;
  }

  //--------------------------------------------------------------------------------
  template <typename T>
  static bool print_as_json(T& obj)
  {
    std::cout << cryptonote::obj_to_json_str(obj) << ENDL;
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block_by_height(uint64_t height)
  {
    std::list<cryptonote::block> blocks;
    m_srv.get_payload_object().get_core().get_blocks(height, 1, blocks);

    if (1 == blocks.size())
    {
      cryptonote::block& block = blocks.front();
      std::cout << "block_id: " << get_block_hash(block) << ENDL;
      print_as_json(block);
    }
    else
    {
      uint64_t current_height;
      crypto::hash top_id;
      m_srv.get_payload_object().get_core().get_blockchain_top(current_height, top_id);
      std::cout << "block wasn't found. Current block chain height: " << current_height << ", requested: " << height << std::endl;
      return false;
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block_by_hash(const std::string& arg)
  {
    crypto::hash block_hash;
    if (!parse_hash256(arg, block_hash))
    {
      return false;
    }

    std::list<crypto::hash> block_ids;
    block_ids.push_back(block_hash);
    std::list<cryptonote::block> blocks;
    std::list<crypto::hash> missed_ids;
    m_srv.get_payload_object().get_core().get_blocks(block_ids, blocks, missed_ids);

    if (1 == blocks.size())
    {
      cryptonote::block block = blocks.front();
      print_as_json(block);
    }
    else
    {
      std::cout << "block wasn't found: " << arg << std::endl;
      return false;
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_block(const std::vector<std::string>& args)
  {
    if (args.empty())
    {
      std::cout << "expected: print_block (<block_hash> | <block_height>)" << std::endl;
      return true;
    }

    const std::string& arg = args.front();
    try
    {
      uint64_t height = boost::lexical_cast<uint64_t>(arg);
      print_block_by_height(height);
    }
    catch (boost::bad_lexical_cast&)
    {
      print_block_by_hash(arg);
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_tx(const std::vector<std::string>& args)
  {
    if (args.empty())
    {
      std::cout << "expected: print_tx <transaction hash>" << std::endl;
      return true;
    }

    const std::string& str_hash = args.front();
    crypto::hash tx_hash;
    if (!parse_hash256(str_hash, tx_hash))
    {
      return true;
    }

    std::vector<crypto::hash> tx_ids;
    tx_ids.push_back(tx_hash);
    std::list<cryptonote::transaction> txs;
    std::list<crypto::hash> missed_ids;
    m_srv.get_payload_object().get_core().get_transactions(tx_ids, txs, missed_ids);

    if (1 == txs.size())
    {
      cryptonote::transaction tx = txs.front();
      print_as_json(tx);
    }
    else
    {
      std::cout << "transaction wasn't found: <" << str_hash << '>' << std::endl;
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_pool(const std::vector<std::string>& args)
  {
    LOG_PRINT_L0("Pool state: " << ENDL << m_srv.get_payload_object().get_core().print_pool(false));
    return true;
  }
  //--------------------------------------------------------------------------------
  bool print_pool_sh(const std::vector<std::string>& args)
  {
    LOG_PRINT_L0("Pool state: " << ENDL << m_srv.get_payload_object().get_core().print_pool(true));
    return true;
  }  //--------------------------------------------------------------------------------
  bool start_mining(const std::vector<std::string>& args)
  {
    if(!args.size())
    {
      std::cout << "Please, specify wallet address to mine for: start_mining <addr> [threads=1]" << std::endl;
      return true;
    }

    cryptonote::account_public_address adr;
    if(!cryptonote::get_account_address_from_str(adr, args.front()))
    {
      std::cout << "target account address has wrong format" << std::endl;
      return true;
    }
    size_t threads_count = 1;
    if(args.size() > 1)
    {
      bool ok = string_tools::get_xtype_from_string(threads_count, args[1]);
      threads_count = (ok && 0 < threads_count) ? threads_count : 1;
    }

    boost::thread::attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);

    m_srv.get_payload_object().get_core().get_miner().start(adr, threads_count, attrs);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool stop_mining(const std::vector<std::string>& args)
  {
    m_srv.get_payload_object().get_core().get_miner().stop();
    return true;
  }
};
