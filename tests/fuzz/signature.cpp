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

#include "include_base_utils.h"
#include "file_io_utils.h"
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "wallet/wallet2.h"
#include "fuzzer.h"

static tools::wallet2 *wallet = NULL;
static cryptonote::account_public_address address;

BEGIN_INIT_SIMPLE_FUZZER()
  static tools::wallet2 local_wallet(cryptonote::TESTNET);
  wallet = &local_wallet;

  static const char * const spendkey_hex = "0b4f47697ec99c3de6579304e5f25c68b07afbe55b71d99620bf6cbf4e45a80f";
  crypto::secret_key spendkey;
  epee::string_tools::hex_to_pod(spendkey_hex, spendkey);

  wallet->init("", boost::none, "", 0, true, epee::net_utils::ssl_support_t::e_ssl_support_disabled);
  wallet->set_subaddress_lookahead(1, 1);
  wallet->generate("", "", spendkey, true, false);

  cryptonote::address_parse_info info;
  if (!cryptonote::get_account_address_from_str_or_url(info, cryptonote::TESTNET, "9uVsvEryzpN8WH2t1WWhFFCG5tS8cBNdmJYNRuckLENFimfauV5pZKeS1P2CbxGkSDTUPHXWwiYE5ZGSXDAGbaZgDxobqDN"))
  {
    std::cerr << "failed to parse address" << std::endl;
    return 1;
  }
  address = info.address;
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  tools::wallet2::message_signature_result_t result = wallet->verify("test", address, std::string((const char*)buf, len));
  std::cout << "Signature " << (result.valid ? "valid" : "invalid") << std::endl;
END_SIMPLE_FUZZER()
