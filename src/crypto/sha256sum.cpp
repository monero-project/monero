// Copyright (c) 2022, The Monero Project
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

#include "crypto/sha256sum.h"

#include <algorithm>       // min
#include <cstdio>          // BUFSIZ
#include <fstream>         // ifstream
#include <ios>             // ios_base::{binary, in, ate}
#include <memory>          // unique_ptr
#include <openssl/evp.h>

#include "misc_log_ex.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "util"

namespace {
    constexpr size_t SHA256_MD_SIZE = 32;
    static_assert(sizeof(crypto::hash::data) >= SHA256_MD_SIZE, "crypto::hash::data is too small for SHA-256");
}

namespace tools
{
    bool sha256sum(const uint8_t *data, size_t len, crypto::hash &hash) noexcept
    {
        if (!EVP_Digest(data, len, (unsigned char*) hash.data, NULL, EVP_sha256(), NULL)) {
            MERROR("Failed to digest with OpenSSL SHA-256 EVP");
            return false;
        }

        return true;
    }

    bool sha256sum(const std::string &filename, crypto::hash &hash) noexcept
    {
        // Open file for reading in binary mode, immediately putting the cursor at the end
        std::ifstream f(filename, std::ios_base::binary | std::ios_base::in | std::ios::ate);
        if (!f)
        {
            MERROR("sha256sum: Could not open file '" << filename << "' for reading");
            return false;
        }

        // Retreive file size and go back to the beginning of the stream
        size_t size_left = f.tellg();
        f.seekg(0, std::ios::beg);

        // Initiate digest context
        EVP_MD_CTX* ctx_raw;
        if (NULL == (ctx_raw = EVP_MD_CTX_new()))
        {
            MERROR("Failed to create OpenSSL EVP digest context");
            return false;
        }

        // Now we move raw_ctx into a unique_ptr so EVP_MD_CTX_free is alwaty called. This is
        // necessary because EVP_MD_CTX is an incomplete type and can't be allocated on the stack.
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(ctx_raw, &EVP_MD_CTX_free);

        // Initialize digest context
        if (!EVP_DigestInit_ex(ctx.get(), EVP_sha256(), NULL))
        {
            MERROR("Failed to initialize OpenSSL SHA-256 EVP digest context");
            return false;
        }

        while (size_left)
        {
            // Read N bytes from f into buf, where N is min(size_left, BUFSIZ)
            char buf[BUFSIZ];
            const std::ifstream::pos_type read_size = std::min(size_left, sizeof(buf));
            f.read(buf, read_size);
            if (!f || !f.good())
            {
                MERROR("sha256sum: I/O error while reading file '" << filename << "'");
                return false;
            }

            // Update digest
            if (!EVP_DigestUpdate(ctx.get(), buf, read_size)) {
                MERROR("Failed to update OpenSSL SHA-256 EVP digest");
                return false;
            }

            size_left -= read_size;
        } // while (size_left)

        // Finalize digest
        if (!EVP_DigestFinal_ex(ctx.get(), (unsigned char*) hash.data, NULL)) {
            MERROR("Failed to finalize OpenSSL SHA-256 EVP digest");
            return false;
        }

        return true;
    }
} // namespace tools
