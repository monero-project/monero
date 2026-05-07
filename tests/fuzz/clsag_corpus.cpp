// Copyright (c) 2017-2020, The Monero Project
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

#include <vector>
#include <boost/archive/portable_binary_oarchive.hpp>

#include "include_base_utils.h"
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctSigs.h"
#include "serialization/binary_archive.h"
#include "serialization/binary_utils.h"
#include "fuzzer.h"
#include "clsag_init.h"

using namespace crypto;
using namespace rct;

BEGIN_INIT_SIMPLE_FUZZER()
  clsag_init();
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  std::cout << "Generating corpus seeds: clsag1 clsag_cout1 clsag_deserialize1 clsag_message1" << std::endl;

  std::vector<std::string> corpus_paths = {
    std::string("clsag1"),
    std::string("clsag_cout1"),
    std::string("clsag_deserialize1"),
    std::string("clsag_message1"),
  };

  std::vector<std::string> corpus_bins(corpus_paths.size());

  serialization::dump_binary(clsag_s, corpus_bins[0]);
  serialization::dump_binary(Cout, corpus_bins[1]);
  serialization::dump_binary(clsag_s, corpus_bins[2]);
  serialization::dump_binary(message, corpus_bins[3]);

  // write CLSAG binary dumps to files
  for (std::size_t i=0; i < corpus_paths.size(); ++i) {
    epee::file_io_utils::save_string_to_file(corpus_paths[i], corpus_bins[i]);
  }
END_SIMPLE_FUZZER()
