// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <array>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

namespace cryptonote {

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
