#include "gtest/gtest.h"

#include "cryptonote_core/cryptonote_format_utils.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "serialization/json_object.h"
#include "rpc/message_data_structs.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <unordered_map>
#include <algorithm>

using namespace cryptonote;
using epee::string_tools::pod_to_hex;

#define ASSERT_HASH_EQ(a,b) ASSERT_EQ(pod_to_hex(a), pod_to_hex(b))

namespace {  // anonymous namespace

const std::vector<std::string> t_blocks =
  {
    "0100d5adc49a053b8818b2b6023cd2d532c6774e164a8fcacd603651cb3ea0cb7f9340b28ec016b4bc4ca301aa0101ff6e08acbb2702eab03067870349139bee7eab2ca2e030a6bb73d4f68ab6a3b6ca937214054cdac0843d028bbe23b57ea9bae53f12da93bb57bf8a2e40598d9fccd10c2921576e987d93cd80b4891302468738e391f07c4f2b356f7957160968e0bfef6e907c3cee2d8c23cbf04b089680c6868f01025a0f41f063e195a966051e3a29e17130a9ce97d48f55285b9bb04bdd55a09ae78088aca3cf0202d0f26169290450fe17e08974789c3458910b4db18361cdc564f8f2d0bdd2cf568090cad2c60e02d6f3483ec45505cc3be841046c7a12bf953ac973939bc7b727e54258e1881d4d80e08d84ddcb0102dae6dfb16d3e28aaaf43e00170b90606b36f35f38f8a3dceb5ee18199dd8f17c80c0caf384a30202385d7e57a4daba4cdd9e550a92dcc188838386e7581f13f09de796cbed4716a42101c052492a077abf41996b50c1b2e67fd7288bcd8c55cdc657b4e22d0804371f6901beb76a82ea17400cd6d7f595f70e1667d2018ed8f5a78d1ce07484222618c3cd"
  , "0100f9adc49a057d3113f562eac36f14afa08c22ae20bbbf8cffa31a4466d24850732cb96f80e9762365ee01ab0101ff6f08cc953502be76deb845c431f2ed9a4862457654b914003693b8cd672abc935f0d97b16380c08db7010291819f2873e3efbae65ecd5a736f5e8a26318b591c21e39a03fb536520ac63ba80dac40902439a10fde02e39e48e0b31e57cc084a07eedbefb8cbea0143aedd0442b189caa80c6868f010227b84449de4cd7a48cbdce8974baf0b6646e03384e32055e705c243a86bef8a58088aca3cf0202fa7bd15e4e7e884307ab130bb9d50e33c5fcea6546042a26f948efd5952459ee8090cad2c60e028695583dbb8f8faab87e3ef3f88fa827db097bbf51761d91924f5c5b74c6631780e08d84ddcb010279d2f247b54690e3b491e488acff16014a825fd740c23988a25df7c4670c1f2580c0caf384a302022599dfa3f8788b66295051d85937816e1c320cdb347a0fba5219e3fe60c83b2421010576509c5672025d28fd5d3f38efce24e1f9aaf65dd3056b2504e6e2b7f19f7800"
  };

const std::vector<std::vector<std::string>> t_transactions =
  {
    {
      "0100010280e08d84ddcb0106010401110701f254220bb50d901a5523eaed438af5d43f8c6d0e54ba0632eb539884f6b7c02008c0a8a50402f9c7cf807ae74e56f4ec84db2bd93cfb02c2249b38e306f5b54b6e05d00d543b8095f52a02b6abb84e00f47f0a72e37b6b29392d906a38468404c57db3dbc5e8dd306a27a880d293ad0302cfc40a86723e7d459e90e45d47818dc0e81a1f451ace5137a4af8110a89a35ea80b4c4c321026b19c796338607d5a2c1ba240a167134142d72d1640ef07902da64fed0b10cfc8088aca3cf02021f6f655254fee84161118b32e7b6f8c31de5eb88aa00c29a8f57c0d1f95a24dd80d0b8e1981a023321af593163cea2ae37168ab926efd87f195756e3b723e886bdb7e618f751c480a094a58d1d0295ed2b08d1cf44482ae0060a5dcc4b7d810a85dea8c62e274f73862f3d59f8ed80a0e5b9c2910102dc50f2f28d7ceecd9a1147f7106c8d5b4e08b2ec77150f52dd7130ee4f5f50d42101d34f90ac861d0ee9fe3891656a234ea86a8a93bf51a237db65baa00d3f4aa196a9e1d89bc06b40e94ea9a26059efc7ba5b2de7ef7c139831ca62f3fe0bb252008f8c7ee810d3e1e06313edf2db362fc39431755779466b635f12f9f32e44470a3e85e08a28fcd90633efc94aa4ae39153dfaf661089d045521343a3d63e8da08d7916753c66aaebd4eefcfe8e58e5b3d266b752c9ca110749fa33fce7c44270386fcf2bed4f03dd5dadb2dc1fd4c505419f8217b9eaec07521f0d8963e104603c926745039cf38d31de6ed95ace8e8a451f5a36f818c151f517546d55ac0f500e54d07b30ea7452f2e93fa4f60bdb30d71a0a97f97eb121e662006780fbf69002228224a96bff37893d47ec3707b17383906c0cd7d9e7412b3e6c8ccf1419b093c06c26f96e3453b424713cdc5c9575f81cda4e157052df11f4c40809edf420f88a3dd1f7909bbf77c8b184a933389094a88e480e900bcdbf6d1824742ee520fc0032e7d892a2b099b8c6edfd1123ce58a34458ee20cad676a7f7cfd80a28f0cb0888af88838310db372986bdcf9bfcae2324480ca7360d22bff21fb569a530e"
    }
  , {
    }
  };

// convert hex string to string that has values based on that hex
// thankfully should automatically ignore null-terminator.
std::string h2b(const std::string& s)
{
  bool upper = true;
  std::string result;
  unsigned char val = 0;
  for (char c : s)
  {
    if (upper)
    {
      val = 0;
      if (c <= 'f' && c >= 'a')
      {
        val = ((c - 'a') + 10) << 4;
      }
      else
      {
        val = (c - '0') << 4;
      }
    }
    else
    {
      if (c <= 'f' && c >= 'a')
      {
        val |= (c - 'a') + 10;
      }
      else
      {
        val |= c - '0';
      }
      result += (char)val;
    }
    upper = !upper;
  }
  return result;
}

std::vector<block> getBlocks()
{
  std::vector<block> blocks;
  for (auto& i : t_blocks)
  {
      block bl;
      blobdata bd = h2b(i);
      parse_and_validate_block_from_blob(bd, bl);
      blocks.push_back(bl);
  }

  return blocks;
}

std::vector<transaction> getTransactions()
{
  std::vector<transaction> transactions;
  for (auto& i : t_transactions)
  {
    for (auto& j : i)
    {
      transaction bl;
      blobdata bd = h2b(j);
      parse_and_validate_tx_from_blob(bd, bl);
      transactions.push_back(bl);
    }
  }

  return transactions;
}

std::unordered_map<crypto::hash, cryptonote::transaction> getBlockTransactions(cryptonote::block& b, std::vector<cryptonote::transaction> txs)
{
  std::unordered_map<crypto::hash, cryptonote::transaction> map;

  for (auto& tx : txs)
  {
    auto hash = get_transaction_hash(tx);

    auto itr = std::find(b.tx_hashes.begin(), b.tx_hashes.end(), hash);

    if (itr != b.tx_hashes.end())
    {
      map.emplace(hash, tx);
    }
  }

  return map;
}

TEST(JsonSerialization, SerializeBlock)
{
  auto blocks = getBlocks();

  rapidjson::Document d;

  d.SetObject();

  rapidjson::Value v;

  v.SetArray();

  for (auto& bl : blocks)
  {
    auto b_as_json = json::toJsonValue(d, bl);

    auto b_from_json = json::fromJsonValue<block>(b_as_json);

    v.PushBack(b_as_json, d.GetAllocator());

    ASSERT_EQ(bl, b_from_json);
  }

  d.AddMember("blocks", v, d.GetAllocator());

  rapidjson::StringBuffer buf;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
  d.Accept(writer);

  // NOTE: Uncomment below for debugging (or just for funsies)
  // std::cout << "Blocks: " << buf.GetString() << std::endl;
}

TEST(JsonSerialization, SerializeTransaction)
{
  auto transactions = getTransactions();

  rapidjson::Document d;

  d.SetObject();

  rapidjson::Value v;

  v.SetArray();

  for (auto& tx : transactions)
  {
    auto tx_as_json = json::toJsonValue(d, tx);

    auto tx_from_json = json::fromJsonValue<transaction>(tx_as_json);

    v.PushBack(tx_as_json, d.GetAllocator());

    ASSERT_EQ(tx, tx_from_json);
  }

  d.AddMember("transactions", v, d.GetAllocator());

  rapidjson::StringBuffer buf;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
  d.Accept(writer);

  // NOTE: Uncomment below for debugging (or just for funsies)
  // std::cout << "Transactions: " << buf.GetString() << std::endl;
}

TEST(JsonSerialization, SerializeUnorderedMap)
{
  std::unordered_map<std::string, std::string> u_map;

  u_map.emplace("foo", "bar");
  u_map.emplace("bar", "baz");

  std::cout << "map init:" << std::endl;
  for (auto& i : u_map)
  {
    std::cout << "  " << i.first << ": " << i.second << std::endl;
  }

  std::cout << std::endl;

  rapidjson::Document d;

  d.SetObject();

  auto map_as_json = json::toJsonValue(d, u_map);

  auto back_to_map = json::fromJsonValue<decltype(u_map)>(map_as_json);

  std::cout << "back to map successful" << std::endl;

  ASSERT_EQ(u_map, back_to_map);

  d.AddMember("map", map_as_json, d.GetAllocator());

  rapidjson::StringBuffer buf;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
  d.Accept(writer);
  std::cout << std::endl << "Map: " << buf.GetString() << std::endl << std::endl;
}

TEST(JsonSerialization, SerializeBlockWithTransactions)
{
  auto transactions = getTransactions();
  auto blocks = getBlocks();

  for (auto& block : blocks)
  {
    rapidjson::Document d;

    d.SetObject();

    auto block_txs = getBlockTransactions(block, transactions);

    cryptonote::rpc::block_with_transactions bwt;

    bwt.block = block;
    bwt.transactions = block_txs;

    auto block_as_json = json::toJsonValue(d, bwt);

    auto back_to_class = json::fromJsonValue<cryptonote::rpc::block_with_transactions>(block_as_json);

    ASSERT_EQ(bwt.block, back_to_class.block);
    ASSERT_EQ(bwt.transactions, back_to_class.transactions);

    d.AddMember("block", block_as_json, d.GetAllocator());

    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
    d.Accept(writer);
    std::cout << std::endl << "Block: " << buf.GetString() << std::endl << std::endl;

  }
}

}  // anonymous namespace
