// Copyright (c) 2020-2024, The Monero Project

// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//  conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//  of conditions and the following disclaimer in the documentation and/or other
//  materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//  used to endorse or promote products derived from this software without specific
//  prior written permission.
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

#include <gtest/gtest.h>

#include "rpc/bootstrap_node_selector.h"

class bootstrap_node_selector : public ::testing::Test
{
protected:
  void SetUp() override
  {
    nodes.insert(white_nodes.begin(), white_nodes.end());
    nodes.insert(gray_nodes.begin(), gray_nodes.end());
  }

  const std::map<std::string, bool> white_nodes = {
    {
      "white_node_1:18089", true
    },
    {
      "white_node_2:18081", true
    }
  };
  const std::map<std::string, bool> gray_nodes = {
    {
      "gray_node_1:18081", false
    },
    {
      "gray_node_2:18089", false
    }
  };

  std::map<std::string, bool> nodes;
};

TEST_F(bootstrap_node_selector, selector_auto_empty)
{
  cryptonote::bootstrap_node::selector_auto selector([]() {
    return std::map<std::string, bool>();
  });

  EXPECT_FALSE(selector.next_node());
}

TEST_F(bootstrap_node_selector, selector_auto_no_credentials)
{
  cryptonote::bootstrap_node::selector_auto selector([this]() {
    return nodes;
  });

  for (size_t fails = 0; fails < nodes.size(); ++fails)
  {
    const auto current = selector.next_node();
    EXPECT_FALSE(current->credentials);

    selector.handle_result(current->address, false);
  }
}

TEST_F(bootstrap_node_selector, selector_auto_success)
{
  cryptonote::bootstrap_node::selector_auto selector([this]() {
    return nodes;
  });

  auto current = selector.next_node();
  for (size_t fails = 0; fails < nodes.size(); ++fails)
  {
    selector.handle_result(current->address, true);

    current = selector.next_node();
    EXPECT_TRUE(white_nodes.count(current->address) > 0);
  }
}

TEST_F(bootstrap_node_selector, selector_auto_failure)
{
  cryptonote::bootstrap_node::selector_auto selector([this]() {
    return nodes;
  });

  auto current = selector.next_node();
  for (size_t fails = 0; fails < nodes.size(); ++fails)
  {
    const auto previous = current;

    selector.handle_result(current->address, false);

    current = selector.next_node();
    EXPECT_NE(current->address, previous->address);
  }
}

TEST_F(bootstrap_node_selector, selector_auto_white_nodes_first)
{
  cryptonote::bootstrap_node::selector_auto selector([this]() {
    return nodes;
  });

  for (size_t iterations = 0; iterations < 2; ++iterations)
  {
    for (size_t fails = 0; fails < white_nodes.size(); ++fails)
    {
      const auto current = selector.next_node();
      EXPECT_TRUE(white_nodes.count(current->address) > 0);

      selector.handle_result(current->address, false);
    }

    for (size_t fails = 0; fails < gray_nodes.size(); ++fails)
    {
      const auto current = selector.next_node();
      EXPECT_TRUE(gray_nodes.count(current->address) > 0);

      selector.handle_result(current->address, false);
    }
  }
}

TEST_F(bootstrap_node_selector, selector_auto_max_nodes)
{
  const size_t max_nodes = nodes.size() / 2;

  bool populated_once = false;
  cryptonote::bootstrap_node::selector_auto selector([this, &populated_once]() {
    if (!populated_once)
    {
      populated_once = true;
      return nodes;
    }

    return std::map<std::string, bool>();
  }, max_nodes);

  std::set<std::string> unique_nodes;

  for (size_t fails = 0; fails < nodes.size(); ++fails)
  {
    const auto current = selector.next_node();
    unique_nodes.insert(current->address);

    selector.handle_result(current->address, false);
  }

  EXPECT_EQ(unique_nodes.size(), max_nodes);
}
