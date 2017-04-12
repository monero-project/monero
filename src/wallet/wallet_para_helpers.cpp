/*
 * Copyright (c) 2017 fireice-uk
 * All rights reserved.
 *
 * Work-for-hire licence 
 * Please read this licence carefully, as this is NOT a free software licence. 
 * This agreement is designed to give you rights to preview and critique the work 
 * for a limited amount of time before a payment is tendered. 
 * 
 * 1. Redistribution and use in source and binary forms WITHOUT modification is 
 * permitted as long as this licence has not expired. You must not modify this work.
 * 
 * 2. As soon as the agreed upon payment is tendered, this licence expires and the 
 * full exclusive copyright is transferred to The Monero Project, which is free to 
 * re-release the work under whatever conditions they see fit.
 * 
 * 3. Unless point 2 comes into effect, this licence will expire on 01/08/2017. All 
 * your rights to use this work will cease. A payment after the date stated will not 
 * be accepted.
 * 
 * 4. This agreement is governed by the law of England and Wales, and is subject to 
 * the non-exclusive jurisdiction of the courts of England and Wales
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" WITHOUT ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER 
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "wallet_para_helpers.h"

using namespace epee;

namespace tools
{

//---------------------------------------------------------------------------------------------------
sthd_blk_fetcher::sthd_blk_fetcher(const std::string &daemon_address, boost::optional<epee::net_utils::http::login> daemon_login)
{
	m_http_client.set_server(daemon_address, daemon_login);
}

bool sthd_blk_fetcher::check_rpc_version()
{
	epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_VERSION::request> req_t = AUTO_VAL_INIT(req_t);
	epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_VERSION::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
	req_t.jsonrpc = "2.0";
	req_t.id = epee::serialization::storage_entry(0);
	req_t.method = "get_version";
	
	bool r = net_utils::invoke_http_json("/json_rpc", req_t, resp_t, m_http_client, std::chrono::milliseconds(rpc_timeout));
	CHECK_AND_ASSERT_MES(r, false, "Failed to connect to daemon");
	CHECK_AND_ASSERT_MES(resp_t.result.status != CORE_RPC_STATUS_BUSY, false, "Failed to connect to daemon");
	CHECK_AND_ASSERT_MES(resp_t.result.status == CORE_RPC_STATUS_OK, false, "Failed to get daemon RPC version");
	
	m_rpc_version = resp_t.result.version;
	return true;
}
//----------------------------------------------------------------------------------------------------
void sthd_blk_fetcher::check_rpc_connect()
{
	if (!m_http_client.is_connected())
	{
		if (!m_http_client.connect(std::chrono::milliseconds(rpc_timeout)))
			throw std::runtime_error("Daemon RPC connection failed.");
	}
	
	if(!check_rpc_version())
		throw std::runtime_error("Version check failed.");
}
//----------------------------------------------------------------------------------------------------
void sthd_blk_fetcher::pull_blocks(uint64_t &blocks_start_height, const std::list<crypto::hash> &short_chain_history, std::list<cryptonote::block_complete_entry> &blocks, std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> &o_indices)
{
	cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request req = AUTO_VAL_INIT(req);
	cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response res = AUTO_VAL_INIT(res);
	
	req.start_height = 0; //We use the chain history
	req.block_ids = short_chain_history;

	if (m_rpc_version >= MAKE_CORE_RPC_VERSION(1, 7))
	{
		MDEBUG("Daemon is recent enough, asking for pruned blocks");
		req.prune = true;
	}
	else
	{
		MDEBUG("Daemon is too old, not asking for pruned blocks");
		req.prune = false;
	}

	bool r = net_utils::invoke_http_bin("/getblocks.bin", req, res, m_http_client, std::chrono::milliseconds(rpc_timeout));
	
	THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "getblocks.bin");
	THROW_WALLET_EXCEPTION_IF(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "getblocks.bin");
	THROW_WALLET_EXCEPTION_IF(res.status != CORE_RPC_STATUS_OK, error::get_blocks_error, res.status);
	THROW_WALLET_EXCEPTION_IF(res.blocks.size() != res.output_indices.size(), error::wallet_internal_error,
		"mismatched blocks (" + boost::lexical_cast<std::string>(res.blocks.size()) + ") and output_indices (" +
		boost::lexical_cast<std::string>(res.output_indices.size()) + ") sizes from daemon");

	blocks_start_height = res.start_height;
	blocks = res.blocks;
	o_indices = res.output_indices;
}

}
