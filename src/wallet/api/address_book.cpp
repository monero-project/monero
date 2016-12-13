// Copyright (c) 2014-2016, The Monero Project
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


#include "address_book.h"
#include "wallet.h"
#include "crypto/hash.h"
#include "wallet/wallet2.h"

#include <vector>

namespace Monero {
  
AddressBook::~AddressBook() {}
  
AddressBookImpl::AddressBookImpl(WalletImpl *wallet)
    : m_wallet(wallet) {}

bool AddressBookImpl::addRow(const std::string &dst_addr , const std::string &payment_id, const std::string &description)
{
  LOG_PRINT_L2("Adding row");
  
  clearStatus();
  
  cryptonote::account_public_address addr;
  bool has_payment_id;
  crypto::hash8 payment_id_short;
  if(!cryptonote::get_account_integrated_address_from_str(addr, has_payment_id, payment_id_short, m_wallet->m_wallet->testnet(), dst_addr)) {
    m_errorString = "Invalid destination address";
    m_errorCode = Invalid_Address;
    return false;
  }

  crypto::hash pid32 = cryptonote::null_hash;
  bool long_pid =  (payment_id.empty())? false : tools::wallet2::parse_long_payment_id(payment_id, pid32);
  if(!payment_id.empty() && !long_pid) {
    m_errorString = "Invalid payment ID";
    m_errorCode = Invalid_Payment_Id;
    return false;
  }

  bool r =  m_wallet->m_wallet->add_address_book_row(addr,pid32,description);
  if (r)
    refresh();
  else
    m_errorCode = General_Error;
  return r;
}

void AddressBookImpl::refresh() 
{
  LOG_PRINT_L2("Refreshing addressbook");
  
  clearRows();
  
  // Fetch from Wallet2 and create vector of AddressBookRow objects
  std::vector<tools::wallet2::address_book_row> rows = m_wallet->m_wallet->get_address_book();
  for (size_t i = 0; i < rows.size(); ++i) {
    tools::wallet2::address_book_row * row = &rows.at(i);
    
    std::string payment_id = (row->m_payment_id == cryptonote::null_hash)? "" : epee::string_tools::pod_to_hex(row->m_payment_id);
    std::string address = cryptonote::get_account_address_as_str(m_wallet->m_wallet->testnet(),row->m_address);

    AddressBookRow * abr = new AddressBookRow(i, address, payment_id, row->m_description);
    m_rows.push_back(abr);
  }
  
}

bool AddressBookImpl::deleteRow(int rowId) 
{
  LOG_PRINT_L2("Deleting address book row " << rowId);
  bool r = m_wallet->m_wallet->delete_address_book_row(rowId);
  if (r)
    refresh();
  return r;
} 

void AddressBookImpl::clearRows() {
   for (auto r : m_rows) {
     delete r;
   }
   m_rows.clear();
}

void AddressBookImpl::clearStatus(){
  m_errorString = "";
  m_errorCode = 0;
}

std::vector<AddressBookRow*> AddressBookImpl::getAll() const
{
  return m_rows;
}


AddressBookImpl::~AddressBookImpl()
{
  clearRows();
}

} // namespace

namespace Bitmonero = Monero;