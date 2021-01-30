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
    
#include "rapidjson/document.h"
#include "cryptonote_core/cryptonote_core.h"

namespace karai {

struct swap_transaction {
    std::string tx_hash;
    std::vector<std::string> info;
};

struct contract_transaction {
    std::string tx_hash;
    std::vector<std::string> info;
};

struct transctions_to_send {
    std::vector<contract_transaction> cts;
    std::vector<swap_transaction> stxs;
};


bool send_new_block(const std::vector<std::pair<std::string, std::string>> data, const std::vector<std::string> &nodes_on_network, const bool &leader, const transctions_to_send &tts , uint64_t &height);

bool process_new_transaction(cryptonote::transaction &tx, transctions_to_send &tts);

bool make_request(std::string body, std::string uri);

std::string jsonString(const rapidjson::Document& d);

std::string create_new_block_json(const std::vector<std::pair<std::string, std::string>> data, const std::vector <std::string> &nodes_on_network, const bool &leader, const transctions_to_send &tts, uint64_t &height);

crypto::hash make_data_hash(crypto::public_key const &pubkey, std::string data);


void handle_block(const cryptonote::block &b, const std::vector<std::pair<cryptonote::transaction, cryptonote::blobdata>>& txs, const cryptonote::block &last_block, const crypto::public_key &my_pubc_key, const crypto::secret_key &my_sec, const std::vector<crypto::public_key> &node_states);


}
