// Copyright (c) 2014-2024, The Monero Project
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

// node.cpp : Defines the entry point for the console application.
//


#include "include_base_utils.h"
#include "version.h"
#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>

#include "common/command_line.h"
#include "console_handler.h"
#include "p2p/net_node.h"
#include "p2p/net_node.inl"
//#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.inl"
#include "core_proxy.h"

#if defined(WIN32)
#include <crtdbg.h>
#endif

namespace po = boost::program_options;
using namespace std;
using namespace epee;
using namespace cryptonote;
using namespace crypto;


BOOST_CLASS_VERSION(nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<tests::proxy_core> >, 1);

int main(int argc, char* argv[])
{

#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif 

  TRY_ENTRY();

  tools::on_startup();
  string_tools::set_module_name_and_folder(argv[0]);

  //set up logging options
  mlog_configure(mlog_get_default_log_path("core_proxy.log"), true);
  mlog_set_log_level(2);


  po::options_description desc("Allowed options");
  command_line::add_arg(desc, cryptonote::arg_data_dir);
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<tests::proxy_core> >::init_options(desc);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return 1;

  MGINFO("Module folder: " << argv[0]);
  MGINFO("Node starting ...");


  //create objects and link them
  tests::proxy_core pr_core;
  cryptonote::t_cryptonote_protocol_handler<tests::proxy_core> cprotocol(pr_core, NULL);
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<tests::proxy_core> > p2psrv {
      cprotocol
    };
  cprotocol.set_p2p_endpoint(&p2psrv);
  //pr_core.set_cryptonote_protocol(&cprotocol);
  //daemon_cmmands_handler dch(p2psrv);

  //initialize objects

  MGINFO("Initializing p2p server...");
  bool res = p2psrv.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize p2p server.");
  MGINFO("P2p server initialized OK");

  MGINFO("Initializing cryptonote protocol...");
  res = cprotocol.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize cryptonote protocol.");
  MGINFO("Cryptonote protocol initialized OK");

  //initialize core here
  MGINFO("Initializing proxy core...");
  res = pr_core.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core");  
  MGINFO("Core initialized OK");

  MGINFO("Starting p2p net loop...");
  p2psrv.run();
  MGINFO("p2p net loop stopped");

  //deinitialize components  
  MGINFO("Deinitializing core...");
  pr_core.deinit();
  MGINFO("Deinitializing cryptonote_protocol...");
  cprotocol.deinit();
  MGINFO("Deinitializing p2p...");
  p2psrv.deinit();


  //pr_core.set_cryptonote_protocol(NULL);
  cprotocol.set_p2p_endpoint(NULL);


  MGINFO("Node stopped.");
  return 0;

  CATCH_ENTRY_L0("main", 1);
}

/*
string tx2str(const cryptonote::transaction& tx, const cryptonote::hash256& tx_hash, const cryptonote::hash256& tx_prefix_hash, const cryptonote::blobdata& blob) {
    stringstream ss;

    ss << "{" << endl;
    ss << "\tversion:" << tx.version << endl;
    ss << "\tunlock_time:" << tx.unlock_time << endl;
    ss << "\t"

    return ss.str();
}*/

bool tests::proxy_core::handle_incoming_tx(const cryptonote::tx_blob_entry& tx_blob, cryptonote::tx_verification_context& tvc, cryptonote::relay_method tx_relay, bool relayed) {
    if (tx_relay != cryptonote::relay_method::block)
        return true;

    crypto::hash tx_hash = null_hash;
    crypto::hash tx_prefix_hash = null_hash;
    transaction tx;

    if (tx_blob.prunable_hash != crypto::null_hash)
    {
        cerr << "WRONG TRANSACTION, pruned blob rejected" << endl;
        return false;
    }

    if (!parse_and_validate_tx_from_blob(tx_blob.blob, tx, tx_hash, tx_prefix_hash)) {
        cerr << "WRONG TRANSACTION BLOB, Failed to parse, rejected" << endl;
        return false;
    }

    cout << "TX " << endl << endl;
    cout << tx_hash << endl;
    cout << tx_prefix_hash << endl;
    cout << tx_blob.blob.size() << endl;
    //cout << string_tools::buff_to_hex_nodelimer(tx_blob) << endl << endl;
    cout << obj_to_json_str(tx) << endl;
    cout << endl << "ENDTX" << endl;

    return true;
}

