// Copyright (c) 2020-2024, The Monero Project

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

#pragma  once

#include <functional>
#include <limits>
#include <map>
#include <string>
#include <utility>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/optional/optional.hpp>

#include "net/http_client.h"

namespace cryptonote
{
namespace bootstrap_node
{

  struct node_info
  {
    std::string address;
    boost::optional<epee::net_utils::http::login> credentials;
  };

  struct selector
  {
    virtual ~selector() = default;

    virtual void handle_result(const std::string &address, bool success) = 0;
    virtual boost::optional<node_info> next_node() = 0;
  };

  class selector_auto : public selector
  {
  public:
    selector_auto(std::function<std::map<std::string, bool>()> get_nodes, size_t max_nodes = 1000)
      : m_get_nodes(std::move(get_nodes))
      , m_max_nodes(max_nodes)
    {}

    void handle_result(const std::string &address, bool success) final;
    boost::optional<node_info> next_node() final;

  private:
    bool has_at_least_one_good_node() const;
    void append_new_nodes();
    void truncate();

  private:
    struct node
    {
      std::string address;
      size_t fails;

      void handle_result(bool success);
    };

    struct by_address {};
    struct by_fails {};

    typedef boost::multi_index_container<
      node,
      boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::tag<by_address>, boost::multi_index::member<node, std::string, &node::address>>,
        boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_fails>, boost::multi_index::member<node, size_t, &node::fails>>
      >
    > nodes_list;

    const std::function<std::map<std::string, bool>()> m_get_nodes;
    const size_t m_max_nodes;
    nodes_list m_nodes;
  };

}
}
