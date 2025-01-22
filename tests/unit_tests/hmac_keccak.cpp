// Copyright (c) 2018-2024, The Monero Project

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

#include "gtest/gtest.h"
#include "../io.h"

extern "C" {
#include "crypto/hmac-keccak.h"
}

#define KECCAK_BLOCKLEN 136

static const struct {
  const char *key;
  const char *inp;
  const char *res;
} keccak_hmac_vectors[] = {
    {"",
     "",
     "042186ec4e98680a0866091d6fb89b60871134b44327f8f467c14e9841d3e97b",
    },
    {"00",
     "",
     "042186ec4e98680a0866091d6fb89b60871134b44327f8f467c14e9841d3e97b",
    },
    {"00000000000000000000000000000000",
     "",
     "042186ec4e98680a0866091d6fb89b60871134b44327f8f467c14e9841d3e97b",
    },
    {"",
     "00",
     "402541d5d678ad30217023a12efbd2f367388dc0253ccddb8f915d187e03d414",
    },
    {"0000000000000000000000000000000000000000000000000000000000000000",
     "0000000000000000000000000000000000000000000000000000000000000000",
     "d164c38ca454176281b3c09dc83cf70bd00290835ff5490356993d368cde0577",
    },
    {"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
     "0000000000000000000000000000000000000000000000000000000000000000",
     "d52c9100301a9546d8c97d7f32e8178fd5f4b0a7646ab761db9e0f503f8b0542",
    },
    {"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
     "d52c9100301a9546d8c97d7f32e8178fd5f4b0a7646ab761db9e0f503f8b0542",
     "5b9d98c5d3249176230d70fdb215ee2f93e4783064c9564384ddc396e63c18cb",
    },
    {"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
     "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
     "98d033ba0c95bb3aaf5709733443cbedba20f6451b710ccba4118fc5523013c8",
    },
    {"65768798a9bacbdcedfe0f2031425364758697a8b9cadbecfd0e1f30415263748596a7b8c9daebfc0d1e2f405162738495a6b7c8d9eafb0c1d2e3f5061728394a5b6c7d8e9fa0b1c2d3e4f60718293a4b5c6d7e8f90a1b2c3d4e5f708192a3b4c5d6e7f8091a2b3c4d5e6f8091a2b3c4d5e6f708192a3b4c5d6e7f90a1b2c3d4e5f60718293a4b5c6d7e8fa0b1c2d3e4f5061728394a5b6c7d8e9fb0c1d2e3f405162738495a6b7c8d9eafc0d1e2f30415263748596a7b8c9daebfd0e1f2031425364758697a8b9cadbecfe0f102132435465768798a9bacbdcedff00112233445566778899aabbccddeef00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f2031425364758697a8b9cadbecfd0e1f30415263748596a7b8c9daebfc0d1e2f405162738495a6b7c8d9eafb0c1d2e3f5061728394",
     "7b8895a2afbcc9d6e3f0fd0a1724313e4b5865727f8c99a6b3c0cddae7f4010e1b2835424f5c697683909daab7c4d1deebf805121f2c394653606d7a8794a1aebbc8d5e2effc091623303d4a5764717e8b98a5b2bfccd9e6f3000d1a2734414e5b6875828f9ca9b6c3d0ddeaf704111e2b3845525f6c798693a0adbac7d4e1eefb0815222f3c495663707d8a97a4b1becbd8e5f2ff0c192633404d5a6774818e9ba8b5c2cfdce9f603101d2a3744515e6b7885929facb9c6d3e0edfa0714212e3b4855626f7c8996a3b0bdcad7e4f1fe0b1825323f4c596673808d9aa7b4c1cedbe8f5020f1c293643505d6a7784919eabb8c5d2dfecf90613202d3a4754616e7b8895a2afbcc9d6e3f0fd0a1724313e4b5865727f8c99a6b3c0cddae7f4010e1b2835424f5c697683909daab7c4d1deebf805121f2c394653606d7a8794a1ae",
     "117b60a7f474fd2245adff82a69429367de8f5263fa96c411986009a743b0e70",
    },
    {"00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f",
     "000d1a2734414e5b6875828f9ca9b6c3d0ddeaf704111e2b3845525f6c798693a0adbac7d4e1eefb0815222f3c495663707d8a97a4b1becbd8e5f2ff0c192633404d5a6774818e9ba8b5c2cfdce9f603101d2a3744515e6b7885929facb9c6d3e0edfa0714212e3b4855626f7c8996a3b0bdcad7e4f1fe0b1825323f4c596673808d9aa7b4c1cedbe8f5020f1c293643505d6a7784919eabb8c5d2dfecf90613202d3a4754616e7b8895a2afbcc9d6e3f0fd0a1724313e4b5865727f8c99a6b3c0cddae7f4010e1b",
     "cbe3ca627c6432c9a66f07c6411fccca894c9b083b55d0773152460142a676ed",
    },
};

static void test_keccak_hmac(const size_t * chunks)
{
  uint8_t inp_buff[1024];
  uint8_t key_buff[1024];
  uint8_t res_exp[32];
  uint8_t res_comp[32];
  size_t len_chunks = 0;
  for(; chunks && chunks[len_chunks] > 0; ++len_chunks);

  for (size_t i = 0; i < (sizeof(keccak_hmac_vectors) / sizeof(*keccak_hmac_vectors)); i++)
  {
    const size_t inp_len = strlen(keccak_hmac_vectors[i].inp)/2;
    const size_t key_len = strlen(keccak_hmac_vectors[i].key)/2;
    ASSERT_TRUE(hexdecode(keccak_hmac_vectors[i].inp, inp_len, inp_buff));
    ASSERT_TRUE(hexdecode(keccak_hmac_vectors[i].key, key_len, key_buff));
    ASSERT_TRUE(hexdecode(keccak_hmac_vectors[i].res, 32, res_exp));
    if (len_chunks == 0)
    {
      hmac_keccak_hash(res_comp, key_buff, key_len, inp_buff, inp_len);
    }
    else
    {
      hmac_keccak_state S;
      hmac_keccak_init(&S, key_buff, key_len);
      size_t inp_passed = 0;
      size_t chunk_size = 0;
      while(inp_passed < inp_len){
        const size_t to_pass = std::min(inp_len - inp_passed, chunks[chunk_size]);
        hmac_keccak_update(&S, inp_buff + inp_passed, to_pass);

        inp_passed += to_pass;
        chunk_size = (chunk_size + 1) % len_chunks;
      }

      hmac_keccak_finish(&S, res_comp);
    }
    ASSERT_EQ(memcmp(res_exp, res_comp, 32), 0);
  }
}


TEST(keccak_hmac, nullptr)
{
  test_keccak_hmac(nullptr);
}

TEST(keccak_hmac, 1)
{
  static const size_t chunks[] = {1, 0};
  test_keccak_hmac(chunks);
}

TEST(keccak_hmac, 1_20)
{
  static const size_t chunks[] = {1, 20, 0};
  test_keccak_hmac(chunks);
}

TEST(keccak_hmac, 136_1)
{
  static const size_t chunks[] = {136, 1, 0};
  test_keccak_hmac(chunks);
}

TEST(keccak_hmac, 137_1)
{
  static const size_t chunks[] = {137, 1, 0};
  test_keccak_hmac(chunks);
}
