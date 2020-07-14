// Copyright (c) 2014-2019, The Monero Project
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

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "string_tools.h"
#include "storages/portable_storage_template_helper.h" // epee json include
#include "serialization/keyvalue_serialization.h"
#include <functional>
#include <vector>

using namespace epee;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
  /**
   * @brief struct for loading a checkpoint from json
   */
  struct t_hashline
  {
    uint64_t height; //!< the height of the checkpoint
    std::string hash; //!< the hash for the checkpoint
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(height)
          KV_SERIALIZE(hash)
        END_KV_SERIALIZE_MAP()
  };

  /**
   * @brief struct for loading many checkpoints from json
   */
  struct t_hash_json {
    std::vector<t_hashline> hashlines; //!< the checkpoint lines from the file
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(hashlines)
        END_KV_SERIALIZE_MAP()
  };

  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {

  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = crypto::null_hash;
    bool r = epee::string_tools::hex_to_pod(hash_str, h);
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
      MINFO("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h);
      return true;
    }else
    {
      MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
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
    if (m_points.empty())
      return 0;
    return m_points.rbegin()->first;
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

  bool checkpoints::init_default_checkpoints(network_type nettype)
  {
    if (nettype == TESTNET)
    {
      return true;
    }
    if (nettype == STAGENET)
    {
      return true;
    }
	add_checkpoint(1, "ed1dd5a452b32bdc13cd11aee5e2485ca69d2a2ae8beb1e28e7da2d30959c799");
  //hf v2
	add_checkpoint(8, "5311cf4bd7a02cb267f89bf9e727aeaf27f669468979876fbd42c3f6a2ed0808");
	//hf v3
  add_checkpoint(100, "a46c1f2818fe83cb65b6a83dc9c4c50eb7eaa00e6a8acf3716549c220f5815cc");
	add_checkpoint(150, "2d38fff19b0412cda9c943531a0cb64deb24786e66dab0bfd777bcef6aef6e01");
	add_checkpoint(200, "f3f517a221e90540a60fe3c1280c4f9a9d3520f99e33e95043a6e6a9f37fd708");
	add_checkpoint(300, "ace104d1eac739e05788a3e304c270a69f9e0ca6175888e01c65f6c36eac7e22");
	add_checkpoint(400, "37cff9f1432e29938b59b3e7a03ed1a8b687567185d0b7c4fc16f1492a6fc6f2");
	add_checkpoint(500, "1728b36b6e122d5dd4fa0771504a8b28b42b72fc31e876859c0ae8894a17d644");
	add_checkpoint(600, "560fdf9acbe419f626eb24675fa31a228d567c3bc317ad69afc1359a78d95ed0");
	add_checkpoint(1000, "99c15f776abcf7f3503d7fdb46c442180c3339ad1a11e712ffdb3abe3f0efe33");
	add_checkpoint(2000, "79d91356d014b0738060071789a1a42d92546c291f7265d69b59ab2945029ee3");
	add_checkpoint(3000, "8eb26476daf8f8c6cd7cfdc9ede3098a71e96dd686289025abba5b7c49e44776");
	add_checkpoint(5000, "4ec06bc529bda640aef055aa3c4677d7cee97e6df4e49c5c685f317a51053be6");
	add_checkpoint(7500, "ff98ff532ad49726ae2fc9a621d9b03ab1e4afdc8b8b9391a99c587e610ca820");
	add_checkpoint(10000, "7229f2a4ecb7186ff89033fca1ef90e459fdb7655fa232e5776d0756609dc0eb");
	add_checkpoint(15000, "9cd32503a1328e3f8110110f86eb829753f6ed81322abd48872fbcecc2f8ad05");
	add_checkpoint(20000, "59eb5decfb889e735d1c53fc382738e8f9e1c8a958d791e27c8e68454224e282");
	add_checkpoint(25000, "46bab31af66503ba8e1b0c68558e379d55a2c1014fce8f9e858a5af3f590f06a");
	add_checkpoint(30000, "98666629df23137db3dd7ea2ec07982b00fc9963eecf52b4eb843e210b8db382");
	add_checkpoint(35000, "35901eeb1084e38f48a9e67201ff9a427828e8a4f942da822cf0d5d97d4d4119");
	add_checkpoint(40000, "d429c64a64e76a2ec557a8083a10d4dca7e77fb94162be4105f8249960460b17");
	//hv v4
  add_checkpoint(45000, "e632e631eeb62c94c40c19c9eb5f04d11f634477e9293cde889a4478c85ef16f");
	add_checkpoint(55000, "7eb0787af2224a66b570a81afcacadc20e90844836b8cb14fe028d9095335077");
	add_checkpoint(65000, "83e732ed2e1250a8fdd1b3a582fff49ae91c863f8f0633705223c94f0b20d7f6");
	add_checkpoint(75000, "8689244fbf5c5e76e99e63aaffe0d09343b12abe950bebd22a49510deb9ab657");
	add_checkpoint(85000, "17ef90e218583495792e2b0a93f13b8e83e2aadc34eadc0cc1c29a22ac756566");
	add_checkpoint(95000, "7521c7e4aaf406f8f0b02a1ffb7f29d31f180aadf4403618aa10df05edf46beb");
	add_checkpoint(100000, "25751d83a443833e2fd91d36885cbaa75c2faa00a9312658440a3fee5a61e5db");
	add_checkpoint(101000, "de2a38cb184870277642a76b48559a34dbee66e7eba46b214d43074bf501eadc");
	add_checkpoint(102000, "b687a556a2d3ade2cc285f3f835d0104bbc23686b26ffe6683f2f824471c824a");
  //hf v5
	add_checkpoint(106950, "c00fa5ecd7c2e08f6b88f39a3fd3acc31e9ee5ef2872e0543324d2c58ad2c57c");
  add_checkpoint(106960, "f244569566052cff5760df8f550af0d6eece2186c91286425ec53d0ab0b139ee");
	add_checkpoint(107000, "82e87f17199b319e0c338f575ebb9865d85da93e497863143029f61a17565c63");
	add_checkpoint(107500, "20c615de5617992239a175acfe0e2e768209d76d2489616c745392ac123b294e");
	add_checkpoint(107600, "16d7770efd009a0c9b5c8d484fd244e71c250374fce62ffc156c650ffd348a3b");
  add_checkpoint(101000, "de2a38cb184870277642a76b48559a34dbee66e7eba46b214d43074bf501eadc");
	add_checkpoint(105000, "fd96f6c1a9147ac5b643b649ff9966143a45e9d61650dd4c02e9a018ccb6ce66");
	add_checkpoint(110000, "e34e24c035c5d1b5d0b888f393dfc3a94b62956729fad8e52a20587f8232f079");
	add_checkpoint(119558, "2fef20abf1adc33dad396ab7f61f2589b4c885e48f1beec78aa6149dc4d3b867");
  //hf v6
	add_checkpoint(181056, "180d0ac84048d1dd57126c38b53c353df90fa73aeb60def9359e21e55b4b2946");
	add_checkpoint(200000, "86b1512952135f298361b216ef9441973f1567f1a7522ada2419ebe015b505a0");
	add_checkpoint(250000, "f6cc274ba450040855a35cea9018d0e47fbb9edff3311ec55beaffe586336605");
	add_checkpoint(300000, "972bb056059e4edba555726be385a27e8dafc41fdf959820f4e97b9d4763938b");

    return true;
  }

  bool checkpoints::load_checkpoints_from_json(const std::string &json_hashfile_fullpath)
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
    if (!epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath))
    {
      MERROR("Error loading checkpoints from " << json_hashfile_fullpath);
      return false;
    }
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

  bool checkpoints::load_checkpoints_from_dns(network_type nettype)
  {
    std::vector<std::string> records;

    // All four MoneroPulse domains have DNSSEC on and valid
    static const std::vector<std::string> dns_urls = {
    };

    static const std::vector<std::string> testnet_dns_urls = {
    };

    static const std::vector<std::string> stagenet_dns_urls = {
    };

    if (!tools::dns_utils::load_txt_records_from_dns(records, nettype == TESTNET ? testnet_dns_urls : nettype == STAGENET ? stagenet_dns_urls : dns_urls))
      return true; // why true ?

    for (const auto& record : records)
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
        if (!epee::string_tools::hex_to_pod(hashStr, hash))
        {
    continue;
        }

        ADD_CHECKPOINT(height, hashStr);
      }
    }
    return true;
  }

  bool checkpoints::load_new_checkpoints(const std::string &json_hashfile_fullpath, network_type nettype, bool dns)
  {
    bool result;

    result = load_checkpoints_from_json(json_hashfile_fullpath);
    if (dns)
    {
      result &= load_checkpoints_from_dns(nettype);
    }

    return result;
  }
}
