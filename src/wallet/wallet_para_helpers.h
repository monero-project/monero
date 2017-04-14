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

#pragma once

#include <string>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <stdexcept>
#include <assert.h>

#include "include_base_utils.h"
#include "wallet_errors.h"
#include "net/http_client.h"
#include "storages/http_abstract_invoke.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "crypto/hash.h"

//COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT=1000
//BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT=10000 
namespace tools
{

//Structure for efficiently managing memory alloc
struct blk_batch
{
	size_t start_height; //32-bits will be good for another 1000 years
	std::list<cryptonote::block_complete_entry> blocks; //vector is a much more appropriate structure here
	std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> o_indices;
	
	blk_batch()
	{
	}
	
	blk_batch(blk_batch&& from) : 
		start_height(from.start_height), blocks(std::move(from.blocks)), o_indices(std::move(from.o_indices))
	{
		
	}
	
	blk_batch& operator=(blk_batch&& from)
	{
		assert(this != &from);
		
		start_height = from.start_height;
		blocks = std::move(from.blocks);
		o_indices = std::move(from.o_indices);
		
		return *this;
	}
	
	// Delete the copy operators to make sure we are moving
	blk_batch(blk_batch const&) = delete;
	blk_batch& operator=(blk_batch const&) = delete;
};

class sthd_blk_fetcher
{
public:
	sthd_blk_fetcher(const std::string &daemon_address, boost::optional<epee::net_utils::http::login> daemon_login);
	void pull_blocks(const std::list<crypto::hash> &short_chain_history, blk_batch &output);

private:
	void check_rpc_connect();
	bool check_rpc_version();
	
	constexpr static uint32_t rpc_timeout  = 200000;
	
	epee::net_utils::http::http_simple_client m_http_client;
	uint32_t m_rpc_version;
};

//Since network is not exactly a CPU intensive task, one size will fit all
class mthd_blk_fetcher
{
public:
	mthd_blk_fetcher(size_t max_q_cap, const std::string &daemon_address, boost::optional<epee::net_utils::http::login> daemon_login);
	void start_fetching(std::list<crypto::hash> &&short_chain_history);
	bool get_batch(blk_batch &output);
	void join_thread();
	
private:
	bool is_finished();
	void fetch_thd_main();
	
	sthd_blk_fetcher fetcher;
		
	std::thread thd;
	std::atomic<bool> thd_exit;
	std::atomic<bool> thd_run;

	size_t max_q_cap;
	std::mutex q_cvm;
	std::condition_variable cv_q_nempty;
	std::condition_variable cv_q_nfull;
	std::queue<blk_batch> dataq;
	
	std::exception_ptr error;
	
	std::list<crypto::hash> fetched_history;
};

}