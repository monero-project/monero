// Copyright (c) 2014, The Monero Project
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

/*!
 * \file daemon_ipc_handlers.h
 * \brief Header for Daemon IPC handlers
 */

#ifndef DAEMON_IPC_HANDLERS_H
#define DAEMON_IPC_HANDLERS_H

#include "include_base_utils.h"
using namespace epee;

#include "cryptonote_core/cryptonote_core.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "common/command_line.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "cryptonote_core/account.h"
#include "misc_language.h"
#include "string_tools.h"
#include "crypto/hash.h"
#include "wap_library.h"
#include "wap_classes.h"
#include "net/http_server_impl_base.h"
#include "cryptonote_core/cryptonote_basic_impl.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

/*!
 * \namespace IPC
 * \brief Namespace pertaining to IPC.
 */
namespace IPC
{
  // A bunch of response statuses and error codes
  const uint64_t STATUS_OK = 0;
  const uint64_t STATUS_CORE_BUSY = 1;
  const uint64_t STATUS_WRONG_ADDRESS = 2;
  const uint64_t STATUS_MINING_NOT_STARTED = 3;
  const uint64_t STATUS_WRONG_BLOCK_ID_LENGTH = 4;
  const uint64_t STATUS_INTERNAL_ERROR = 5;
  const uint64_t STATUS_INVALID_TX = 6;
  const uint64_t STATUS_TX_VERIFICATION_FAILED = 7;
  const uint64_t STATUS_TX_NOT_RELAYED = 8;
  const uint64_t STATUS_RANDOM_OUTS_FAILED = 9;
  const uint64_t STATUS_MINING_NOT_STOPPED = 10;
  const uint64_t STATUS_NOT_MINING = 11;
  const uint64_t STATUS_INVALID_LOG_LEVEL = 12;
  const uint64_t STATUS_ERROR_STORING_BLOCKCHAIN = 13;
  const uint64_t STATUS_HEIGHT_TOO_BIG = 14;
  const uint64_t STATUS_RESERVE_SIZE_TOO_BIG = 15;
  const uint64_t STATUS_BLOCK_NOT_FOUND = 16;
  /*!
   * \namespace Daemon
   * \brief Namespace pertaining to Daemon IPC.
   */
  namespace Daemon
  {
    void get_height(wap_proto_t *message);
    void start_mining(wap_proto_t *message);
    void stop_mining(wap_proto_t *message);
    void get_info(wap_proto_t *message);
    void get_peer_list(wap_proto_t *message);
    void get_mining_status(wap_proto_t *message);
    void set_log_hash_rate(wap_proto_t *message);
    void set_log_level(wap_proto_t *message);
    void start_save_graph(wap_proto_t *message);
    void stop_save_graph(wap_proto_t *message);
    void get_block_hash(wap_proto_t *message);
    void get_block_template(wap_proto_t *message);
    void retrieve_blocks(wap_proto_t *message);
    void send_raw_transaction(wap_proto_t *message);
    void get_output_indexes(wap_proto_t *message);
    void get_random_outs(wap_proto_t *message);
    void save_bc(wap_proto_t *message);
    void get_hard_fork_info(wap_proto_t *message);
    void get_connections_list(wap_proto_t *message);
    void stop_daemon(wap_proto_t *message);
    void get_block_by_height(wap_proto_t *message);
    void get_block_by_hash(wap_proto_t *message);

    /*!
     * \brief initializes it with objects necessary to handle IPC requests and starts
     *        IPC server
     * 
     * \param p_core cryptonote core object
     * \param p_p2p  p2p object
     * \param p_testnet testnet mode or not
     */
    void init(cryptonote::core &p_core,
      nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > &p_p2p,
      bool p_testnet);

    /*!
     * \brief stops the IPC server
     * 
     * \param p_core cryptonote core object
     * \param p_p2p  p2p object
     * \param p_testnet testnet mode or not
     */
    void stop();
  }
}

#endif
