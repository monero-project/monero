// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <system_error>

namespace cryptonote {

class INodeObserver {
public:
  virtual void initCompleted(std::error_code result) {}

  virtual void peerCountUpdated(size_t count) {}
  virtual void lastLocalBlockHeightUpdated(uint64_t height) {}
  virtual void lastKnownBlockHeightUpdated(uint64_t height) {}

  virtual void blockchainReorganized(uint64_t height) {}
};

class INode {
public:
  virtual ~INode() = 0;
  virtual void init() = 0;
  virtual void shutdown() = 0;

  virtual void addObserver(INodeObserver* observer) = 0;
  virtual void removeObserver(INodeObserver* observer) = 0;

  virtual size_t getPeerCount() = 0;
  virtual uint64_t getLastLocalBlockHeight() = 0;
  virtual uint64_t getLastKnownBlockHeight() = 0;
};

}
