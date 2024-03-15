// Copyright (c) 2017-2024, The Monero Project
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

#include "subaddress.h"
#include "wallet.h"
#include "crypto/hash.h"
#include "wallet/wallet2.h"
#include "common_defines.h"

#include <vector>

namespace Monero {
  
Subaddress::~Subaddress() {}
  
SubaddressImpl::SubaddressImpl(WalletImpl *wallet)
    : m_wallet(wallet) {}

void SubaddressImpl::addRow(uint32_t accountIndex, const std::string &label)
{
  m_wallet->m_wallet->add_subaddress(accountIndex, label);
  refresh(accountIndex);
}

void SubaddressImpl::setLabel(uint32_t accountIndex, uint32_t addressIndex, const std::string &label)
{
  try
  {
    m_wallet->m_wallet->set_subaddress_label({accountIndex, addressIndex}, label);
    refresh(accountIndex);
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("setLabel: " << e.what());
  }
}

void SubaddressImpl::refresh(uint32_t accountIndex) 
{
  LOG_PRINT_L2("Refreshing subaddress");
  
  clearRows();
  for (size_t i = 0; i < m_wallet->m_wallet->get_num_subaddresses(accountIndex); ++i)
  {
    m_rows.push_back(new SubaddressRow(i, m_wallet->m_wallet->get_subaddress_as_str({accountIndex, (uint32_t)i}), m_wallet->m_wallet->get_subaddress_label({accountIndex, (uint32_t)i})));
  }
}

void SubaddressImpl::clearRows() {
   for (auto r : m_rows) {
     delete r;
   }
   m_rows.clear();
}

std::vector<SubaddressRow*> SubaddressImpl::getAll() const
{
  return m_rows;
}

SubaddressImpl::~SubaddressImpl()
{
  clearRows();
}

} // namespace
