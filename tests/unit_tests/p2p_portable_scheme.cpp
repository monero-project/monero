#include <gtest/gtest.h>

#include "byte_slice.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "p2p/p2p_protocol_defs.h"
#include "storages/portable_storage_template_helper.h"

#include <vector>
#include <string>

TEST(portable_scheme_binary_codec_large_packets, dos_1)
{
  std::vector<uint8_t> sample{0x01, 0x11, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x04, 0x01, 0x61, 0x8c, 0x32, 0xd9, 0xf5, 0x05};
  if (sample.size() < 100 * 1000 * 1000) {
    std::size_t offset = sample.size();
    sample.resize(100 * 1000 * 1000);
    for (const std::vector<uint8_t> e{0x04, 0x00, 0x8d, 0x00}; offset + e.size() < sample.size(); offset += e.size()) {
      std::memcpy(sample.data() + offset, e.data(), e.size());
    }
  }
  EXPECT_TRUE(sample.size() >= 99e+6 and sample.size() < 105e+6);
  nodetool::COMMAND_PING::request in{};
  EXPECT_TRUE(epee::serialization::load_t_from_binary(in, {sample.data(), sample.size()}));
}

TEST(portable_scheme_binary_codec_large_packets, dos_2)
{
  std::vector<uint8_t> sample{0x01, 0x11, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x04, 0x01, 0x61, 0x8c, 0xbe, 0x83, 0xd7, 0x17};
  sample.resize(100 * 1000 * 1000, {});
  EXPECT_TRUE(sample.size() >= 99e+6 and sample.size() < 105e+6);
  nodetool::COMMAND_PING::request in{};
  EXPECT_TRUE(epee::serialization::load_t_from_binary(in, {sample.data(), sample.size()}));
}

TEST(portable_scheme_binary_codec_large_packets, too_small_txs_blobs)
{
  using T = cryptonote::NOTIFY_NEW_TRANSACTIONS::request;
  epee::byte_slice sample;
  if (sample.empty()) {
    T out{};
    out.txs.resize(100 * 1000 * 1000 / (1 + 41 - 1), std::string(41 - 1, '\x00'));
    epee::serialization::store_t_to_binary(out, sample);
  }
  EXPECT_TRUE(sample.size() >= 99e+6 and sample.size() < 105e+6);
  T in{};
  EXPECT_FALSE(epee::serialization::load_t_from_binary(in, {sample.data(), sample.size()}));
}

TEST(portable_scheme_binary_codec_large_packets, smallest_txs_blobs)
{
  using T = cryptonote::NOTIFY_NEW_TRANSACTIONS::request;
  epee::byte_slice sample;
  if (sample.empty()) {
    T out{};
    out.txs.resize(100 * 1000 * 1000 / (1 + 41), std::string(41, '\x00'));
    epee::serialization::store_t_to_binary(out, sample);
  }
  EXPECT_TRUE(sample.size() >= 99e+6 and sample.size() < 105e+6);
  T in{};
  EXPECT_TRUE(epee::serialization::load_t_from_binary(in, {sample.data(), sample.size()}));
}

TEST(portable_scheme_binary_codec_large_packets, too_small_block_blobs)
{
  using T = cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request;
  epee::byte_slice sample;
  if (sample.empty()) {
    T out{};
    std::string blob(72 - 1, '\x00');
    out.blocks.resize(100 * 1000 * 1000 / (blob.size() + 9), {});
    for (auto &e: out.blocks) {
      e.block = blob;
    }
    epee::serialization::store_t_to_binary(out, sample);
  }
  EXPECT_TRUE(sample.size() >= 99e+6 and sample.size() < 105e+6);
  T in{};
  EXPECT_FALSE(epee::serialization::load_t_from_binary(in, {sample.data(), sample.size()}));

}

TEST(portable_scheme_binary_codec_large_packets, smallest_block_blobs)
{
  using T = cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request;
  epee::byte_slice sample;
  if (sample.empty()) {
    T out{};
    std::string blob(72, '\x00');
    out.blocks.resize(100 * 1000 * 1000 / (blob.size() + 9), {});
    for (auto &e: out.blocks) {
      e.block = blob;
    }
    epee::serialization::store_t_to_binary(out, sample);
  }
  EXPECT_TRUE(sample.size() >= 99e+6 and sample.size() < 105e+6);
  T in{};
  EXPECT_TRUE(epee::serialization::load_t_from_binary(in, {sample.data(), sample.size()}));

}

TEST(portable_scheme_binary_codec_large_packets, too_small_compact_txs_blobs)
{
  using T = cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request;
  epee::byte_slice sample;
  if (sample.empty()) {
    T out{};
    out.blocks.resize(1);
    std::string block_blob(72, '\x00');
    std::string tx_blob(41 - 1, '\x00');
    out.blocks[0].block = block_blob;
    out.blocks[0].txs.resize(100 * 1000 * 1000 / (1 + tx_blob.size()), tx_blob);
    epee::serialization::store_t_to_binary(out, sample);
  }
  EXPECT_TRUE(sample.size() >= 99e+6 and sample.size() < 105e+6);
  T in{};
  EXPECT_FALSE(epee::serialization::load_t_from_binary(in, {sample.data(), sample.size()}));
}

TEST(portable_scheme_binary_codec_large_packets, smallest_compact_txs_blobs)
{
  using T = cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request;
  epee::byte_slice sample;
  if (sample.empty()) {
    T out{};
    out.blocks.resize(1);
    std::string block_blob(72, '\x00');
    std::string tx_blob(41, '\x00');
    out.blocks[0].block = block_blob;
    out.blocks[0].txs.resize(100 * 1000 * 1000 / (1 + tx_blob.size()), tx_blob);
    epee::serialization::store_t_to_binary(out, sample);
  }
  EXPECT_TRUE(sample.size() >= 99e+6 and sample.size() < 105e+6);
  T in{};
  EXPECT_TRUE(epee::serialization::load_t_from_binary(in, {sample.data(), sample.size()}));
}
