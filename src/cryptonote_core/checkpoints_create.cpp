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

#include "checkpoints_create.h"
#include "common/dns_utils.h"
#include "include_base_utils.h"
#include <sstream>
#include <random>
#include "storages/portable_storage_template_helper.h" // epee json include

namespace cryptonote
{

struct t_hashline 
{
	uint64_t height;
	std::string hash;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE(hash)
      END_KV_SERIALIZE_MAP()
};

struct t_hash_json {
 	std::vector<t_hashline> hashlines;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hashlines)
      END_KV_SERIALIZE_MAP()
};

bool create_checkpoints(cryptonote::checkpoints& checkpoints)
{      
  ADD_CHECKPOINT(1,     "771fbcd656ec1464d3a02ead5e18644030007a0fc664c0a964d30922821a8148");
  ADD_CHECKPOINT(10,    "c0e3b387e47042f72d8ccdca88071ff96bff1ac7cde09ae113dbb7ad3fe92381");
  ADD_CHECKPOINT(100,   "ac3e11ca545e57c49fca2b4e8c48c03c23be047c43e471e1394528b1f9f80b2d");
  ADD_CHECKPOINT(1000,  "5acfc45acffd2b2e7345caf42fa02308c5793f15ec33946e969e829f40b03876");
  ADD_CHECKPOINT(10000, "c758b7c81f928be3295d45e230646de8b852ec96a821eac3fea4daf3fcac0ca2");
  ADD_CHECKPOINT(22231, "7cb10e29d67e1c069e6e11b17d30b809724255fee2f6868dc14cfc6ed44dfb25");
  ADD_CHECKPOINT(29556, "53c484a8ed91e4da621bb2fa88106dbde426fe90d7ef07b9c1e5127fb6f3a7f6");
  ADD_CHECKPOINT(50000, "0fe8758ab06a8b9cb35b7328fd4f757af530a5d37759f9d3e421023231f7b31c");
  ADD_CHECKPOINT(80000, "a62dcd7b536f22e003ebae8726e9e7276f63d594e264b6f0cd7aab27b66e75e3");
  ADD_CHECKPOINT(202612, "bbd604d2ba11ba27935e006ed39c9bfdd99b76bf4a50654bc1e1e61217962698");
  ADD_CHECKPOINT(202613, "e2aa337e78df1f98f462b3b1e560c6b914dec47b610698b7b7d1e3e86b6197c2");
  ADD_CHECKPOINT(202614, "c29e3dc37d8da3e72e506e31a213a58771b24450144305bcba9e70fa4d6ea6fb");
  ADD_CHECKPOINT(205000, "5d3d7a26e6dc7535e34f03def711daa8c263785f73ec1fadef8a45880fde8063");
  ADD_CHECKPOINT(220000, "9613f455933c00e3e33ac315cc6b455ee8aa0c567163836858c2d9caff111553");
  ADD_CHECKPOINT(230300, "bae7a80c46859db355556e3a9204a337ae8f24309926a1312323fdecf1920e61");
  ADD_CHECKPOINT(230700, "93e631240ceac831da1aebfc5dac8f722c430463024763ebafa888796ceaeedf");
  ADD_CHECKPOINT(231350, "b5add137199b820e1ea26640e5c3e121fd85faa86a1e39cf7e6cc097bdeb1131");
  ADD_CHECKPOINT(232150, "955de8e6b6508af2c24f7334f97beeea651d78e9ade3ab18fec3763be3201aa8");

  return true;
}

bool load_checkpoints_from_json(cryptonote::checkpoints& checkpoints, std::string json_hashfile_fullpath)
{
  boost::system::error_code errcode;
  if (! (boost::filesystem::exists(json_hashfile_fullpath, errcode)))
  {
    LOG_PRINT_L1("Blockchain checkpoints file not found");
    return true;
  }

  LOG_PRINT_L1("Adding checkpoints from blockchain hashfile");

  uint64_t prev_max_height = checkpoints.get_max_height();
  LOG_PRINT_L1("Hard-coded max checkpoint height is " << prev_max_height);
  t_hash_json hashes;
  epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath);
  for (std::vector<t_hashline>::const_iterator it = hashes.hashlines.begin(); it != hashes.hashlines.end(); )
  {
      uint64_t height;
      height = it->height;
      if (height <= prev_max_height) {
	LOG_PRINT_L1("ignoring checkpoint height " << height);
      } else {
	std::string blockhash = it->hash;
	LOG_PRINT_L1("Adding checkpoint height " << height << ", hash=" << blockhash);
	ADD_CHECKPOINT(height, blockhash);
      }
      ++it;
  }

  return true;
}

bool load_checkpoints_from_dns(cryptonote::checkpoints& checkpoints)
{
  // All four MoneroPulse domains have DNSSEC on and valid
  static const std::vector<std::string> dns_urls = { "checkpoints.moneropulse.se"
						   , "checkpoints.moneropulse.org"
						   , "checkpoints.moneropulse.net"
						   , "checkpoints.moneropulse.co"
  };
  bool avail, valid;
  std::vector<std::string> records;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, dns_urls.size() - 1);
  size_t first_index = dis(gen);

  size_t cur_index = first_index;
  do
  {
    records = tools::DNSResolver::instance().get_txt_record(dns_urls[cur_index], avail, valid);
    if (records.size() == 0 || (avail && !valid))
    {
      cur_index++;
      if (cur_index == dns_urls.size())
      {
	cur_index = 0;
      }
      continue;
    }
    break;
  } while (cur_index != first_index);

  if (records.size() == 0)
  {
    LOG_PRINT_L1("Fetching MoneroPulse checkpoints failed, no TXT records available.");
    return true;
  }

  if (avail && !valid)
  {
    LOG_PRINT_L0("WARNING: MoneroPulse failed DNSSEC validation and/or returned no records");
    return true;
  }

  for (auto& record : records)
  {
    auto pos = record.find(":");
    if (pos != std::string::npos)
    {
      uint64_t height;
      crypto::hash hash;

      // parse the first part as uint64_t,
      // if this fails move on to the next record
      std::stringstream ss(record.substr(0, pos));
      if (!(ss >> height))
      {
	continue;
      }

      // parse the second part as crypto::hash,
      // if this fails move on to the next record
      std::string hashStr = record.substr(pos + 1);
      if (!epee::string_tools::parse_tpod_from_hex_string(hashStr, hash))
      {
	continue;
      }

      ADD_CHECKPOINT(height, hashStr);
    }
  }
  return true;
}

bool load_new_checkpoints(cryptonote::checkpoints& checkpoints, std::string json_hashfile_fullpath)
{
  // TODO: replace hard-coded url with const string or #define
  return (load_checkpoints_from_json(checkpoints, json_hashfile_fullpath) && load_checkpoints_from_dns(checkpoints));
}

}  // namespace cryptonote
