// Copyright (c) 2014-2022, The Monero Project
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
#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
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
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str, const std::string& difficulty_str)
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
    if (!difficulty_str.empty())
    {
      try
      {
        difficulty_type difficulty(difficulty_str);
        if (m_difficulty_points.count(height))
        {
          CHECK_AND_ASSERT_MES(difficulty == m_difficulty_points[height], false, "Difficulty checkpoint at given height already exists, and difficulty for new checkpoint was different!");
        }
        m_difficulty_points[height] = difficulty;
      }
      catch (...)
      {
        LOG_ERROR("Failed to parse difficulty checkpoint: " << difficulty_str);
        return false;
      }
    }
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
  //---------------------------------------------------------------------------
  const std::map<uint64_t, difficulty_type>& checkpoints::get_difficulty_points() const
  {
    return m_difficulty_points;
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
      ADD_CHECKPOINT2(0,     "48ca7cd3c8de5b6a4d53d2861fbdaedca141553559f9be9520068053cda8430b", "0x1");
      ADD_CHECKPOINT2(1000000, "46b690b710a07ea051bc4a6b6842ac37be691089c0f7758cfeec4d5fc0b4a258", "0x7aaad7153");
      ADD_CHECKPOINT2(1058600, "12904f6b4d9e60fd875674e07147d2c83d6716253f046af7b894c3e81da7e1bd", "0x971efd119");
      ADD_CHECKPOINT2(1450000, "87562ca6786f41556b8d5b48067303a57dc5ca77155b35199aedaeca1550f5a0", "0xa639e2930e");
      return true;
    }
    if (nettype == STAGENET)
    {
      ADD_CHECKPOINT2(0,       "76ee3cc98646292206cd3e86f74d88b4dcc1d937088645e9b0cbca84b7ce74eb", "0x1");
      ADD_CHECKPOINT2(10000,   "1f8b0ce313f8b9ba9a46108bfd285c45ad7c2176871fd41c3a690d4830ce2fd5", "0x1d73ba");
      ADD_CHECKPOINT2(550000,  "409f68cddd8e74b37469b41c1e61250d81c5776b42264f416d5d27c4626383ed", "0x5f3d4d03e");
      return true;
    }
    ADD_CHECKPOINT2(1,     "771fbcd656ec1464d3a02ead5e18644030007a0fc664c0a964d30922821a8148", "0x2");
    ADD_CHECKPOINT2(10,    "c0e3b387e47042f72d8ccdca88071ff96bff1ac7cde09ae113dbb7ad3fe92381", "0x2a974");
    ADD_CHECKPOINT2(100,   "ac3e11ca545e57c49fca2b4e8c48c03c23be047c43e471e1394528b1f9f80b2d", "0x35d14b");
    ADD_CHECKPOINT2(1000,  "5acfc45acffd2b2e7345caf42fa02308c5793f15ec33946e969e829f40b03876", "0x36a0373");
    ADD_CHECKPOINT2(10000, "c758b7c81f928be3295d45e230646de8b852ec96a821eac3fea4daf3fcac0ca2", "0x60a91390");
    ADD_CHECKPOINT2(22231, "7cb10e29d67e1c069e6e11b17d30b809724255fee2f6868dc14cfc6ed44dfb25", "0x1e288793d");
    ADD_CHECKPOINT2(29556, "53c484a8ed91e4da621bb2fa88106dbde426fe90d7ef07b9c1e5127fb6f3a7f6", "0x71f64cce8");
    ADD_CHECKPOINT2(50000, "0fe8758ab06a8b9cb35b7328fd4f757af530a5d37759f9d3e421023231f7b31c", "0x893044b400");
    ADD_CHECKPOINT2(80000, "a62dcd7b536f22e003ebae8726e9e7276f63d594e264b6f0cd7aab27b66e75e3", "0x5cc113f1076");
    ADD_CHECKPOINT2(202612, "bbd604d2ba11ba27935e006ed39c9bfdd99b76bf4a50654bc1e1e61217962698", "0x73310a259eb2");
    ADD_CHECKPOINT2(202613, "e2aa337e78df1f98f462b3b1e560c6b914dec47b610698b7b7d1e3e86b6197c2", "0x733154039b97");
    ADD_CHECKPOINT2(202614, "c29e3dc37d8da3e72e506e31a213a58771b24450144305bcba9e70fa4d6ea6fb", "0x73319dc90cb6");
    ADD_CHECKPOINT2(205000, "5d3d7a26e6dc7535e34f03def711daa8c263785f73ec1fadef8a45880fde8063", "0x75fcc3d85123");
    ADD_CHECKPOINT2(220000, "9613f455933c00e3e33ac315cc6b455ee8aa0c567163836858c2d9caff111553", "0x89cfed0cae3c");
    ADD_CHECKPOINT2(230300, "bae7a80c46859db355556e3a9204a337ae8f24309926a1312323fdecf1920e61", "0x967d13e5baa9");
    ADD_CHECKPOINT2(230700, "93e631240ceac831da1aebfc5dac8f722c430463024763ebafa888796ceaeedf", "0x96fb9663ebe7");
    ADD_CHECKPOINT2(231350, "b5add137199b820e1ea26640e5c3e121fd85faa86a1e39cf7e6cc097bdeb1131", "0x97b9919177bf");
    ADD_CHECKPOINT2(232150, "955de8e6b6508af2c24f7334f97beeea651d78e9ade3ab18fec3763be3201aa8", "0x98a038b612e8");
    ADD_CHECKPOINT2(249380, "654fb0a81ce3e5caf7e3264a70f447d4bd07586c08fa50f6638cc54da0a52b2d", "0xac9739634e6d");
    ADD_CHECKPOINT2(460000, "75037a7aed3e765db96c75bcf908f59d690a5f3390baebb9edeafd336a1c4831", "0x167799549bdda");
    ADD_CHECKPOINT2(500000, "2428f0dbe49796be05ed81b347f53e1f7f44aed0abf641446ec2b94cae066b02", "0x188ce145e4ba9");
    ADD_CHECKPOINT2(600000, "f5828ebf7d7d1cb61762c4dfe3ccf4ecab2e1aad23e8113668d981713b7a54c5", "0x1d9f3759e1554");
    ADD_CHECKPOINT2(700000, "12be9b3d210b93f574d2526abb9c1ab2a881b479131fd0d4f7dac93875f503cd", "0x2201e4ee39c2c");
    ADD_CHECKPOINT2(825000, "56503f9ad766774b575be3aff73245e9d159be88132c93d1754764f28da2ff60", "0x27565a442d5df");
    ADD_CHECKPOINT2(900000, "d9958d0e7dcf91a5a7b11de225927bf7efc6eb26240315ce12372be902cc1337", "0x2a6334031546e");
    ADD_CHECKPOINT2(913193, "5292d5d56f6ba4de33a58d9a34d263e2cb3c6fee0aed2286fd4ac7f36d53c85f", "0x2aefe7f40f5ea");
    ADD_CHECKPOINT2(1000000, "a886ef5149902d8342475fee9bb296341b891ac67c4842f47a833f23c00ed721", "0x2edd71370f0e5");
    ADD_CHECKPOINT2(1100000, "3fd720c5c8b3072fc1ccda922dec1ef25f9ed88a1e6ad4103d0fe00b180a5903", "0x390eb0035c53a");
    ADD_CHECKPOINT2(1150000, "1dd16f626d18e1e988490dfd06de5920e22629c972c58b4d8daddea0038627b2", "0x422d5662e9e37");
    ADD_CHECKPOINT2(1200000, "fa7d13a90850882060479d100141ff84286599ae39c3277c8ea784393f882d1f", "0x4c73503fc4aa3");
    ADD_CHECKPOINT2(1300000, "31b34272343a44a9f4ac7de7a8fcf3b7d8a3124d7d6870affd510d2f37e74cd0", "0x723f49bc249d5");
    ADD_CHECKPOINT2(1390000, "a8f5649dd4ded60eedab475f2bec8c934681c07e3cf640e9be0617554f13ff6c", "0xb4bba65e2841b");
    ADD_CHECKPOINT2(1450000, "ac94e8860093bc7c83e4e91215cba1d663421ecf4067a0ae609c3a8b52bcfac2", "0x11a4aabdca9511");
    ADD_CHECKPOINT2(1530000, "01759bce497ec38e63c78b1038892169203bb78f87e488172f6b854fcd63ba7e", "0x2819ce9f9e91e5");
    ADD_CHECKPOINT2(1579000, "7d0d7a2346373afd41ed1e744a939fc5d474a7dbaa257be5c6fff4009e789241", "0x357a590e7dda83");
    ADD_CHECKPOINT2(1668900, "ac2dcaf3d2f58ffcf8391639f0f1ebafcb8eac43c49479c7c37f611868d07568", "0x474226e475cc3b");
    ADD_CHECKPOINT2(1775600, "1c6e01c661dc22cab939e79ec6a5272190624ce8356d2f7b958e4f9a57fdb05e", "0x5e3b9d206a27c6");
    ADD_CHECKPOINT2(1856000, "9b57f17f29c71a3acd8a7904b93c41fa6eb8d2b7c73936ce4f1702d14880ba29", "0x6bde5e1caccee1");
    ADD_CHECKPOINT2(1958000, "98a5d6e51afdf3146e0eefb10a66e8648d8d4d5c2742be8835e976ba217c9bb2", "0x79dd46d2a0971a");
    ADD_CHECKPOINT2(2046000, "5e867f0b8baefed9244a681df97fc885d8ab36c3dfcd24c7a3abf3b8ac8b8314", "0x9cb8b6ff2978c6");
    ADD_CHECKPOINT2(2092500, "c4e00820c9c7989b49153d5e90ae095a18a11d990e82fcc3be54e6ed785472b5", "0xb4e585a31369cb");
    ADD_CHECKPOINT2(2182500, "0d22b5f81982eff21d094af9e821dc2007e6342069e3b1a37b15d97646353124", "0xead4a874083492");
    ADD_CHECKPOINT2(2661600, "41c9060e8426012238e8a26da26fcb90797436896cc70886a894c2c560bcccf2", "0x2e0d87526ff161f");
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
    static const std::vector<std::string> dns_urls = { "checkpoints.moneropulse.se"
						     , "checkpoints.moneropulse.org"
						     , "checkpoints.moneropulse.net"
						     , "checkpoints.moneropulse.co"
    };

    static const std::vector<std::string> testnet_dns_urls = { "testpoints.moneropulse.se"
							     , "testpoints.moneropulse.org"
							     , "testpoints.moneropulse.net"
							     , "testpoints.moneropulse.co"
    };

    static const std::vector<std::string> stagenet_dns_urls = { "stagenetpoints.moneropulse.se"
                   , "stagenetpoints.moneropulse.org"
                   , "stagenetpoints.moneropulse.net"
                   , "stagenetpoints.moneropulse.co"
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
