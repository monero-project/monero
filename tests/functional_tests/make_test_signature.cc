// Copyright (c) 2019, The Monero Project
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

#include <stdio.h>
#include "string_tools.h"
#include "rpc/rpc_payment_signature.h"

int main(int argc, const char **argv)
{
  TRY_ENTRY();
  if (argc > 2)
  {
    fprintf(stderr, "usage: %s <secret_key>\n", argv[0]);
    return 1;
  }

  crypto::secret_key skey;

  if (argc == 1)
  {
    crypto::public_key pkey;
    crypto::random32_unbiased((unsigned char*)skey.data);
    crypto::secret_key_to_public_key(skey, pkey);
    printf("%s %s\n", epee::string_tools::pod_to_hex(skey).c_str(), epee::string_tools::pod_to_hex(pkey).c_str());
    return 0;
  }

  if (!epee::string_tools::hex_to_pod(argv[1], skey))
  {
    fprintf(stderr, "invalid secret key\n");
    return 1;
  }
  std::string signature = cryptonote::make_rpc_payment_signature(skey);
  printf("%s\n", signature.c_str());
  return 0;
  CATCH_ENTRY_L0("main()", 1);
}
