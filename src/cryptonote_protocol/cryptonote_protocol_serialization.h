#pragma once

#include "cryptonote_protocol_defs.h"
#include "p2p/portable_scheme/scheme.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

namespace portable_scheme {

namespace scheme_space {

template<>
struct scheme<std::string> {
  using S = scheme<container_pod_tag<std::string>>;
  using tags = S::tags;
  using read_t = S::read_t;
  using write_t = S::write_t;
};

constexpr std::size_t MIN_TX_BLOB_SIZE = 41;
constexpr std::size_t MIN_BLOCK_BLOB_SIZE = 72;

template<>
struct scheme<cryptonote::tx_blob_entry> {
  using T = cryptonote::tx_blob_entry;
  using tags = map_tag<
    field_tag<KEY("blob"), span_tag<MIN_TX_BLOB_SIZE>, optional_t::required>,
    field_tag<KEY("prunable_hash"), span_tag<sizeof(T::prunable_hash)>, optional_t::required>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(blob)
    READ_FIELD_POD(prunable_hash)
  END_READ()

  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(blob)
    WRITE_FIELD_POD(prunable_hash)
  END_WRITE()
};

struct compact_tx_t {
  using tags = span_tag<MIN_TX_BLOB_SIZE>;
  using T = cryptonote::tx_blob_entry;
  using S = scheme<container_pod_tag<decltype(T::blob)>>;
  struct read_t: S::read_t {
    read_t(const T &t): S::read_t(t.blob) {}
  };
  struct write_t: S::write_t {
    write_t(T &t): S::write_t(t.blob) {}
  };
};

template<>
struct scheme<cryptonote::block_complete_entry> {
  using T = cryptonote::block_complete_entry;
  using compact_txs_t = scheme<container_tag<decltype(T::txs), compact_tx_t, 0, std::size_t(-1), 0, 2>>;
  using tags = map_tag<
    field_tag<KEY("block"), span_tag<MIN_BLOCK_BLOB_SIZE>, optional_t::required>,
    field_tag<KEY("block_weight"), base_tag<std::uint64_t>>,
    field_tag<KEY("pruned"), base_tag<bool>>,
    field_tag<KEY("txs"), scheme<container_tag<decltype(T::txs)>>::tags>,
    field_tag<KEY("txs"), compact_txs_t::tags>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(block)
    READ_FIELD_WITH_DEFAULT(block_weight, 0)
    READ_FIELD_WITH_DEFAULT(pruned, false)
    READ_FIELD_LIST_OPT(txs, {return t.pruned;})
    READ_FIELD_LIST_CUSTOM_OPT(txs, compact_txs_t, {return not t.pruned;})
  END_READ()
  BEGIN_WRITE_RAW(
    private:
      T &t;
    public:
      write_t(T &t): t(t) { t.txs.clear(); }
  )
    WRITE_FIELD_LIST_POD(block)
    WRITE_FIELD_WITH_DEFAULT(block_weight, 0)
    WRITE_FIELD_WITH_DEFAULT(pruned, false)
    WRITE_FIELD_LIST_OPT(txs)
    WRITE_FIELD_LIST_CUSTOM_OPT(txs, compact_txs_t)
  END_WRITE()
};

template<>
struct scheme<cryptonote::NOTIFY_NEW_BLOCK::request> {
  using T = cryptonote::NOTIFY_NEW_BLOCK::request;
  using tags = map_tag<
    field_tag<KEY("b"), scheme<decltype(T::b)>::tags>,
    field_tag<KEY("current_blockchain_height"), base_tag<std::uint64_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(b)
    READ_FIELD(current_blockchain_height)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(b)
    WRITE_FIELD(current_blockchain_height)
  END_WRITE()
};

template<>
struct scheme<cryptonote::NOTIFY_NEW_TRANSACTIONS::request> {
  using T = cryptonote::NOTIFY_NEW_TRANSACTIONS::request;
  using tags = map_tag<
    field_tag<KEY("_"), span_tag<>>,
    field_tag<KEY("dandelionpp_fluff"), base_tag<bool>>,
    field_tag<KEY("txs"), list_tag<span_tag<MIN_TX_BLOB_SIZE>>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(_)
    READ_FIELD_WITH_DEFAULT(dandelionpp_fluff, true)
    READ_FIELD_LIST(txs)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(_)
    WRITE_FIELD_WITH_DEFAULT(dandelionpp_fluff, true)
    WRITE_FIELD_LIST(txs)
  END_WRITE()
};

template<>
struct scheme<cryptonote::NOTIFY_REQUEST_GET_OBJECTS::request> {
  using T = cryptonote::NOTIFY_REQUEST_GET_OBJECTS::request;
  using tags = map_tag<
    field_tag<KEY("blocks"), span_tag<>>,
    field_tag<KEY("prune"), base_tag<bool>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(blocks)
    READ_FIELD_WITH_DEFAULT(prune, false)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(blocks)
    WRITE_FIELD_WITH_DEFAULT(prune, false)
  END_WRITE()
};

template<>
struct scheme<cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request> {
  using T = cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request;
  using tags = map_tag<
    field_tag<KEY("blocks"), scheme<container_tag<decltype(T::blocks)>>::tags>,
    field_tag<KEY("current_blockchain_height"), base_tag<std::uint64_t>>,
    field_tag<KEY("missed_ids"), span_tag<>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST(blocks)
    READ_FIELD(current_blockchain_height)
    READ_FIELD_LIST_POD(missed_ids)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST(blocks)
    WRITE_FIELD(current_blockchain_height)
    WRITE_FIELD_LIST_POD(missed_ids)
  END_WRITE()
};

template<>
struct scheme<cryptonote::CORE_SYNC_DATA> {
  using T = cryptonote::CORE_SYNC_DATA;
  using tags = map_tag<
    field_tag<KEY("cumulative_difficulty"), base_tag<std::uint64_t>>,
    field_tag<KEY("cumulative_difficulty_top64"), base_tag<std::uint64_t>>,
    field_tag<KEY("current_height"), base_tag<std::uint64_t>>,
    field_tag<KEY("pruning_seed"), base_tag<std::uint32_t>>,
    field_tag<KEY("top_id"), span_tag<>>,
    field_tag<KEY("top_version"), base_tag<std::uint8_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(cumulative_difficulty)
    READ_FIELD(cumulative_difficulty_top64)
    READ_FIELD(current_height)
    READ_FIELD_WITH_DEFAULT(pruning_seed, 0)
    READ_FIELD_POD(top_id)
    READ_FIELD_WITH_DEFAULT(top_version, 0)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(cumulative_difficulty)
    WRITE_FIELD_WITH_DEFAULT(cumulative_difficulty_top64, 0)
    WRITE_FIELD(current_height)
    WRITE_FIELD_WITH_DEFAULT(pruning_seed, 0)
    WRITE_FIELD_POD(top_id)
    WRITE_FIELD_WITH_DEFAULT(top_version, 0)
  END_WRITE()
};

template<>
struct scheme<cryptonote::NOTIFY_REQUEST_CHAIN::request> {
  using T = cryptonote::NOTIFY_REQUEST_CHAIN::request;
  using tags = map_tag<
    field_tag<KEY("block_ids"), span_tag<>>,
    field_tag<KEY("prune"), base_tag<bool>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(block_ids)
    READ_FIELD_WITH_DEFAULT(prune, false)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(block_ids)
    WRITE_FIELD_WITH_DEFAULT(prune, false)
  END_WRITE()
};

template<>
struct scheme<cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::request> {
  using T = cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::request;
  using tags = map_tag<
    field_tag<KEY("cumulative_difficulty"), base_tag<std::uint64_t>>,
    field_tag<KEY("cumulative_difficulty_top64"), base_tag<std::uint64_t>>,
    field_tag<KEY("first_block"), span_tag<>>,
    field_tag<KEY("m_block_ids"), span_tag<>>,
    #if 1
    field_tag<KEY("m_block_weights"), span_tag<>>,
    #else
    field_tag<KEY("m_block_weights"), span_tag<>>,
    field_tag<KEY("m_block_weights_"), list_tag<base_tag<std::uint64_t>>>,
    #endif
    field_tag<KEY("start_height"), base_tag<std::uint64_t>>,
    field_tag<KEY("total_height"), base_tag<std::uint64_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(cumulative_difficulty)
    READ_FIELD(cumulative_difficulty_top64)
    READ_FIELD_LIST_POD(first_block)
    READ_FIELD_LIST_POD(m_block_ids)
    #if 1
    READ_FIELD_LIST_POD_FORCE(m_block_weights)
    #else
    READ_FIELD_LIST_POD_OPT_FORCE(m_block_weights)
    READ_FIELD_LIST_OPT_NAME("m_block_weights_", t.m_block_weights)
    #endif
    READ_FIELD(start_height)
    READ_FIELD(total_height)
  END_READ()
  #if 1
  BEGIN_WRITE(T)
  #else
  BEGIN_WRITE_RAW(
    private:
      T &t;
    public:
      write_t(T &t): t(t) { t.m_block_weights.clear(); }
  )
  #endif
    WRITE_FIELD(cumulative_difficulty)
    WRITE_FIELD(cumulative_difficulty_top64)
    WRITE_FIELD_LIST_POD(first_block)
    WRITE_FIELD_LIST_POD(m_block_ids)
    #if 1
    WRITE_FIELD_LIST_POD_FORCE(m_block_weights)
    #else
    WRITE_FIELD_LIST_POD_OPT_FORCE(m_block_weights)
    WRITE_FIELD_LIST_OPT_NAME("m_block_weights_", t.m_block_weights)
    #endif
    WRITE_FIELD(start_height)
    WRITE_FIELD(total_height)
  END_WRITE()
};

template<>
struct scheme<cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::request> {
  using T = cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::request;
  using tags = map_tag<
    field_tag<KEY("b"), scheme<decltype(T::b)>::tags>,
    field_tag<KEY("current_blockchain_height"), base_tag<std::uint64_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD(b)
    READ_FIELD(current_blockchain_height)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(b)
    WRITE_FIELD(current_blockchain_height)
  END_WRITE()
};

template<>
struct scheme<cryptonote::NOTIFY_REQUEST_FLUFFY_MISSING_TX::request> {
  using T = cryptonote::NOTIFY_REQUEST_FLUFFY_MISSING_TX::request;
  using tags = map_tag<
    field_tag<KEY("block_hash"), span_tag<>>,
    field_tag<KEY("current_blockchain_height"), base_tag<std::uint64_t>>,
    #if 1
    field_tag<KEY("missing_tx_indices"), span_tag<>>
    #else
    field_tag<KEY("missing_tx_indices"), span_tag<>>,
    field_tag<KEY("missing_tx_indices_"), list_tag<base_tag<std::uint64_t>>>
    #endif
  >;
  BEGIN_READ(T)
    READ_FIELD_POD(block_hash)
    READ_FIELD(current_blockchain_height)
    #if 1
    READ_FIELD_LIST_POD_FORCE(missing_tx_indices)
    #else
    READ_FIELD_LIST_POD_OPT_FORCE(missing_tx_indices)
    READ_FIELD_LIST_OPT_NAME("missing_tx_indices_", t.missing_tx_indices)
    #endif
  END_READ()
  #if 1
  BEGIN_WRITE(T)
  #else
  BEGIN_WRITE_RAW(
    private:
      T &t;
    public:
      write_t(T &t): t(t) { t.missing_tx_indices.clear(); }
  )
  #endif
    WRITE_FIELD_POD(block_hash)
    WRITE_FIELD(current_blockchain_height)
    #if 1
    WRITE_FIELD_LIST_POD_FORCE(missing_tx_indices)
    #else
    WRITE_FIELD_LIST_POD_OPT_FORCE(missing_tx_indices)
    WRITE_FIELD_LIST_OPT_NAME("missing_tx_indices_", t.missing_tx_indices)
    #endif
  END_WRITE()
};

template<>
struct scheme<cryptonote::NOTIFY_GET_TXPOOL_COMPLEMENT::request> {
  using T = cryptonote::NOTIFY_GET_TXPOOL_COMPLEMENT::request;
  using tags = map_tag<
    field_tag<KEY("hashes"), span_tag<>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(hashes)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(hashes)
  END_WRITE()
};

}

}
