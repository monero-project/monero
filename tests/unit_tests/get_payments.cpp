// Copyright (c) 2014-2019, The Monero Project
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

// FIXME: move this into a full wallet2 unit test suite, if possible

#include "gtest/gtest.h"

#include "wallet/wallet2.h"
#include <string>


class GetPayments : public ::testing::Test 
{
        protected:
                virtual void SetUp() 
                {
                        try
                        {
                                m_wallet.generate("", password, recovery_key, true, false);
                        }
                        catch (const std::exception& e)
                        {
                                LOG_ERROR("failed to generate wallet: " << e.what());
                                throw;
                        }

                        payment.m_block_height = 7;
                        transfer.m_block_height = 7;
                        block_height_that_has_transaction = payment.m_block_height;

                        crypto::hash payment_id;
                        actual_payments.insert( {{payment_id, payment}} ); 
                        actual_confirmed_txs.insert( {{payment_id, transfer}} ); 

                        matched_payments.clear();
                        confirmed_payments.clear();
                        ASSERT_EQ(0, matched_payments.size());
                }

                virtual void TearDown()
                {
                }


                tools::wallet2 m_wallet;
                const std::string password = "testpass";
                crypto::secret_key recovery_key = crypto::secret_key();

                uint64_t threshold = 0;
                uint64_t max_height = (uint64_t)-1;

                uint64_t block_height_that_has_transaction;

                // for use with get_payments()
                tools::wallet2::payment_details payment;
                std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> matched_payments;
                tools::wallet2::payment_container actual_payments; 

                // for use with get_payments_out()
                tools::wallet2::confirmed_transfer_details transfer;
                std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> confirmed_payments;
                std::unordered_map<crypto::hash, tools::wallet2::confirmed_transfer_details> actual_confirmed_txs;
};


TEST_F(GetPayments, NotFoundBelowThreshold)
{
        m_wallet.get_payments(matched_payments, actual_payments,
                        ++block_height_that_has_transaction, block_height_that_has_transaction);
        ASSERT_EQ(0, matched_payments.size());
}
TEST_F(GetPayments, NotFoundBelowThreshold_Out)
{
        m_wallet.get_payments_out(confirmed_payments, actual_confirmed_txs,
                        ++block_height_that_has_transaction, block_height_that_has_transaction);
        ASSERT_EQ(0, confirmed_payments.size());
}


TEST_F(GetPayments, NotFoundAboveMaxHeight)
{
        m_wallet.get_payments(matched_payments, actual_payments,
                        block_height_that_has_transaction, --block_height_that_has_transaction);
        ASSERT_EQ(0, matched_payments.size());
}
TEST_F(GetPayments, NotFoundAboveMaxHeight_Out)
{
        m_wallet.get_payments_out(confirmed_payments, actual_confirmed_txs,
                        block_height_that_has_transaction, --block_height_that_has_transaction);
        ASSERT_EQ(0, confirmed_payments.size());
}


TEST_F(GetPayments, NotFoundIfNegative)
{
        m_wallet.get_payments(matched_payments, actual_payments,
                        -1, -9999);
        ASSERT_EQ(0, matched_payments.size());
}
TEST_F(GetPayments, NotFoundIfNegative_Out)
{
        m_wallet.get_payments_out(confirmed_payments, actual_confirmed_txs,
                        -1, -9999);
        ASSERT_EQ(0, confirmed_payments.size());
}


TEST_F(GetPayments, NotFoundIfNegativeReversed)
{
        m_wallet.get_payments(matched_payments, actual_payments,
                        -9999, -1);
        ASSERT_EQ(0, matched_payments.size());
}
TEST_F(GetPayments, NotFoundIfNegativeReversed_Out)
{
        m_wallet.get_payments_out(confirmed_payments, actual_confirmed_txs,
                        -9999, -1);
        ASSERT_EQ(0, confirmed_payments.size());
}


TEST_F(GetPayments, NotFoundIfThresholdAboveMaxHeight)
{
        m_wallet.get_payments(matched_payments, actual_payments,
                        max_height, threshold);
        ASSERT_EQ(0, matched_payments.size());
}
TEST_F(GetPayments, NotFoundIfThresholdAboveMaxHeight_Out)
{
        m_wallet.get_payments_out(confirmed_payments, actual_confirmed_txs,
                        max_height, threshold);
        ASSERT_EQ(0, confirmed_payments.size());
}


TEST_F(GetPayments, IsFoundWithinZeroAndMax)
{
        m_wallet.get_payments(matched_payments, actual_payments,
                        threshold, max_height);
        ASSERT_EQ(1, matched_payments.size());
}
TEST_F(GetPayments, IsFoundWithinZeroAndMax_Out)
{
        m_wallet.get_payments_out(confirmed_payments, actual_confirmed_txs,
                        threshold, max_height);
        ASSERT_EQ(1, confirmed_payments.size());
}


TEST_F(GetPayments, NotFoundAtThreshold)
{
        m_wallet.get_payments(matched_payments, actual_payments,
                        block_height_that_has_transaction, block_height_that_has_transaction);
        ASSERT_EQ(0, matched_payments.size());
}
TEST_F(GetPayments, NotFoundAtThreshold_Out)
{
        m_wallet.get_payments_out(confirmed_payments, actual_confirmed_txs,
                        block_height_that_has_transaction, block_height_that_has_transaction);
        ASSERT_EQ(0, confirmed_payments.size());
}


TEST_F(GetPayments, IsFoundAtMaxHeight)
{
        m_wallet.get_payments(matched_payments, actual_payments,
                        threshold, block_height_that_has_transaction);
        ASSERT_EQ(1, matched_payments.size());
}
TEST_F(GetPayments, IsFoundAtMaxHeight_Out)
{
        m_wallet.get_payments_out(confirmed_payments, actual_confirmed_txs,
                        threshold, block_height_that_has_transaction);
        ASSERT_EQ(1, confirmed_payments.size());
}
