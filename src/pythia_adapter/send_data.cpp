// Copyright (c) 2020 Harrison Hesslink
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
    

#include <math.h>
#include <iostream>

#include "rapidjson/document.h"
#include "net/http_client.h"
#include "send_data.h"
#include "int-util.h"
#include "vector"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

namespace karai {

    crypto::hash make_data_hash(crypto::public_key const &pubkey, std::string data)
	{
		char buf[256] = "SUP"; // Meaningless magic bytes
		crypto::hash result;
		memcpy(buf + 4, reinterpret_cast<const void *>(&pubkey), sizeof(pubkey));
		memcpy(buf + 4 + sizeof(pubkey), reinterpret_cast<const void *>(&data), sizeof(data));
		crypto::cn_fast_hash(buf, sizeof(buf), result);

		return result;
	}

    bool process_new_transaction(const cryptonote::transaction &tx, transctions_to_send &tts) {

        std::string eth_address = cryptonote::get_eth_address_from_tx_extra(tx.extra);
        if (eth_address != "") {
            //swap transaction
            std::cout << "swap" << std::endl;
            uint64_t amount = cryptonote::get_burned_amount_from_tx_extra(tx.extra);
            if (!amount == 0){
                swap_transaction stx; 
                std::string tx_hash = epee::string_tools::pod_to_hex(tx.hash);
                stx.tx_hash = tx_hash;
                stx.info.push_back(std::to_string(amount));
                stx.info.push_back(eth_address);
                tts.stxs.push_back(stx);
            }
        }

        return true;
    }

    void handle_block(const cryptonote::block &b, const std::vector<std::pair<cryptonote::transaction, cryptonote::blobdata>>& txs, const cryptonote::block &last_block, const crypto::public_key &my_pubc_key, const crypto::secret_key &my_sec, const std::vector<crypto::public_key> &node_keys)
    {
        karai::transctions_to_send tts;
        for (const auto& tx_pair : txs)
        {
            if (!process_new_transaction(tx_pair.first, tts)) 
            {
                continue;
            }
        }

        crypto::public_key last_winner_pubkey = cryptonote::get_service_node_winner_from_tx_extra(last_block.miner_tx.extra);
        crypto::public_key winner_pubkey = cryptonote::get_service_node_winner_from_tx_extra(b.miner_tx.extra);

        bool leader = epee::string_tools::pod_to_hex(my_pubc_key) == epee::string_tools::pod_to_hex(last_winner_pubkey);
    
        std::vector<std::pair<std::string, std::string>> payload_data;
        std::vector <std::string> nodes_on_network;
        nodes_on_network.reserve(node_keys.size());

        for (auto key : node_keys) 
        {
            nodes_on_network.push_back(epee::string_tools::pod_to_hex(key));
        }

        uint64_t height = cryptonote::get_block_height(b);
        payload_data.push_back(std::make_pair("pubkey", epee::string_tools::pod_to_hex(my_pubc_key)));

        //bool 
        bool r = karai::send_new_block(payload_data, nodes_on_network, leader, tts, height);
    }

    bool send_new_block(const std::vector<std::pair<std::string, std::string>> data, const std::vector<std::string> &nodes_on_network, const bool &leader, const transctions_to_send &tts, uint64_t &height)
    {
        std::string body = create_new_block_json(data, nodes_on_network, leader, tts, height);
        
        LOG_PRINT_L1("Data: " + body);

        bool r = make_request(body, "/api/v1/new_block");

        return r;
    }

    bool make_request(std::string body, std::string uri)
    {
        epee::net_utils::http::http_simple_client http_client;
        epee::net_utils::http::http_response_info res;
        const epee::net_utils::http::http_response_info *res_info = &res;
        epee::net_utils::http::fields_list fields;

        std::string url = "127.0.0.1";
        http_client.set_server(url, "4203",  boost::none);

        bool r =  http_client.invoke_post(uri, body, std::chrono::seconds(10), &res_info, fields);
        http_client.disconnect();
        return r;
    }

    //todo split this up into functions
     std::string create_new_block_json(const std::vector<std::pair<std::string, std::string>> data, const std::vector <std::string> &nodes_on_network, const bool &leader, const transctions_to_send &tts, uint64_t &height)
    {
        rapidjson::Document d;

        d.SetObject();
        rapidjson::Document::AllocatorType& allocator = d.GetAllocator();

        rapidjson::Value node_json(rapidjson::kArrayType);
        rapidjson::Value swaps(rapidjson::kArrayType);
        rapidjson::Value requests(rapidjson::kArrayType);

        for (auto it : nodes_on_network) 
        {
            rapidjson::Value v;
            v.SetString(it.c_str(), allocator);
            node_json.PushBack(v, allocator);
        }

        for (auto it : tts.stxs) 
        {
            rapidjson::Value this_stx(rapidjson::kArrayType);

            for (auto info : it.info) 
            {
                rapidjson::Value v;
                v.SetString(info.c_str(), allocator);
                this_stx.PushBack(v, allocator);
            }

            swaps.PushBack(this_stx, allocator);
        }

        d.AddMember("swaps", swaps, allocator);

        
        for (auto it : tts.cts) 
        {
            rapidjson::Value cts(rapidjson::kArrayType);

            for (auto info : it.info) 
            {
                std::cout << info << std::endl;
                rapidjson::Value v;
                v.SetString(info.c_str(), allocator);
                cts.PushBack(v, allocator);
            }

            requests.PushBack(cts, allocator);
        }

        d.AddMember("requests", requests, allocator);

        for (auto it : data) 
        {
            if (it.second == "")
                return "";

            rapidjson::Value v;
            rapidjson::Value k;
            k.SetString(it.first.c_str(), allocator);
            v.SetString(it.second.c_str(), allocator);
            d.AddMember(k, v, allocator);
        }

        d.AddMember("nodes", node_json, allocator);

        rapidjson::Value l;
        l.SetBool(leader);
        d.AddMember("leader", l, allocator); 

        rapidjson::Value h;
        h.SetInt64(height);
        d.AddMember("height", h, allocator);

        return jsonString(d);
    }


    std::string jsonString(const rapidjson::Document& d)
    {
        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        d.Accept(writer);

        return strbuf.GetString();
    }

}