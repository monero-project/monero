// Copyright (c) 2017, The Monero Project
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
#include "cryptonote_protocol/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "wallet/wallet2.h"
#include "fuzzer.h"

class ColdOutputsFuzzer: public Fuzzer
{
public:
  ColdOutputsFuzzer(): wallet(true) {}
  virtual int init();
  virtual int run(const std::string &filename);

private:
  tools::wallet2 wallet;
};

int ColdOutputsFuzzer::init()
{
  static const char * const spendkey_hex = "0b4f47697ec99c3de6579304e5f25c68b07afbe55b71d99620bf6cbf4e45a80f";
  crypto::secret_key spendkey;
  epee::string_tools::hex_to_pod(spendkey_hex, spendkey);

  try
  {
    boost::filesystem::remove("/tmp/cold-outputs-test.keys");
    boost::filesystem::remove("/tmp/cold-outputs-test.address.txt");
    boost::filesystem::remove("/tmp/cold-outputs-test");

    wallet.init("");
    wallet.generate("/tmp/cold-outputs-test", "", spendkey, true, false);

    boost::filesystem::remove("/tmp/cold-outputs-test.keys");
    boost::filesystem::remove("/tmp/cold-outputs-test.address.txt");
    boost::filesystem::remove("/tmp/cold-outputs-test");
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error on ColdOutputsFuzzer::init: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}

int ColdOutputsFuzzer::run(const std::string &filename)
{
  std::string s;

  if (!epee::file_io_utils::load_file_to_string(filename, s))
  {
    std::cout << "Error: failed to load file " << filename << std::endl;
    return 1;
  }
  s = std::string("\x01\x16serialization::archive") + s;
  try
  {
    std::vector<tools::wallet2::transfer_details> outputs;
    std::stringstream iss;
    iss << s;
    boost::archive::portable_binary_iarchive ar(iss);
    ar >> outputs;
    size_t n_outputs = wallet.import_outputs(outputs);
    std::cout << boost::lexical_cast<std::string>(n_outputs) << " outputs imported" << std::endl;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to import outputs: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, const char **argv)
{
  ColdOutputsFuzzer fuzzer;
  return run_fuzzer(argc, argv, fuzzer);
}

