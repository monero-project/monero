#pragma once

#include "service_node_rules.h"

#include <map>
#include <vector>
#include <random>

namespace service_nodes {
    using swarm_snode_map_t = std::map<swarm_id_t, std::vector<crypto::public_key>>;
    struct swarm_size {
        swarm_id_t swarm_id;
        size_t size;
    };
    struct excess_pool_snode {
        crypto::public_key public_key;
        swarm_id_t swarm_id;
    };

    void calc_swarm_changes(swarm_snode_map_t& swarm_to_snodes, uint64_t seed);

#ifdef UNIT_TEST
    size_t calc_excess(const swarm_snode_map_t &swarm_to_snodes);
    size_t calc_threshold(const swarm_snode_map_t &swarm_to_snodes);
    crypto::public_key steal_from_excess_pool(swarm_snode_map_t &swarm_to_snodes, std::mt19937_64 &mt);
    void create_new_swarm_from_excess(swarm_snode_map_t &swarm_to_snodes, std::mt19937_64 &mt);
    void calc_swarm_sizes(const swarm_snode_map_t &swarm_to_snodes, std::vector<swarm_size> &sorted_swarm_sizes);
    void assign_snodes(const std::vector<crypto::public_key> &snode_pubkeys, swarm_snode_map_t &swarm_to_snodes, std::mt19937_64 &mt, size_t percentile);
    void get_excess_pool(size_t threshold, const swarm_snode_map_t& swarm_to_snodes, std::vector<excess_pool_snode>& pool_snodes, size_t& excess);
    const excess_pool_snode& pick_from_excess_pool(const std::vector<excess_pool_snode>& excess_pool, std::mt19937_64 &mt);
    void remove_excess_snode_from_swarm(const excess_pool_snode& excess_snode, swarm_snode_map_t &swarm_to_snodes);
#endif
}
