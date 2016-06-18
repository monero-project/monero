// Copyright (c) 2016, The Monero Project
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

#pragma once

#include "daemon_messages.h"
#include "rpc_handler.h"
#include "cryptonote_core/cryptonote_core.h"

namespace cryptonote
{

namespace rpc
{


class DaemonHandler : public RpcHandler
{
  public:

    DaemonHandler(cryptonote::core& c) : m_core(c) { }

    ~DaemonHandler() { }

    void handle(GetHeight::Request& req, GetHeight::Response& res);

    void handle(GetBlocksFast::Request& req, GetBlocksFast::Response& res);

    void handle(GetHashesFast::Request& req, GetHashesFast::Response& res);

    void handle(GetTransactions::Request& req, GetTransactions::Response& res);

    void handle(IsKeyImageSpent::Request& req, IsKeyImageSpent::Response& res);

    void handle(GetTxGlobalOutputIndexes::Request& req, GetTxGlobalOutputIndexes::Response& res);

    void handle(GetRandomOutputsForAmounts::Request& req, GetRandomOutputsForAmounts::Response& res);

    void handle(SendRawTx::Request& req, SendRawTx::Response& res);

    void handle(StartMining::Request& req, StartMining::Response& res);

    void handle(GetInfo::Request& req, GetInfo::Response& res);

    void handle(StopMining::Request& req, StopMining::Response& res);

    void handle(MiningStatus::Request& req, MiningStatus::Response& res);

    void handle(SaveBC::Request& req, SaveBC::Response& res);

    void handle(GetBlockCount::Request& req, GetBlockCount::Response& res);

    void handle(GetBlockHash::Request& req, GetBlockHash::Response& res);

    void handle(GetBlockTemplate::Request& req, GetBlockTemplate::Response& res);

    void handle(SubmitBlock::Request& req, SubmitBlock::Response& res);

    void handle(GetLastBlockHeader::Request& req, GetLastBlockHeader::Response& res);

    void handle(GetBlockHeaderByHash::Request& req, GetBlockHeaderByHash::Response& res);

    void handle(GetBlockHeaderByHeight::Request& req, GetBlockHeaderByHeight::Response& res);

    void handle(GetBlock::Request& req, GetBlock::Response& res);

    void handle(GetPeerList::Request& req, GetPeerList::Response& res);

    void handle(SetLogHashRate::Request& req, SetLogHashRate::Response& res);

    void handle(SetLogLevel::Request& req, SetLogLevel::Response& res);

    void handle(GetTransactionPool::Request& req, GetTransactionPool::Response& res);

    void handle(GetConnections::Request& req, GetConnections::Response& res);

    void handle(GetBlockHeadersRange::Request& req, GetBlockHeadersRange::Response& res);

    void handle(StopDaemon::Request& req, StopDaemon::Response& res);

    void handle(FastExit::Request& req, FastExit::Response& res);

    void handle(OutPeers::Request& req, OutPeers::Response& res);

    void handle(StartSaveGraph::Request& req, StartSaveGraph::Response& res);

    void handle(StopSaveGraph::Request& req, StopSaveGraph::Response& res);

    void handle(HardForkInfo::Request& req, HardForkInfo::Response& res);

    void handle(GetBans::Request& req, GetBans::Response& res);

    void handle(SetBans::Request& req, SetBans::Response& res);

    void handle(FlushTransactionPool::Request& req, FlushTransactionPool::Response& res);

    void handle(GetOutputHistogram::Request& req, GetOutputHistogram::Response& res);

    std::string handle(std::string& request);

  private:
    cryptonote::core& m_core;
};

}  // namespace rpc

}  // namespace cryptonote
