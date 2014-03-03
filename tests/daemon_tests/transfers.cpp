// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include <sstream>
#include "wallet/wallet.h"
#include "rpc/core_rpc_server.h"
#include "cryptonote_core/account.h"
#include "net/http_client_abstract_invoke.h"
using namespace std;
using namespace epee::misc_utils;
using namespace cryptonote;

string daemon_address = "http://localhost:23400";

#define ACCS 5

TEST(Transfers, Transfers)
{
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_3);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);

  cout << "TESTING: transfers" << endl;

  net_utils::http::http_simple_client http_client;
  wallet miner, accs[100], receiver;
  miner.generate();
  ASSERT_TRUE(miner.init());
  ASSERT_TRUE(miner.store("miner.b2wallet"));
  cout << "miner: " << miner.get_account().get_public_address_str() << endl;

  for (int i = 0; i < ACCS; i++) {
    ostringstream s;
    s << "acc" << setw(2) << setfill('0') << i << ".b2wallet";
    accs[i].generate();
    assert(accs[i].init());
    assert(accs[i].store(s.str()));
  }
  receiver.generate();
  assert(receiver.init());
  receiver.store("receiver.b2wallet");

  {
    COMMAND_RPC_START_MINE::request req;
    req.miner_address = miner.get_account().get_public_address_str();
    req.threads_count = 1;
    COMMAND_RPC_START_MINE::response res;
    bool r = net_utils::http::invoke_http_json_remote_command(daemon_address + "/start_mine", req, res, http_client);
    ASSERT_TRUE(r);
  }

  string s;
  //getline(cin, s);
  sleep_no_w(1000);
  ASSERT_TRUE(miner.refresh());
  cout << "miner balance: " << miner.balance() << endl;

  vector<pair<account_public_address, uint64_t>> d_accs;
  for (int i = 0; i < ACCS; i++)
    d_accs.push_back(make_pair(accs[i].get_account().get_keys().m_account_address, 1));
  ASSERT_TRUE(miner.transfer(d_accs));

  //getline(cin, s);
  sleep_no_w(1000);
  for (int i = 0; i < ACCS; i++) {
    ASSERT_TRUE(accs[i].refresh());
    ASSERT_TRUE(accs[i].transfer(receiver.get_account().get_keys().m_account_address, 1));
  }

  //getline(cin, s);
  cout << "wait for block" << endl;
  sleep_no_w(10000);
  receiver.refresh();
  ASSERT_TRUE(receiver.balance() == ACCS);
  cout << "OK" << endl;
}
