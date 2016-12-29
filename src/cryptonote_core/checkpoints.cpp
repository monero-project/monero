// Copyright (c) 2014-2016, The Monero Project
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

#include "include_base_utils.h"

using namespace epee;

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "include_base_utils.h"
#include <sstream>
#include <random>

namespace
{
  bool dns_records_match(const std::vector<std::string>& a, const std::vector<std::string>& b)
  {
    if (a.size() != b.size()) return false;

    for (const auto& record_in_a : a)
    {
      bool ok = false;
      for (const auto& record_in_b : b)
      {
	if (record_in_a == record_in_b)
	{
	  ok = true;
	  break;
	}
      }
      if (!ok) return false;
    }

    return true;
  }
} // anonymous namespace

namespace cryptonote
{
  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = null_hash;
    bool r = epee::string_tools::parse_tpod_from_hex_string(hash_str, h);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse checkpoint hash string into binary representation!");

    // return false if adding at a height we already have AND the hash is different
    if (m_points.count(height))
    {
      CHECK_AND_ASSERT_MES(h == m_points[height], false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
    }
    m_points[height] = h;
    return true;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
  {
    return !m_points.empty() && (height <= (--m_points.end())->first);
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h, bool& is_a_checkpoint) const
  {
    auto it = m_points.find(height);
    is_a_checkpoint = it != m_points.end();
    if(!is_a_checkpoint)
      return true;

    if(it->second == h)
    {
      LOG_PRINT_GREEN("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h, LOG_LEVEL_1);
      return true;
    }else
    {
      LOG_ERROR("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
      return false;
    }
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h) const
  {
    bool ignored;
    return check_block(height, h, ignored);
  }
  //---------------------------------------------------------------------------
  //FIXME: is this the desired behavior?
  bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const
  {
    if (0 == block_height)
      return false;

    auto it = m_points.upper_bound(blockchain_height);
    // Is blockchain_height before the first checkpoint?
    if (it == m_points.begin())
      return true;

    --it;
    uint64_t checkpoint_height = it->first;
    return checkpoint_height < block_height;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_max_height() const
  {
    std::map< uint64_t, crypto::hash >::const_iterator highest = 
        std::max_element( m_points.begin(), m_points.end(),
                         ( boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _1) < 
                           boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _2 ) ) );
    return highest->first;
  }
  //---------------------------------------------------------------------------
  const std::map<uint64_t, crypto::hash>& checkpoints::get_points() const
  {
    return m_points;
  }

  bool checkpoints::check_for_conflicts(const checkpoints& other) const
  {
    for (auto& pt : other.get_points())
    {
      if (m_points.count(pt.first))
      {
        CHECK_AND_ASSERT_MES(pt.second == m_points.at(pt.first), false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
      }
    }
    return true;
  }

  bool checkpoints::init_default_checkpoints()
  {
    ADD_CHECKPOINT(1,     "69fafa86832bf3f9c75795ad5406220576a134606be0a1979a676b9c818a8b8c");
    ADD_CHECKPOINT(10,    "4b9cd47d5c6df1f8a4c3a92c76178edb2e9c9998a1907866b294e89146eaceb4");
    ADD_CHECKPOINT(100,   "70b06fa4096230d0b8f7e963393b043856d7ecd3ecde3a1579fd1234d6d22e7b");
    /*ADD_CHECKPOINT(1000,  "fc6772bbf36013cb3132de346840951591c6a66d55a7d35b5a8d8101eeaf8271");
    ADD_CHECKPOINT(10000, "c758b7c81f928be3295d45e230646de8b852ec96a821eac3fea4daf3fcac0ca2");*/
    
    return true;
  }

  bool checkpoints::load_checkpoints_from_json(const std::string json_hashfile_fullpath)
  {
    boost::system::error_code errcode;
    if (! (boost::filesystem::exists(json_hashfile_fullpath, errcode)))
    {
      LOG_PRINT_L1("Blockchain checkpoints file not found");
      return true;
    }

    LOG_PRINT_L1("Adding checkpoints from blockchain hashfile");

    uint64_t prev_max_height = get_max_height();
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

  bool checkpoints::load_checkpoints_from_dns(bool testnet)
  {
    // All four SumoPulse domains have DNSSEC on and valid
    static const std::vector<std::string> dns_urls = 
		{							
									 "checkpoints.sumopulse.stream"
						     , "checkpoints.sumopulse.download"
						     /*, "checkpoints.sumopulse.win"
						     , "checkpoints.sumopulse.bid"*/
    };

    static const std::vector<std::string> testnet_dns_urls = { 
										 "testpoints.sumopulse.stream"
							     , "testpoints.sumopulse.download"
							     /*, "testpoints.sumopulse.win"
							     , "testpoints.sumopulse.bid"*/
    };

    std::vector<std::vector<std::string> > records;
    records.resize(dns_urls.size());

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, dns_urls.size() - 1);
    size_t first_index = dis(gen);

    bool avail, valid;
    size_t cur_index = first_index;
    do
    {
      std::string url;
      if (testnet)
      {
        url = testnet_dns_urls[cur_index];
      }
      else
      {
        url = dns_urls[cur_index];
      }

      records[cur_index] = tools::DNSResolver::instance().get_txt_record(url, avail, valid);
      if (!avail)
      {
        records[cur_index].clear();
        LOG_PRINT_L2("DNSSEC not available for checkpoint update at URL: " << url << ", skipping.");
      }
      if (!valid)
      {
        records[cur_index].clear();
        LOG_PRINT_L2("DNSSEC validation failed for checkpoint update at URL: " << url << ", skipping.");
      }

      cur_index++;
      if (cur_index == dns_urls.size())
      {
        cur_index = 0;
      }
      records[cur_index].clear();
    } while (cur_index != first_index);

    size_t num_valid_records = 0;

    for( const auto& record_set : records)
    {
      if (record_set.size() != 0)
      {
        num_valid_records++;
      }
    }

    if (num_valid_records < 2)
    {
      LOG_PRINT_L0("WARNING: no two valid SumoPulse DNS checkpoint records were received");
      return true;
    }

    int good_records_index = -1;
    for (size_t i = 0; i < records.size() - 1; ++i)
    {
      if (records[i].size() == 0) continue;

      for (size_t j = i + 1; j < records.size(); ++j)
      {
        if (dns_records_match(records[i], records[j]))
        {
    good_records_index = i;
    break;
        }
      }
      if (good_records_index >= 0) break;
    }

    if (good_records_index < 0)
    {
      LOG_PRINT_L0("WARNING: no two SumoPulse DNS checkpoint records matched");
      return true;
    }

    for (auto& record : records[good_records_index])
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

  bool checkpoints::load_new_checkpoints(const std::string json_hashfile_fullpath, bool testnet, bool dns)
  {
    bool result;

    result = load_checkpoints_from_json(json_hashfile_fullpath);
    if (dns)
    {
      result &= load_checkpoints_from_dns(testnet);
    }

    return result;
  }
}
