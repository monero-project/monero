
#include <boost/optional/optional.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <vector>

#include "byte_stream.h"
#include "crypto/hash.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "serialization/json_object.h"
#include "rpc/daemon_messages.h"


namespace test
{
    cryptonote::transaction
    make_miner_transaction(cryptonote::account_public_address const& to)
    {
        cryptonote::transaction tx{};
        if (!cryptonote::construct_miner_tx(0, 0, 5000, 500, 500, to, tx))
            throw std::runtime_error{"transaction construction error"};

        crypto::hash id{0};
        if (!cryptonote::get_transaction_hash(tx, id))
            throw std::runtime_error{"could not get transaction hash"};

        return tx;
    }

    cryptonote::transaction
    make_transaction(
        cryptonote::account_keys const& from,
        std::vector<cryptonote::transaction> const& sources,
        std::vector<cryptonote::account_public_address> const& destinations,
        bool rct,
        bool bulletproof)
    {
        std::uint64_t source_amount = 0;
        std::vector<cryptonote::tx_source_entry> actual_sources;
        for (auto const& source : sources)
        {
            std::vector<cryptonote::tx_extra_field> extra_fields;
            if (!cryptonote::parse_tx_extra(source.extra, extra_fields))
                throw std::runtime_error{"invalid transaction"};

            cryptonote::tx_extra_pub_key key_field{};
            if (!cryptonote::find_tx_extra_field_by_type(extra_fields, key_field))
                throw std::runtime_error{"invalid transaction"};

            for (auto const input : boost::adaptors::index(source.vout))
            {
                source_amount += input.value().amount;
                auto const& key = boost::get<cryptonote::txout_to_key>(input.value().target);

                actual_sources.push_back(
                    {{}, 0, key_field.pub_key, {}, std::size_t(input.index()), input.value().amount, rct, rct::identity()}
                );

                for (unsigned ring = 0; ring < 10; ++ring)
                    actual_sources.back().push_output(input.index(), key.key, input.value().amount);
            }
        }

        std::vector<cryptonote::tx_destination_entry> to;
        for (auto const& destination : destinations)
            to.push_back({(source_amount / destinations.size()), destination, false});

        cryptonote::transaction tx{};

        crypto::secret_key tx_key{};
        std::vector<crypto::secret_key> extra_keys{};

        std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
        subaddresses[from.m_account_address.m_spend_public_key] = {0,0};

        fcmp_pp::ProofParams fcmp_pp_params = {};

        if (!cryptonote::construct_tx_and_get_tx_key(from, subaddresses, actual_sources, to, boost::none, {}, tx, tx_key, extra_keys, fcmp_pp_params, rct, { bulletproof ? rct::RangeProofPaddedBulletproof : rct::RangeProofBorromean, bulletproof ? 2 : 0 }))
            throw std::runtime_error{"transaction construction error"};

        return tx;
    }
}

namespace
{
    template<typename T>
    T test_json(const T& value)
    {
      epee::byte_stream buffer;
      {
        rapidjson::Writer<epee::byte_stream> dest{buffer};
        cryptonote::json::toJsonValue(dest, value);
      }

      rapidjson::Document doc;
      doc.Parse(reinterpret_cast<const char*>(buffer.data()), buffer.size());
      if (doc.HasParseError())
      {
        throw cryptonote::json::PARSE_FAIL();
      }

      T out{};
      cryptonote::json::fromJsonValue(doc, out);
      return out;
    }
} // anonymous

TEST(JsonSerialization, VectorBytes)
{
    EXPECT_EQ(std::vector<std::uint8_t>{}, test_json(std::vector<std::uint8_t>{}));
    EXPECT_EQ(std::vector<std::uint8_t>{0x00}, test_json(std::vector<std::uint8_t>{0x00}));
}

TEST(JsonSerialization, InvalidVectorBytes)
{
    rapidjson::Document doc;
    doc.SetString("1");

    std::vector<std::uint8_t> out;
    EXPECT_THROW(cryptonote::json::fromJsonValue(doc, out), cryptonote::json::BAD_INPUT);
}

