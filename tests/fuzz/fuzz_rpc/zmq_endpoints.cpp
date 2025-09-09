#include "cryptonote_basic/events.h"
#include "cryptonote_core/tx_pool.h"
#include "rpc/zmq_pub.h"
#include "rpc_endpoints.h"
#include <cstring>
#include <map>

using namespace cryptonote;
using namespace cryptonote::listener;

void fuzz_sub_request(zmq_pub& pub, FuzzedDataProvider& provider) {
  std::string sub = provider.ConsumeRandomLengthString(64);
  pub.sub_request(boost::string_ref(sub));
}

void fuzz_send_chain_main(zmq_pub& pub, FuzzedDataProvider& provider) {
  uint64_t height = provider.ConsumeIntegral<uint64_t>();
  size_t blk_count = provider.ConsumeIntegralInRange<size_t>(0, 4);
  std::vector<block> blocks;

  for (size_t i = 0; i < blk_count; ++i) {
    block b{};
    b.major_version = provider.ConsumeIntegral<uint8_t>();
    b.minor_version = provider.ConsumeIntegral<uint8_t>();
    b.timestamp = provider.ConsumeIntegral<uint64_t>();
    b.prev_id = crypto::null_hash;
    b.nonce = provider.ConsumeIntegral<uint32_t>();
    blocks.push_back(b);
  }

  pub.send_chain_main(height, epee::span<const block>(blocks.data(), blocks.size()));
}

void fuzz_send_miner_data(zmq_pub& pub, FuzzedDataProvider& provider) {
  uint8_t major = provider.ConsumeIntegral<uint8_t>();
  uint64_t h = provider.ConsumeIntegral<uint64_t>();

  crypto::hash prev_id, seed_hash;
  std::memset(&prev_id, 0x01, sizeof(prev_id));
  std::memset(&seed_hash, 0x02, sizeof(seed_hash));
  difficulty_type diff = provider.ConsumeIntegral<uint64_t>();

  uint64_t median_weight = provider.ConsumeIntegral<uint64_t>();
  uint64_t coins = provider.ConsumeIntegral<uint64_t>();
  std::vector<tx_block_template_backlog_entry> backlog;
  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 3);

  for (size_t i = 0; i < count; ++i) {
    tx_block_template_backlog_entry entry;
    entry.weight = provider.ConsumeIntegral<uint64_t>();
    entry.fee = provider.ConsumeIntegral<uint64_t>();
    backlog.push_back(entry);
  }

  pub.send_miner_data(major, h, prev_id, seed_hash, diff, median_weight, coins, backlog);
}

void fuzz_send_txpool_add(zmq_pub& pub, FuzzedDataProvider& provider) {
  size_t count = provider.ConsumeIntegralInRange<size_t>(0, 3);
  std::vector<txpool_event> events;
  for (size_t i = 0; i < count; ++i) {
    txpool_event evt{};

    evt.res = provider.ConsumeBool();

    auto blob = provider.ConsumeRandomLengthString(128);
    if (!parse_and_validate_tx_from_blob(blob, evt.tx)) {
      continue;
    }

    evt.hash = get_transaction_hash(evt.tx);
    evt.blob_size = blob.size();
    evt.weight = provider.ConsumeIntegral<uint64_t>();

    events.push_back(std::move(evt));
  }

  pub.send_txpool_add(std::move(events));
}

// Map all zmq targets
std::map<int, std::function<void(zmq_pub&, FuzzedDataProvider&)>> zmq_targets = {
  {0, fuzz_sub_request},
  {1, fuzz_send_chain_main},
  {2, fuzz_send_miner_data},
  {3, fuzz_send_txpool_add},
};
