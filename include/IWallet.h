// Copyright (c) 2014-2018, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <array>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

namespace CryptoNote {

typedef size_t TransactionId;
typedef size_t TransferId;
typedef std::array<uint8_t, 32> TransacitonHash;

struct Transfer {
  std::string address;
  int64_t amount;
};

const TransactionId INVALID_TRANSACTION_ID    = std::numeric_limits<TransactionId>::max();
const TransferId INVALID_TRANSFER_ID          = std::numeric_limits<TransferId>::max();
const uint64_t UNCONFIRMED_TRANSACTION_HEIGHT = std::numeric_limits<uint64_t>::max();

struct Transaction {
  TransferId      firstTransferId;
  size_t          transferCount;
  int64_t         totalAmount;
  uint64_t        fee;
  TransacitonHash hash;
  bool            isCoinbase;
  uint64_t        blockHeight;
  uint64_t        timestamp;
  std::string     extra;
};

class IWalletObserver {
public:
  virtual void initCompleted(std::error_code result) {}
  virtual void saveCompleted(std::error_code result) {}
  virtual void synchronizationProgressUpdated(uint64_t current, uint64_t total) {}
  virtual void actualBalanceUpdated(uint64_t actualBalance) {}
  virtual void pendingBalanceUpdated(uint64_t pendingBalance) {}
  virtual void externalTransactionCreated(TransactionId transactionId) {}
  virtual void sendTransactionCompleted(TransactionId transactionId, std::error_code result) {}
  virtual void transactionUpdated(TransactionId transactionId) {}
};

class IWallet {
public:
  virtual ~IWallet() = 0;
  virtual void addObserver(IWalletObserver* observer) = 0;
  virtual void removeObserver(IWalletObserver* observer) = 0;

  virtual void initAndGenerate(const std::string& password) = 0;
  virtual void initAndLoad(std::istream& source, const std::string& password) = 0;
  virtual void shutdown() = 0;

  virtual void save(std::ostream& destination, bool saveDetailed = true, bool saveCache = true) = 0;

  virtual std::error_code changePassword(const std::string& oldPassword, const std::string& newPassword) = 0;

  virtual std::string getAddress() = 0;

  virtual uint64_t actualBalance() = 0;
  virtual uint64_t pendingBalance() = 0;

  virtual size_t getTransactionCount() = 0;
  virtual size_t getTransferCount() = 0;

  virtual TransactionId findTransactionByTransferId(TransferId transferId) = 0;
  
  virtual bool getTransaction(TransactionId transactionId, Transaction& transaction) = 0;
  virtual bool getTransfer(TransferId transferId, Transfer& transfer) = 0;

  virtual TransactionId sendTransaction(const Transfer& transfer, uint64_t fee, const std::string& extra = "", uint64_t mixIn = 0, uint64_t unlockTimestamp = 0) = 0;
  virtual TransactionId sendTransaction(const std::vector<Transfer>& transfers, uint64_t fee, const std::string& extra = "", uint64_t mixIn = 0, uint64_t unlockTimestamp = 0) = 0;
  virtual std::error_code cancelTransaction(size_t transferId) = 0;
};

}