TEST(JsonSerialization, DaemonInfo)
{
  cryptonote::rpc::DaemonInfo info{};
  info.height = 154544;
  info.target_height = 15345435;
  info.top_block_height = 2344;
  info.wide_difficulty = cryptonote::difficulty_type{"100000000000000000005443"};
  info.difficulty = 200376420520695107;
  info.target = 7657567;
  info.tx_count = 355;
  info.tx_pool_size = 45435;
  info.alt_blocks_count = 43535;
  info.outgoing_connections_count = 1444;
  info.incoming_connections_count = 1444;
  info.white_peerlist_size = 14550;
  info.grey_peerlist_size = 34324;
  info.mainnet = true;
  info.testnet = true;
  info.stagenet = true;
  info.nettype = "main";
  info.top_block_hash = crypto::hash{1};
  info.wide_cumulative_difficulty = cryptonote::difficulty_type{"200000000000000000005543"};
  info.cumulative_difficulty = 400752841041384871;
  info.block_size_limit = 4324234;
  info.block_weight_limit = 3434;
  info.block_size_median = 3434;
  info.adjusted_time = 4535;
  info.block_weight_median = 43535;
  info.start_time = 34535;
  info.version = "1.0";

  const auto info_copy = test_json(info);

  EXPECT_EQ(info.height, info_copy.height);
  EXPECT_EQ(info.target_height, info_copy.target_height);
  EXPECT_EQ(info.top_block_height, info_copy.top_block_height);
  EXPECT_EQ(info.wide_difficulty, info_copy.wide_difficulty);
  EXPECT_EQ(info.difficulty, info_copy.difficulty);
  EXPECT_EQ(info.target, info_copy.target);
  EXPECT_EQ(info.tx_count, info_copy.tx_count);
  EXPECT_EQ(info.tx_pool_size, info_copy.tx_pool_size);
  EXPECT_EQ(info.alt_blocks_count, info_copy.alt_blocks_count);
  EXPECT_EQ(info.outgoing_connections_count, info_copy.outgoing_connections_count);
  EXPECT_EQ(info.incoming_connections_count, info_copy.incoming_connections_count);
  EXPECT_EQ(info.white_peerlist_size, info_copy.white_peerlist_size);
  EXPECT_EQ(info.grey_peerlist_size, info_copy.grey_peerlist_size);
  EXPECT_EQ(info.mainnet, info_copy.mainnet);
  EXPECT_EQ(info.testnet, info_copy.testnet);
  EXPECT_EQ(info.stagenet, info_copy.stagenet);
  EXPECT_EQ(info.nettype, info_copy.nettype);
  EXPECT_EQ(info.top_block_hash, info_copy.top_block_hash);
  EXPECT_EQ(info.wide_cumulative_difficulty, info_copy.wide_cumulative_difficulty);
  EXPECT_EQ(info.cumulative_difficulty, info_copy.cumulative_difficulty);
  EXPECT_EQ(info.block_size_limit, info_copy.block_size_limit);
  EXPECT_EQ(info.block_weight_limit, info_copy.block_weight_limit);
  EXPECT_EQ(info.block_size_median, info_copy.block_size_median);
  EXPECT_EQ(info.adjusted_time, info_copy.adjusted_time);
  EXPECT_EQ(info.block_weight_median, info_copy.block_weight_median);
  EXPECT_EQ(info.start_time, info_copy.start_time);
  EXPECT_EQ(info.version, info_copy.version);
}

TEST(JsonSerialization, MinerTransaction)
{
    cryptonote::account_base acct;
    acct.generate();
    const auto miner_tx = test::make_miner_transaction(acct.get_keys().m_account_address);

    crypto::hash tx_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(miner_tx, tx_hash));

    cryptonote::transaction miner_tx_copy = test_json(miner_tx);

    crypto::hash tx_copy_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(miner_tx_copy, tx_copy_hash));
    EXPECT_EQ(tx_hash, tx_copy_hash);

    cryptonote::blobdata tx_bytes{};
    cryptonote::blobdata tx_copy_bytes{};

    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(miner_tx, tx_bytes));
    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(miner_tx_copy, tx_copy_bytes));

    EXPECT_EQ(tx_bytes, tx_copy_bytes);
}

TEST(JsonSerialization, RegularTransaction)
{
    cryptonote::account_base acct1;
    acct1.generate();

    cryptonote::account_base acct2;
    acct2.generate();

    const auto miner_tx = test::make_miner_transaction(acct1.get_keys().m_account_address);
    const auto tx = test::make_transaction(
        acct1.get_keys(), {miner_tx}, {acct2.get_keys().m_account_address}, false, false
    );

    crypto::hash tx_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx, tx_hash));

    cryptonote::transaction tx_copy = test_json(tx);

    crypto::hash tx_copy_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx_copy, tx_copy_hash));
    EXPECT_EQ(tx_hash, tx_copy_hash);

    cryptonote::blobdata tx_bytes{};
    cryptonote::blobdata tx_copy_bytes{};

    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx, tx_bytes));
    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx_copy, tx_copy_bytes));

    EXPECT_EQ(tx_bytes, tx_copy_bytes);
}

TEST(JsonSerialization, RingctTransaction)
{
    cryptonote::account_base acct1;
    acct1.generate();

    cryptonote::account_base acct2;
    acct2.generate();

    const auto miner_tx = test::make_miner_transaction(acct1.get_keys().m_account_address);
    const auto tx = test::make_transaction(
        acct1.get_keys(), {miner_tx}, {acct2.get_keys().m_account_address}, true, false
    );

    crypto::hash tx_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx, tx_hash));

    cryptonote::transaction tx_copy = test_json(tx);

    crypto::hash tx_copy_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx_copy, tx_copy_hash));
    EXPECT_EQ(tx_hash, tx_copy_hash);

    cryptonote::blobdata tx_bytes{};
    cryptonote::blobdata tx_copy_bytes{};

    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx, tx_bytes));
    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx_copy, tx_copy_bytes));

    EXPECT_EQ(tx_bytes, tx_copy_bytes);
}

TEST(JsonSerialization, BulletproofTransaction)
{
    cryptonote::account_base acct1;
    acct1.generate();

    cryptonote::account_base acct2;
    acct2.generate();

    const auto miner_tx = test::make_miner_transaction(acct1.get_keys().m_account_address);
    const auto tx = test::make_transaction(
        acct1.get_keys(), {miner_tx}, {acct2.get_keys().m_account_address}, true, true
    );

    crypto::hash tx_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx, tx_hash));

    cryptonote::transaction tx_copy = test_json(tx);

    crypto::hash tx_copy_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx_copy, tx_copy_hash));
    EXPECT_EQ(tx_hash, tx_copy_hash);

    cryptonote::blobdata tx_bytes{};
    cryptonote::blobdata tx_copy_bytes{};

    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx, tx_bytes));
    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx_copy, tx_copy_bytes));

    EXPECT_EQ(tx_bytes, tx_copy_bytes);
}

TEST(JsonRpcSerialization, HandlerFromJson)
{
  cryptonote::rpc::FullMessage req_full("{\"jsonrpc\":\"2.0\",\"method\":\"get_hashes_fast\",\"params\":[1]}", true);
  cryptonote::rpc::GetHashesFast::Request request{};
  EXPECT_THROW(request.fromJson(req_full.getMessage()), cryptonote::json::WRONG_TYPE);
}