bool tests::proxy_core::handle_incoming_txs(const std::vector<tx_blob_entry>& tx_blobs, std::vector<tx_verification_context>& tvc, cryptonote::relay_method tx_relay, bool relayed)
{
    tvc.resize(tx_blobs.size());
    size_t i = 0;
    for (const auto &tx_blob: tx_blobs)
    {
      if (!handle_incoming_tx(tx_blob, tvc[i], tx_relay, relayed))
          return false;
      ++i;
    }
    return true;
}

bool tests::proxy_core::handle_incoming_block(const cryptonote::blobdata& block_blob, const cryptonote::block *block_, cryptonote::block_verification_context& bvc, bool update_miner_blocktemplate) {
    block b = AUTO_VAL_INIT(b);

    if(!parse_and_validate_block_from_blob(block_blob, b)) {
        cerr << "Failed to parse and validate new block" << endl;
        return false;
    }

    crypto::hash h;
    crypto::hash lh;
    cout << "BLOCK" << endl << endl;
    cout << (h = get_block_hash(b)) << endl;
    cout << (lh = get_block_longhash(NULL, b, 0, 0)) << endl;
    cout << get_transaction_hash(b.miner_tx) << endl;
    cout << ::get_object_blobsize(b.miner_tx) << endl;
    //cout << string_tools::buff_to_hex_nodelimer(block_blob) << endl;
    cout << obj_to_json_str(b) << endl;

    cout << endl << "ENDBLOCK" << endl << endl;

    if (!add_block(h, lh, b, block_blob))
        return false;

    return true;
}

bool tests::proxy_core::get_short_chain_history(std::list<crypto::hash>& ids) {
    build_short_history(ids, m_lastblk);
    return true;
}

void tests::proxy_core::get_blockchain_top(uint64_t& height, crypto::hash& top_id) {
    height = 0;
    top_id = get_block_hash(m_genesis);
}

bool tests::proxy_core::init(const boost::program_options::variables_map& /*vm*/) {
    generate_genesis_block(m_genesis, config::GENESIS_TX, config::GENESIS_NONCE);
    crypto::hash h = get_block_hash(m_genesis);
    add_block(h, get_block_longhash(NULL, m_genesis, 0, 0), m_genesis, block_to_blob(m_genesis));
    return true;
}

bool tests::proxy_core::have_block_unlocked(const crypto::hash& id, int *where) {
    if (m_hash2blkidx.end() == m_hash2blkidx.find(id))
        return false;
    if (where) *where = HAVE_BLOCK_MAIN_CHAIN;
    return true;
}

bool tests::proxy_core::have_block(const crypto::hash& id, int *where) {
    return have_block_unlocked(id, where);
}

void tests::proxy_core::build_short_history(std::list<crypto::hash> &m_history, const crypto::hash &m_start) {
    m_history.push_front(get_block_hash(m_genesis));
    /*std::unordered_map<crypto::hash, tests::block_index>::const_iterator cit = m_hash2blkidx.find(m_lastblk);

    do {
        m_history.push_front(cit->first);

        size_t n = 1 << m_history.size();
        while (m_hash2blkidx.end() != cit && crypto::null_hash != cit->second.blk.prev_id && n > 0) {
            n--;
            cit = m_hash2blkidx.find(cit->second.blk.prev_id);
        }
    } while (m_hash2blkidx.end() != cit && get_block_hash(cit->second.blk) != cit->first);*/
}

bool tests::proxy_core::add_block(const crypto::hash &_id, const crypto::hash &_longhash, const cryptonote::block &_blk, const cryptonote::blobdata &_blob) {
    size_t height = 0;

    if (crypto::null_hash != _blk.prev_id) {
        std::unordered_map<crypto::hash, tests::block_index>::const_iterator cit = m_hash2blkidx.find(_blk.prev_id);
        if (m_hash2blkidx.end() == cit) {
            cerr << "ERROR: can't find previous block with id \"" << _blk.prev_id << "\"" << endl;
            return false;
        }

        height = cit->second.height + 1;
    }

    m_known_block_list.push_back(_id);

    block_index bi(height, _id, _longhash, _blk, _blob, txes);
    m_hash2blkidx.insert(std::make_pair(_id, bi));
    txes.clear();
    m_lastblk = _id;

    return true;
}
