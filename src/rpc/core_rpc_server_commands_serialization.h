#pragma once

#include "rpc/core_rpc_server_commands_defs.h"
#include "p2p/portable_scheme/scheme.h"
#include "cryptonote_protocol/cryptonote_protocol_serialization.h"

namespace portable_scheme {

namespace scheme_space {

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request> {
  using T = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request;
  using tags = map_tag<
    field_tag<KEY("block_ids"), span_tag<>>,
    field_tag<KEY("client"), span_tag<>>,
    field_tag<KEY("no_miner_tx"), base_tag<bool>>,
    field_tag<KEY("prune"), base_tag<bool>>,
    field_tag<KEY("start_height"), base_tag<std::uint64_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(block_ids)
    READ_FIELD_LIST_POD(client)
    READ_FIELD_WITH_DEFAULT(no_miner_tx, false)
    READ_FIELD(prune)
    READ_FIELD(start_height)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(block_ids)
    WRITE_FIELD_LIST_POD(client)
    WRITE_FIELD_WITH_DEFAULT(no_miner_tx, false)
    WRITE_FIELD(prune)
    WRITE_FIELD(start_height)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::tx_output_indices> {
  using T = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::tx_output_indices;
  using tags = map_tag<
    field_tag<KEY("indices"), list_tag<base_tag<std::uint64_t>, 1>, optional_t::exception>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST(indices)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST(indices)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> {
  using T = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices;
  using indices_t = scheme<container_tag<decltype(T::indices), scheme<decltype(T::indices)::value_type>, 0, std::size_t(-1), 514 + 1, 2>>;
  using tags = map_tag<
    field_tag<KEY("indices"), indices_t::tags>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_CUSTOM(indices, indices_t)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_CUSTOM(indices, indices_t)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response> {
  using T = cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response;
  using output_indices_t = scheme<container_tag<
    decltype(T::output_indices),
    scheme<decltype(T::output_indices)::value_type>,
    0,
    COMMAND_RPC_GET_BLOCKS_FAST_MAX_BLOCK_COUNT
  >>;
  using tags = map_tag<
    field_tag<KEY("blocks"), scheme<container_tag<decltype(T::blocks)>>::tags>,
    field_tag<KEY("credits"), base_tag<std::uint64_t>>,
    field_tag<KEY("current_height"), base_tag<std::uint64_t>>,
    field_tag<KEY("output_indices"), output_indices_t::tags>,
    field_tag<KEY("start_height"), base_tag<std::uint64_t>>,
    field_tag<KEY("status"), scheme<container_pod_tag<decltype(T::status)>>::tags>,
    field_tag<KEY("top_hash"), span_tag<>>,
    field_tag<KEY("untrusted"), base_tag<bool>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST(blocks)
    READ_FIELD(credits)
    READ_FIELD(current_height)
    READ_FIELD_LIST_CUSTOM(output_indices, output_indices_t)
    READ_FIELD(start_height)
    READ_FIELD_LIST_POD(status)
    READ_FIELD_LIST_POD(top_hash)
    READ_FIELD(untrusted)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST(blocks)
    WRITE_FIELD(credits)
    WRITE_FIELD(current_height)
    WRITE_FIELD_LIST_CUSTOM(output_indices, output_indices_t)
    WRITE_FIELD(start_height)
    WRITE_FIELD_LIST_POD(status)
    WRITE_FIELD_LIST_POD(top_hash)
    WRITE_FIELD(untrusted)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request> {
  using T = cryptonote::COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request;
  using tags = map_tag<
    field_tag<KEY("client"), span_tag<>>,
    field_tag<KEY("heights"), list_tag<base_tag<std::uint64_t>>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(client)
    READ_FIELD_LIST(heights)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(client)
    WRITE_FIELD_LIST(heights)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response> {
  using T = cryptonote::COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response;
  using tags = map_tag<
    field_tag<KEY("blocks"), scheme<container_tag<decltype(T::blocks)>>::tags>,
    field_tag<KEY("credits"), base_tag<std::uint64_t>>,
    field_tag<KEY("status"), scheme<container_pod_tag<decltype(T::status)>>::tags>,
    field_tag<KEY("top_hash"), span_tag<>>,
    field_tag<KEY("untrusted"), base_tag<bool>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST(blocks)
    READ_FIELD(credits)
    READ_FIELD_LIST_POD(status)
    READ_FIELD_LIST_POD(top_hash)
    READ_FIELD(untrusted)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST(blocks)
    WRITE_FIELD(credits)
    WRITE_FIELD_LIST_POD(status)
    WRITE_FIELD_LIST_POD(top_hash)
    WRITE_FIELD(untrusted)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_HASHES_FAST::request> {
  using T = cryptonote::COMMAND_RPC_GET_HASHES_FAST::request;
  using tags = map_tag<
    field_tag<KEY("block_ids"), span_tag<>>,
    field_tag<KEY("client"), span_tag<>>,
    field_tag<KEY("start_height"), base_tag<std::uint64_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(block_ids)
    READ_FIELD_LIST_POD(client)
    READ_FIELD(start_height)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(block_ids)
    WRITE_FIELD_LIST_POD(client)
    WRITE_FIELD(start_height)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_HASHES_FAST::response> {
  using T = cryptonote::COMMAND_RPC_GET_HASHES_FAST::response;
  using tags = map_tag<
    field_tag<KEY("credits"), base_tag<std::uint64_t>>,
    field_tag<KEY("current_height"), base_tag<std::uint64_t>>,
    field_tag<KEY("m_block_ids"), span_tag<>>,
    field_tag<KEY("start_height"), base_tag<std::uint64_t>>,
    field_tag<KEY("status"), scheme<container_pod_tag<decltype(T::status)>>::tags>,
    field_tag<KEY("top_hash"), span_tag<>>,
    field_tag<KEY("untrusted"), base_tag<bool>>
  >;
  BEGIN_READ(T)
    READ_FIELD(credits)
    READ_FIELD(current_height)
    READ_FIELD_LIST_POD(m_block_ids)
    READ_FIELD(start_height)
    READ_FIELD_LIST_POD(status)
    READ_FIELD_LIST_POD(top_hash)
    READ_FIELD(untrusted)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(credits)
    WRITE_FIELD(current_height)
    WRITE_FIELD_LIST_POD(m_block_ids)
    WRITE_FIELD(start_height)
    WRITE_FIELD_LIST_POD(status)
    WRITE_FIELD_LIST_POD(top_hash)
    WRITE_FIELD(untrusted)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request> {
  using T = cryptonote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request;
  using tags = map_tag<
    field_tag<KEY("client"), span_tag<>>,
    field_tag<KEY("txid"), span_tag<>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(client)
    READ_FIELD_POD(txid)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(client)
    WRITE_FIELD_POD(txid)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response> {
  using T = cryptonote::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response;
  using tags = map_tag<
    field_tag<KEY("credits"), base_tag<std::uint64_t>>,
    field_tag<KEY("o_indexes"), list_tag<base_tag<std::uint64_t>>>,
    field_tag<KEY("status"), scheme<container_pod_tag<decltype(T::status)>>::tags>,
    field_tag<KEY("top_hash"), span_tag<>>,
    field_tag<KEY("untrusted"), base_tag<bool>>
  >;
  BEGIN_READ(T)
    READ_FIELD(credits)
    READ_FIELD_LIST(o_indexes)
    READ_FIELD_LIST_POD(status)
    READ_FIELD_LIST_POD(top_hash)
    READ_FIELD(untrusted)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(credits)
    WRITE_FIELD_LIST(o_indexes)
    WRITE_FIELD_LIST_POD(status)
    WRITE_FIELD_LIST_POD(top_hash)
    WRITE_FIELD(untrusted)
  END_WRITE()
};

template<>
struct scheme<cryptonote::get_outputs_out> {
  using T = cryptonote::get_outputs_out;
  using tags = map_tag<
    field_tag<KEY("amount"), base_tag<std::uint64_t>, optional_t::required>,
    field_tag<KEY("index"), base_tag<std::uint64_t>, optional_t::required>
  >;
  BEGIN_READ(T)
    READ_FIELD(amount)
    READ_FIELD(index)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(amount)
    WRITE_FIELD(index)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::request> {
  using T = cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::request;
  using tags = map_tag<
    field_tag<KEY("client"), span_tag<>>,
    field_tag<KEY("get_txid"), base_tag<bool>>,
    field_tag<KEY("outputs"), scheme<container_tag<decltype(T::outputs)>>::tags>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST_POD(client)
    READ_FIELD_WITH_DEFAULT(get_txid, true)
    READ_FIELD_LIST(outputs)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST_POD(client)
    WRITE_FIELD_WITH_DEFAULT(get_txid, true)
    WRITE_FIELD_LIST(outputs)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::outkey> {
  using T = cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::outkey;
  using tags = map_tag<
    field_tag<KEY("height"), base_tag<std::uint64_t>, optional_t::required>,
    field_tag<KEY("key"), scheme<pod_tag<decltype(T::key)>>::tags, optional_t::required>,
    field_tag<KEY("mask"), scheme<pod_tag<decltype(T::mask)>>::tags, optional_t::required>,
    field_tag<KEY("txid"), scheme<pod_tag<decltype(T::txid)>>::tags, optional_t::required>,
    field_tag<KEY("unlocked"), base_tag<bool>, optional_t::required>
  >;
  BEGIN_READ(T)
    READ_FIELD(height)
    READ_FIELD_POD(key)
    READ_FIELD_POD(mask)
    READ_FIELD_POD(txid)
    READ_FIELD(unlocked)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(height)
    WRITE_FIELD_POD(key)
    WRITE_FIELD_POD(mask)
    WRITE_FIELD_POD(txid)
    WRITE_FIELD(unlocked)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::response> {
  using T = cryptonote::COMMAND_RPC_GET_OUTPUTS_BIN::response;
  using tags = map_tag<
    field_tag<KEY("credits"), base_tag<std::uint64_t>>,
    field_tag<KEY("outs"), scheme<container_tag<decltype(T::outs)>>::tags>,
    field_tag<KEY("status"), scheme<container_pod_tag<decltype(T::status)>>::tags>,
    field_tag<KEY("top_hash"), span_tag<>>,
    field_tag<KEY("untrusted"), base_tag<bool>>
  >;
  BEGIN_READ(T)
    READ_FIELD(credits)
    READ_FIELD_LIST(outs)
    READ_FIELD_LIST_POD(status)
    READ_FIELD_LIST_POD(top_hash)
    READ_FIELD(untrusted)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(credits)
    WRITE_FIELD_LIST(outs)
    WRITE_FIELD_LIST_POD(status)
    WRITE_FIELD_LIST_POD(top_hash)
    WRITE_FIELD(untrusted)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request> {
  using T = cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::request;
  using tags = map_tag<
    field_tag<KEY("amounts"), list_tag<base_tag<std::uint64_t>>>,
    field_tag<KEY("binary"), base_tag<bool>>,
    field_tag<KEY("client"), span_tag<>>,
    field_tag<KEY("compress"), base_tag<bool>>,
    field_tag<KEY("cumulative"), base_tag<bool>>,
    field_tag<KEY("from_height"), base_tag<std::uint64_t>>,
    field_tag<KEY("to_height"), base_tag<std::uint64_t>>
  >;
  BEGIN_READ(T)
    READ_FIELD_LIST(amounts)
    READ_FIELD_WITH_DEFAULT(binary, true)
    READ_FIELD_LIST_POD(client)
    READ_FIELD_WITH_DEFAULT(compress, false)
    READ_FIELD_WITH_DEFAULT(cumulative, false)
    READ_FIELD_WITH_DEFAULT(from_height, 0)
    READ_FIELD_WITH_DEFAULT(to_height, 0)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_LIST(amounts)
    WRITE_FIELD_WITH_DEFAULT(binary, true)
    WRITE_FIELD_LIST_POD(client)
    WRITE_FIELD_WITH_DEFAULT(compress, false)
    WRITE_FIELD_WITH_DEFAULT(cumulative, false)
    WRITE_FIELD_WITH_DEFAULT(from_height, 0)
    WRITE_FIELD_WITH_DEFAULT(to_height, 0)
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::distribution> {
  using T = cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::distribution;
  using tags = map_tag<
    field_tag<KEY("amount"), base_tag<std::uint64_t>, optional_t::required>,
    field_tag<KEY("base"), base_tag<std::uint64_t>, optional_t::required>,
    field_tag<KEY("binary"), base_tag<bool>, optional_t::required>,
    field_tag<KEY("compress"), base_tag<bool>, optional_t::required>,
    field_tag<KEY("compressed_data"), span_tag<>>,
    field_tag<KEY("distribution"), list_tag<base_tag<std::uint64_t>>>,
    #if 1
    field_tag<KEY("distribution"), span_tag<>>,
    #endif
    field_tag<KEY("start_height"), base_tag<std::uint64_t>, optional_t::required>
  >;
  BEGIN_READ_RAW(
    private:
      const T &t;
      const std::string compressed_data;
    public:
      read_t(const T &t): t(t), compressed_data(t.compress and t.binary ? compress_integer_array(t.data.distribution) : std::string{}) {}
  )
    READ_FIELD(amount)
    READ_FIELD(binary)
    READ_FIELD(compress)
    READ_FIELD_NAME("base", t.data.base)
    READ_FIELD_NAME("start_height", t.data.start_height)
    #if 1
    READ_FIELD_LIST_OPT_NAME("distribution", t.data.distribution, {return not t.binary;})
    READ_FIELD_LIST_POD_OPT_NAME_FORCE("distribution", t.data.distribution, {return t.binary and not t.compress;})
    READ_FIELD_LIST_POD_OPT_NAME("compressed_data", compressed_data, {return t.binary and t.compress;})
    #else
    READ_FIELD_LIST_OPT_NAME("distribution", t.data.distribution, {return not t.binary and not t.compress;})
    READ_FIELD_LIST_POD_OPT_NAME("compressed_data", compressed_data, {return t.binary and t.compress;})
    #endif
  END_READ()

  BEGIN_WRITE_RAW(
    private:
      T &t;
      std::string compressed_data{};
    public:
      write_t(T &t): t(t) { t.data.distribution.clear(); }
  )
    WRITE_FIELD(amount)
    WRITE_FIELD(binary)
    WRITE_FIELD(compress)
    WRITE_FIELD_NAME("base", t.data.base)
    WRITE_FIELD_NAME("start_height", t.data.start_height)
    #if 1
    WRITE_FIELD_LIST_OPT_NAME("distribution", t.data.distribution)
    WRITE_FIELD_LIST_POD_OPT_NAME_FORCE("distribution", t.data.distribution)
    WRITE_FIELD_LIST_POD_OPT_NAME("compressed_data", compressed_data)
    #else
    WRITE_FIELD_LIST_OPT_NAME("distribution", t.data.distribution)
    WRITE_FIELD_LIST_POD_OPT_NAME("compressed_data", compressed_data)
    #endif
    WRITE_CHECK({
      if (t.binary and t.compress)
        t.data.distribution = decompress_integer_array<std::uint64_t>(compressed_data);
      return true;
    })
  END_WRITE()
};

template<>
struct scheme<cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response> {
  using T = cryptonote::COMMAND_RPC_GET_OUTPUT_DISTRIBUTION::response;
  using distributions_t = scheme<container_tag<decltype(T::distributions), scheme<decltype(T::distributions)::value_type>, 0, std::size_t(-1), 0, 2>>;
  using tags = map_tag<
    field_tag<KEY("credits"), base_tag<std::uint64_t>>,
    field_tag<KEY("distributions"), distributions_t::tags>,
    field_tag<KEY("status"), scheme<container_pod_tag<decltype(T::status)>>::tags>,
    field_tag<KEY("top_hash"), span_tag<>>,
    field_tag<KEY("untrusted"), base_tag<bool>>
  >;
  BEGIN_READ(T)
    READ_FIELD(credits)
    READ_FIELD_LIST_CUSTOM(distributions, distributions_t)
    READ_FIELD_LIST_POD(status)
    READ_FIELD_LIST_POD(top_hash)
    READ_FIELD(untrusted)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(credits)
    WRITE_FIELD_LIST_CUSTOM(distributions, distributions_t)
    WRITE_FIELD_LIST_POD(status)
    WRITE_FIELD_LIST_POD(top_hash)
    WRITE_FIELD(untrusted)
  END_WRITE()
};

}

}
