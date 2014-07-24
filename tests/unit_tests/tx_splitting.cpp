#include "gtest/gtest.h"

#include "wallet/wallet2.h"


uint64_t sumForDest(const std::vector<std::vector<cryptonote::tx_destination_entry>>& pSplittedDestinations, size_t pDestIndex) {
  uint64_t lSum = 0;
  for(const std::vector<cryptonote::tx_destination_entry>& lDestinationsPart : pSplittedDestinations) {
    lSum += lDestinationsPart.at(pDestIndex).amount;
  }

  return lSum;
}

TEST(tx_splitting_case,tx_splitting)
{

  /* For readability, unit tests are not run using loop functions, except for sum helpers */
  
  const std::string& lAddrStr0 = "47AaoeKcfBVSg9EM8hELDt5Fg93s4oBfsg1nYijmsvf1JifoSCDQvEkW6meF8mV63bCixZXrkP5yZ22o9UcfiTeR1TkSEDz";
  const std::string& lAddrStr1 = "48YCiaYuvKSABq4Fj37iMmDeKWcnqCMd2PuoJVMUYCRnFcUFq1QKXjqJ8dg3PCvroJLtY86besAxaGVkzAArvHTiUAUcDHG";
  const std::string& lAddrStr2 = "46EU1986tjJWXGyjCHGNvKN2qCH1nw1wxBLuq2oPpjnvNu7N9WuuaEX49S6oGateDDgnRA5hmRA8R2msWPnUrcnDNusLFKf";
  const std::string& lAddrStr3 = "46ctfLBhgyzJm61oTtDfu9GbVhG7ito4fNiQjdZcB5bL3mz5ejQrPD29uEkDHFzCVTHGFqAdG456w6ivYp7K23SiGREUiQe";
  const std::string& lAddrStr4 = "48eMBLKczDKHJUiUNhYaSQSMxb7HXnTwFAjkv2TiXTHfi2KMDYpxHQRGPYKCuhCXsRXYCKK1bcj4w5eLeQZ2tbUXDtzQofn";
  const std::string& lAddrStr5 = "43ZE5HLaiXjNBikSsPKBFZ2ev3euK56L2Woywfrx3Wfeg3eWsFyDg8EfbwmDznrEYyJJWeJncRputU1Ufxv6Pbz6JtkumKs";
  const std::string& lAddrStr6 = "45udkKf9WqBL5EDrwM9JsJ4DKGjBp1AYeTy2ksDTE6U29fYfiH63o689XFtNBpmWxTUY7EudSuaeW9hPFUC1xLwmUmdpEHz";

  cryptonote::account_public_address lAddr0;
  cryptonote::account_public_address lAddr1;
  cryptonote::account_public_address lAddr2;
  cryptonote::account_public_address lAddr3;
  cryptonote::account_public_address lAddr4;
  cryptonote::account_public_address lAddr5;
  cryptonote::account_public_address lAddr6;

  ASSERT_TRUE(get_account_address_from_str(lAddr0, lAddrStr0));
  ASSERT_TRUE(get_account_address_from_str(lAddr1, lAddrStr1));
  ASSERT_TRUE(get_account_address_from_str(lAddr2, lAddrStr2));
  ASSERT_TRUE(get_account_address_from_str(lAddr3, lAddrStr3));
  ASSERT_TRUE(get_account_address_from_str(lAddr4, lAddrStr4));
  ASSERT_TRUE(get_account_address_from_str(lAddr5, lAddrStr5));
  ASSERT_TRUE(get_account_address_from_str(lAddr6, lAddrStr6));

  tools::wallet2 lWallet2;
  cryptonote::tx_destination_entry lDest0(3,lAddr0);
  cryptonote::tx_destination_entry lDest1(5,lAddr1);
  cryptonote::tx_destination_entry lDest2(7,lAddr2);
  cryptonote::tx_destination_entry lDest3(11,lAddr3);
  cryptonote::tx_destination_entry lDest4(51,lAddr4);
  cryptonote::tx_destination_entry lDest5(101,lAddr5);
  cryptonote::tx_destination_entry lDest6(500000000000000,lAddr6); /* 500 XMR */

  std::vector<cryptonote::tx_destination_entry> lDests;
  lDests.push_back(lDest0);
  lDests.push_back(lDest1);
  lDests.push_back(lDest2);
  lDests.push_back(lDest3);
  lDests.push_back(lDest4);
  lDests.push_back(lDest5);
  lDests.push_back(lDest6);


  {
    const std::vector<std::vector<cryptonote::tx_destination_entry>> lSplittedDestinations = tools::wallet2::split_amounts(lDests, 0);


    ASSERT_EQ(1, lSplittedDestinations.size()) << "Splitted transactions have at least one part";

    ASSERT_EQ(7, lSplittedDestinations.at(0).size()) << "Splitted destinations doesn't match with original destinations count";

    /* Tx splitting of 1 didn't altered destinations */
    ASSERT_EQ(3, lSplittedDestinations.at(0).at(0).amount); /* Dest 1 of part 1 */
    ASSERT_EQ(5, lSplittedDestinations.at(0).at(1).amount); /* Dest 2 of part 1 */
    ASSERT_EQ(7, lSplittedDestinations.at(0).at(2).amount); /* etc... */
    ASSERT_EQ(11, lSplittedDestinations.at(0).at(3).amount);
    ASSERT_EQ(51, lSplittedDestinations.at(0).at(4).amount);
    ASSERT_EQ(101, lSplittedDestinations.at(0).at(5).amount);
    ASSERT_EQ(500000000000000, lSplittedDestinations.at(0).at(6).amount);

  }

  {
    const std::vector<std::vector<cryptonote::tx_destination_entry>> lSplittedDestinations = tools::wallet2::split_amounts(lDests, 1);


    ASSERT_EQ(1, lSplittedDestinations.size()) << "Splitted transactions parts size doesn't match asked split count";

    ASSERT_EQ(7, lSplittedDestinations.at(0).size()) << "Splitted destinations doesn't match with original destinations count";

    /* Tx splitting of 1 didn't altered destinations */
    ASSERT_EQ(3, lSplittedDestinations.at(0).at(0).amount);
    ASSERT_EQ(5, lSplittedDestinations.at(0).at(1).amount);
    ASSERT_EQ(7, lSplittedDestinations.at(0).at(2).amount);
    ASSERT_EQ(11, lSplittedDestinations.at(0).at(3).amount);
    ASSERT_EQ(51, lSplittedDestinations.at(0).at(4).amount);
    ASSERT_EQ(101, lSplittedDestinations.at(0).at(5).amount);
    ASSERT_EQ(500000000000000, lSplittedDestinations.at(0).at(6).amount);
  }

  {
    const std::vector<std::vector<cryptonote::tx_destination_entry>> lSplittedDestinations = tools::wallet2::split_amounts(lDests, 2);


    ASSERT_EQ(2, lSplittedDestinations.size()) << "Splitted transactions parts size doesn't match asked split count";

    ASSERT_EQ(7, lSplittedDestinations.at(0).size()) << "Splitted destinations doesn't match with original destinations count";
    ASSERT_EQ(7, lSplittedDestinations.at(1).size()) << "Splitted destinations doesn't match with original destinations count";

    /* We are sending whole XMR for each dest */
    ASSERT_EQ(3, sumForDest(lSplittedDestinations, 0));
    ASSERT_EQ(5, sumForDest(lSplittedDestinations, 1));
    ASSERT_EQ(7, sumForDest(lSplittedDestinations, 2));
    ASSERT_EQ(11, sumForDest(lSplittedDestinations, 3));
    ASSERT_EQ(51, sumForDest(lSplittedDestinations, 4));
    ASSERT_EQ(101, sumForDest(lSplittedDestinations, 5));
    ASSERT_EQ(500000000000000, sumForDest(lSplittedDestinations, 6));


    /* Expecting following repartition (original tx split algorithm) */
    EXPECT_EQ(1, lSplittedDestinations.at(0).at(0).amount);
    EXPECT_EQ(2, lSplittedDestinations.at(0).at(1).amount);
    EXPECT_EQ(3, lSplittedDestinations.at(0).at(2).amount);
    EXPECT_EQ(5, lSplittedDestinations.at(0).at(3).amount);
    EXPECT_EQ(25, lSplittedDestinations.at(0).at(4).amount);
    EXPECT_EQ(50, lSplittedDestinations.at(0).at(5).amount);
    EXPECT_EQ(250000000000000, lSplittedDestinations.at(0).at(6).amount);

    EXPECT_EQ(2, lSplittedDestinations.at(1).at(0).amount);
    EXPECT_EQ(3, lSplittedDestinations.at(1).at(1).amount);
    EXPECT_EQ(4, lSplittedDestinations.at(1).at(2).amount);
    EXPECT_EQ(6, lSplittedDestinations.at(1).at(3).amount);
    EXPECT_EQ(26, lSplittedDestinations.at(1).at(4).amount);
    EXPECT_EQ(51, lSplittedDestinations.at(1).at(5).amount);
    EXPECT_EQ(250000000000000, lSplittedDestinations.at(1).at(6).amount);

  }

  {
    const std::vector<std::vector<cryptonote::tx_destination_entry>> lSplittedDestinations = tools::wallet2::split_amounts(lDests, 3);


    ASSERT_EQ(3, lSplittedDestinations.size()) << "Splitted transactions parts size doesn't match asked split count";

    ASSERT_EQ(7, lSplittedDestinations.at(0).size()) << "Splitted destinations doesn't match with original destinations count";
    ASSERT_EQ(7, lSplittedDestinations.at(1).size()) << "Splitted destinations doesn't match with original destinations count";
    ASSERT_EQ(7, lSplittedDestinations.at(2).size()) << "Splitted destinations doesn't match with original destinations count";

    /* We are sending whole XMR for each dest */
    ASSERT_EQ(3, sumForDest(lSplittedDestinations, 0));
    ASSERT_EQ(5, sumForDest(lSplittedDestinations, 1));
    ASSERT_EQ(7, sumForDest(lSplittedDestinations, 2));
    ASSERT_EQ(11, sumForDest(lSplittedDestinations, 3));
    ASSERT_EQ(51, sumForDest(lSplittedDestinations, 4));
    ASSERT_EQ(101, sumForDest(lSplittedDestinations, 5));
    ASSERT_EQ(500000000000000, sumForDest(lSplittedDestinations, 6));


    /* Expecting following repartition (original tx split algorithm) */
    EXPECT_EQ(1, lSplittedDestinations.at(0).at(0).amount);
    EXPECT_EQ(1, lSplittedDestinations.at(0).at(1).amount);
    EXPECT_EQ(2, lSplittedDestinations.at(0).at(2).amount);
    EXPECT_EQ(3, lSplittedDestinations.at(0).at(3).amount);
    EXPECT_EQ(17, lSplittedDestinations.at(0).at(4).amount);
    EXPECT_EQ(33, lSplittedDestinations.at(0).at(5).amount);
    EXPECT_EQ(166666666666666, lSplittedDestinations.at(0).at(6).amount);

    EXPECT_EQ(1, lSplittedDestinations.at(1).at(0).amount);
    EXPECT_EQ(1, lSplittedDestinations.at(1).at(1).amount);
    EXPECT_EQ(2, lSplittedDestinations.at(1).at(2).amount);
    EXPECT_EQ(3, lSplittedDestinations.at(1).at(3).amount);
    EXPECT_EQ(17, lSplittedDestinations.at(1).at(4).amount);
    EXPECT_EQ(33, lSplittedDestinations.at(1).at(5).amount);
    EXPECT_EQ(166666666666666, lSplittedDestinations.at(1).at(6).amount);

    EXPECT_EQ(1, lSplittedDestinations.at(2).at(0).amount);
    EXPECT_EQ(3, lSplittedDestinations.at(2).at(1).amount);
    EXPECT_EQ(3, lSplittedDestinations.at(2).at(2).amount);
    EXPECT_EQ(5, lSplittedDestinations.at(2).at(3).amount);
    EXPECT_EQ(17, lSplittedDestinations.at(2).at(4).amount);
    EXPECT_EQ(35, lSplittedDestinations.at(2).at(5).amount);
    EXPECT_EQ(166666666666668, lSplittedDestinations.at(2).at(6).amount);
  }

  {
    const std::vector<std::vector<cryptonote::tx_destination_entry>> lSplittedDestinations = tools::wallet2::split_amounts(lDests, 4);


    ASSERT_EQ(4, lSplittedDestinations.size()) << "Splitted transactions parts size doesn't match asked split count";

    ASSERT_EQ(7, lSplittedDestinations.at(0).size()) << "Splitted destinations doesn't match with original destinations count";
    ASSERT_EQ(7, lSplittedDestinations.at(1).size()) << "Splitted destinations doesn't match with original destinations count";
    ASSERT_EQ(7, lSplittedDestinations.at(2).size()) << "Splitted destinations doesn't match with original destinations count";
    ASSERT_EQ(7, lSplittedDestinations.at(3).size()) << "Splitted destinations doesn't match with original destinations count";

    /* We are sending whole XMR for each dest */
    ASSERT_EQ(3, sumForDest(lSplittedDestinations, 0));
    ASSERT_EQ(5, sumForDest(lSplittedDestinations, 1));
    ASSERT_EQ(7, sumForDest(lSplittedDestinations, 2));
    ASSERT_EQ(11, sumForDest(lSplittedDestinations, 3));
    ASSERT_EQ(51, sumForDest(lSplittedDestinations, 4));
    ASSERT_EQ(101, sumForDest(lSplittedDestinations, 5));
    ASSERT_EQ(500000000000000, sumForDest(lSplittedDestinations, 6));


    /* Expecting following repartition (original tx split algorithm) */

    /* 
      Note that we have a destination amount of 0 (zero) when amount<split_count. 
      This is not critical since such small amount (3.10^-12 XMR) are almost useless.
      But zero-amount destinations could be either dropped or checked later, avoiding useless fees.
    */
    EXPECT_EQ(0, lSplittedDestinations.at(0).at(0).amount);
    EXPECT_EQ(1, lSplittedDestinations.at(0).at(1).amount);
    EXPECT_EQ(1, lSplittedDestinations.at(0).at(2).amount);
    EXPECT_EQ(2, lSplittedDestinations.at(0).at(3).amount);
    EXPECT_EQ(12, lSplittedDestinations.at(0).at(4).amount);
    EXPECT_EQ(25, lSplittedDestinations.at(0).at(5).amount);
    EXPECT_EQ(125000000000000, lSplittedDestinations.at(0).at(6).amount);

    EXPECT_EQ(0, lSplittedDestinations.at(1).at(0).amount);
    EXPECT_EQ(1, lSplittedDestinations.at(1).at(1).amount);
    EXPECT_EQ(1, lSplittedDestinations.at(1).at(2).amount);
    EXPECT_EQ(2, lSplittedDestinations.at(1).at(3).amount);
    EXPECT_EQ(12, lSplittedDestinations.at(1).at(4).amount);
    EXPECT_EQ(25, lSplittedDestinations.at(1).at(5).amount);
    EXPECT_EQ(125000000000000, lSplittedDestinations.at(1).at(6).amount);

    EXPECT_EQ(0, lSplittedDestinations.at(2).at(0).amount);
    EXPECT_EQ(1, lSplittedDestinations.at(2).at(1).amount);
    EXPECT_EQ(1, lSplittedDestinations.at(2).at(2).amount);
    EXPECT_EQ(2, lSplittedDestinations.at(2).at(3).amount);
    EXPECT_EQ(12, lSplittedDestinations.at(2).at(4).amount);
    EXPECT_EQ(25, lSplittedDestinations.at(2).at(5).amount);
    EXPECT_EQ(125000000000000, lSplittedDestinations.at(2).at(6).amount);

    EXPECT_EQ(3, lSplittedDestinations.at(3).at(0).amount);
    EXPECT_EQ(2, lSplittedDestinations.at(3).at(1).amount);
    EXPECT_EQ(4, lSplittedDestinations.at(3).at(2).amount);
    EXPECT_EQ(5, lSplittedDestinations.at(3).at(3).amount);
    EXPECT_EQ(15, lSplittedDestinations.at(3).at(4).amount);
    EXPECT_EQ(26, lSplittedDestinations.at(3).at(5).amount);
    EXPECT_EQ(125000000000000, lSplittedDestinations.at(3).at(6).amount);
  }

  
}
