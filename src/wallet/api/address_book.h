// Copyright (c) 2014-2024, The Monero Project
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

#include "wallet/api/wallet2_api.h"
#include "wallet/wallet2.h"

namespace Monero {

class WalletImpl;

class AddressBookImpl : public AddressBook
{
public:
    AddressBookImpl(WalletImpl * wallet);
    ~AddressBookImpl();
    
    // Fetches addresses from Wallet2
    void refresh() override;
    std::vector<AddressBookRow*> getAll() const override;
    bool addRow(const std::string &dst_addr , const std::string &payment_id, const std::string &description) override;
    bool setDescription(std::size_t index, const std::string &description) override;
    bool deleteRow(std::size_t rowId) override;
     
    // Error codes. See AddressBook:ErrorCode enum in wallet2_api.h
    std::string errorString() const override {return m_errorString;}
    int errorCode() const override {return m_errorCode;}

    int lookupPaymentID(const std::string &payment_id) const override;
    
private:
    void clearRows();
    void clearStatus();
    
private:
    WalletImpl *m_wallet;
    std::vector<AddressBookRow*> m_rows;
    std::string m_errorString;
    int m_errorCode;
};

}
