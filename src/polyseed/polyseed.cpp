// Copyright (c) 2023, The Monero Project
// Copyright (c) 2021, tevador <tevador@gmail.com>
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#include "polyseed.hpp"
#include "pbkdf2.h"

#include <sodium/core.h>
#include <sodium/utils.h>
#include <sodium/randombytes.h>
#include <utf8proc.h>

#include <cstring>
#include <algorithm>
#include <array>

namespace polyseed {

    inline size_t utf8_norm(const char* str, polyseed_str norm, utf8proc_option_t options) {
      utf8proc_int32_t buffer[POLYSEED_STR_SIZE];
      utf8proc_ssize_t result;

      result = utf8proc_decompose(reinterpret_cast<const uint8_t*>(str), 0, buffer, POLYSEED_STR_SIZE, options);
      if (result < 0 || result > (POLYSEED_STR_SIZE - 1)) {
        throw std::runtime_error("Unicode normalization failed");
      }

      result = utf8proc_reencode(buffer, result, options);
      if (result < 0 || result > POLYSEED_STR_SIZE) {
        throw std::runtime_error("Unicode normalization failed");
      }

      strcpy(norm, reinterpret_cast<const char*>(buffer));
      sodium_memzero(buffer, POLYSEED_STR_SIZE);
      return result;
    }

    static size_t utf8_nfc(const char* str, polyseed_str norm) {
        // Note: UTF8PROC_LUMP is used here to replace the ideographic space with a regular space for Japanese phrases
        // to allow wallets to split on ' '.
        return utf8_norm(str, norm, (utf8proc_option_t)(UTF8PROC_NULLTERM | UTF8PROC_STABLE | UTF8PROC_COMPOSE | UTF8PROC_STRIPNA));
    }

    static size_t utf8_nfkd(const char* str, polyseed_str norm) {
        return utf8_norm(str, norm, (utf8proc_option_t)(UTF8PROC_NULLTERM | UTF8PROC_STABLE | UTF8PROC_DECOMPOSE | UTF8PROC_COMPAT | UTF8PROC_STRIPNA));
    }

    struct dependency {
        dependency();
        std::vector<language> languages;
    };

    static dependency deps;

    dependency::dependency() {
        if (sodium_init() == -1) {
            throw std::runtime_error("sodium_init failed");
        }

        polyseed_dependency pd;
        pd.randbytes = &randombytes_buf;
        pd.pbkdf2_sha256 = &crypto_pbkdf2_sha256;
        pd.memzero = &sodium_memzero;
        pd.u8_nfc = &utf8_nfc;
        pd.u8_nfkd = &utf8_nfkd;
        pd.time = nullptr;
        pd.alloc = nullptr;
        pd.free = nullptr;

        polyseed_inject(&pd);

        for (int i = 0; i < polyseed_get_num_langs(); ++i) {
            languages.push_back(language(polyseed_get_lang(i)));
        }
    }

    static language invalid_lang;

    const std::vector<language>& get_langs() {
        return deps.languages;
    }

    const language& get_lang_by_name(const std::string& name) {
        for (auto& lang : deps.languages) {
            if (name == lang.name_en()) {
                return lang;
            }
            if (name == lang.name()) {
                return lang;
            }
        }
        return invalid_lang;
    }

    inline void data::check_init() const {
        if (valid()) {
            throw std::runtime_error("already initialized");
        }
    }

    static std::array<const char*, 8> error_desc = {
        "Success",
        "Wrong number of words in the phrase",
        "Unknown language or unsupported words",
        "Checksum mismatch",
        "Unsupported seed features",
        "Invalid seed format",
        "Memory allocation failure",
        "Phrase matches more than one language"
    };

    static error get_error(polyseed_status status) {
        if (status > 0 && status < sizeof(error_desc) / sizeof(const char*)) {
            return error(error_desc[(int)status], status);
        }
        return error("Unknown error", status);
    }

    void data::create(feature_type features) {
        check_init();
        auto status = polyseed_create(features, &m_data);
        if (status != POLYSEED_OK) {
            throw get_error(status);
        }
    }

    void data::split(const language& lang, polyseed_phrase& words) {
        check_init();
        if (!lang.valid()) {
            throw std::runtime_error("invalid language");
        }
    }

    void data::load(polyseed_storage storage) {
        check_init();
        auto status = polyseed_load(storage, &m_data);
        if (status != POLYSEED_OK) {
            throw get_error(status);
        }
    }

    void data::load(const crypto::secret_key &key) {
        polyseed_storage d;
        memcpy(&d, &key.data, 32);
        auto status = polyseed_load(d, &m_data);
        if (status != POLYSEED_OK) {
            throw get_error(status);
        }
    }

    language data::decode(const char* phrase) {
        check_init();
        const polyseed_lang* lang;
        auto status = polyseed_decode(phrase, m_coin, &lang, &m_data);
        if (status != POLYSEED_OK) {
            throw get_error(status);
        }
        return language(lang);
    }
}
